/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <base/math.h>

#include <engine/keys.h>
#include <engine/shared/config.h>
#include <engine/storage.h>

#include <game/collision.h>
#include <game/client/gameclient.h>
#include <game/client/component.h>
#include <game/client/components/chat.h>
#include <game/client/components/menus.h>
#include <game/client/components/scoreboard.h>
#include <game/client/components/blockhelp.h>
#include <game/client/components/autorun.h>
#include <game/client/components/skins.h>

#include "controls.h"

#define GRENADE_DODGE_TICKS 15

CControls::CControls()
{
	mem_zero(&m_LastData, sizeof(m_LastData));

	m_FakeInput = FAKEINPUT_NONE;
	m_BlockingPlayer = -1;
	m_GrenadeDangerous = false;
	m_InputLocked = false;
}

void CControls::OnReset()
{
	m_LastData.m_Direction = 0;
	m_LastData.m_Hook = 0;
	// simulate releasing the fire button
	if((m_LastData.m_Fire&1) != 0)
		m_LastData.m_Fire++;
	m_LastData.m_Fire &= INPUT_STATE_MASK;
	m_LastData.m_Jump = 0;
	m_InputData = m_LastData;

	m_InputDirectionLeft = 0;
	m_InputDirectionRight = 0;

	for(int i = 0; i < NUM_FAKEINPUTDATA; i++)
		m_FakeInputData[i] = 0;
}

void CControls::OnRelease()
{
	OnReset();
}

void CControls::OnPlayerDeath()
{
	m_LastData.m_WantedWeapon = m_InputData.m_WantedWeapon = 0;
}

static void ConKeyInputState(IConsole::IResult *pResult, void *pUserData)
{
	((int *)pUserData)[0] = pResult->GetInteger(0);
}

static void ConKeyInputCounter(IConsole::IResult *pResult, void *pUserData)
{
	int *v = (int *)pUserData;
	if(((*v)&1) != pResult->GetInteger(0))
		(*v)++;
	*v &= INPUT_STATE_MASK;
}

struct CInputSet
{
	CControls *m_pControls;
	int *m_pVariable;
	int m_Value;
};

static void ConKeyInputSet(IConsole::IResult *pResult, void *pUserData)
{
	CInputSet *pSet = (CInputSet *)pUserData;
	if(pResult->GetInteger(0))
		*pSet->m_pVariable = pSet->m_Value;
}

static void ConKeyInputNextPrevWeapon(IConsole::IResult *pResult, void *pUserData)
{
	CInputSet *pSet = (CInputSet *)pUserData;
	ConKeyInputCounter(pResult, pSet->m_pVariable);
	pSet->m_pControls->m_InputData.m_WantedWeapon = 0;
}

void CControls::OnConsoleInit()
{
	// game commands
	Console()->Register("+left", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputDirectionLeft, "Move left");
	Console()->Register("+right", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputDirectionRight, "Move right");
	Console()->Register("+jump", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputData.m_Jump, "Jump");
	Console()->Register("+hook", "", CFGFLAG_CLIENT, ConKeyInputState, &m_InputData.m_Hook, "Hook");
	Console()->Register("+fire", "", CFGFLAG_CLIENT, ConKeyInputCounter, &m_InputData.m_Fire, "Fire");

	Console()->Register("+swift", "", CFGFLAG_CLIENT, ConKeyInputState, &m_FakeInputData[FAKEINPUT_SWIFT], "Swift");
	Console()->Register("+fly", "", CFGFLAG_CLIENT, ConKeyInputState, &m_FakeInputData[FAKEINPUT_FLY], "Fly");
	Console()->Register("+block", "", CFGFLAG_CLIENT, ConKeyInputState, &m_FakeInputData[FAKEINPUT_BLOCK], "Block set");
	Console()->Register("+block_set", "", CFGFLAG_CLIENT, ConKeyInputState, &m_FakeInputData[FAKEINPUT_BLOCK_SET], "Block");
	Console()->Register("+grenade", "", CFGFLAG_CLIENT, ConKeyInputState, &m_FakeInputData[FAKEINPUT_GRENADEAIM], "Aimassistance for Grenade");
	Console()->Register("+step", "", CFGFLAG_CLIENT, ConKeyInputState, &m_FakeInputData[FAKEINPUT_STEP], "Step");
	Console()->Register("+vibrate", "", CFGFLAG_CLIENT, ConKeyInputState, &m_FakeInputData[FAKEINPUT_VIBRATE], "Step");

	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 1}; Console()->Register("+weapon1", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to hammer"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 2}; Console()->Register("+weapon2", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to gun"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 3}; Console()->Register("+weapon3", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to shotgun"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 4}; Console()->Register("+weapon4", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to grenade"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_WantedWeapon, 5}; Console()->Register("+weapon5", "", CFGFLAG_CLIENT, ConKeyInputSet, (void *)&s_Set, "Switch to rifle"); }

	{ static CInputSet s_Set = {this, &m_InputData.m_NextWeapon, 0}; Console()->Register("+nextweapon", "", CFGFLAG_CLIENT, ConKeyInputNextPrevWeapon, (void *)&s_Set, "Switch to next weapon"); }
	{ static CInputSet s_Set = {this, &m_InputData.m_PrevWeapon, 0}; Console()->Register("+prevweapon", "", CFGFLAG_CLIENT, ConKeyInputNextPrevWeapon, (void *)&s_Set, "Switch to previous weapon"); }
}

void CControls::OnMessage(int Msg, void *pRawMsg)
{
	if(Msg == NETMSGTYPE_SV_WEAPONPICKUP)
	{
		CNetMsg_Sv_WeaponPickup *pMsg = (CNetMsg_Sv_WeaponPickup *)pRawMsg;
		if(g_Config.m_ClAutoswitchWeapons)
			m_InputData.m_WantedWeapon = pMsg->m_Weapon+1;
	}
}

int CControls::SnapInput(int *pData)
{
	static int64 LastSendTime = 0;
	bool Send = false;

	// update player state
	if(m_pClient->m_pChat->IsActive())
		m_InputData.m_PlayerFlags = PLAYERFLAG_CHATTING;
	else if(m_pClient->m_pMenus->IsActive())
		m_InputData.m_PlayerFlags = PLAYERFLAG_IN_MENU;
	else
		m_InputData.m_PlayerFlags = PLAYERFLAG_PLAYING;

	if(m_pClient->m_pScoreboard->Active())
		m_InputData.m_PlayerFlags |= PLAYERFLAG_SCOREBOARD;

	if(m_LastData.m_PlayerFlags != m_InputData.m_PlayerFlags)
		Send = true;

	m_LastData.m_PlayerFlags = m_InputData.m_PlayerFlags;

	// we freeze the input if chat or menu is activated
	if(!(m_InputData.m_PlayerFlags&PLAYERFLAG_PLAYING) && !m_InputLocked)
	{
		OnReset();

		mem_copy(pData, &m_InputData, sizeof(m_InputData));

		// send once a second just to be sure
		if(time_get() > LastSendTime + time_freq())
			Send = true;
	}
	else
	{
		SetInput();

		// check if we need to send input
		if(m_InputData.m_Direction != m_LastData.m_Direction) Send = true;
		else if(m_InputData.m_Jump != m_LastData.m_Jump) Send = true;
		else if(m_InputData.m_Fire != m_LastData.m_Fire) Send = true;
		else if(m_InputData.m_Hook != m_LastData.m_Hook) Send = true;
		else if(m_InputData.m_WantedWeapon != m_LastData.m_WantedWeapon) Send = true;
		else if(m_InputData.m_NextWeapon != m_LastData.m_NextWeapon) Send = true;
		else if(m_InputData.m_PrevWeapon != m_LastData.m_PrevWeapon) Send = true;

		// send at at least 10hz
		if(time_get() > LastSendTime + time_freq()/25)
			Send = true;
	}

	// copy and return size
	m_LastData = m_InputData;

	if(!Send)
		return 0;

	if(g_Config.m_PdlFakeScoreboardOpen)
		m_InputData.m_PlayerFlags |= 8;//scoreboard is always opened

	LastSendTime = time_get();
	mem_copy(pData, &m_InputData, sizeof(m_InputData));
	return sizeof(m_InputData);
}

