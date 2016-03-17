#pragma once

#include <base/tl/array.h>

typedef void (*HttpRequestCallback)(void *pResObj, char *pResult); 

struct CRequest
{
	HttpRequestCallback m_CallbackFunc;
	NETSOCKET m_Sock;
	void *m_pResObj;
};

// warning: your mother is fat
class CTranslator
{
private:
	array<CRequest> m_pRequests;

	// escape function
	static char * EscapeStr(const char * From);
	// &amp; => &, &gt => > etc.
	static char * UnquoteStr(const char * From);
public:
	void Tick();

	static char *GetResult(char *pRequest);

	void Translate(HttpRequestCallback Callback, void *pResObj, const char * String, const char *pLanguageFrom, const char * TargetLanguageCode);
};