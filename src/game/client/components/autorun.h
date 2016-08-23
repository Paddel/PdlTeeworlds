#pragma once

#include <game/mapitems.h>
#include <game/client/component.h>

class CAutoRun : public CComponent, public CTextureUser
{
private:
	bool m_NextTileFound;
	int64 m_CampPointTime;
	vec2 m_CampPos;
	CTile *m_pTiles;
	int m_MapWidth;
	int m_MapHeight;
	int m_DrawMode;

	int GetIndex(vec2 Pos);

public:
	CAutoRun();

	virtual void InitTextures();

	virtual void OnInit();
	virtual void OnMapLoad();
	virtual void OnRender();
	virtual bool OnInput(IInput::CEvent e);
	virtual void OnConsoleInit();

	void SnapInput(CNetObj_PlayerInput *pInput, int ClientID, int DummyID);

	static void ConMapSave(IConsole::IResult *pResult, void *pUserData);
	static void ConMapLoad(IConsole::IResult *pResult, void *pUserData);

	CTile *MapTiles()  { return m_pTiles; }
	int MapWidth() const { return m_MapWidth; }
	int MapHeight() const { return m_MapHeight; }
};