void CControls::OnRender()
{
	if(Input()->KeyDown(KEY_MOUSE_3))
	{
		mem_copy(&m_LockedInput, &m_InputData, sizeof(m_LockedInput));
		m_InputLocked = !m_InputLocked;
	}

	// update target pos
	if(m_pClient->m_Snap.m_pGameInfoObj && !m_pClient->m_Snap.m_SpecInfo.m_Active)
		m_TargetPos = m_pClient->m_LocalCharacterPos + m_MousePos;
	else if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_UsePosition)
		m_TargetPos = m_pClient->m_Snap.m_SpecInfo.m_Position + m_MousePos;
	else
		m_TargetPos = m_MousePos;

	if(m_FakeInput == FAKEINPUT_FLY)
	{
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0.4f,0.4f,0,0.5f);
		RenderTools()->DrawCircle(m_FlyPos.x, m_FlyPos.y, 16, 4);
		Graphics()->QuadsEnd();
	}

	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.4f,0.4f,0,0.5f);
	RenderTools()->DrawCircle(m_OutFreezePos.x, m_OutFreezePos.y, 16, 8);
	//RenderTools()->DrawCircle(m_LastGrenadePos.x, m_LastGrenadePos.y, 16, 8);
	Graphics()->QuadsEnd();

	if (g_Config.m_PdlGrenadeDodge)
	{
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		if(m_GrenadeDangerous == false)
			Graphics()->SetColor(0.0f, 0.0f, 0.4f, 0.5f);
		else
			Graphics()->SetColor(0.4f, 0.0f, 0.0f, 0.5f);

		//RenderTools()->DrawCircle(m_OutFreezePos.x, m_OutFreezePos.y, 16, 8);
		RenderTools()->DrawCircle(m_LastGrenadePos.x, m_LastGrenadePos.y, m_GrenadeDangerous?32:16, 8);
		Graphics()->QuadsEnd();
	}

	if(g_Config.m_PdlGrenadeKills)
		GrenadeKills();

	static int s_TextureArrow = Graphics()->LoadTexture("pdl_enemy_arrow.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	if(g_Config.m_PdlBlockArrow && m_BlockingPlayer >= 0 && m_BlockingPlayer < MAX_CLIENTS 
		&& m_pClient->m_Snap.m_aCharacters[m_BlockingPlayer].m_Active && m_pClient->m_Snap.m_LocalClientID != m_BlockingPlayer
		&& m_pClient->m_Snap.m_pLocalCharacter && m_pClient->m_Snap.m_pLocalPrevCharacter)
	{
		CNetObj_CharacterCore EnemyChar;
		m_pClient->m_aClients[m_BlockingPlayer].m_Predicted.Write(&EnemyChar);
		vec2 EnemyPos = vec2(EnemyChar.m_X, EnemyChar.m_Y);

		CNetObj_Character Char = *m_pClient->m_Snap.m_pLocalCharacter;
		CNetObj_Character PrevChar = *m_pClient->m_Snap.m_pLocalPrevCharacter;
		m_pClient->m_PredictedChar.Write(&Char);
		m_pClient->m_PredictedPrevChar.Write(&PrevChar);
		float IntraTick = Client()->PredIntraGameTick();
		vec2 LocalPos = mix(vec2(PrevChar.m_X, PrevChar.m_Y), vec2(Char.m_X, Char.m_Y), IntraTick);

		if(distance(LocalPos, EnemyPos) > g_Config.m_PdlBlockArrowDistance)
		{
			vec2 Direction = normalize(EnemyPos - LocalPos);
			vec2 ArrowPos = LocalPos + Direction * 35.0f;
			float Angle = GetAngle(Direction) + pi;

			Graphics()->TextureSet(s_TextureArrow);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			Graphics()->QuadsSetRotation(Angle);
			float w = 44.0f / 3.8f, h = 136.0f / 3.8f;
			IGraphics::CQuadItem QuadItem(ArrowPos.x - w * 0.5f, ArrowPos.y - h * 0.5f, w, h);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}
	}
}

