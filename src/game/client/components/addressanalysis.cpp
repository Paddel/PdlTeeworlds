
#include <engine/shared/config.h>

#include "addressanalysis.h"

#define SCHEMA_NAME "teeworlds"
#define TABLE_ADR "addresses"
#define TABLE_ACC "accounts"

void CAddressAnalysis::OnInit()
{
	m_DatabaseAddress.Init("31.186.250.72", "paddel", "Hurensohn123", SCHEMA_NAME);
}

void CAddressAnalysis::ResultWhois(int Index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows)
{
	CAddressAnalysis *pThis = (CAddressAnalysis *)pData;
	
	static char aBuf[128];
	if (Index == 0)
		mem_zero(aBuf, sizeof(aBuf));
	str_fcat(aBuf, sizeof(aBuf), "%s", pResult);
	if(Index != 2)
		str_append(aBuf, " ", sizeof(aBuf));
	if (Index == 2)
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "client", aBuf);
}

void CAddressAnalysis::ResultWhoisRange(int Index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows)
{
	CAddressAnalysis *pThis = (CAddressAnalysis *)pData;

	static char aBuf[128];
	if (Index == 0)
		mem_zero(aBuf, sizeof(aBuf));
	str_fcat(aBuf, sizeof(aBuf), "%s", pResult);
	if (Index != 2)
		str_append(aBuf, " ", sizeof(aBuf));
	if (Index == 2)
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "client", aBuf);
}

void CAddressAnalysis::ResultWhoisRangeAddr(int Index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows)
{
	CAddressAnalysis *pThis = (CAddressAnalysis *)pData;

	int RangePos = -1;
	if (const char *pFirstDot = str_find(pResult, "."))
		if((pFirstDot - pResult) < str_length(pResult))
			if (const char *pSecondDot = str_find(pFirstDot + 1, "."))
				RangePos = pSecondDot - pResult;

	pResult[RangePos] = '\0';//cut for only range

	char aQuery[QUERY_MAX_STR_LEN];
	mem_zero(&aQuery, sizeof(aQuery));
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.%s WHERE address like '", SCHEMA_NAME, TABLE_ADR);
	CDatabase::PreventInjectionAppend(aQuery, pResult, sizeof(aQuery));
	str_append(aQuery, ".%%' ORDER BY date DESC;", sizeof(aQuery));
	pThis->m_DatabaseAddress.QueryThread(aQuery, &ResultWhoisRange, pThis);
}

void CAddressAnalysis::ResultAccountData(int Index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows)
{
	CAddressAnalysis *pThis = (CAddressAnalysis *)pData;
	char aLineName[32];
	static char aBuf[256];

	if (Index == 0)
		mem_zero(aBuf, sizeof(aBuf));

	switch (Index)
	{
	case 0: str_copy(aLineName, "name", sizeof(aLineName)); break;
	case 1: str_copy(aLineName, "pass", sizeof(aLineName)); break;
	case 2: str_copy(aLineName, "vip", sizeof(aLineName)); break;
	case 3: str_copy(aLineName, "pages", sizeof(aLineName)); break;
	case 4: str_copy(aLineName, "level", sizeof(aLineName)); break;
	case 5: str_copy(aLineName, "exp", sizeof(aLineName)); break;
	case 6: str_copy(aLineName, "ip", sizeof(aLineName)); break;
	case 7: str_copy(aLineName, "weaponkits", sizeof(aLineName)); break;
	case 8: str_copy(aLineName, "slots", sizeof(aLineName)); break;
	}

	str_fcat(aBuf, sizeof(aBuf), "%s=%s", aLineName, pResult);
	if (Index != 8)
		str_append(aBuf, " ", sizeof(aBuf));
	if (Index == 8)
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "client", aBuf);
}

void CAddressAnalysis::ConWhoisID(IConsole::IResult *pResult, void *pUserData)
{
	CAddressAnalysis *pThis = (CAddressAnalysis *)pUserData;
	int ClientID = pResult->GetInteger(0);
	pThis->Whois(pThis->m_pClient->m_aClients[ClientID].m_aName);
}

void CAddressAnalysis::ConWhoisIDAll(IConsole::IResult *pResult, void *pUserData)
{
	CAddressAnalysis *pThis = (CAddressAnalysis *)pUserData;
	int ClientID = pResult->GetInteger(0);
	pThis->WhoisAll(pThis->m_pClient->m_aClients[ClientID].m_aName);
}

void CAddressAnalysis::ConWhoisIDRange(IConsole::IResult *pResult, void *pUserData)
{
	CAddressAnalysis *pThis = (CAddressAnalysis *)pUserData;
	int ClientID = pResult->GetInteger(0);
	pThis->WhoisRange(pThis->m_pClient->m_aClients[ClientID].m_aName);
}

void CAddressAnalysis::ConWhoisName(IConsole::IResult *pResult, void *pUserData)
{
	CAddressAnalysis *pThis = (CAddressAnalysis *)pUserData;
	pThis->Whois(pResult->GetString(0));
}

void CAddressAnalysis::ConWhoisNameAll(IConsole::IResult *pResult, void *pUserData)
{
	CAddressAnalysis *pThis = (CAddressAnalysis *)pUserData;
	pThis->WhoisAll(pResult->GetString(0));
}

void CAddressAnalysis::ConWhoisNameRange(IConsole::IResult *pResult, void *pUserData)
{
	CAddressAnalysis *pThis = (CAddressAnalysis *)pUserData;
	pThis->WhoisRange(pResult->GetString(0));
}

