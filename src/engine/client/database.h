#pragma once

typedef void (*ResultFunction)(int index, char *pResult, int pResultSize, void *pData);

#define QUERY_MAX_STR_LEN 512

class CDatabase
{
private:
	char m_aAddr[32]; char m_aUserName[128]; char m_aPass[128]; char m_aSchema[64];
	bool m_Connected;

	bool InitConnection(char *pAddr, char *pUserName, char *pPass, char *pSchema);

public:
	CDatabase();

	void Init(char *pAddr, char *pUserName, char *pPass, char *pTable);

	void QueryThread(char *command, ResultFunction ResultCallback, void *pData);
	int Query(char *command, ResultFunction ResultCallback, void *pData);
	void GetResult(ResultFunction ResultCallback, void *pData);

	void PreventInjectionAppend(char *pDst, char *pStr, int DstSize);
	void AddQueryStr(char *pDst, char *pStr, int DstSize);
	void AddQueryInt(char *pDst, int Val, int DstSize);

	static char *GetDatabaseValue(char *pStr);

	bool GetConnected() const { return m_Connected; }
};