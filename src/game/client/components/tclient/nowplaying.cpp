#include "nowplaying.h"

#include <chrono>
#include <cstdio>

#include <base/str.h>
#include <engine/shared/config.h>
#include <engine/textrender.h>

#include <game/client/gameclient.h>
#include <game/client/ui.h>

void CNowPlaying::OnInit()
{
	m_Running = true;
	m_Thread = std::thread([this]() { Poll(); });
}

void CNowPlaying::OnShutdown()
{
	m_Running = false;
	if(m_Thread.joinable())
		m_Thread.join();
}

void CNowPlaying::Poll()
{
	while(m_Running)
	{
		char aLine[256];
		aLine[0] = '\0';
#if defined(CONF_FAMILY_UNIX)
		// playerctl speaks to every MPRIS player, so this covers Spotify, a
		// browser tab and a local player alike without any setup.
		FILE *pPipe = popen("playerctl metadata --format '{{artist}} - {{title}}' 2>/dev/null", "r");
		if(pPipe)
		{
			if(fgets(aLine, sizeof(aLine), pPipe) == nullptr)
				aLine[0] = '\0';
			pclose(pPipe);
		}
#endif
		for(int i = 0; aLine[i] != '\0'; i++)
		{
			if(aLine[i] == '\n' || aLine[i] == '\r')
			{
				aLine[i] = '\0';
				break;
			}
		}
		// a player with no metadata answers with a bare separator
		if(str_comp(aLine, " - ") == 0)
			aLine[0] = '\0';

		{
			const std::lock_guard<std::mutex> Guard(m_Lock);
			str_copy(m_aTrack, aLine);
		}

		// two seconds between lookups, checked often so quitting stays instant
		for(int i = 0; i < 20 && m_Running; i++)
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}
}

void CNowPlaying::OnRender()
{
	if(!g_Config.m_TcNowPlaying)
		return;
	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		return;

	char aTrack[256];
	{
		const std::lock_guard<std::mutex> Guard(m_Lock);
		str_copy(aTrack, m_aTrack);
	}
	if(aTrack[0] == '\0')
		return;

	Ui()->MapScreen();
	const CUIRect Screen = *Ui()->Screen();
	const float FontSize = 7.0f;
	const float Width = TextRender()->TextWidth(FontSize, aTrack, -1);

	CUIRect Line;
	Line.w = Width + 10.0f;
	Line.h = FontSize + 5.0f;
	Line.x = Screen.w - Line.w - 6.0f;
	Line.y = Screen.h - Line.h - 6.0f;

	Line.Draw(ColorRGBA(0.0f, 0.0f, 0.0f, 0.35f), IGraphics::CORNER_ALL, 3.0f);
	TextRender()->TextColor(ColorRGBA(0.84f, 0.87f, 0.96f, 0.85f));
	Ui()->DoLabel(&Line, aTrack, FontSize, TEXTALIGN_MC);
	TextRender()->TextColor(TextRender()->DefaultTextColor());
}
