
#include <base/system.h>
#include <engine/shared/config.h>

#if defined(CONF_FAMILY_WINDOWS)
#include "winsock2.h"
#endif

#include <mysql.h>

#include "database.h"

static LOCK s_QueryLock = NULL;
static int s_ReconnectVal = 0;

struct CThreadFeed
{
	ResultFunction ResultCallback;
	void *pResultData;
	char m_aCommand[512];
	int *m_pResult;
	char m_aAddr[32]; char m_aUserName[128]; char m_aPass[128]; char m_aSchema[64];
};


static void ExecuteQuery(void *pData)
{
	MYSQL_RES *pResult = NULL;
	MYSQL_ROW Field = NULL;
	MYSQL *pConn = NULL;
	CThreadFeed *pFeed = (CThreadFeed *)pData;

	pConn = mysql_init(NULL);
	if (pConn == NULL)
	{
		if (g_Config.m_Debug)
			dbg_msg("Database", "Initialing connection failed: '%s'", mysql_error(pConn));
		return;
	}

	mysql_options(pConn, MYSQL_SET_CHARSET_NAME, "utf8");

	if (mysql_real_connect(pConn, pFeed->m_aAddr, pFeed->m_aUserName, pFeed->m_aPass, pFeed->m_aSchema, 0, NULL, 0) == NULL)
	{
		if (g_Config.m_Debug)
			dbg_msg("Database", "Connecting failed: '%s'", mysql_error(pConn));
        return;
	}

	mysql_query(pConn, pFeed->m_aCommand);//Main-cmd
	if(g_Config.m_Debug)
		dbg_msg("Database", pFeed->m_aCommand);

	{//error
		int err = mysql_errno(pConn);
		if(err)
		{
			if (g_Config.m_Debug)
				dbg_msg("Database", "Query failed: '%s", mysql_error(pConn));
			mysql_close(pConn);
			return;
		}
	}

	if(pFeed->ResultCallback == 0x0)
	{
		mysql_close(pConn);
		return;
	}

	pResult = mysql_store_result(pConn);

	if(!pResult)//no Results. DONE
	{
		*pFeed->m_pResult = -1;
		mysql_close(pConn);
		return;
	}

	int count = (int)pResult->row_count;
	
	if(count == 0)
	{
		*pFeed->m_pResult = -1;
		mysql_close(pConn);
		return;
	}

	for(int i = 0; i < count; i++)
	{
		Field = mysql_fetch_row(pResult);
		int affected = mysql_num_fields(pResult);

		for(int x = 0; x < affected; x++)
			pFeed->ResultCallback(x, CDatabase::GetDatabaseValue(Field[x]), sizeof(Field[x]), pFeed->pResultData, i, count);
	}
	mysql_close(pConn);
}

static void QueryThreadFunction(void *pData)//only for threads
{
	CThreadFeed *pFeed = (CThreadFeed *)pData;

	lock_wait(s_QueryLock);
	ExecuteQuery(pData);
	lock_release(s_QueryLock);

	delete pFeed;
}

void CDatabase::QueryTestConnection(void *pData)
{
	CDatabase *pThis = (CDatabase *)pData;

	lock_wait(s_QueryLock);

	MYSQL *pConn = mysql_init(NULL);

	if (pConn == NULL)
	{
		dbg_msg("Database", "Initialing connection to %s*%s failed: '%s'", pThis->m_aAddr, pThis->m_aSchema, mysql_error(pConn));
		pThis->m_Connected = false;
		lock_release(s_QueryLock);
		return;
	}

	if (mysql_real_connect(pConn, pThis->m_aAddr, pThis->m_aUserName, pThis->m_aPass, pThis->m_aSchema, 0, NULL, 0) == NULL)
	{
		dbg_msg("Database", "Connecting to %s*%s failed: '%s'", pThis->m_aAddr, pThis->m_aSchema, mysql_error(pConn));
		pThis->m_Connected = false;
		lock_release(s_QueryLock);
		return;
	}


	dbg_msg("Database", "Connected to %s*%s", pThis->m_aAddr, pThis->m_aSchema);
	pThis->m_Connected = true;
	mysql_close(pConn);

	lock_release(s_QueryLock);
}

CDatabase::CDatabase()
{
	m_Connected = false;
	m_ReconnectVal = false;
	mem_zero(m_aAddr, sizeof(m_aAddr));
	mem_zero(m_aUserName, sizeof(m_aUserName));
	mem_zero(m_aPass, sizeof(m_aPass));
	mem_zero(m_aSchema, sizeof(m_aSchema));
}