void CAddressAnalysis::ConWhoisAddress(IConsole::IResult *pResult, void *pUserData)
{
	CAddressAnalysis *pThis = (CAddressAnalysis *)pUserData;
	pThis->WhoisAddress(pResult->GetString(0));
}

void CAddressAnalysis::ConAccountDataAddress(IConsole::IResult *pResult, void *pUserData)
{
	CAddressAnalysis *pThis = (CAddressAnalysis *)pUserData;
	pThis->AccountDataAddress(pResult->GetString(0));
}



void CAddressAnalysis::OnConsoleInit()
{
	Console()->Register("whois_id", "i", CFGFLAG_CLIENT, ConWhoisID, this, "Find all names and ip-addresses");
	Console()->Register("whois_id_all", "i", CFGFLAG_CLIENT, ConWhoisIDAll, this, "Find all names and ip-addresses");
	Console()->Register("whois_id_range", "i", CFGFLAG_CLIENT, ConWhoisIDRange, this, "Find all names and ip-addresses");
	Console()->Register("whois_name", "r", CFGFLAG_CLIENT, ConWhoisName, this, "Find all names and ip-addresses");
	Console()->Register("whois_name_all", "r", CFGFLAG_CLIENT, ConWhoisName, this, "Find all names and ip-addresses");
	Console()->Register("whois_name_range", "r", CFGFLAG_CLIENT, ConWhoisNameRange, this, "Find all names and ip-addresses");
	Console()->Register("whois_adr", "r", CFGFLAG_CLIENT, ConWhoisAddress, this, "Find all names and ip-addresses");

	Console()->Register("accountdata_adr", "r", CFGFLAG_CLIENT, ConAccountDataAddress, this, "Find accountdata");
}

void CAddressAnalysis::Whois(const char *pName)
{
	if (pName[0] == '\0')
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "client", "Client not online");
		return;
	}

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "--<Address analysis for '%s'>--", pName);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "client", aBuf);

	char aQuery[QUERY_MAX_STR_LEN];
	mem_zero(&aQuery, sizeof(aQuery));
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.%s WHERE address = (SELECT address FROM %s.%s WHERE name = ", SCHEMA_NAME, TABLE_ADR, SCHEMA_NAME, TABLE_ADR);
	CDatabase::AddQueryStr(aQuery, pName, sizeof(aQuery));
	str_append(aQuery, " ORDER BY date DESC LIMIT 1) ORDER BY date DESC;", sizeof(aQuery));
	m_DatabaseAddress.QueryThread(aQuery, &ResultWhois, this);
}

void CAddressAnalysis::WhoisAll(const char *pName)
{
	if (pName[0] == '\0')
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "client", "Client not online");
		return;
	}

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "--<Address analysis for every '%s'>--", pName);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "client", aBuf);

	char aQuery[QUERY_MAX_STR_LEN];
	mem_zero(&aQuery, sizeof(aQuery));
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.%s WHERE address IN (SELECT address FROM %s.%s WHERE name = ", SCHEMA_NAME, TABLE_ADR, SCHEMA_NAME, TABLE_ADR);
	CDatabase::AddQueryStr(aQuery, pName, sizeof(aQuery));
	str_append(aQuery, ") ORDER BY date DESC;", sizeof(aQuery));
	m_DatabaseAddress.QueryThread(aQuery, &ResultWhois, this);
}


void CAddressAnalysis::WhoisRange(const char *pName)
{
	if (pName[0] == '\0')
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "client", "Client not online");
		return;
	}

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "--<Address-Range analysis for '%s'>--", pName);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "client", aBuf);

	char aQuery[QUERY_MAX_STR_LEN];
	mem_zero(&aQuery, sizeof(aQuery));
	str_format(aQuery, sizeof(aQuery), "SELECT address FROM %s.%s WHERE name = ", SCHEMA_NAME, TABLE_ADR);
	CDatabase::AddQueryStr(aQuery, pName, sizeof(aQuery));
	str_append(aQuery, " ORDER BY date DESC LIMIT 1;", sizeof(aQuery));
	m_DatabaseAddress.Query(aQuery, &ResultWhoisRangeAddr, this);
}

void CAddressAnalysis::WhoisAddress(const char *pAddress)
{
	if (pAddress[0] == '\0')
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "client", "Invalid Address");
		return;
	}

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "--<Address analysis for '%s'>--", pAddress);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "client", aBuf);

	char aQuery[QUERY_MAX_STR_LEN];
	mem_zero(&aQuery, sizeof(aQuery));
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.%s WHERE address like ", SCHEMA_NAME, TABLE_ADR);
	CDatabase::AddQueryStr(aQuery, pAddress, sizeof(aQuery));
	str_append(aQuery, " ORDER BY date DESC;", sizeof(aQuery));
	m_DatabaseAddress.QueryThread(aQuery, &ResultWhois, this);
}

void CAddressAnalysis::AccountDataAddress(const char *pAddress)
{
	if (pAddress[0] == '\0')
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "client", "Invalid Address");
		return;
	}

	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "--<Account analysis for '%s'>--", pAddress);
	Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "client", aBuf);

	char aQuery[QUERY_MAX_STR_LEN];
	mem_zero(&aQuery, sizeof(aQuery));
	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.%s WHERE ip = ", SCHEMA_NAME, TABLE_ACC);
	CDatabase::AddQueryStr(aQuery, pAddress, sizeof(aQuery));
	str_append(aQuery, ";", sizeof(aQuery));
	m_DatabaseAddress.QueryThread(aQuery, &ResultAccountData, this);
}