void CControls::GrenadeKills()
{
	CNetObj_CharacterCore LocalChar;
	vec2 FuturePos[GRENADE_DODGE_TICKS];
	int LocalID = m_pClient->m_Snap.m_LocalClientID;
	if (LocalID < 0 || LocalID >= MAX_CLIENTS || Client()->State() != IClient::STATE_ONLINE || m_pClient->m_Snap.m_pGameInfoObj == NULL || m_pClient->m_Snap.m_pLocalCharacter == NULL)
		return;

	int LocalTeam = 0;

	bool isTeam = false;

	if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS && m_pClient->m_Snap.m_pLocalInfo)
	{
		isTeam = true;
		LocalTeam = m_pClient->m_Snap.m_pLocalInfo->m_Team;
	}

	int Weapon = m_pClient->m_Snap.m_pLocalCharacter->m_Weapon;

	m_pClient->m_aClients[LocalID].m_Predicted.Write(&LocalChar);

	vec2 LocalPos = vec2(LocalChar.m_X, LocalChar.m_Y);

	if(Weapon == WEAPON_GRENADE)
	{
		vec2 EnemyPos[MAX_CLIENTS][90];
		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			CWorldCore TempWorld;
			CCharacterCore CharCores;
			CNetObj_CharacterCore TempCore;
			mem_zero(&CharCores, sizeof(CharCores));
			if (!m_pClient->m_Snap.m_aCharacters[i].m_Active || i == LocalID || (isTeam && LocalTeam == m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team))
				continue;

			CharCores.Init(&TempWorld, m_pClient->Collision());
			m_pClient->m_aClients[i].m_Predicted.Write(&TempCore);
			CharCores.Read(&TempCore);

			//fill known input
			CharCores.m_Input.m_Direction = TempCore.m_Direction;

			for (int d = 0; d < 90; d++)
			{
				CharCores.Tick(true);
				CharCores.Move();
				CharCores.Quantize();
				EnemyPos[i][d] = CharCores.m_Pos;
			}
		}


		float Curvature = m_pClient->m_Tuning.m_GrenadeCurvature;
		float Speed = m_pClient->m_Tuning.m_GrenadeSpeed;

		for (int i = 0; i < s_NumDirection; i++)
			m_DirectionHit[i] = -1;

		int DirectionID = 0;
		for (float i = 0; i < 2 * pi; i += 0.01f, DirectionID++)
		{
			vec2 Direction = normalize(vec2(sinf(i), cosf(i)));

			for (int d = 1; d < 18; d++)
			{
				float Tick = d / 10.0f;
				vec2 ProjStartPos = LocalPos + Direction*28.0f*0.75f;
				vec2 PrevPos = CalcPos(ProjStartPos, Direction, Curvature, Speed, Tick - 1 / 10.0f);
				vec2 Pos = CalcPos(ProjStartPos, Direction, Curvature, Speed, Tick);

				if (Collision()->CheckPoint(Pos.x, Pos.y))//Collision()->IntersectLine(Pos, PrevPos, NULL, NULL))
					break;

				for (int p = 0; p < MAX_CLIENTS; p++)
				{
					if (!m_pClient->m_Snap.m_aCharacters[p].m_Active || p == LocalID)
						continue;

					//intersection
					vec2 DifPos = EnemyPos[p][d]-Pos;
					if (abs(DifPos.x) > 28 + 6 + 4 || abs(DifPos.y) > 28 + 6 + 4)//if one coordinate is over range
						continue;
					//if (DifPos.x*DifPos.x + DifPos.y * DifPos.y > 128)
						//continue;

					vec2 IntersectPos = closest_point_on_line(PrevPos, Pos, EnemyPos[p][d]);
					float Len = distance(EnemyPos[p][d], IntersectPos);
					if (Len < 28)
					{
						m_DirectionHit[DirectionID] = d * 5;
						break;
					}
				}

				if(m_DirectionHit[DirectionID] != -1)
					break;
			}
		}

		Graphics()->TextureSet(-1);
		Graphics()->LinesBegin();
	
		DirectionID = 0;
		for (float i = 0; i < 2 * pi; i += 0.01f, DirectionID++)
		{
			vec2 Direction = normalize(vec2(sinf(i), cosf(i)));
			if (m_DirectionHit[DirectionID] == -1)
				continue;

			float Color = m_DirectionHit[DirectionID]/50.0f;
			Graphics()->SetColor(1- Color, 0.1f, 0.1f, 0.75f);

			for (int d = 0; d < 30; d++)
			{
				float Tick = d / (50.0f*0.3f);
				vec2 ProjStartPos = LocalPos + Direction*28.0f*0.75f;
				vec2 Pos = CalcPos(ProjStartPos, Direction, Curvature, Speed, Tick);
				vec2 NextPos = CalcPos(ProjStartPos, Direction, Curvature, Speed, Tick + 1 / (50.0f*0.3f));

				if (Collision()->IntersectLine(Pos, NextPos, NULL, NULL))
					break;

				IGraphics::CLineItem Line = IGraphics::CLineItem(NextPos.x, NextPos.y, Pos.x, Pos.y);
				Graphics()->LinesDraw(&Line, 1);
			}
		}

		Graphics()->LinesEnd();
	}
	else if(Weapon == WEAPON_RIFLE || Weapon == WEAPON_GUN || Weapon == WEAPON_SHOTGUN)
	{
		float MaxDistance = 600.0f;
		if(Weapon == WEAPON_RIFLE)
			MaxDistance = m_pClient->m_Tuning.m_LaserReach;

		for (int i = 0; i < s_NumDirection; i++)
			m_DirectionHit[i] = -1;

		int DirectionID = 0;
		for (float i = 0; i < 2 * pi; i += 0.01f, DirectionID++)
		{
			vec2 Direction = normalize(vec2(sinf(i), cosf(i)));
			vec2 EndPos = LocalPos + Direction * MaxDistance;
			Collision()->IntersectLine(LocalPos, EndPos, NULL, &EndPos);

			for (int i = 0; i < MAX_CLIENTS; i++)
			{
				CWorldCore TempWorld;
				CNetObj_CharacterCore TempCore;
				if (!m_pClient->m_Snap.m_aCharacters[i].m_Active || i == LocalID || (isTeam && LocalTeam == m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team))
					continue;

				m_pClient->m_aClients[i].m_Predicted.Write(&TempCore);
				vec2 EnemyPos = vec2(TempCore.m_X, TempCore.m_Y);

				float Len = distance(LocalPos, EnemyPos);
				if (Len > MaxDistance)
					continue;

				vec2 Closest = closest_point_on_line(LocalPos, EndPos, EnemyPos);

				if (distance(Closest, EnemyPos) < 12.0f)
				{
					m_DirectionHit[DirectionID] = Len;
					break;
				}

			}
		}

		Graphics()->TextureSet(-1);
		Graphics()->LinesBegin();
	
		DirectionID = 0;
		for (float i = 0; i < 2 * pi; i += 0.01f, DirectionID++)
		{
			vec2 Direction = normalize(vec2(sinf(i), cosf(i)));
			if (m_DirectionHit[DirectionID] == -1)
				continue;

			float Color = m_DirectionHit[DirectionID]/MaxDistance;
			Graphics()->SetColor(1- Color, 0.1f, 0.1f, 0.75f);
			vec2 EndPos = LocalPos + Direction * MaxDistance;

			Collision()->IntersectLine(LocalPos, EndPos, NULL, &EndPos);

			IGraphics::CLineItem Line = IGraphics::CLineItem(EndPos.x, EndPos.y, LocalPos.x, LocalPos.y);
			Graphics()->LinesDraw(&Line, 1);
		}

		Graphics()->LinesEnd();
	}
}

bool CControls::OnMouseMove(float x, float y)
{
	if((m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED) ||
		(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_pChat->IsActive()))
		return false;

	m_MousePos += vec2(x, y); // TODO: ugly
	ClampMousePos();

	return true;
}

void CControls::ClampMousePos()
{
	if(m_pClient->m_Snap.m_SpecInfo.m_Active && !m_pClient->m_Snap.m_SpecInfo.m_UsePosition)
	{
		m_MousePos.x = clamp(m_MousePos.x, 200.0f, Collision()->GetWidth()*32-200.0f);
		m_MousePos.y = clamp(m_MousePos.y, 200.0f, Collision()->GetHeight()*32-200.0f);

	}
	else
	{
		float CameraMaxDistance = 200.0f;
		float FollowFactor = g_Config.m_ClMouseFollowfactor/100.0f;
		float MouseMax = min(CameraMaxDistance/FollowFactor + g_Config.m_ClMouseDeadzone, (float)g_Config.m_ClMouseMaxDistance);

		if(length(m_MousePos) > MouseMax)
			m_MousePos = normalize(m_MousePos)*MouseMax;
	}
}


