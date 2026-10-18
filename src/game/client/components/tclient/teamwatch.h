#ifndef GAME_CLIENT_COMPONENTS_TCLIENT_TEAMWATCH_H
#define GAME_CLIENT_COMPONENTS_TCLIENT_TEAMWATCH_H

#include <engine/console.h>
#include <engine/shared/protocol.h>

#include <game/client/component.h>

// Two things about the people you run maps with.
//
// The watch: a teammate who has been frozen for a while and has nobody coming
// for them is easy to miss, so they get named with the seconds they have been
// waiting.
//
// The tally: how long you have spent in a team with each player, kept between
// sessions, so the person you actually play with is a fact and not a guess.
class CTeamWatch : public CComponent
{
public:
	int Sizeof() const override { return sizeof(*this); }
	void OnInit() override;
	void OnShutdown() override;
	void OnConsoleInit() override;
	void OnReset() override;
	void OnRender() override;

private:
	enum
	{
		MAX_PARTNERS = 256,
	};

	struct SPartner
	{
		char m_aName[MAX_NAME_LENGTH];
		float m_Seconds;
	};

	// per-connection freeze tracking, indexed by client id
	float m_aFrozenSince[MAX_CLIENTS] = {};
	bool m_aWasFrozen[MAX_CLIENTS] = {};

	SPartner m_aPartners[MAX_PARTNERS] = {};
	int m_NumPartners = 0;
	float m_LastTally = 0.0f;
	bool m_Dirty = false;

	SPartner *FindPartner(const char *pName, bool Create);
	void AddTime(const char *pName, float Seconds);
	void Load();
	void Save();

	static void ConPartners(IConsole::IResult *pResult, void *pUserData);
};

#endif
