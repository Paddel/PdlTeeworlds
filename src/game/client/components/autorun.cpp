
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/storage.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>

#include <game/client/components/camera.h>
#include <game/client/components/controls.h>

#include "autorun.h"

static int gs_TextureAutoRun = -1;

CAutoRun::CAutoRun()
{
	m_MapWidth = 0;
	m_MapHeight = 0;
	m_pTiles = 0;
	m_DrawMode = 1;
}

void CAutoRun::InitTextures()
{
	gs_TextureAutoRun = Graphics()->LoadTexture("pdl_autorun.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
}

void CAutoRun::OnInit()
{
	InitTextures();
	Graphics()->AddTextureUser(this);
}

void CAutoRun::OnMapLoad()
{
	m_MapWidth = Layers()->GameLayer()->m_Width;
	m_MapHeight = Layers()->GameLayer()->m_Height;
	m_pTiles = new CTile[m_MapWidth*m_MapHeight];
	for(int i = 0; i < m_MapWidth*m_MapHeight; i++)
	{
		m_pTiles[i].m_Index = 0;
		m_pTiles[i].m_Flags = 0;
		m_pTiles[i].m_Skip = 0;
		m_pTiles[i].m_Reserved = i;
	}
}

void CAutoRun::OnRender()
{
	if(Client()->State() != IClient::STATE_ONLINE || g_Config.m_PdlAutorunDraw == 0)
		return;

	vec2 Center = m_pClient->m_pCamera->m_Center;
	float Points[4];
	CUIRect Screen;
	Graphics()->GetScreen(&Screen.x, &Screen.y, &Screen.w, &Screen.h);

	RenderTools()->MapscreenToWorld(Center.x, Center.y, 1.0f, 1.0f,
		0, 0, Graphics()->ScreenAspect(), 1.0f, Points);
	Graphics()->MapScreen(Points[0], Points[1], Points[2], Points[3]);

	Graphics()->TextureSet(gs_TextureAutoRun);

	
	Graphics()->BlendNone();
	vec4 Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
	RenderTools()->RenderTilemap(m_pTiles, m_MapWidth, m_MapHeight, 32.0f, Color, TILERENDERFLAG_EXTEND|LAYERRENDERFLAG_OPAQUE,
									NULL, this, -1, 0);
	Graphics()->BlendNormal();
	RenderTools()->RenderTilemap(m_pTiles, m_MapWidth, m_MapHeight, 32.0f, Color, TILERENDERFLAG_EXTEND|LAYERRENDERFLAG_TRANSPARENT,
									NULL, this, -1, 0);

	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);
}

//void CAutoRun::ShowInfo()
//{
//	float ScreenX0, ScreenY0, ScreenX1, ScreenY1;
//	Graphics()->GetScreen(&ScreenX0, &ScreenY0, &ScreenX1, &ScreenY1);
//	Graphics()->TextureSet(m_pEditor->Client()->GetDebugFont());
//	Graphics()->QuadsBegin();
//
//	int StartY = max(0, (int)(ScreenY0/32.0f)-1);
//	int StartX = max(0, (int)(ScreenX0/32.0f)-1);
//	int EndY = min((int)(ScreenY1/32.0f)+1, m_Height);
//	int EndX = min((int)(ScreenX1/32.0f)+1, m_Width);
//
//	for(int y = StartY; y < EndY; y++)
//		for(int x = StartX; x < EndX; x++)
//		{
//			int c = x + y*m_Width;
//			if(m_pTiles[c].m_Index)
//			{
//				char aBuf[64];
//				str_format(aBuf, sizeof(aBuf), "%i", m_pTiles[c].m_Index);
//				m_pEditor->Graphics()->QuadsText(x*32, y*32, 16.0f, aBuf);
//
//				char aFlags[4] = {	m_pTiles[c].m_Flags&TILEFLAG_VFLIP ? 'V' : ' ',
//									m_pTiles[c].m_Flags&TILEFLAG_HFLIP ? 'H' : ' ',
//									m_pTiles[c].m_Flags&TILEFLAG_ROTATE? 'R' : ' ',
//									0};
//				m_pEditor->Graphics()->QuadsText(x*32, y*32+16, 16.0f, aFlags);
//			}
//			x += m_pTiles[c].m_Skip;
//		}
//
//	Graphics()->QuadsEnd();
//	Graphics()->MapScreen(ScreenX0, ScreenY0, ScreenX1, ScreenY1);
//}

bool CAutoRun::OnInput(IInput::CEvent e)
{
	if(Client()->State() != IClient::STATE_ONLINE || g_Config.m_PdlAutorunDraw == 0)
		return false;

	if(e.m_Flags&IInput::FLAG_PRESS)
	{
		if(e.m_Key == KEY_MOUSE_1)
		{
			vec2 MousePosWorld = m_pClient->m_pControls->m_TargetPos;

			int Nx = clamp(round(MousePosWorld.x)/32, 0, m_MapWidth-1);
			int Ny = clamp(round(MousePosWorld.y)/32, 0, m_MapHeight-1);
			int Tile = Ny*m_MapWidth+Nx;

			m_pTiles[Tile].m_Index = m_DrawMode;

			return true;
		}
		else if(e.m_Key >= '1' && e.m_Key <= '9')
		{
			m_DrawMode = e.m_Key - '1';
			return true;
		}
	}

	return false;
}

