#include "fngfeed.h"

#include <algorithm>

#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>
#include <generated/protocol.h>

// How far a run has to get before it is worth saying out loud.
static const struct
{
	int m_At;
	const char *m_pText;
} s_aCallouts[] = {
	{3, "TRIPLE"},
	{5, "RAMPAGE"},
	{7, "UNSTOPPABLE"},
	{10, "DOMINATING"},
	{15, "GODLIKE"},
};

void CFngFeed::OnReset()
{
	m_Num = 0;
	m_Streak = 0;
	m_Best = 0;
	m_Shown = 0;
	m_CalloutTime = -100.0f;
	m_aCallout[0] = '\0';
}

void CFngFeed::Push(int Killer, int Victim)
{
	if(Killer < 0 || Killer >= MAX_CLIENTS || Victim < 0 || Victim >= MAX_CLIENTS)
		return;

	for(int i = std::min(m_Num, (int)MAX_ENTRIES - 1); i > 0; i--)
		m_aEntries[i] = m_aEntries[i - 1];

	SEntry &Entry = m_aEntries[0];
	str_copy(Entry.m_aKiller, GameClient()->m_aClients[Killer].m_aName);
	str_copy(Entry.m_aVictim, GameClient()->m_aClients[Victim].m_aName);
	Entry.m_Time = Client()->LocalTime();
	Entry.m_Mine = Killer == GameClient()->m_Snap.m_LocalClientId;
	m_Num = std::min(m_Num + 1, (int)MAX_ENTRIES);
}

void CFngFeed::Announce()
{
	for(const auto &Callout : s_aCallouts)
	{
		if(m_Streak >= Callout.m_At && Callout.m_At > m_Shown)
		{
			str_copy(m_aCallout, Callout.m_pText);
			m_Shown = Callout.m_At;
			m_CalloutTime = Client()->LocalTime();
		}
	}
}

void CFngFeed::OnMessage(int Msg, void *pRawMsg)
{
	if(Msg != NETMSGTYPE_SV_KILLMSG)
		return;

	const CNetMsg_Sv_KillMsg *pKill = static_cast<CNetMsg_Sv_KillMsg *>(pRawMsg);
	const int Local = GameClient()->m_Snap.m_LocalClientId;

	Push(pKill->m_Killer, pKill->m_Victim);

	if(pKill->m_Victim == Local)
	{
		// going into the spikes yourself ends the run
		m_Streak = 0;
		m_Shown = 0;
	}
	else if(pKill->m_Killer == Local)
	{
		m_Streak++;
		m_Best = std::max(m_Best, m_Streak);
		Announce();
	}
}

void CFngFeed::OnRender()
{
	if(!g_Config.m_TcFngFeed)
		return;
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	const float Now = Client()->LocalTime();
	Ui()->MapScreen();
	const CUIRect Screen = *Ui()->Screen();

	// the feed itself, newest on top, each line fading out after a while
	const float FontSize = 6.0f;
	float y = Screen.h * 0.34f;
	for(int i = 0; i < m_Num; i++)
	{
		const SEntry &Entry = m_aEntries[i];
		const float Age = Now - Entry.m_Time;
		if(Age > 7.0f)
			break;
		const float Fade = Age > 6.0f ? 1.0f - (Age - 6.0f) : 1.0f;

		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "%s > %s", Entry.m_aKiller, Entry.m_aVictim);

		CUIRect Line;
		Line.x = 6.0f;
		Line.y = y;
		Line.w = TextRender()->TextWidth(FontSize, aBuf, -1) + 8.0f;
		Line.h = FontSize + 4.0f;
		Line.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.30f * Fade), IGraphics::CORNER_ALL, 2.0f);
		TextRender()->TextColor(Entry.m_Mine ?
						ColorRGBA(1.0f, 0.74f, 0.35f, Fade) :
						ColorRGBA(0.82f, 0.85f, 0.94f, 0.75f * Fade));
		Ui()->DoLabel(&Line, aBuf, FontSize, TEXTALIGN_MC);
		y += Line.h + 1.5f;
	}

	// the run, and the shout when it reaches a mark
	if(m_Streak >= 2)
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "x%d  (лучшая %d)", m_Streak, m_Best);
		CUIRect Line;
		Line.w = TextRender()->TextWidth(7.0f, aBuf, -1) + 10.0f;
		Line.h = 11.0f;
		Line.x = 6.0f;
		Line.y = Screen.h * 0.34f - Line.h - 3.0f;
		Line.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.35f), IGraphics::CORNER_ALL, 2.0f);
		TextRender()->TextColor(ColorRGBA(1.0f, 0.80f, 0.40f, 1.0f));
		Ui()->DoLabel(&Line, aBuf, 7.0f, TEXTALIGN_MC);
	}

	const float CalloutAge = Now - m_CalloutTime;
	if(m_aCallout[0] != '\0' && CalloutAge < 1.8f)
	{
		// a short swell in, then out, so it reads without covering the game
		const float In = std::min(CalloutAge / 0.18f, 1.0f);
		const float Out = CalloutAge > 1.3f ? 1.0f - (CalloutAge - 1.3f) / 0.5f : 1.0f;
		const float Alpha = std::clamp(std::min(In, Out), 0.0f, 1.0f);
		const float Size = 16.0f + (1.0f - In) * 8.0f;

		CUIRect Line;
		Line.x = 0.0f;
		Line.y = Screen.h * 0.22f;
		Line.w = Screen.w;
		Line.h = Size + 4.0f;
		TextRender()->TextColor(ColorRGBA(1.0f, 0.72f, 0.28f, Alpha));
		Ui()->DoLabel(&Line, m_aCallout, Size, TEXTALIGN_MC);
	}

	TextRender()->TextColor(TextRender()->DefaultTextColor());
}
