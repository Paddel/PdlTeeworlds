#pragma once

#include <engine/client/database.h>
#include <game/client/component.h>

class CPlayerCollection : public CComponent, public CDatabase
{
private:
	char m_aLastName[MAX_CLIENTS][32];

public:
	CPlayerCollection();

	void OnRconLine(const char *pLine);
	void Check(const char *pName);

	virtual void OnRender();

	static void CheckResult(int index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows);
	static void CheckGetIp(int index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows);
};