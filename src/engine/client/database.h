#pragma once

typedef void (*ResultFunction)(int index, char *pResult, int pResultSize, void *pData);

#define QUERY_MAX_STR_LEN 512

class CDatabase
{
private:
	bool m_Connected;

	bool InitConnection(char *pAddr, char *pUserName, char *pPass, char *pSchema);
	void CloseConnection();

	char *GetDatabaseValue(char *pStr);

public:
	CDatabase();

	void Init(char *pAddr, char *pUserName, char *pPass, char *pTable);

	int Query(char *command);
	void GetResult(ResultFunction ResultCallback, void *pData);

	void AddQueryStr(char *pDest, char *pStr);
	void AddQueryInt(char *pDest, int Val);

	bool GetConnected() const { return m_Connected; }
};