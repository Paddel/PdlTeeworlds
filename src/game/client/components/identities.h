#pragma once

#include <engine/client/database.h>
#include <game/client/component.h>

class CIdentities : public CComponent, public CDatabase
{
private:
	void FillPlayerInfo(IGameClient::CPlayerInfo &Info, CGameClient::CClientData Data);
	bool FilteredName(char *pName, int Size);
	void SavePlayerItem(int ClientID);

public:

	struct CPlayerItem
	{
		int64 m_LastChange;
		IGameClient::CPlayerInfo m_Info;
		int m_Latency;
	} m_aPlayerItems[MAX_CLIENTS];

public:
	CIdentities();

	IGameClient::CPlayerInfo RandomPlayerInfo();

	static void ResultRandomPlayerInfo(int Index, char *pResult, int pResultSize, void *pData);

	static void GetMenuCountResult(int Index, char *pResult, int pResultSize, void *pData);
	int GetMenuCount(char *pCondition);
	void GetMenuIdentity(ResultFunction ResultFunc, void *pData, int Page, int PageSize, char *pCondition);

	void SerializeIdentity(char *pDst, int DstSize, CPlayerItem &Identity);
	void WriteIdentity(char *pStr, int Size, CPlayerItem &Identity);

	virtual void OnInit();
	virtual void OnRender();
};