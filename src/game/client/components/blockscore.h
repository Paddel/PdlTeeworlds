#pragma once

#include <engine/client/database.h>
#include <game/client/component.h>

enum
{
	BLOCKSCORE_TILE_EMPTY = 0,
	BLOCKSCORE_TILE_V3,
	BLOCKSCORE_TILE_WAYBLOCK,
	BLOCKSCORE_TILE_RACEBLOCK,
	BLOCKSCORE_TILE_STARBLOCK,
	NUM_BLOCKSCORE_TILES,
};

static const char *aTypeNames[] = { "V3", "wayblock", "raceblock", "starblock" };

struct CChatReplyMsg
{
	int m_ClientID;
	char m_aMsg[512];
};

class CBlockScore : public CComponent, public CTextureUser, public CDatabase
{
private:
	array<CChatReplyMsg> m_ChatReplyMsgs;
	CTile *m_pTiles;
	char m_aServerAddress[256];
	char m_aLastLoadedMap[256];
	int m_MapWidth;
	int m_MapHeight;
	int m_DrawMode;
	int64 m_UpdateScoreTime;
	int64 m_ChatTime;
	int64 m_ExplenationTime;

	int GetIndex(vec2 Pos);
	int GetHooedBy(int ClientID);
	void DoBlockScores();

	void IncreaseScore(int ClientID, int Type, bool Loose);
	void FillBlockerInformation(char *pStr, int StringSize, int ClientID, char *pName);

	void UpdateScores();

	void AutoStay();
	static float GetTotalScore(CGameClient::CClientData *pClient);
	void PrintTop3();
	void WriteExplenation(int ClientID, char *pAdd);
	void WriteUncompleteProfile(char *pTo, int ClientID, char *pName, int Type, int Activity);
	void WriteProfile(int ClientID, char *pName, int Type);
	void InvalidChatMsg(int ClientID, const char *pMsg);
	void AnalyseMessage(int ClientID, const char *pMsgOrg);
	void ChatSupport();

public:
	CBlockScore();

	virtual void OnRender();
	virtual void OnMapLoad();
	virtual void OnConsoleInit();
	virtual void OnInit();
	virtual bool OnInput(IInput::CEvent e);
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual void OnStateChange(int NewState, int OldState);

	virtual void InitTextures();

	static void ConMapSave(IConsole::IResult *pResult, void *pUserData);
	static void ConMapLoad(IConsole::IResult *pResult, void *pUserData);

	static void WriteBlockScore(int index, char *pResult, int pResultSize, void *pData);
	static void WriteTop3(int index, char *pResult, int pResultSize, void *pData);

	CTile *MapTiles()  { return m_pTiles; }
	int MapWidth() const { return m_MapWidth; }
	int MapHeight() const { return m_MapHeight; }
};