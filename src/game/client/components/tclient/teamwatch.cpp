#include "teamwatch.h"

#include <algorithm>

#include <base/io.h>
#include <base/str.h>
#include <base/types.h>
#include <base/vmath.h>
#include <engine/console.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>
#include <engine/storage.h>
#include <engine/textrender.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>

static const char *PARTNERS_FILE = "kizashi_partners.txt";

void CTeamWatch::OnInit()
{
	Load();
}

void CTeamWatch::OnShutdown()
{
	Save();
}

void CTeamWatch::OnReset()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		m_aFrozenSince[i] = 0.0f;
		m_aWasFrozen[i] = false;
	}
	m_LastTally = 0.0f;
	Save();
}

CTeamWatch::SPartner *CTeamWatch::FindPartner(const char *pName, bool Create)
{
	for(int i = 0; i < m_NumPartners; i++)
	{
		if(str_comp(m_aPartners[i].m_aName, pName) == 0)
			return &m_aPartners[i];
	}
	if(!Create || m_NumPartners >= MAX_PARTNERS)
		return nullptr;
	SPartner *pNew = &m_aPartners[m_NumPartners++];
	str_copy(pNew->m_aName, pName);
	pNew->m_Seconds = 0.0f;
	return pNew;
}

void CTeamWatch::AddTime(const char *pName, float Seconds)
{
	if(pName[0] == '\0')
		return;
	SPartner *pPartner = FindPartner(pName, true);
	if(pPartner)
	{
		pPartner->m_Seconds += Seconds;
		m_Dirty = true;
	}
}

void CTeamWatch::Load()
{
	IOHANDLE File = Storage()->OpenFile(PARTNERS_FILE, IOFLAG_READ, IStorage::TYPE_SAVE);
	if(!File)
		return;
	CLineReader Reader;
	if(Reader.OpenFile(File))
	{
		while(const char *pLine = Reader.Get())
		{
			// "<seconds> <name>", the name may hold spaces so it runs to the end
			const char *pSpace = str_find(pLine, " ");
			if(!pSpace)
				continue;
			char aSeconds[32];
			str_truncate(aSeconds, sizeof(aSeconds), pLine, pSpace - pLine);
			SPartner *pPartner = FindPartner(pSpace + 1, true);
			if(pPartner)
				pPartner->m_Seconds = str_tofloat(aSeconds);
		}
	}
	m_Dirty = false;
}

void CTeamWatch::Save()
{
	if(!m_Dirty)
		return;
	IOHANDLE File = Storage()->OpenFile(PARTNERS_FILE, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(!File)
		return;
	for(int i = 0; i < m_NumPartners; i++)
	{
		char aLine[128];
		str_format(aLine, sizeof(aLine), "%.0f %s\n", m_aPartners[i].m_Seconds, m_aPartners[i].m_aName);
		io_write(File, aLine, str_length(aLine));
	}
	io_close(File);
	m_Dirty = false;
}

void CTeamWatch::ConPartners(IConsole::IResult *pResult, void *pUserData)
{
	CTeamWatch *pSelf = (CTeamWatch *)pUserData;

	SPartner aSorted[MAX_PARTNERS];
	const int Num = pSelf->m_NumPartners;
	for(int i = 0; i < Num; i++)
		aSorted[i] = pSelf->m_aPartners[i];
	std::sort(aSorted, aSorted + Num, [](const SPartner &a, const SPartner &b) {
		return a.m_Seconds > b.m_Seconds;
	});

	if(Num == 0)
	{
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "kizashi", "пока никого: время считается, когда вы в одной команде");
		return;
	}

	pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "kizashi", "с кем вы проходите карты:");
	for(int i = 0; i < std::min(Num, 10); i++)
	{
		const int Total = (int)aSorted[i].m_Seconds;
		char aBuf[160];
		if(Total >= 3600)
			str_format(aBuf, sizeof(aBuf), "%2d. %-20s %d ч %02d мин", i + 1, aSorted[i].m_aName, Total / 3600, (Total % 3600) / 60);
		else
			str_format(aBuf, sizeof(aBuf), "%2d. %-20s %d мин", i + 1, aSorted[i].m_aName, Total / 60);
		pSelf->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "kizashi", aBuf);
	}
}

