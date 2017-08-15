
#include <base/string_seperation.h>
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
	Info.m_UseCustomColor = Data.m_UseCustomColor;
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
	str_format(aQuery, sizeof(aQuery), "REPLACE INTO %s.%s(name, clan, country, skin, costumcolor, color_body, color_feet, latency, date) VALUES (", SCHEMA_NAME, TABLE_NAME);
	AddQueryStr(aQuery, m_aPlayerItems[ClientID].m_Info.m_aName, sizeof(aQuery));
	str_append(aQuery, ", ", sizeof(aQuery));
	AddQueryStr(aQuery, m_aPlayerItems[ClientID].m_Info.m_aClan, sizeof(aQuery));
	str_append(aQuery, ", ", sizeof(aQuery));
	AddQueryInt(aQuery, m_aPlayerItems[ClientID].m_Info.m_Country, sizeof(aQuery));
	str_append(aQuery, ", ", sizeof(aQuery));
	AddQueryStr(aQuery, m_aPlayerItems[ClientID].m_Info.m_aSkin, sizeof(aQuery));
	str_append(aQuery, ", ", sizeof(aQuery));
	AddQueryInt(aQuery, m_aPlayerItems[ClientID].m_Info.m_UseCustomColor, sizeof(aQuery));
	str_append(aQuery, ", ", sizeof(aQuery));
	AddQueryInt(aQuery, m_aPlayerItems[ClientID].m_Info.m_ColorBody, sizeof(aQuery));
	str_append(aQuery, ", ", sizeof(aQuery));
	AddQueryInt(aQuery, m_aPlayerItems[ClientID].m_Info.m_ColorFeet, sizeof(aQuery));
	str_append(aQuery, ", ", sizeof(aQuery));
	AddQueryInt(aQuery, m_aPlayerItems[ClientID].m_Latency, sizeof(aQuery));
	str_append(aQuery, ", NOW());", sizeof(aQuery));
	QueryThread(aQuery, 0x0, 0x0);
}

void CIdentities::ResultRandomPlayerInfo(int Index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows)
{
	IGameClient::CPlayerInfo *pInfo = (IGameClient::CPlayerInfo *)pData;
	switch (Index)
	{
	case 0: str_copy(pInfo->m_aName, pResult, sizeof(pInfo->m_aName)); break;
	case 1: str_copy(pInfo->m_aClan, pResult, sizeof(pInfo->m_aClan)); break;
	case 2: pInfo->m_Country = str_toint(pResult); break;
	case 3: str_copy(pInfo->m_aSkin, pResult, sizeof(pInfo->m_aSkin)); break;
	case 4: pInfo->m_UseCustomColor = str_toint(pResult); break;
	case 5: pInfo->m_ColorBody = str_toint(pResult); break;
	case 6: pInfo->m_ColorFeet = str_toint(pResult); break;
	}
}

IGameClient::CPlayerInfo CIdentities::RandomPlayerInfo()
{
	IGameClient::CPlayerInfo Info;
	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.%s ORDER BY RAND() LIMIT 1", SCHEMA_NAME, TABLE_NAME);
	int res = Query(aQuery, &ResultRandomPlayerInfo, &Info);
	return Info;
}

void CIdentities::OnInit()
{
	char aQuery[QUERY_MAX_STR_LEN];
	CDatabase::Init("78.47.53.206", "taschenrechner", "hades", SCHEMA_NAME);
	str_format(aQuery, sizeof(aQuery), "CREATE TABLE IF NOT EXISTS %s (name VARCHAR(45) BINARY NOT NULL, clan VARCHAR(45), country INT DEFAULT -1, skin VARCHAR(45), customcolor INT, color_body INT, color_feet INT, latency INT, date DATETIME,  PRIMARY KEY (name)) CHARACTER SET utf8 ;", TABLE_NAME);
	QueryThread(aQuery, 0x0, 0x0);
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

		if (pInfo == 0x0)
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

		if(pPlayerInfo->m_Latency <= 0)
			continue;

		//enough filter
		m_aPlayerItems[i].m_Latency = pPlayerInfo->m_Latency;
		m_aPlayerItems[i].m_LastChange = time_get();

		mem_copy(&LastInfo, &CurInfo, sizeof(IGameClient::CPlayerInfo));

		SavePlayerItem(i);
	}
}

void CIdentities::GetMenuCountResult(int Index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows)
{
	int *pNum = (int *) pData;
	*pNum = str_toint(pResult);
}

int CIdentities::GetMenuCount(char *pCondition)
{
	int Result = 0;
	char aQuery[QUERY_MAX_STR_LEN];
	char aWhereCondition[256] = { };
	if(pCondition[0] != 0x0 && str_find(pCondition, ";") == 0x0)
		str_format(aWhereCondition, sizeof(aWhereCondition), "%s", pCondition);
	str_format(aQuery, sizeof(aQuery), "SELECT count(*) FROM %s.%s %s", SCHEMA_NAME, TABLE_NAME, aWhereCondition);
	int res = Query(aQuery, &GetMenuCountResult, &Result);
	return Result;
}

