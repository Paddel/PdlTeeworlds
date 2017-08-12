#pragma once

#include <engine/client/database.h>
#include <game/client/component.h>

class CAddressAnalysis : public CComponent
{
private:
	CDatabase m_DatabaseAddress;

	static void ResultWhois(int Index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows);
	static void ResultWhoisRange(int Index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows);
	static void ResultWhoisRangeAddr(int Index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows);
	static void ResultAccountData(int Index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows);
	

	static void ConWhoisID(IConsole::IResult *pResult, void *pUserData);
	static void ConWhoisIDAll(IConsole::IResult *pResult, void *pUserData);
	static void ConWhoisIDRange(IConsole::IResult *pResult, void *pUserData);
	static void ConWhoisName(IConsole::IResult *pResult, void *pUserData);
	static void ConWhoisNameAll(IConsole::IResult *pResult, void *pUserData);
	static void ConWhoisNameRange(IConsole::IResult *pResult, void *pUserData);
	static void ConWhoisAddress(IConsole::IResult *pResult, void *pUserData);
	static void ConAccountDataAddress(IConsole::IResult *pResult, void *pUserData);
public:
	virtual void OnInit();
	virtual void OnConsoleInit();

	void Whois(const char *pName);
	void WhoisAll(const char *pName);
	void WhoisRange(const char *pName);
	void WhoisAddress(const char *pAddress);
	void AccountDataAddress(const char *pAddress);
};