void CControls::SetInput()
{
	if(m_FakeInput < NUM_FAKEINPUTDATA)
		m_FakeInput = FAKEINPUT_NONE;

	for(int i = 0; i < NUM_FAKEINPUTDATA; i++)
	{
		if(m_FakeInputData[i])
			m_FakeInput = i;
	}

	if(m_LastFakeInput != m_FakeInput)
	{
		/*m_InputData.m_Jump = 0;
		if(m_LastFakeInput != FAKEINPUT_BLOCK)
		m_InputData.m_Hook = 0;*/
	}

	switch(m_FakeInput)
	{
	case FAKEINPUT_NONE: InputNormal(); break;
	case FAKEINPUT_SWIFT: Swift(); break;
	case FAKEINPUT_FLY: Fly(); break;
	case FAKEINPUT_BLOCK: Block(); break;
	case FAKEINPUT_BLOCK_SET: BlockSet(); break;
	case FAKEINPUT_SAVEJUMP: SaveJump(); break;
	case FAKEINPUT_AUTOUNFREEZE: AutoUnfreeze(); break;
	case FAKEINPUT_GRENADEAIM: GrenadeAim(); break;
	case FAKEINPUT_STEP: Step(); break;
	case FAKEINPUT_VIBRATE: Vibrate(); break;
	};

	if(g_Config.m_PdlBlockHelp)
	{
		m_pClient->m_pBlockHelp->SnapInput(&m_InputData);
	}
	else if(g_Config.m_PdlAutorunActive)
	{
		int LocalID = m_pClient->m_Snap.m_LocalClientID;
		m_pClient->m_pAutoRun->SnapInput(&m_InputData, LocalID, -1);
	}

	if(g_Config.m_PdlGrenadeDodge)
		GrenadeDodge();

	m_LastFakeInput = m_FakeInput;
}

void CControls::JoystickInput()
{
	float ActionAxe = Input()->JoystickAxes(2);
	float ActionAxe2 = Input()->JoystickAxes(3);
	float MoveAxe = Input()->JoystickAxes(0);
	float MouseAxeY = Input()->JoystickAxes(1);
	float MouseAxeQX = Input()->JoystickAxes(5);
	float MouseAxeQY = Input()->JoystickAxes(4);
	float JoystickSens = g_Config.m_JsSens/100.0f;

	//move
	if(MoveAxe > 0.5f)
		m_InputData.m_Direction = 1;
	else if(MoveAxe < -0.5f)
		m_InputData.m_Direction = -1;

	//mouse
	//if(fabs(MouseAxeQX) > 0.1f)
		m_MousePos.x = MouseAxeQX*300.0f;
	//if(fabs(MouseAxeQY) > 0.1f)
		m_MousePos.y = MouseAxeQY*300.0f;

	ClampMousePos();

	//jump
	m_InputData.m_Jump = ActionAxe > 0.5f/*|MouseAxeY < -0.5f*/;

	//hook
	m_InputData.m_Hook = ActionAxe2 > 0.5f;

	//dbg_msg(0, "%i %i", m_InputData.m_NextWeapon, m_InputData.m_PrevWeapon);

	static bool s_NextPressed = false;
	if(Input()->JoystickButton(2))
	{
		if(s_NextPressed == false)
		{
			m_InputData.m_NextWeapon += 2;
			s_NextPressed = true;
		}
	}
	else
		s_NextPressed = false;

	static bool s_PrevPressed = false;
	if(Input()->JoystickButton(1))
	{
		if(s_PrevPressed == false)
		{
			m_InputData.m_PrevWeapon += 2;
			s_PrevPressed = true;
		}
	}
	else
		s_PrevPressed = false;

	//fire
	if(Input()->JoystickButton(5) | Input()->JoystickButton(4))
	{
		if(m_InputData.m_Fire%2 == 0)
			m_InputData.m_Fire++;
	}
	else
	{
		if(m_InputData.m_Fire%2 == 1)
			m_InputData.m_Fire++;
	}

	static bool s_KillSent = false;
	if(Input()->JoystickButton(3))
	{
		if(s_KillSent == false)
		{
			m_pClient->SendKill(-1);
			s_KillSent = true;
		}
	}
	else
		s_KillSent = false;
}


//void CControls::JoystickInput()
//{
//	float MoveAxe = Input()->JoystickAxes(2);
//	float MouseAxeX = Input()->JoystickAxes(4);
//	float MouseAxeY = Input()->JoystickAxes(3);
//	float MouseAxeQX = Input()->JoystickAxes(0);
//	float MouseAxeQY = Input()->JoystickAxes(1);
//	float JoystickSens = g_Config.m_JsSens/100.0f;
//
//	//move
//	if(MoveAxe > 0.5f)
//		m_InputData.m_Direction = -1;
//	else if(MoveAxe < -0.5f)
//		m_InputData.m_Direction = 1;
//
//	//mouse
//	if(fabs(MouseAxeX) > 0.6f)
//		m_MousePos.x += MouseAxeX*JoystickSens;
//	if(fabs(MouseAxeY) > 0.6f)
//		m_MousePos.y += MouseAxeY*JoystickSens;
//
//	if(fabs(MouseAxeQX) > 0.1f)
//		m_MousePos.x = MouseAxeQX*300.0f;
//	if(fabs(MouseAxeQY) > 0.1f)
//		m_MousePos.y = MouseAxeQY*300.0f;
//
//	ClampMousePos();
//
//	//jump
//	m_InputData.m_Jump = Input()->JoystickButton(0);
//
//	//hook
//	m_InputData.m_Hook = Input()->JoystickButton(2);
//
//	//dbg_msg(0, "%i %i", m_InputData.m_NextWeapon, m_InputData.m_PrevWeapon);
//
//	static bool s_NextPressed = false;
//	if(Input()->JoystickButton(4))
//	{
//		if(s_NextPressed == false)
//		{
//			m_InputData.m_NextWeapon += 2;
//			s_NextPressed = true;
//		}
//	}
//	else
//		s_NextPressed = false;
//
//	static bool s_PrevPressed = false;
//	if(Input()->JoystickButton(5))
//	{
//		if(s_PrevPressed == false)
//		{
//			m_InputData.m_PrevWeapon += 2;
//			s_PrevPressed = true;
//		}
//	}
//	else
//		s_PrevPressed = false;
//
//	//fire
//	if(Input()->JoystickButton(1))
//	{
//		if(m_InputData.m_Fire%2 == 0)
//			m_InputData.m_Fire++;
//	}
//	else
//	{
//		if(m_InputData.m_Fire%2 == 1)
//			m_InputData.m_Fire++;
//	}
//
//	static bool s_KillSent = false;
//	if(Input()->JoystickButton(3))
//	{
//		if(s_KillSent == false)
//		{
//			m_pClient->SendKill(-1);
//			s_KillSent = true;
//		}
//	}
//	else
//		s_KillSent = false;
//}

void CControls::InputNormal()
{
	m_InputData.m_TargetX = (int)m_MousePos.x;
	m_InputData.m_TargetY = (int)m_MousePos.y;
	if(!m_InputData.m_TargetX && !m_InputData.m_TargetY)
	{
		m_InputData.m_TargetX = 1;
		m_MousePos.x = 1;
	}

	// set direction
	m_InputData.m_Direction = 0;
	if(m_InputDirectionLeft && !m_InputDirectionRight)
		m_InputData.m_Direction = -1;
	if(!m_InputDirectionLeft && m_InputDirectionRight)
		m_InputData.m_Direction = 1;

	if(g_Config.m_Josystick)
		JoystickInput();

	if(g_Config.m_PdlInputlockActive == 0 && m_InputLocked == true)
		m_InputLocked = false;

	if(m_InputLocked == true)
	{
		mem_copy(&m_InputData, &m_LockedInput, sizeof(m_InputData));
		vec2 MousePos = normalize(vec2(m_LockedInput.m_TargetX, m_LockedInput.m_TargetY)) * (250 + sinf(time_get() / time_freq()) * 200);
		m_InputData.m_TargetX = MousePos.x; m_InputData.m_TargetY = MousePos.y;
	}

	// stress testing
	if(g_Config.m_DbgStress)
	{
		float t = Client()->LocalTime();
		mem_zero(&m_InputData, sizeof(m_InputData));

		m_InputData.m_Direction = ((int)t/2)&1;
		m_InputData.m_Jump = ((int)t);
		m_InputData.m_Fire = ((int)(t*10));
		m_InputData.m_Hook = ((int)(t*2))&1;
		m_InputData.m_WantedWeapon = ((int)t)%NUM_WEAPONS;
		m_InputData.m_TargetX = (int)(sinf(t*3)*100.0f);
		m_InputData.m_TargetY = (int)(cosf(t*3)*100.0f);
	}
}

