

#include <engine/textrender.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include "controls.h"

#include "blockhelp.h"


CBlockHelp::CBlockHelp()
{
	mem_zero(&HelpActive, sizeof(HelpActive));
}

bool CBlockHelp::WhouldTouchFreeze(CNetObj_PlayerInput Input, CNetObj_Character Char)
{
	CWorldCore TempWorld;
	CCharacterCore TempCore;

	mem_zero(&TempCore, sizeof(TempCore));
	TempCore.Init(&TempWorld, m_pClient->Collision());
	TempCore.Read(&Char);

	TempCore.m_Input = Input;

	for(int i = 0; i < 22; i++)//
	{
		Char.m_Tick++;
		TempCore.Tick(true);
		TempCore.Move();
		TempCore.Quantize();

		int FutureTile = m_pClient->Collision()->GetCollisionAt(TempCore.m_Pos);

		if(FutureTile == 9)
		{
			return true;
		}
	}

	return false;
}

bool CBlockHelp::WhouldTouchFreezeUp(CNetObj_PlayerInput Input, CNetObj_Character Char)
{
	CWorldCore TempWorld;
	CCharacterCore TempCore;

	mem_zero(&TempCore, sizeof(TempCore));
	TempCore.Init(&TempWorld, m_pClient->Collision());
	TempCore.Read(&Char);

	TempCore.m_Input = Input;

	for (int i = 0; i < 35; i++)//
	{
		Char.m_Tick++;
		TempCore.Tick(true);
		TempCore.Move();
		TempCore.Quantize();

		if (TempCore.m_Vel.y >= 0.0f)
			return false;

		int FutureTile = m_pClient->Collision()->GetCollisionAt(TempCore.m_Pos);
		if (FutureTile == 9)
		{
			return true;
		}
	}

	return false;
}

//void CBlockHelp::SnapInput(CNetObj_PlayerInput *pInput)
//{
//	CNetObj_Character Char;
//	CNetObj_Character PrevChar;
//	CNetObj_PlayerInput Input;
//	CNetObj_PlayerInput TempInput;
//	float IntraTick = 0.0f;
//	vec2 Position = vec2(0, 0);
//
//	m_pClient->m_PredictedChar.Write(&Char);
//	m_pClient->m_PredictedPrevChar.Write(&PrevChar);
//	IntraTick = Client()->PredIntraGameTick();
//	Position = mix(vec2(PrevChar.m_X, PrevChar.m_Y), vec2(Char.m_X, Char.m_Y), IntraTick);
//
//	Input = *((CNetObj_PlayerInput*)pInput);
//
//	if(WhouldTouchFreeze(Input, Char) == false)
//		return;
//
//	//lets see what we can do against touching the freeze
//
//	if(Input.m_Direction != 1)
//	{//what happens when we move right?
//		TempInput = Input;
//		TempInput.m_Direction = 1;
//		if(WhouldTouchFreeze(TempInput, Char) == false)//we dont touch freeze anymore!
//		{
//			TextRender()->Text(0, Position.x+16, Position.y-32, 28.0f, ">", 0);
//			pInput->m_Direction = 1;
//		}
//	}
//
//	if(Input.m_Direction != -1)
//	{//what happens when we move left?
//		TempInput = Input;
//		TempInput.m_Direction = -1;
//		if(WhouldTouchFreeze(TempInput, Char) == false)//we dont touch freeze anymore!
//		{
//			TextRender()->Text(0, Position.x-42, Position.y-32, 28.0f, "<", 0);
//			pInput->m_Direction = -1;
//		}
//	}
//
//	if(Input.m_Jump == false)
//	{//what happens when we jump?
//		TempInput = Input;
//		TempInput.m_Jump = true;
//		if(WhouldTouchFreeze(TempInput, Char) == false)//we dont touch freeze anymore!
//		{
//			TextRender()->Text(0, Position.x-16, Position.y-32, 28.0f, "^", 0);
//			pInput->m_Jump = true;
//		}
//	}
//}

