/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_BINDS_H
#define GAME_CLIENT_COMPONENTS_BINDS_H
#include <game/client/component.h>
#include <engine/keys.h>

#define CODE_LEN 256

struct CBind
{
	char m_aCode[CODE_LEN];
	char m_aAltCode[CODE_LEN];
	
	char *operator()(bool Alt = false) { return Alt ? m_aAltCode : m_aCode; }
};

class CBinds : public CComponent
{
	CBind m_aaKeyBindings[KEY_LAST];

	int GetKeyID(const char *pKeyName);

	static void ConBindAlt(IConsole::IResult *pResult, void *pUserData);
	static void ConBind(IConsole::IResult *pResult, void *pUserData);
	static void ConUnbind(IConsole::IResult *pResult, void *pUserData);
	static void ConUnbindAll(IConsole::IResult *pResult, void *pUserData);
	static void ConDumpBinds(IConsole::IResult *pResult, void *pUserData);
	class IConsole *GetConsole() const { return Console(); }

	static void ConfigSaveCallback(class IConfig *pConfig, void *pUserData);

public:
	CBinds();

	class CBindsSpecial : public CComponent
	{
	public:
		CBinds *m_pBinds;
		virtual bool OnInput(IInput::CEvent Event);
	};

	CBindsSpecial m_SpecialBinds;

	void Bind(int KeyID, const char *pStr, bool Alt = false);
	void SetDefaults();
	void UnbindAll();
	const char *Get(int KeyID);
	const char *GetKey(const char *pBindStr);

	virtual void OnConsoleInit();
	virtual bool OnInput(IInput::CEvent Event);
	virtual void OnRender();

	IInput *GetInput() const { return Input(); }
};
#endif