void CAutoRun::OnConsoleInit()
{
	Console()->Register("ar_map_save", "s", CFGFLAG_CLIENT, ConMapSave, this, "");
	Console()->Register("ar_map_load", "s", CFGFLAG_CLIENT, ConMapLoad, this, "");
}

/*if(m_pClient->m_PredictedChar.m_Colliding)
				pInput->m_Jump = 1;*/


void CAutoRun::SnapInput(CNetObj_PlayerInput *pInput, int ClientID, int DummyID)
{
	static int s_Direction = 0;
	static int64 s_KillTime = 0;

	CNetObj_CharacterCore Character;
	m_pClient->m_aClients[ClientID].m_Predicted.Write(&Character);
	vec2 LocalPos = vec2(Character.m_X, Character.m_Y);
	vec2 LocalVel = vec2(Character.m_VelX, Character.m_VelY);

	int Tile = GetIndex(LocalPos);

	if(m_pClient->m_Snap.m_aCharacters[ClientID].m_Active == false)
	{
		s_KillTime = time_get();
		pInput->m_Fire++;
		pInput->m_Hook = 0;
	}

	pInput->m_WantedWeapon = WEAPON_HAMMER;

	if(pInput->m_Jump)
		pInput->m_Jump = 0;

	if(s_KillTime + time_freq() * 7.0f < time_get())
		m_pClient->SendKillDummy(DummyID);

	if(s_Direction != 0 && m_pClient->m_aClients[ClientID].m_Predicted.m_Colliding)
		pInput->m_Jump = 1;

	if(Tile == 1)
		m_pClient->SendKillDummy(DummyID);
	if(Tile == 2)
	{
		/*if(Character.m_HookState == HOOK_IDLE)
		{
			pInput->m_TargetX = 0;
			pInput->m_TargetY = 1;
			pInput->m_Hook = 1;
		}
		else if(Character.m_HookState == HOOK_RETRACTED)
			pInput->m_Hook = 0;*/

		s_KillTime = time_get();
		s_Direction = 0;
	}
	if(Tile == 3)
		s_KillTime = time_get();
	if(Tile == 4)
		s_Direction = -1;
	if(Tile == 5)
		s_Direction = 1;
	if(Tile == 6)
		pInput->m_Jump = 1;
	

	pInput->m_Direction = s_Direction;

}

