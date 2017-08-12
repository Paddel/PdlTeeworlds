
#include <sstream> // TODO: Remove

#include <base/stringseperation.h>

#include "playercollection.h"

#define SCHEMA_NAME "paddel"
#define ENTER_LINE "[server]: player has entered the game."

CPlayerCollection::CPlayerCollection()
{
	CDatabase::Init("localhost", "root", "poseidon", SCHEMA_NAME);
}

void CPlayerCollection::OnRconLine(const char *pLine)
{
	char aLine[256];
	unsigned int ClientID = -1;
	std::stringstream ss;
	str_copy(aLine, pLine, sizeof(aLine));
	bool IsHex = false;
	char aIDStr[64];


	if(str_comp_num(pLine, ENTER_LINE, str_length(ENTER_LINE)) == 0)
		IsHex = true;

	if(IsHex)
		str_copy(aIDStr, "ClientID=", sizeof(aIDStr));
	else
		str_copy(aIDStr, "id=", sizeof(aIDStr));

	char *pClientIDStr = (char *)str_find(aLine, aIDStr);
	if(pClientIDStr == NULL)
		return;

	pClientIDStr += str_length(aIDStr);
	char *pID = GetSepStr(' ', &pClientIDStr);
	if(pID[0] == 0)
		return;

	if(IsHex)
	{
		ss << std::hex << pID;
		ss >> ClientID;
	}
	else
		ClientID = atoi(pID);

	str_copy(aLine, pLine, sizeof(aLine));
	pClientIDStr = (char *)str_find(aLine, "addr=");
	if(pClientIDStr == NULL)
		return;
	pClientIDStr += str_length("addr=");
	char *pAddr = GetSepStr(' ', &pClientIDStr);

	if(ClientID < 0 || ClientID >= MAX_CLIENTS)
		return;

	str_copy(m_pClient->m_aClients[ClientID].m_aAddr, pAddr, sizeof(m_pClient->m_aClients[ClientID].m_aAddr));
}

void CPlayerCollection::CheckResult(int index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows)
{
	CPlayerCollection *pThis = (CPlayerCollection *)pData;
	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "PlayerCheck", pResult);
}

void CPlayerCollection::CheckGetIp(int index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows)
{
	CPlayerCollection *pThis = (CPlayerCollection *)pData;
	char aIP[256];
	char aQuery[QUERY_MAX_STR_LEN];

	str_copy(aIP, pResult, sizeof(aIP));
	for(int i = 0; i < str_length(aIP); i++)
	{
		if(aIP[i] == ':')
		{
			aIP[i] = 0;
			break;
		}
	}

	str_format(aQuery, sizeof(aQuery), "SELECT name FROM %s.playercollection WHERE instr(ip, {$\"%s\"})", SCHEMA_NAME, aIP);
	int res = pThis->Query(aQuery, &CheckResult, pThis);
	
	if(res != -1)
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "PlayerCheck", "<Complete ip>");
	

	str_copy(aIP, pResult, sizeof(aIP));
	int DotCount = 0;
	for(int i = 0; i < str_length(aIP); i++)
	{
		if(aIP[i] == '.')
			DotCount++;

		if(DotCount == 2)
		{
			aIP[i] = 0;
			break;
		}
	}

	str_format(aQuery, sizeof(aQuery), "SELECT name FROM %s.playercollection WHERE instr(ip, {$\"%s\"})", SCHEMA_NAME, aIP);
	res = pThis->Query(aQuery, &CheckResult, pThis);
	
	if(res != -1)
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "PlayerCheck", "<ip range>");
}

void CPlayerCollection::Check(const char *pName)
{
	char aQuery[QUERY_MAX_STR_LEN];
	str_format(aQuery, sizeof(aQuery), "SELECT ip FROM %s.playercollection WHERE name=\"%s\"", SCHEMA_NAME, pName);
	int res = Query(aQuery, &CheckGetIp, this);
}

void CPlayerCollection::OnRender()
{
	char aQuery[QUERY_MAX_STR_LEN];

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_pClient->m_aClients[i].m_Active == false ||
			m_pClient->m_aClients[i].m_aAddr[0] == 0 ||
			m_pClient->m_aClients[i].m_aName[0] == 0 ||
			str_comp(m_pClient->m_aClients[i].m_aName, m_aLastName[i]) == 0)
			continue;

		mem_zero(&aQuery, sizeof(aQuery));
		str_copy(aQuery, "REPLACE INTO playercollection(name, ip) VALUES (", sizeof(aQuery));
		AddQueryStr(aQuery, m_pClient->m_aClients[i].m_aName, sizeof(aQuery));
		strcat(aQuery, ", ");
		AddQueryStr(aQuery, m_pClient->m_aClients[i].m_aAddr, sizeof(aQuery));
		strcat(aQuery, ");");
		Query(aQuery, NULL, 0x0);

		str_copy(m_aLastName[i], m_pClient->m_aClients[i].m_aName, sizeof(m_aLastName[i]));
	}
}