void CTeamWatch::OnConsoleInit()
{
	Console()->Register("kz_partners", "", CFGFLAG_CLIENT, ConPartners, this, "С кем вы чаще всего проходите карты");
}

void CTeamWatch::OnRender()
{
	const int Local = GameClient()->m_Snap.m_LocalClientId;
	if(Local < 0 || Client()->State() != IClient::STATE_ONLINE)
		return;

	const float Now = Client()->LocalTime();
	const float Delta = m_LastTally > 0.0f ? std::clamp(Now - m_LastTally, 0.0f, 1.0f) : 0.0f;
	m_LastTally = Now;

	const int MyTeam = GameClient()->m_Teams.Team(Local);

	struct SStuck
	{
		const char *m_pName;
		float m_Seconds;
	};
	SStuck aStuck[MAX_CLIENTS];
	int NumStuck = 0;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(i == Local || !GameClient()->m_Snap.m_apPlayerInfos[i] || !GameClient()->m_Snap.m_aCharacters[i].m_Active)
		{
			m_aWasFrozen[i] = false;
			continue;
		}
		const bool SameTeam = GameClient()->m_Teams.Team(i) == MyTeam;
		if(!SameTeam)
		{
			m_aWasFrozen[i] = false;
			continue;
		}

		// a run together only counts inside a real team, not in team 0
		if(MyTeam != 0 && Delta > 0.0f)
			AddTime(GameClient()->m_aClients[i].m_aName, Delta);

		const CGameClient::CClientData &Other = GameClient()->m_aClients[i];
		const bool Frozen = Other.m_FreezeEnd > 0 || Other.m_DeepFrozen;
		if(!Frozen)
		{
			m_aWasFrozen[i] = false;
			continue;
		}
		if(!m_aWasFrozen[i])
		{
			m_aWasFrozen[i] = true;
			m_aFrozenSince[i] = Now;
		}

		const float Stuck = Now - m_aFrozenSince[i];
		if(Stuck < (float)g_Config.m_TcFreezeWatchSeconds)
			continue;

		// somebody free and close by counts as help already on the way
		bool HelpNear = false;
		for(int j = 0; j < MAX_CLIENTS && !HelpNear; j++)
		{
			if(j == i || !GameClient()->m_Snap.m_aCharacters[j].m_Active)
				continue;
			if(GameClient()->m_Teams.Team(j) != MyTeam)
				continue;
			const CGameClient::CClientData &Helper = GameClient()->m_aClients[j];
			if(Helper.m_FreezeEnd > 0 || Helper.m_DeepFrozen)
				continue;
			if(distance(Helper.m_RenderPos, Other.m_RenderPos) < 320.0f)
				HelpNear = true;
		}
		if(HelpNear)
			continue;

		aStuck[NumStuck].m_pName = Other.m_aName;
		aStuck[NumStuck].m_Seconds = Stuck;
		NumStuck++;
	}

	if(!g_Config.m_TcFreezeWatch || NumStuck == 0)
		return;

	Ui()->MapScreen();
	const CUIRect Screen = *Ui()->Screen();
	const float FontSize = 6.5f;
	float y = Screen.h * 0.5f - (NumStuck * (FontSize + 4.0f)) * 0.5f;

	for(int i = 0; i < NumStuck; i++)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "%s во фризе %.0f с", aStuck[i].m_pName, aStuck[i].m_Seconds);

		CUIRect Line;
		Line.w = TextRender()->TextWidth(FontSize, aBuf, -1) + 10.0f;
		Line.h = FontSize + 4.0f;
		Line.x = Screen.w - Line.w - 6.0f;
		Line.y = y;
		Line.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.38f), IGraphics::CORNER_ALL, 2.0f);
		// the longer the wait, the hotter the line reads
		const float Heat = std::clamp((aStuck[i].m_Seconds - (float)g_Config.m_TcFreezeWatchSeconds) / 8.0f, 0.0f, 1.0f);
		TextRender()->TextColor(ColorRGBA(1.0f, 0.86f - 0.46f * Heat, 0.60f - 0.50f * Heat, 1.0f));
		Ui()->DoLabel(&Line, aBuf, FontSize, TEXTALIGN_MC);
		y += Line.h + 2.0f;
	}
	TextRender()->TextColor(TextRender()->DefaultTextColor());
}
