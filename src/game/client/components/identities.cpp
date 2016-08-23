
#include <sstream>

#include <base/stringseperation.h>
#include <engine/shared/config.h>

#include "identities.h"

#define SCHEMA_NAME "paddel"
#define TABLE_NAME "identities"
#define CHANGE_TIME 2.0f

CIdentities::CIdentities()
{
}

void CIdentities::FillPlayerInfo(IGameClient::CPlayerInfo &Info, CGameClient::CClientData Data)
{
	str_copy(Info.m_aName, Data.m_aName, sizeof(Info.m_aName));
	str_copy(Info.m_aClan, Data.m_aClan, sizeof(Info.m_aClan));
	Info.m_Country = Data.m_Country;
	str_copy(Info.m_aSkin, Data.m_aSkinName, sizeof(Info.m_aSkin));
	Info.m_UseCostumColor = Data.m_UseCustomColor;
	Info.m_ColorBody = Data.m_ColorBody;
	Info.m_ColorFeet = Data.m_ColorFeet;
}

bool CIdentities::FilteredName(char *pName, int Len)
{
	//time
	if (Len > 3 && pName[0] == '[' && pName[Len - 1] == ']')
	{
		if (str_find(pName, ":"))
			return true;
	}

	//dummy
	if (Len >= 3 && pName[2] == ')' && pName[0] == '(')
	{
		if (pName[1] >= '1' && pName[1] <= '9')
			return true;
	}

	return false;
}

void CIdentities::SavePlayerItem(int ClientID)
{
	char aQuery[QUERY_MAX_STR_LEN];
	mem_zero(&aQuery, sizeof(aQuery));
	str_format(aQuery, sizeof(aQuery), "REPLACE INTO %s.%s(name, clan, country, skin, costumcolor, color_body, color_feet, latency) VALUES (", SCHEMA_NAME, TABLE_NAME);
	AddQueryStr(aQuery, m_aPlayerItems[ClientID].m_Info.m_aName);
	strcat(aQuery, ", ");
	AddQueryStr(aQuery, m_aPlayerItems[ClientID].m_Info.m_aClan);
	strcat(aQuery, ", ");
	AddQueryInt(aQuery, m_aPlayerItems[ClientID].m_Info.m_Country);
	strcat(aQuery, ", ");
	AddQueryStr(aQuery, m_aPlayerItems[ClientID].m_Info.m_aSkin);
	strcat(aQuery, ", ");
	AddQueryInt(aQuery, m_aPlayerItems[ClientID].m_Info.m_UseCostumColor);
	strcat(aQuery, ", ");
	AddQueryInt(aQuery, m_aPlayerItems[ClientID].m_Info.m_ColorBody);
	strcat(aQuery, ", ");
	AddQueryInt(aQuery, m_aPlayerItems[ClientID].m_Info.m_ColorFeet);
	strcat(aQuery, ", ");
	AddQueryInt(aQuery, m_aPlayerItems[ClientID].m_Latency);
	strcat(aQuery, ");");
	Query(aQuery);
}

void CIdentities::ResultRandomPlayerInfo(int Index, char *pResult, int pResultSize, void *pData)
{
	IGameClient::CPlayerInfo *pInfo = (IGameClient::CPlayerInfo *)pData;
	switch (Index)
	{
	case 0: str_copy(pInfo->m_aName, pResult, sizeof(pInfo->m_aName)); break;
	case 1: str_copy(pInfo->m_aClan, pResult, sizeof(pInfo->m_aClan)); break;
	case 2: pInfo->m_Country = atoi(pResult); break;
	case 3: str_copy(pInfo->m_aSkin, pResult, sizeof(pInfo->m_aSkin)); break;
	case 4: pInfo->m_UseCostumColor = atoi(pResult); break;
	case 5: pInfo->m_ColorBody = atoi(pResult); break;
	case 6: pInfo->m_ColorFeet = atoi(pResult); break;
	}
}

IGameClient::CPlayerInfo CIdentities::RandomPlayerInfo()
{
	IGameClient::CPlayerInfo Info;
	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.%s ORDER BY RAND() LIMIT 1", SCHEMA_NAME, TABLE_NAME);
	int res = Query(aQuery);

	if (res == -1)
		return Info;


	GetResult(&ResultRandomPlayerInfo, &Info);//load
	return Info;
}

void CIdentities::OnInit()
{
	CDatabase::Init("78.47.53.206", "taschenrechner", "hades", SCHEMA_NAME);
}

void CIdentities::OnRender()
{
	if(g_Config.m_PdlUpdateIdentities == 0)
		return;

	void *pPrevInfo;
	void *pInfo;

	for (int i = 0; i < MAX_CLIENTS; i++)
	{
		CGameClient::CClientData &ClientData = m_pClient->m_aClients[i];

		if (!m_pClient->m_Snap.m_aCharacters[i].m_Active)
			continue;

		pInfo = Client()->SnapFindItem(IClient::SNAP_CURRENT, NETOBJTYPE_PLAYERINFO, i);

		if (pInfo == NULL)
			continue;

		const CNetObj_PlayerInfo *pPlayerInfo = (const CNetObj_PlayerInfo *)pInfo;

		IGameClient::CPlayerInfo CurInfo;
		IGameClient::CPlayerInfo &LastInfo = m_aPlayerItems[i].m_Info;
		FillPlayerInfo(CurInfo, ClientData);

		if (mem_comp(&CurInfo, &LastInfo, sizeof(IGameClient::CPlayerInfo)) == 0)
			continue; //nothing changed

		if (str_comp(CurInfo.m_aName, LastInfo.m_aName) != 0)
		{//new entry
			if (CurInfo.m_aName[0] == '\0' || FilteredName(CurInfo.m_aName, str_length(CurInfo.m_aName)))
			{
				mem_copy(&LastInfo, &CurInfo, sizeof(IGameClient::CPlayerInfo));
				continue;
			}
		}
		else
		{
			if (m_aPlayerItems[i].m_LastChange + time_freq() * CHANGE_TIME > time_get())
				continue;
		}

		//enough filter
		m_aPlayerItems[i].m_Latency = pPlayerInfo->m_Latency;
		m_aPlayerItems[i].m_LastChange = time_get();

		mem_copy(&LastInfo, &CurInfo, sizeof(IGameClient::CPlayerInfo));

		SavePlayerItem(i);
	}
}