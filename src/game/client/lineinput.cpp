
#include <engine/keys.h>
#include <engine/shared/config.h>
#include "lineinput.h"

CLineInput::CLineInput()
{
	Clear();
}

void CLineInput::Clear()
{
	mem_zero(m_Str, sizeof(m_Str));
	m_Len = 0;
	m_CursorPos = 0;
	m_NumChars = 0;
}

void CLineInput::Set(const char *pString)
{
	str_copy(m_Str, pString, sizeof(m_Str));
	m_Len = str_length(m_Str);
	m_CursorPos = m_Len;
	m_NumChars = 0;
	int Offset = 0;
	while(pString[Offset])
	{
		Offset = str_utf8_forward(pString, Offset);
		++m_NumChars;
	}
}

bool CLineInput::Manipulate(IInput::CEvent e, char *pStr, int StrMaxSize, int StrMaxChars, int *pStrLenPtr, int *pCursorPosPtr, int *pNumCharsPtr)
{
	int NumChars = *pNumCharsPtr;
	int CursorPos = *pCursorPosPtr;
	int Len = *pStrLenPtr;
	bool Changes = false;

	if(CursorPos > Len)
		CursorPos = Len;

	int Code = e.m_Unicode;
	int k = e.m_Key;

	if(Code == 22 && g_Config.m_PdlInputCopyPaste)
	{//paste
		char *pPasting = clipboard_get();
		int PastinLength = str_length(pPasting);
		for(int i = 0; i < PastinLength; i++)
		{
			int CharSize = 1;
			if (Len < StrMaxSize - CharSize && CursorPos < StrMaxSize - CharSize && NumChars < StrMaxChars)
			{
				mem_move(pStr + CursorPos + CharSize, pStr + CursorPos, Len-CursorPos+1); // +1 == null term
				pStr[CursorPos] = pPasting[i];
				CursorPos += CharSize;
				Len += CharSize;
				if(CharSize > 0)
					++NumChars;
				Changes = true;
			}
			else
				break;
		}
	}
	else if(Code == 3 && g_Config.m_PdlInputCopyPaste)
	{//copy
		clipboard_set(pStr, NumChars + 1);
	}
	// 127 is produced on Mac OS X and corresponds to the delete key
	else if (!(Code >= 0 && Code < 32) && Code != 127)
	{
		char Tmp[8];
		int CharSize = str_utf8_encode(Tmp, Code);

		if (Len < StrMaxSize - CharSize && CursorPos < StrMaxSize - CharSize && NumChars < StrMaxChars)
		{
			mem_move(pStr + CursorPos + CharSize, pStr + CursorPos, Len-CursorPos+1); // +1 == null term
			for(int i = 0; i < CharSize; i++)
				pStr[CursorPos+i] = Tmp[i];
			CursorPos += CharSize;
			Len += CharSize;
			if(CharSize > 0)
				++NumChars;
			Changes = true;
		}
	}

	if(e.m_Flags&IInput::FLAG_PRESS)
	{
		if (k == KEY_BACKSPACE && CursorPos > 0)
		{
			int NewCursorPos = str_utf8_rewind(pStr, CursorPos);
			int CharSize = CursorPos-NewCursorPos;
			mem_move(pStr+NewCursorPos, pStr+CursorPos, Len - NewCursorPos - CharSize + 1); // +1 == null term
			CursorPos = NewCursorPos;
			Len -= CharSize;
			if(CharSize > 0)
				--NumChars;
			Changes = true;
		}
		else if (k == KEY_DELETE && CursorPos < Len)
		{
			int p = str_utf8_forward(pStr, CursorPos);
			int CharSize = p-CursorPos;
			mem_move(pStr + CursorPos, pStr + CursorPos + CharSize, Len - CursorPos - CharSize + 1); // +1 == null term
			Len -= CharSize;
			if(CharSize > 0)
				--NumChars;
			Changes = true;
		}
		else if (k == KEY_LEFT && CursorPos > 0)
			CursorPos = str_utf8_rewind(pStr, CursorPos);
		else if (k == KEY_RIGHT && CursorPos < Len)
			CursorPos = str_utf8_forward(pStr, CursorPos);
		else if (k == KEY_HOME)
			CursorPos = 0;
		else if (k == KEY_END)
			CursorPos = Len;
	}

	*pNumCharsPtr = NumChars;
	*pCursorPosPtr = CursorPos;
	*pStrLenPtr = Len;

	return Changes;
}

void CLineInput::ProcessInput(IInput::CEvent e)
{
	Manipulate(e, m_Str, MAX_SIZE, MAX_CHARS, &m_Len, &m_CursorPos, &m_NumChars);
}
