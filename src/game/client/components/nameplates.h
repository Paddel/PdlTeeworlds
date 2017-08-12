
#ifndef GAME_CLIENT_COMPONENTS_NAMEPLATES_H
#define GAME_CLIENT_COMPONENTS_NAMEPLATES_H
#include <game/client/component.h>

class CNamePlates : public CComponent
{
	int m_InputShowIds;

	void RenderNameplate(
		const CNetObj_Character *pPrevChar,
		const CNetObj_Character *pPlayerChar,
		const CNetObj_PlayerInfo *pPlayerInfo
	);

public:
	CNamePlates();

	virtual void OnConsoleInit();
	virtual void OnRender();
};

#endif