void CControls::Swift()
{
	InputNormal();

	if(m_LastFakeInput != FAKEINPUT_SWIFT)
		m_SwiftDirection = m_SwiftLastMove;

	if(m_InputDirectionLeft)
		m_SwiftLastMove = -1;
	else if(m_InputDirectionRight)
		m_SwiftLastMove = 1;

	m_InputData.m_Direction = m_SwiftDirection;
	m_SwiftDirection = m_SwiftDirection==1?-1:1;
}

bool CControls::FlyCollidingTee(vec2 From, vec2 To)
{
	if(m_pClient->m_Tuning.m_PlayerHooking == 0)
		return false;

	int LocalID = m_pClient->m_Snap.m_LocalClientID;
	if(LocalID < 0 || LocalID >= MAX_CLIENTS)
		return false;

	for(int i = 0, j = 0; i < MAX_CLIENTS; i++)
	{
		vec2 EnemyPos;
		CGameClient::CSnapState::CCharacterInfo EnemyChar;
		const void *pInfo = NULL;

		if(!m_pClient->m_Snap.m_aCharacters[i].m_Active || i == LocalID)
			continue;

		EnemyChar = m_pClient->m_Snap.m_aCharacters[i];
		pInfo = Client()->SnapFindItem(IClient::SNAP_CURRENT, NETOBJTYPE_PLAYERINFO, i);
		//EnemyVel = mix(vec2(EnemyChar.m_Prev.m_VelX, EnemyChar.m_Prev.m_VelY), vec2(EnemyChar.m_Cur.m_VelX, EnemyChar.m_Cur.m_VelY), Client()->IntraGameTick()) * (LocalPing*(50 / 1000) * 2) ;
		EnemyPos = mix(vec2(EnemyChar.m_Prev.m_X, EnemyChar.m_Prev.m_Y), vec2(EnemyChar.m_Cur.m_X, EnemyChar.m_Cur.m_Y), Client()->IntraGameTick());

		vec2 Closest = closest_point_on_line(From, To, EnemyPos);
			
		if(distance(Closest, EnemyPos) <= 40)
			return true;
	}
	return false;
}

int CControls::FlybotGetWallPos(vec2 TeePos, vec2 *WallTop, vec2 *WallBot)
{
	float ClosestWallTop = 320.0f;
	float ClosestWallBot = m_pClient->m_Tuning.m_HookLength;
	int FoundWalls = 0;
	
	for(float i = 0; i < pi*0.5-0.7f; i += 0.01f)
	{
		vec2 TempWallPos;
		vec2 PosTopRight = TeePos+vec2(sinf(i), cosf(i+pi))*ClosestWallTop;
		vec2 PosTopLeft = TeePos+vec2(-sinf(i), cosf(i+pi))*ClosestWallTop;
		vec2 PosBotRight = TeePos+vec2(sinf(i), cosf(i+2.0f*pi))*ClosestWallBot;
		vec2 PosBotLeft = TeePos+vec2(-sinf(i), cosf(i+2.0f*pi))*ClosestWallBot;
		if((FoundWalls&1) == 0 && FlyCollidingTee(TeePos, PosTopRight) == false)
		{
			vec2 PosTopRightEdge = TeePos+vec2(sinf(i-0.05f), cosf(i-0.05f+pi))*ClosestWallTop;//no edges
			if(Collision()->IntersectLine(TeePos, PosTopRight, 0x0, &TempWallPos) == 1 &&
				Collision()->IntersectLine(TeePos, PosTopRightEdge, 0x0, NULL) == 1)
			{
				float dist = distance(TeePos, TempWallPos);
				if(dist < ClosestWallTop)
				{
					ClosestWallTop = dist;
					FoundWalls |= 1;
					if(WallTop)
						*WallTop = TempWallPos;
				}
			}
		}

		if((FoundWalls&1) == 0 && FlyCollidingTee(TeePos, PosTopLeft) == false)
		{
			vec2 PosTopLeftEdge = TeePos+vec2(-sinf(i-0.05f), cosf(i-0.05f))*ClosestWallTop;
			if(Collision()->IntersectLine(TeePos, PosTopLeft, 0x0, &TempWallPos) == 1 &&
				Collision()->IntersectLine(TeePos, PosTopLeftEdge, 0x0, NULL) == 1)
			{
				float dist = distance(TeePos, TempWallPos);
				if(dist < ClosestWallTop)
				{
					ClosestWallTop = dist;
					FoundWalls |= 1;
					if(WallTop)
						*WallTop = TempWallPos;
				}
			}
		}

		if((FoundWalls&2) == 0 && FlyCollidingTee(TeePos, PosBotRight) == false)
		{
			if(Collision()->IntersectLine(TeePos, PosBotRight, 0x0, &TempWallPos) == 1)
			{
				float dist = distance(TeePos, TempWallPos);
				if(dist < ClosestWallBot)
				{
					ClosestWallBot = dist;
					FoundWalls |= 2;
					if(WallBot)
						*WallBot = TempWallPos;
				}
			}
		}

		if((FoundWalls&2) == 0 && FlyCollidingTee(TeePos, PosBotLeft) == false)
		{
			if(Collision()->IntersectLine(TeePos, PosBotLeft, 0x0, &TempWallPos) == 1)
			{
				float dist = distance(TeePos, TempWallPos);
				if(dist < ClosestWallBot)
				{
					ClosestWallBot = dist;
					FoundWalls |= 2;
					if(WallBot)
						*WallBot = TempWallPos;
				}
			}
		}
	}

	return FoundWalls;
}

