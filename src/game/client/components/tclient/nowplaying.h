#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_NOWPLAYING_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_NOWPLAYING_H

#include <atomic>
#include <mutex>
#include <thread>

#include <game/client/component.h>

// Shows whatever is playing on this machine. The track is read from any
// MPRIS-capable player through playerctl, on its own thread, so a player that
// answers slowly never costs a frame.
class CNowPlaying : public CComponent
{
public:
	int Sizeof() const override { return sizeof(*this); }
	void OnInit() override;
	void OnShutdown() override;
	void OnRender() override;

private:
	void Poll();

	std::thread m_Thread;
	std::atomic<bool> m_Running{false};
	std::mutex m_Lock;
	char m_aTrack[256] = {0};
};

#endif
