#pragma once

#include <game/client/component.h>

class CBlockHelp : public CComponent
{
private:

	bool WhouldTouchFreeze(CNetObj_PlayerInput Input, CNetObj_Character Char);
public:
	CBlockHelp();

	void SnapInput(CNetObj_PlayerInput *pInput);
	//virtual void OnRender();
};