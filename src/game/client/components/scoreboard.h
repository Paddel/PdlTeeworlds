
#ifndef GAME_CLIENT_COMPONENTS_SCOREBOARD_H
#define GAME_CLIENT_COMPONENTS_SCOREBOARD_H
#include <game/client/component.h>

#include <base/tl/sorted_array.h>

class CScoreboard : public CComponent
{
	struct CPlayerItem
	{
		int m_ID;
		class CGameClient *m_pClient;
		bool operator<(const CPlayerItem &Other);
	};

	enum
	{
		SORTION_ID = 0,
		SORTION_NAME,
		SORTION_SCORE,
		SORTION_CLAN,
		SORTION_LATENCY,
		SORTION_COUNTRY,
	};

	void RenderGoals(float x, float y, float w);
	void RenderSpectators(float x, float y, float w);
	void RenderScoreboard(float x, float y, float w, float h, int Team, const char *pTitle, int Corners);
	void RenderScoreboard64(float x, float y, float w, float h, const char *pTitle);
	void RenderRecordingNotification(float x);

	void UpdatePlayerList();

	static void ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData);

	const char *GetClanName(int Team);
	int GetSortion(const char *pSort);

	bool m_Active;
	int m_NumPlayers;
	int m_NumSpectators;
	sorted_array<CPlayerItem> m_lPlayers;
	int m_Sortion;

public:
	CScoreboard();
	virtual void OnReset();
	virtual void OnConsoleInit();
	virtual void OnRender();
	virtual void OnRelease();
	virtual void OnInit();

	int GetSortion() const { return m_Sortion; }

	bool Active();
};

#endif
