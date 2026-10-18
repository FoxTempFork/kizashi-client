#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_FNGFEED_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_FNGFEED_H

#include <engine/shared/protocol.h>

#include <game/client/component.h>

// A knockout feed for FNG: who put whom into the spikes, and how far your own
// run has got. The streak climbs while you keep scoring and resets the moment
// you are the one who goes in.
class CFngFeed : public CComponent
{
public:
	int Sizeof() const override { return sizeof(*this); }
	void OnReset() override;
	void OnMessage(int Msg, void *pRawMsg) override;
	void OnRender() override;

private:
	enum
	{
		MAX_ENTRIES = 6,
		NUM_CALLOUTS = 5,
	};

	struct SEntry
	{
		char m_aKiller[MAX_NAME_LENGTH];
		char m_aVictim[MAX_NAME_LENGTH];
		float m_Time;
		bool m_Mine;
	};

	SEntry m_aEntries[MAX_ENTRIES] = {};
	int m_Num = 0;

	int m_Streak = 0;
	int m_Best = 0;
	int m_Shown = 0; // highest callout already announced for this run
	float m_CalloutTime = -100.0f;
	char m_aCallout[64] = {0};

	void Push(int Killer, int Victim);
	void Announce();
};

#endif
