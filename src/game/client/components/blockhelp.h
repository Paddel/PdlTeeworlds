#pragma once

#include <game/client/component.h>

class CBlockHelp : public CComponent
{
	enum
	{
		HELP_SMARTHAMMER=0,

		NUM_HELPS,
	};

private:
	bool HelpActive[NUM_HELPS];

	bool WhouldTouchFreeze(CNetObj_PlayerInput Input, CNetObj_Character Char);
	bool WhouldTouchFreezeUp(CNetObj_PlayerInput Input, CNetObj_Character Char);

public:
	CBlockHelp();

	void SnapInput(CNetObj_PlayerInput *pInput);
	void SmartHammer(CNetObj_PlayerInput *pInput);
	virtual void OnRender();
};