void CDatabase::Init(char *pAddr, char *pUserName, char *pPass, char *pSchema)
{
	if (s_QueryLock == NULL)
		s_QueryLock = lock_create();

	InitConnection(pAddr, pUserName, pPass, pSchema);
}


bool CDatabase::InitConnection(char *pAddr, char *pUserName, char *pPass, char *pSchema)
{
	str_copy(m_aAddr, pAddr, sizeof(m_aAddr));
	str_copy(m_aUserName, pUserName, sizeof(m_aUserName));
	str_copy(m_aPass, pPass, sizeof(m_aPass));
	str_copy(m_aSchema, pSchema, sizeof(m_aSchema));

	thread_create(QueryTestConnection, this);
	return true;
}

void CDatabase::CheckReconnect()
{
	if (m_ReconnectVal == s_ReconnectVal)
		return;

	thread_create(QueryTestConnection, this);

	m_ReconnectVal = s_ReconnectVal;
}

void CDatabase::QueryThread(char *command, ResultFunction ResultCallback, void *pData)
{
	CheckReconnect();

	if(m_Connected == false)
		return;

	int Result = 0;

	CThreadFeed *pFeed = new CThreadFeed();
	pFeed->ResultCallback = ResultCallback;
	pFeed->pResultData = pData;
	str_copy(pFeed->m_aCommand, command, sizeof(pFeed->m_aCommand));
	pFeed->m_pResult = &Result;
	str_copy(pFeed->m_aAddr, m_aAddr, sizeof(pFeed->m_aAddr));
	str_copy(pFeed->m_aUserName, m_aUserName, sizeof(pFeed->m_aUserName));
	str_copy(pFeed->m_aPass, m_aPass, sizeof(pFeed->m_aPass));
	str_copy(pFeed->m_aSchema, m_aSchema, sizeof(pFeed->m_aSchema));

	//dbg_msg(0, "nt");
	thread_create(QueryThreadFunction, pFeed);
}

int CDatabase::Query(char *command, ResultFunction ResultCallback, void *pData)
{
	CheckReconnect();

	if(m_Connected == false)
		return -1;

	int Result = 0;

	CThreadFeed Feed;
	Feed.ResultCallback = ResultCallback;
	Feed.pResultData = pData;
	str_copy(Feed.m_aCommand, command, sizeof(Feed.m_aCommand));
	Feed.m_pResult = &Result;
	str_copy(Feed.m_aAddr, m_aAddr, sizeof(Feed.m_aAddr));
	str_copy(Feed.m_aUserName, m_aUserName, sizeof(Feed.m_aUserName));
	str_copy(Feed.m_aPass, m_aPass, sizeof(Feed.m_aPass));
	str_copy(Feed.m_aSchema, m_aSchema, sizeof(Feed.m_aSchema));

	ExecuteQuery(&Feed);
	return Result;
}

char *CDatabase::GetDatabaseValue(char *pStr)
{
	if(pStr == 0)
		return "";
	return pStr;
}

void CDatabase::PreventInjectionAppend(char *pDst, const char *pStr, int DstSize)
{
	int Len = str_length(pStr);
	int DstPos = str_length(pDst);
	for(int i = 0; i < Len; i++)
	{
		if(DstPos >= DstSize - 3)
		{
			dbg_msg(0, "size");
			return;
		}

		if(pStr[i] == '\\' || pStr[i] == '\'' || pStr[i] == '\"')
			pDst[DstPos++] = '\\';

		pDst[DstPos++] = pStr[i];
	}
	pDst[DstPos] = '\0';
}

void CDatabase::AddQueryStr(char *pDst, const char *pStr, int DstSize)
{
	str_append(pDst, "\'", DstSize);
	PreventInjectionAppend(pDst, pStr, DstSize);
	str_append(pDst, "\'", DstSize);
}

void CDatabase::AddQueryInt(char *pDst, int Val, int DstSize)
{
	char aBuf[128];
	str_format(aBuf, sizeof(aBuf), "%i", Val);
	str_append(pDst, "\'", DstSize);
	str_append(pDst, aBuf, DstSize);
	str_append(pDst, "\'", DstSize);
}

void CDatabase::Reconnect()
{
	s_ReconnectVal++;
}