void CIdentities::GetMenuIdentity(ResultFunction ResultFunc, void *pData, int Page, int PageSize, char *pCondition)
{
	char aQuery[QUERY_MAX_STR_LEN];
	char aWhereCondition[256] = { };
	if(pCondition[0] != 0x0 && str_find(pCondition, ";") == 0x0)
		str_format(aWhereCondition, sizeof(aWhereCondition), "%s ", pCondition);

	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.%s %sLIMIT %i OFFSET %i", SCHEMA_NAME, TABLE_NAME, aWhereCondition, PageSize, Page);
	QueryThread(aQuery, ResultFunc, pData);
}

void CIdentities::SerializeIdentity(char *pDst, int DstSize, CPlayerItem &Identity)
{
	str_copy(pDst, "Identity\r\n", DstSize);
	str_fcat(pDst, DstSize, "Name=%s\r\n", Identity.m_Info.m_aName);
	str_fcat(pDst, DstSize, "Clan=%s\r\n", Identity.m_Info.m_aClan);
	str_fcat(pDst, DstSize, "Country=%i\r\n", Identity.m_Info.m_Country);
	str_fcat(pDst, DstSize, "Skin=%s\r\n", Identity.m_Info.m_aSkin);
	str_fcat(pDst, DstSize, "UseCustomColor=%i\r\n", Identity.m_Info.m_UseCustomColor);
	str_fcat(pDst, DstSize, "ColorBody=%i\r\n", Identity.m_Info.m_ColorBody);
	str_fcat(pDst, DstSize, "ColorFeet=%i\r\n", Identity.m_Info.m_ColorFeet);
	str_fcat(pDst, DstSize, "Latency=%i\r\n", Identity.m_Latency);
}

void CIdentities::WriteIdentity(char *pStr, int Size, CPlayerItem &Identity)
{
	char aBuf[512];
	if(str_comp_num(pStr, "Identity\r\n", str_length("Identity\r\n"))  != 0 || Size > 512)
		return;

	str_copy(aBuf, pStr, sizeof(aBuf));

	if(const char *pPos = str_find(pStr, "Name="))
	{
		char *pName = aBuf + (pPos - pStr) + str_length("Name=");
		const char *pVal = GetSepStr('\r', &pName);
		str_copy(Identity.m_Info.m_aName, pVal, sizeof(Identity.m_Info.m_aName));
	}
	if(const char *pPos = str_find(pStr, "Clan="))
	{
		char *pClan = aBuf + (pPos - pStr) + str_length("Clan=");
		const char *pVal = GetSepStr('\r', &pClan);
		str_copy(Identity.m_Info.m_aClan, pVal, sizeof(Identity.m_Info.m_aClan));
	}
	if(const char *pPos = str_find(pStr, "Country="))
	{
		char *pCountry = aBuf + (pPos - pStr) + str_length("Country=");
		const char *pVal = GetSepStr('\r', &pCountry);
		Identity.m_Info.m_Country = str_toint(pVal);
	}
	if(const char *pPos = str_find(pStr, "Skin="))
	{
		char *pSkin = aBuf + (pPos - pStr) + str_length("Skin=");
		const char *pVal = GetSepStr('\r', &pSkin);
		str_copy(Identity.m_Info.m_aSkin, pVal, sizeof(Identity.m_Info.m_aSkin));
	}
	if(const char *pPos = str_find(pStr, "UseCustomColor="))
	{
		char *pUseCostumColor = aBuf + (pPos - pStr) + str_length("UseCustomColor=");
		const char *pVal = GetSepStr('\r', &pUseCostumColor);
		Identity.m_Info.m_UseCustomColor = str_toint(pVal);
	}
	if(const char *pPos = str_find(pStr, "ColorBody="))
	{
		char *pColorBody = aBuf + (pPos - pStr) + str_length("ColorBody=");
		const char *pVal = GetSepStr('\r', &pColorBody);
		Identity.m_Info.m_ColorBody = str_toint(pVal);
	}
	if(const char *pPos = str_find(pStr, "ColorFeet="))
	{
		char *pColorFeet = aBuf + (pPos - pStr) + str_length("ColorFeet=");
		const char *pVal = GetSepStr('\r', &pColorFeet);
		Identity.m_Info.m_ColorFeet = str_toint(pVal);
	}
	if(const char *pPos = str_find(pStr, "Latency="))
	{
		char *pLatency = aBuf + (pPos - pStr) + str_length("Latency=");
		const char *pVal = GetSepStr('\r', &pLatency);
		Identity.m_Latency = str_toint(pVal);
	}
}