void CControls::Fly()
{
	int LocalID = m_pClient->m_Snap.m_LocalClientID;
	if(LocalID < 0 || LocalID >= MAX_CLIENTS)
		return;

	CNetObj_CharacterCore LocalChar;
	m_pClient->m_aClients[LocalID].m_Predicted.Write(&LocalChar);
	vec2 LocalVel = vec2(LocalChar.m_VelX, LocalChar.m_VelY);
	vec2 LocalPos = vec2(LocalChar.m_X, LocalChar.m_Y);
	vec2 PosDif = m_FlyPos-LocalPos;
	float PhysSize = 28.0f;
	static float s_VelAbscissaBorder = 400;

	vec2 WallPosTop = vec2(0, 0);
	vec2 WallPosBot = vec2(0, 0);
	int FoundWall = FlybotGetWallPos(LocalPos, &WallPosTop, &WallPosBot);

	if(m_LastFakeInput != FAKEINPUT_FLY)
		m_FlyPos = LocalPos;

	if((FoundWall&1) && ((PosDif.y < 0 && LocalVel.y > 0) || (PosDif.y < -50 && LocalVel.y > -500)))
	{// hook
		m_InputData.m_Hook = 1;

		vec2 Dir = normalize(WallPosTop-LocalPos);
		m_InputData.m_TargetX = Dir.x*1000;
		m_InputData.m_TargetY = Dir.y*1000;

		if(LocalChar.m_HookY > LocalPos.y && LocalChar.m_HookState == HOOK_GRABBED)
			m_InputData.m_Hook = 0;
	}
	else if((FoundWall&2) && LocalVel.y < -700)
	{
		m_InputData.m_Hook = 1;

		vec2 Dir = normalize(WallPosBot-LocalPos);
		m_InputData.m_TargetX = Dir.x*1000;
		m_InputData.m_TargetY = Dir.y*1000;
		if(LocalChar.m_HookY < LocalPos.y && LocalChar.m_HookState == HOOK_GRABBED)
			m_InputData.m_Hook = 0;
	}
	else
		m_InputData.m_Hook = 0;

	if((LocalChar.m_HookedPlayer != -1 && LocalChar.m_HookState == HOOK_GRABBED) || LocalChar.m_HookState == HOOK_RETRACTED)
		m_InputData.m_Hook = 0;

	if(abs(LocalVel.x) > s_VelAbscissaBorder && abs(LocalVel.y) > 700)
	{
		m_InputData.m_Direction = LocalVel.x > 0? -1: 1;
	}
	else if(abs(PosDif.x) > 4)
	{
		if(PosDif.x < -4)//maybe a bit tolerance
			m_InputData.m_Direction = -1;
		else if(PosDif.x > 4)
			m_InputData.m_Direction = 1;
		else
			m_InputData.m_Direction = 0;
	}
	else
	{
		m_InputData.m_Direction = LocalVel.x > 0? -1: 1;
	}

	// get ground state
	bool Grounded = false;
	if(Collision()->CheckPoint(LocalPos.x+PhysSize/2, LocalPos.y+PhysSize/2+5))
		Grounded = true;
	if(Collision()->CheckPoint(LocalPos.x-PhysSize/2, LocalPos.y+PhysSize/2+5))
		Grounded = true;

	if(Grounded && PosDif.y < -50)
		m_InputData.m_Jump = 1;
	else
		m_InputData.m_Jump = 0;

	//if(m_InputFlybot && TimeDif > time_freq()*0.5f)
	if(m_pClient->m_pChat->IsActive() == false && m_pClient->m_pMenus->IsActive() == false)
	{
		vec2 NewPos = vec2(0, 0);
		if(Input()->KeyPressed('w'))
			NewPos = vec2(m_FlyPos.x, m_FlyPos.y-16);
		if(Input()->KeyPressed('s'))
			NewPos = vec2(m_FlyPos.x, m_FlyPos.y+16);
		if(Input()->KeyPressed('a'))
			NewPos = vec2(m_FlyPos.x-16, m_FlyPos.y);
		if(Input()->KeyPressed('d'))
			NewPos = vec2(m_FlyPos.x+16, m_FlyPos.y);

		if(Input()->KeyPressed(KEY_MOUSE_2))
		{
			NewPos = LocalPos+m_MousePos;
		}
		if(NewPos.x != 0.0f || NewPos.y != 0.0f)
		{//check if we have space to hook in the new Pos
			int Walls = FlybotGetWallPos(NewPos, NULL, NULL);
			if((Walls&1) == 1)//Found Wall on top
				m_FlyPos = NewPos;
		}
	}

	if(m_InputData.m_Fire&1)
	{
		m_InputData.m_Hook = 0;
		m_InputData.m_TargetX = (int)m_MousePos.x;
		m_InputData.m_TargetY = (int)m_MousePos.y;
		m_InputData.m_Fire++;
	}
}

void CControls::Block()
{
	if(m_BlockingPlayer == -1)
		return;

	CNetObj_Character pCurChar = m_pClient->m_Snap.m_aCharacters[m_BlockingPlayer].m_Cur;
	vec2 Pos = vec2(pCurChar.m_X,pCurChar.m_Y);
	vec2 OwnPos = m_pClient->m_LocalCharacterPos;
	static int s_LastHookState = m_InputData.m_Hook;

	if(m_InputData.m_Hook && !s_LastHookState && m_pClient->m_Snap.m_aCharacters[m_BlockingPlayer].m_Active)
	{
		m_MousePos = normalize(Pos-OwnPos)*length(m_MousePos);
	}

	s_LastHookState = m_InputData.m_Hook;

	InputNormal();
}

void CControls::BlockSet()
{
	InputNormal();

	static int s_PressedTick = 0;
	int OwnID = m_pClient->m_Snap.m_LocalClientID;

	if(m_LastFakeInput != FAKEINPUT_BLOCK_SET)
	{
		s_PressedTick = 0;
		return;
	}

	if(!s_PressedTick)
	{
		int ClosestPlayer = -1;
		int ClosestDist = 128;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!m_pClient->m_Snap.m_aCharacters[i].m_Active || i == OwnID)
				continue;

			CNetObj_Character pCurChar = m_pClient->m_Snap.m_aCharacters[i].m_Cur;
			vec2 EnemyPos = vec2(pCurChar.m_X,pCurChar.m_Y);
			int Dist = distance(EnemyPos, m_pClient->m_LocalCharacterPos + m_MousePos);

			if(Dist < ClosestDist)
			{
				ClosestPlayer = i;
				ClosestDist = Dist;
			}
		}

		if(ClosestPlayer != -1)
		{
			m_BlockingPlayer = ClosestPlayer;
		}
	}

	if(s_PressedTick == 20)
	{
		m_BlockingPlayer = -1;
	}
	else
		s_PressedTick++;
}

void CControls::SaveJump()
{
	InputNormal();

	if(Client()->IsDDRace() == false)
		return;

	int LocalID = m_pClient->m_Snap.m_LocalClientID;
	if(LocalID < 0 || LocalID >= MAX_CLIENTS)
		return;

	//int LocalPing = m_pClient->m_Snap.m_paPlayerInfos[LocalID]->m_Latency;
	CNetObj_CharacterCore LocalChar;
	m_pClient->m_aClients[LocalID].m_Predicted.Write(&LocalChar);
	vec2 LocalPos = vec2(LocalChar.m_X, LocalChar.m_Y);
	vec2 LocalVel = vec2(LocalChar.m_VelX, LocalChar.m_VelY);

	LocalPos.y += g_Config.m_PdlSaveJumpShift;

	int Tile = m_pClient->Collision()->GetCollisionAt(LocalPos.x, LocalPos.y);

	static bool ReleaseJump = false;

	if(ReleaseJump)
	{
		m_InputData.m_Jump = 0;
		ReleaseJump = false;
	}

	if(Tile == 9 && m_InputData.m_Jump == 0)
	{
		m_InputData.m_Jump = 1;
		ReleaseJump = true;

		if(g_Config.m_PdlSaveJumpShoot)
		{
			m_InputData.m_TargetX = 0;
			m_InputData.m_TargetY = -128;
			m_InputData.m_Fire += 2;
		}
	}
}