/*void CAutoRun::SnapInput(CNetObj_PlayerInput *pInput)
{
	static int CampLeaveKillTime = 30.0f;
	static int LastActive = 0;
	int LocalID = m_pClient->m_Snap.m_LocalClientID;
	int LocalPing = m_pClient->m_Snap.m_paPlayerInfos[LocalID]->m_Latency;
	CNetObj_CharacterCore LocalChar;
	m_pClient->m_aClients[LocalID].m_Predicted.Write(&LocalChar);
	vec2 LocalPos = vec2(LocalChar.m_X, LocalChar.m_Y);
	vec2 LocalVel = vec2(LocalChar.m_VelX, LocalChar.m_VelY);
	int Tile = GetIndex(LocalPos);

	if(m_pClient->m_Snap.m_aCharacters[LocalID].m_Active == false)
		pInput->m_Fire++;

	pInput->m_WantedWeapon = WEAPON_HAMMER;

	//give time to reach camp on spawn
	if(LastActive == 0 && m_pClient->m_Snap.m_aCharacters[LocalID].m_Active)
		m_CampPointTime = time_get() + time_freq() * CampLeaveKillTime;
	//kill if too long not camped reached
	if(m_NextTileFound && m_pClient->m_Snap.m_aCharacters[LocalID].m_Active && m_CampPointTime < time_get())
		m_pClient->SendKill(-1);

	if(pInput->m_Jump)
		pInput->m_Jump = 0;

	if(Tile == 4)
	{
		pInput->m_Jump = 1;
	}

	//direction
	if(m_NextTileFound)
	{
		vec2 DistPos = LocalPos-m_CampPos;
		if(abs(DistPos.x) > 8)
		{
			if(DistPos.x < 0)
				pInput->m_Direction = 1;
			else
				pInput->m_Direction = -1;

			if(m_pClient->m_PredictedChar.m_Colliding)
				pInput->m_Jump = 1;

		}
		else
		{//camp reached
			if(LocalChar.m_HookState == HOOK_IDLE)
			{
				pInput->m_TargetX = 0;
				pInput->m_TargetY = 1;
				pInput->m_Hook = 1;
			}
			else if(LocalChar.m_HookState == HOOK_RETRACTED)
				pInput->m_Hook = 0;

			m_CampPointTime = time_get() + time_freq() * CampLeaveKillTime;
		}
	}

	//fail hook?
	if(LocalChar.m_HookState == HOOK_GRABBED && LocalChar.m_HookedPlayer == -1 && distance(m_CampPos, vec2(LocalChar.m_HookX, LocalChar.m_HookY)) > 64)
		pInput->m_Hook = 0;

	//block
	for(int i = 0, j = 0; i < MAX_CLIENTS; i++)
	{
		vec2 EnemyPos;
		vec2 EnemyVel;
		CGameClient::CSnapState::CCharacterInfo EnemyChar;
		const void *pInfo = NULL;

		if(!m_pClient->m_Snap.m_aCharacters[i].m_Active || i == LocalID)
			continue;

		EnemyChar = m_pClient->m_Snap.m_aCharacters[i];
		pInfo = Client()->SnapFindItem(IClient::SNAP_CURRENT, NETOBJTYPE_PLAYERINFO, i);
		//EnemyPos = mix(vec2(EnemyChar.m_Prev.m_X, EnemyChar.m_Prev.m_Y), vec2(EnemyChar.m_Cur.m_X, EnemyChar.m_Cur.m_Y), Client()->IntraGameTick());

		EnemyVel = mix(vec2(EnemyChar.m_Prev.m_VelX, EnemyChar.m_Prev.m_VelY), vec2(EnemyChar.m_Cur.m_VelX, EnemyChar.m_Cur.m_VelY), Client()->IntraGameTick()) * (LocalPing*(50 / 1000) * 2) ;
		EnemyPos = mix(vec2(EnemyChar.m_Prev.m_X, EnemyChar.m_Prev.m_Y), vec2(EnemyChar.m_Cur.m_X, EnemyChar.m_Cur.m_Y), Client()->IntraGameTick()) + EnemyVel;

		if(GetIndex(EnemyPos) != 2)
			continue;

		if(distance(EnemyPos, LocalPos) > 360)
			continue;

		if(Collision()->IntersectLine(EnemyPos, LocalPos, NULL, NULL))
			continue;

		vec2 HookPos = EnemyPos-LocalPos;
		pInput->m_TargetX = HookPos.x;
		pInput->m_TargetY = HookPos.y;
		pInput->m_Hook = 1;

		if((LocalChar.m_HookState == HOOK_GRABBED && LocalChar.m_HookedPlayer == -1) ||
			LocalChar.m_HookState == HOOK_RETRACTED)
			pInput->m_Hook = 0;
	}

	LastActive = m_pClient->m_Snap.m_aCharacters[LocalID].m_Active;
}*/

int CAutoRun::GetIndex(vec2 Pos)
{
	int Nx = clamp(round(Pos.x)/32, 0, m_MapWidth-1);
	int Ny = clamp(round(Pos.y)/32, 0, m_MapHeight-1);
	int Tile = Ny*m_MapWidth+Nx;
	return m_pTiles[Tile].m_Index;
}

void CAutoRun::ConMapSave(IConsole::IResult *pResult, void *pUserData)
{
	CAutoRun *pThis = (CAutoRun*)pUserData;
	const char *pName = pResult->GetString(0);
	char aFilename[256];
	char aBuf[32];

	str_format(aFilename, sizeof(aFilename), "Paddel/%s.ar", pName);
	IOHANDLE File = pThis->Storage()->OpenFile(aFilename, IOFLAG_WRITE, IStorage::TYPE_SAVE);
	if(File)
	{
		for(int i = 0; i < pThis->MapWidth()*pThis->MapHeight(); i++)
		{
			itoa(pThis->MapTiles()[i].m_Index, aBuf, 10);
			io_write(File, aBuf, 1);
		}
		io_close(File);
	}
}

void CAutoRun::ConMapLoad(IConsole::IResult *pResult, void *pUserData)
{
	CAutoRun *pThis = (CAutoRun*)pUserData;
	const char *pName = pResult->GetString(0);
	char aFilename[256];
	char aBuf[32];
	CTile *pTiles = pThis->MapTiles();
	CLineReader LineReader;
	int Tile = 0;
	

	str_format(aFilename, sizeof(aFilename), "Paddel/%s.ar", pName);
	IOHANDLE File = pThis->Storage()->OpenFile(aFilename, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
	{
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Autorun", "File not found.");
		return;
	}

	LineReader.Init(File);

	while(char *pLine = LineReader.Get())
	{
		bool Error = false;
		for(int i = 0; i < str_length(pLine); i++)
		{
			if(Tile >= pThis->MapWidth()*pThis->MapHeight())
			{
				Error = true;
				break;
			}

			char IndexChar[3];
			str_format(IndexChar, sizeof(IndexChar), "%c", (char *)pLine[i]);
			int Index = atoi(IndexChar);
			pTiles[Tile].m_Index = Index;
			Tile++;
		}

		if(Error)
		{
			pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, "Autorun", "Map file does not fit!");
			break;
		}
	}
}