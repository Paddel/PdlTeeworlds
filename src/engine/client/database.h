#pragma once

typedef void (*ResultFunction)(int index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows);

#define QUERY_MAX_STR_LEN 512

class CDatabase
{
private:
	char m_aAddr[32]; char m_aUserName[128]; char m_aPass[128]; char m_aSchema[64];
	bool m_Connected;
	int m_ReconnectVal;

	static void QueryTestConnection(void *pData);
	bool InitConnection(char *pAddr, char *pUserName, char *pPass, char *pSchema);

	void CheckReconnect();

public:
	CDatabase();

	void Init(char *pAddr, char *pUserName, char *pPass, char *pTable);

	void QueryThread(char *command, ResultFunction ResultCallback, void *pData);
	int Query(char *command, ResultFunction ResultCallback, void *pData);
	void GetResult(ResultFunction ResultCallback, void *pData);

	static void PreventInjectionAppend(char *pDst, const char *pStr, int DstSize);
	static void AddQueryStr(char *pDst, const char *pStr, int DstSize);
	static void AddQueryInt(char *pDst, int Val, int DstSize);

	static char *GetDatabaseValue(char *pStr);

	static void Reconnect();

	bool GetConnected() const { return m_Connected; }
};