void CControls::AutoUnfreeze()
{
	InputNormal();


	float LaserReach = m_pClient->m_Tuning.m_LaserReach;
	CNetObj_CharacterCore LocalChar;
	int LocalID = m_pClient->m_Snap.m_LocalClientID;
	if(LocalID < 0 || LocalID >= MAX_CLIENTS)
		return;

	m_pClient->m_aClients[LocalID].m_Predicted.Write(&LocalChar);
	vec2 LocalPos = vec2(LocalChar.m_X, LocalChar.m_Y);
	vec2 LocalVel = vec2(LocalChar.m_VelX / 256.0f, LocalChar.m_VelY / 256.0f);

	if(m_pClient->m_Snap.m_pLocalCharacter && m_pClient->m_Snap.m_pLocalCharacter->m_Weapon != WEAPON_RIFLE)
		return;

	CWorldCore TempWorld;
	CCharacterCore TempCore;
	mem_zero(&TempCore, sizeof(TempCore));
	TempCore.Init(&TempWorld, m_pClient->Collision());
	TempCore.Read(&LocalChar);

	int Tile = m_pClient->Collision()->GetCollisionAt(LocalPos+LocalVel);
	if(Tile != 9)
		return;

	int FreezeState = 0;

	for(int i = 0; i < round(m_pClient->m_Tuning.m_LaserBounceDelay/20.0f); i++)//
	{
		LocalChar.m_Tick++;
		TempCore.Tick(false);
		TempCore.Move();
		TempCore.Quantize();

		int FutureTile = m_pClient->Collision()->GetCollisionAt(TempCore.m_Pos);

		if(FutureTile == 9)
			FreezeState = 1;
		else if(FreezeState == 1)
			FreezeState = 2;
	}

	TempCore.Write(&LocalChar);

	m_OutFreezePos = vec2(LocalChar.m_X, LocalChar.m_Y);

	if(FreezeState != 2)
		return;//no chance to unfreeze

	float ClosestLen = 28.0f;
	bool Found = false;
	vec2 BestCyclePos;

	for(float a = 0; a < 2 * pi; a += pi/180.0f)
	{
		vec2 CyclePos = LocalPos + vec2(cosf(a), sinf(a)) * LaserReach;
		
		vec2 WallPos =  vec2(0, 0);
		vec2 From = LocalPos;
		vec2 To = CyclePos;

		if(!Collision()->IntersectLine(From, To, 0x0, &WallPos))
			continue;//no wall found
		
		int WallDistance = distance(From, WallPos);
		int RestDistance = LaserReach - WallDistance;
		
		if(WallDistance > LaserReach || RestDistance <= 0)
			continue;
		
		vec2 TempPos = WallPos;
		vec2 TempDir = normalize(WallPos - From)*4.0f;
		
		Collision()->MovePoint(&TempPos, &TempDir, 1.0f, 0); // 0.0f = 1.0f
		From = TempPos;
		To = WallPos + normalize(TempDir) * (RestDistance - m_pClient->m_Tuning.m_LaserBounceCost);
		
		vec2 IntersectPos = closest_point_on_line(From, To, m_OutFreezePos);
		float Dist = distance(m_OutFreezePos, IntersectPos);
		if(Dist < ClosestLen)
		{
			ClosestLen = Dist;
			BestCyclePos = CyclePos;
			Found = true;
		}
	}

	if(Found)
	{
		m_MousePos = normalize(BestCyclePos-LocalPos)*length(m_MousePos);
		m_InputData.m_TargetX = (int)m_MousePos.x;
		m_InputData.m_TargetY = (int)m_MousePos.y;
		m_InputData.m_Fire+=2;
	}
}

bool CControls::GrenadeWillHit(CNetObj_PlayerInput Input, vec2 *pOut)
{
	CNetObj_CharacterCore LocalChar;
	vec2 FuturePos[GRENADE_DODGE_TICKS];
	int LocalID = m_pClient->m_Snap.m_LocalClientID;
	if (LocalID < 0 || LocalID >= MAX_CLIENTS)
		return false;

	m_pClient->m_aClients[LocalID].m_Predicted.Write(&LocalChar);

	CWorldCore TempWorld;
	CCharacterCore TempCore;

	mem_zero(&TempCore, sizeof(TempCore));
	TempCore.Init(&TempWorld, m_pClient->Collision());
	TempCore.Read(&LocalChar);

	TempCore.m_Input = Input;

	for (int i = 1; i < GRENADE_DODGE_TICKS; i++)
	{
		LocalChar.m_Tick++;
		TempCore.Tick(true);
		TempCore.Move();
		TempCore.Quantize();

		FuturePos[i] = TempCore.m_Pos;
	}


	int Num = Client()->SnapNumItems(IClient::SNAP_CURRENT);
	for (int i = 0; i < Num; i++)
	{
		IClient::CSnapItem Item;
		const void *pData = Client()->SnapGetItem(IClient::SNAP_CURRENT, i, &Item);

		if (Item.m_Type != NETOBJTYPE_PROJECTILE)
			continue;

		const CNetObj_Projectile *pCurrent = (const CNetObj_Projectile *)pData;

		if (pCurrent->m_Type != WEAPON_GRENADE)
			continue;

		float Curvature = m_pClient->m_Tuning.m_GrenadeCurvature;
		float Speed = m_pClient->m_Tuning.m_GrenadeSpeed;

		static float s_LastGameTickTime = Client()->GameTickTime();
		if (m_pClient->m_Snap.m_pGameInfoObj && !(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_PAUSED))
			s_LastGameTickTime = Client()->GameTickTime();
		float Ct = (Client()->PrevGameTick() - pCurrent->m_StartTick) / (float)SERVER_TICK_SPEED + s_LastGameTickTime;
		if (Ct < 0.0f)
			break; // projectile havn't been shot yet

		vec2 StartPos(pCurrent->m_X, pCurrent->m_Y);
		vec2 StartVel(pCurrent->m_VelX / 100.0f, pCurrent->m_VelY / 100.0f);

		for (int i = 1; i < GRENADE_DODGE_TICKS; i++)
		{
			
			if (distance(StartPos, m_LastFirePos) <= 80.0f)//own grenade
				continue;

			float Tick = Ct + i / 50.0f;
			vec2 Pos = CalcPos(StartPos, StartVel, Curvature, Speed, Tick);
			vec2 PrevPos = CalcPos(StartPos, StartVel, Curvature, Speed, Tick - 0.001f);

			//intersection
			vec2 IntersectPos = closest_point_on_line(PrevPos, Pos, FuturePos[i]);
			float Len = distance(FuturePos[i], IntersectPos);
			if (Len < 28 + 6)
			{
				if(pOut)
					*pOut = Pos;

				return true;
			}
		}
	}

	return false;
}