void CBlockHelp::SnapInput(CNetObj_PlayerInput *pInput)
{
	SmartHammer(pInput);
}

void CBlockHelp::SmartHammer(CNetObj_PlayerInput *pInput)
{
	vec2 OwnPos = vec2(0, 0);
	vec2 EnemyPos = vec2(0, 0);
	int BlockingPlayer = m_pClient->m_pControls->m_BlockingPlayer;

	HelpActive[HELP_SMARTHAMMER] = false;
	if (m_pClient->m_Snap.m_pLocalCharacter == 0x0 || BlockingPlayer == -1 ||
		m_pClient->m_Snap.m_aCharacters[BlockingPlayer].m_Active == false || m_pClient->m_Snap.m_pLocalCharacter->m_Weapon != WEAPON_HAMMER
		|| m_pClient->m_Snap.m_aCharacters[BlockingPlayer].m_Cur.m_Weapon == WEAPON_NINJA)
		return;

	OwnPos = vec2(m_pClient->m_Snap.m_pLocalCharacter->m_X, m_pClient->m_Snap.m_pLocalCharacter->m_Y);
	EnemyPos = vec2(m_pClient->m_Snap.m_aCharacters[BlockingPlayer].m_Cur.m_X, m_pClient->m_Snap.m_aCharacters[BlockingPlayer].m_Cur.m_Y);
	vec2 ProjStartPos = OwnPos + normalize(vec2(pInput->m_TargetX, pInput->m_TargetY))*28.0f*0.75f;


	if (distance(OwnPos, EnemyPos) > 60.0f)//out of hammer reach
		return;

	CNetObj_PlayerInput Input;
	mem_zero(&Input, sizeof(Input));
	CNetObj_Character Char = m_pClient->m_Snap.m_aCharacters[BlockingPlayer].m_Cur;
	vec2 HammerAdd = vec2(0.f, -1.f) + normalize(normalize(EnemyPos - ProjStartPos) + vec2(0.f, -1.1f)) * 10.0f * 256.0f;
	Char.m_VelX += HammerAdd.x; Char.m_VelY += HammerAdd.y;

	if(Char.m_HookState != 0)
		Input.m_Hook = 1;

	if(WhouldTouchFreezeUp(Input, Char) == false)
		return;

	HelpActive[HELP_SMARTHAMMER] = true;

	if (distance(ProjStartPos, EnemyPos) > 40.0f)//wrong direction
		return;

	static int64 s_HammerTime = time_get();

	if (s_HammerTime + time_freq() * 0.3f > time_get())
		return;

	pInput->m_Fire += 2;

	s_HammerTime = time_get();

}

void CBlockHelp::OnRender()
{
	CNetObj_Character Char;
	CNetObj_Character PrevChar;
	float IntraTick = 0.0f;
	vec2 Position = vec2(0, 0);
	
	m_pClient->m_PredictedChar.Write(&Char);
	m_pClient->m_PredictedPrevChar.Write(&PrevChar);
	IntraTick = Client()->PredIntraGameTick();
	Position = mix(vec2(PrevChar.m_X, PrevChar.m_Y), vec2(Char.m_X, Char.m_Y), IntraTick);

	if (HelpActive[HELP_SMARTHAMMER])
	{
		static const int s_HammerWidth = 70 / 3;
		static const int s_HammerHeight = 41 / 3;
		//TextRender()->Text(0, Position.x - 16, Position.y - 48, 28.0f, "h", 0);
		static int s_TextureHammer = Graphics()->LoadTexture("pdl_hammer.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
		Graphics()->TextureSet(s_TextureHammer);
		Graphics()->QuadsBegin();

		//Graphics()->SetColor(0.05f, 0.05f, 0.05f, 1.0f);
		IGraphics::CQuadItem QuadItem(Position.x - 6 - s_HammerWidth / 2, Position.y - 32 - s_HammerHeight / 2, s_HammerWidth, s_HammerHeight);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}
}