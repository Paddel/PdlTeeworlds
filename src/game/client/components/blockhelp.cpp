

#include <engine/textrender.h>
#include <engine/shared/config.h>

#include "blockhelp.h"


CBlockHelp::CBlockHelp()
{

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

void CBlockHelp::SnapInput(CNetObj_PlayerInput *pInput)
{
	CNetObj_Character Char;
	CNetObj_Character PrevChar;
	CNetObj_PlayerInput Input;
	CNetObj_PlayerInput TempInput;
	float IntraTick = 0.0f;
	vec2 Position = vec2(0, 0);

	m_pClient->m_PredictedChar.Write(&Char);
	m_pClient->m_PredictedPrevChar.Write(&PrevChar);
	IntraTick = Client()->PredIntraGameTick();
	Position = mix(vec2(PrevChar.m_X, PrevChar.m_Y), vec2(Char.m_X, Char.m_Y), IntraTick);

	Input = *((CNetObj_PlayerInput*)pInput);

	if(WhouldTouchFreeze(Input, Char) == false)
		return;

	//lets see what we can do against touching the freeze

	if(Input.m_Direction != 1)
	{//what happens when we move right?
		TempInput = Input;
		TempInput.m_Direction = 1;
		if(WhouldTouchFreeze(TempInput, Char) == false)//we dont touch freeze anymore!
		{
			TextRender()->Text(0, Position.x+16, Position.y-32, 28.0f, ">", 0);
			pInput->m_Direction = 1;
		}
	}

	if(Input.m_Direction != -1)
	{//what happens when we move left?
		TempInput = Input;
		TempInput.m_Direction = -1;
		if(WhouldTouchFreeze(TempInput, Char) == false)//we dont touch freeze anymore!
		{
			TextRender()->Text(0, Position.x-42, Position.y-32, 28.0f, "<", 0);
			pInput->m_Direction = -1;
		}
	}

	if(Input.m_Jump == false)
	{//what happens when we jump?
		TempInput = Input;
		TempInput.m_Jump = true;
		if(WhouldTouchFreeze(TempInput, Char) == false)//we dont touch freeze anymore!
		{
			TextRender()->Text(0, Position.x-16, Position.y-32, 28.0f, "^", 0);
			pInput->m_Jump = true;
		}
	}
}