void CControls::GrenadeDodge()
{
	CNetObj_CharacterCore LocalChar;
	int LocalID = m_pClient->m_Snap.m_LocalClientID;
	if (LocalID < 0 || LocalID >= MAX_CLIENTS || Client()->State() != IClient::STATE_ONLINE)
		return;

	m_pClient->m_aClients[LocalID].m_Predicted.Write(&LocalChar);
	vec2 LocalPos = vec2(LocalChar.m_X, LocalChar.m_Y);

	static bool s_ReleaseJump = false;
	if (s_ReleaseJump)
	{
		m_InputData.m_Jump = false;
		s_ReleaseJump = false;
	}

	if (m_InputData.m_Fire & 1)
		m_LastFirePos = LocalPos;

	//InputNormal();

	if (Client()->State() < IClient::STATE_ONLINE)
		return;

	CNetObj_PlayerInput Input;
	Input = m_InputData;
	if (GrenadeWillHit(Input, &m_LastGrenadePos) == false)//wont be hit
		return;

	m_GrenadeDangerous = false;

	if (Input.m_Jump == false)
	{
		Input.m_Jump = true;
		if (GrenadeWillHit(Input, NULL) == false)//jump to dodge
		{
			m_InputData.m_Jump = true;
			s_ReleaseJump = true;
			return;
		}
	}

	Input = m_InputData;
	if (Input.m_Direction != 0)
	{
		Input.m_Direction = 0;
		if (GrenadeWillHit(Input, NULL) == false)//stop moving to dodge
		{
			m_InputData.m_Direction = 0;
			return;
		}
	}

	Input = m_InputData;
	int CurDir = m_LastGrenadePos.x < LocalPos.x ? 1 : -1;

	if (Input.m_Direction != CurDir)
	{
		Input.m_Direction = CurDir;
		if (GrenadeWillHit(Input, NULL) == false)//move right to dodge
		{
			m_InputData.m_Direction = CurDir;
			return;
		}
	}

	Input = m_InputData;
	CurDir *= -1;
	if (Input.m_Direction != CurDir)
	{
		Input.m_Direction = CurDir;
		if (GrenadeWillHit(Input, NULL) == false)//move left to dodge
		{
			m_InputData.m_Direction = CurDir;
			return;
		}
	}

	m_GrenadeDangerous = true;
}

void CControls::GrenadeAim()
{
	static bool Fire = false;

	InputNormal();

	bool isTeam = false;

	if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
		isTeam = true;

	int MyAngle = GetAngle(vec2(m_InputData.m_TargetX, m_InputData.m_TargetY)) * 180.0f / pi + 90;
	bool FoundVeryClosePlayer = false;
	{
		CNetObj_CharacterCore LocalChar;
		int LocalID = m_pClient->m_Snap.m_LocalClientID;
		int LocalTeam = m_pClient->m_Snap.m_pLocalInfo->m_Team;
		if (LocalID >= 0 && LocalID < MAX_CLIENTS && Client()->State() == IClient::STATE_ONLINE)
		{

			m_pClient->m_aClients[LocalID].m_Predicted.Write(&LocalChar);

			vec2 LocalPos = vec2(LocalChar.m_X, LocalChar.m_Y);

			for (int i = 0, j = 0; i < MAX_CLIENTS; i++)
			{
				vec2 EnemyPos;
				CGameClient::CSnapState::CCharacterInfo EnemyChar;
				const void *pInfo = NULL;

				if (!m_pClient->m_Snap.m_aCharacters[i].m_Active || i == LocalID || (isTeam && LocalTeam != m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team))
					continue;

				EnemyChar = m_pClient->m_Snap.m_aCharacters[i];
				pInfo = Client()->SnapFindItem(IClient::SNAP_CURRENT, NETOBJTYPE_PLAYERINFO, i);
				//EnemyVel = mix(vec2(EnemyChar.m_Prev.m_VelX, EnemyChar.m_Prev.m_VelY), vec2(EnemyChar.m_Cur.m_VelX, EnemyChar.m_Cur.m_VelY), Client()->IntraGameTick()) * (LocalPing*(50 / 1000) * 2) ;
				EnemyPos = mix(vec2(EnemyChar.m_Prev.m_X, EnemyChar.m_Prev.m_Y), vec2(EnemyChar.m_Cur.m_X, EnemyChar.m_Cur.m_Y), Client()->IntraGameTick());

				if (distance(LocalPos, EnemyPos) <= 128)
				{
					int DirectionAngle = GetAngle(normalize(EnemyPos - LocalPos))* 180.0f / pi + 90;
					int AngleDist = abs((MyAngle - DirectionAngle + 180) % 360 - 180);
					if (AngleDist < 40)
					{
						float ClosestAngleRad = (DirectionAngle - 90) * pi / 180.0f;
						m_MousePos = vec2(cosf(ClosestAngleRad), sinf(ClosestAngleRad))*length(m_MousePos);
						Fire = true;
						FoundVeryClosePlayer = true;
					}
				}
			}
		}
	}

	if (Fire)
	{
		m_InputData.m_Fire += 2;
		Fire = false;
	}
	else if (m_InputData.m_Fire & 1)
	{
		

		if (FoundVeryClosePlayer == false)
		{
			int ClosestAngleDist = 720;
			int ClosestAngle = 0;
			int DirectionID = 0;
			for (float i = 0; i < 2 * pi; i += 0.01f, DirectionID++)
			{
				vec2 Direction = normalize(vec2(sinf(i), cosf(i)));
				if (m_DirectionHit[DirectionID] == -1)
					continue;

				int DirectionAngle = GetAngle(Direction)* 180.0f / pi + 90;

				int AngleDist = abs((MyAngle - DirectionAngle + 180) % 360 - 180);
				if (AngleDist < ClosestAngleDist)
				{
					ClosestAngleDist = AngleDist;
					ClosestAngle = DirectionAngle;
				}
			}

			//dbg_msg(0, "%i %i %i", ClosestAngleDist, ClosestAngle, MyAngle);

			if (ClosestAngleDist < 60)
			{
				float ClosestAngleRad = (ClosestAngle - 90) * pi / 180.0f;
				m_MousePos = vec2(cosf(ClosestAngleRad), sinf(ClosestAngleRad))*length(m_MousePos);
				Fire = true;
				m_InputData.m_Fire--;
			}
		}
	}
	else
	{

	}
}

void CControls::Step()
{
	InputNormal();

	static int s_State = 0;
	static int s_Direction = 0;
	static int64 s_PressTime;
	static int s_SwiftDir = 0;

	if (s_State == 0)
	{
		if (m_InputData.m_Direction)
			s_State = 1;

		s_Direction = m_InputData.m_Direction;
	}
	else if (s_State == 1)
	{
		s_PressTime = time_get();
		m_InputData.m_Direction = -s_Direction;
		s_State = 2;
		s_SwiftDir = 0;
	}
	else
	{
		if (m_InputData.m_Direction == 0)
			s_State = 0;

		if (s_State == 2)
		{
			if (m_InputData.m_Direction == 0)
				s_State = 0;

			m_InputData.m_Direction = 0;

			if (s_PressTime + time_freq() * 0.5f < time_get())
				s_State = 3;
		}
		else if (s_State == 3)
		{
			
			if (s_SwiftDir == 0)
				s_SwiftDir = s_Direction;
			else if (s_SwiftDir == s_Direction)
				s_SwiftDir = -s_Direction;
			else
				s_SwiftDir = 0;

			m_InputData.m_Direction = s_SwiftDir;
		}
	}
}

void CControls::Vibrate()
{
	static int s_State = 0;

	switch(s_State)
	{
		case 0: m_InputData.m_Direction = 1; break;
		case 1: m_InputData.m_Direction = -1; break;
		case 2: m_InputData.m_Direction = 0; break;
		case 3: m_InputData.m_Direction = -1; break;
		case 4: m_InputData.m_Direction = 1; break;
		case 5: m_InputData.m_Direction = 0; break;
	}

	s_State = (s_State +1)%6;
}