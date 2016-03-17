
#include <base/system.h>
#include <engine/shared/config.h>
#include <string.h>

#include "translate.h"

static NETSOCKET invalid_socket = {NETTYPE_INVALID, -1, -1};

bool HttpRequest(CRequest *pRequest, char *pMethod, const char * Host, const char * Path, char * Buffer, int BufferSize)
{
	NETSOCKET Socket = invalid_socket;
	NETADDR HostAddress;
	char aNetBuff[1024];
	char aAddrStr[NETADDR_MAXSTRSIZE];
	net_addr_str(&HostAddress, aAddrStr, sizeof(aAddrStr), 80);

	if(net_host_lookup(Host, &HostAddress, NETTYPE_IPV4) != 0)
	{
		dbg_msg("HTTP-Request", "Error running host lookup");
		return false;
	}

	//Connect
	int socketID = create_http_socket();
	if(socketID < 0)
	{
		dbg_msg("HTTP-Request", "Error creating socket");
		return false;
	}

	Socket.type = NETTYPE_IPV4;
	Socket.ipv4sock = socketID;
	HostAddress.port = 80;
	
	if(net_tcp_connect(Socket, &HostAddress) != 0)
	{
		net_tcp_close(Socket);
		dbg_msg("HTTP-Request","Error connecting to host");
		return false;
	}

	net_set_non_blocking(Socket);

	str_format(aNetBuff, sizeof(aNetBuff), "%s %s HTTP/1.0\nHost: %s\n\n", pMethod, Path, Host);
	net_tcp_send(Socket, aNetBuff, str_length(aNetBuff));
	pRequest->m_Sock = Socket;

	return true;
}

void CTranslator::Tick()
{
	for(int i = 0; i < m_pRequests.size(); i++)
	{
		static const int BufferSize = 4024;
		char aBuffer[BufferSize];
		CRequest *pRequest = &m_pRequests[i];

		int Received = net_tcp_recv(pRequest->m_Sock, aBuffer, BufferSize - 1);
		if(Received <= 0)
			continue;
		aBuffer[Received + 1] = 0;
		if(pRequest->m_CallbackFunc)
			pRequest->m_CallbackFunc(pRequest->m_pResObj, aBuffer);
		//dbg_msg("recv", "Received %i\n'%s'", Received, aBuffer);
		
		m_pRequests.remove_index(i);
	}
}

char * CTranslator::EscapeStr(const char * From)
{
        unsigned long Len = str_length(From);
        unsigned long DestLen = Len * 4;
        char * Result = new char[DestLen + 1];
        memset(Result, 0, DestLen + 1);
        
        unsigned long Char;
        const char * Text = From;
        char * ToText = Result;
        unsigned long i;
        for (i = 0; i < Len; i++)
        {
                if ((From[i] >= 'a' && From[i] <= 'z') || (From[i] >= 'A' && From[i] <= 'Z') || (From[i] >= '0' && From[i] <= '9'))
                        *(ToText++) = From[i];
                else
                {
                        *(ToText++) = '%';
                        str_format(ToText, 3, "%02x", ((unsigned int)From[i])%0x100);
                        ToText += 2;
                }
        }
        
        return Result;
}

char * CTranslator::UnquoteStr(const char * From)
{
	unsigned long Len = str_length(From);
	char * Result = new char[Len + 1];
	memset(Result, 0, Len + 1);
	
	char * ToText = Result;
	for (unsigned long i = 0; i < Len; i++)
	{
		if (From[i] == '&')
		{
			if (str_find_nocase(From + i + 1, "amp;") == From + i + 1)
			{
				*(ToText++) = '&';
				i += 4;
			}
			else if (str_find_nocase(From + i + 1, "gt;") == From + i + 1)
			{
				*(ToText++) = '>';
				i += 3;
			}
			else if (str_find_nocase(From + i + 1, "lt;") == From + i + 1)
			{
				*(ToText++) = '<';
				i += 3;
			}
			else if (str_find_nocase(From + i + 1, "quot;") == From + i + 1)
			{
				*(ToText++) = '"';
				i += 5;
			}
			else
			{
				*(ToText++) = From[i];
			}
		}
		else
		{
			*(ToText++) = From[i];
		}
	}
	
	return Result;
}

void CTranslator::Translate(HttpRequestCallback Callback, void *pResObj, const char * String, const char *pLanguageFrom, const char * TargetLanguageCode)
{
	const int RequestSize = 4096;
	const int BufferSize = 4096;

	char * EscapedStr = EscapeStr(String);

	char * Request = new char[RequestSize + BufferSize];
	char * Buffer = Request + RequestSize;
	str_format(Request, RequestSize, "/V2/Ajax.svc/Translate?oncomplete=translate&appId=E9559C52BA30BACD8F868787AC847EF9A3B54EDB&from=%s&to=%s&text=%s", pLanguageFrom, TargetLanguageCode, EscapedStr);

	delete [] EscapedStr;

	Buffer[0] = 0;
	
	CRequest NewRequest;
	bool work = HttpRequest(&NewRequest, "GET", "api.microsofttranslator.com", Request, Buffer, BufferSize);
	NewRequest.m_CallbackFunc = Callback;
	NewRequest.m_pResObj = pResObj;

	if (!work)
	{
		delete [] Request;
		
		return;
	}

	m_pRequests.add(NewRequest);
}

char *CTranslator::GetResult(char *pRequest)
{
	
	char * TranslatedText = (char *)str_find_nocase(pRequest, "translate(\"");
	// nothing translated, nothing to return
	if (!TranslatedText)
		return 0;

	TranslatedText += str_length("translate(\"");
	char * TranslationEnd = (char *)str_find_nocase(TranslatedText, "\");");
	// incomplete string
	if (!TranslationEnd)
		return 0;
	
	TranslationEnd[0] = 0;
	
	char * Result = UnquoteStr(TranslatedText);

	return Result;
}