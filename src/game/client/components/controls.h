/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef GAME_CLIENT_COMPONENTS_CONTROLS_H
#define GAME_CLIENT_COMPONENTS_CONTROLS_H
#include <base/vmath.h>
#include <game/client/component.h>

enum
{
	FAKEINPUT_NONE=-1,
	FAKEINPUT_SWIFT=0,
	FAKEINPUT_FLY,
	FAKEINPUT_BLOCK,
	FAKEINPUT_BLOCK_SET,
	FAKEINPUT_GRENADEAIM,
	FAKEINPUT_STEP,
	FAKEINPUT_VIBRATE,
	NUM_FAKEINPUTDATA,
	FAKEINPUT_SAVEJUMP=NUM_FAKEINPUTDATA,
	FAKEINPUT_AUTOUNFREEZE,
	//FLAFL=NUM_FAKEINPUTS,
};


class CControls : public CComponent
{
private:
	int m_FakeInputData[NUM_FAKEINPUTDATA];
	vec2 m_FlyPos;
	vec2 m_OutFreezePos;
	int m_SwiftLastMove;
	int m_SwiftDirection;
	vec2 m_LastGrenadePos;
	bool m_GrenadeDangerous;
	bool m_InputLocked;
	CNetObj_PlayerInput m_LockedInput;

	static const int s_NumDirection = (int)(2 * 3.14f / 0.01f) + 1;
	int m_DirectionHit[s_NumDirection];//grenade hit directions
	vec2 m_LastFirePos;

public:
	int m_FakeInput;
	int m_LastFakeInput;
	int m_BlockingPlayer;
	vec2 m_MousePos;
	vec2 m_TargetPos;

	CNetObj_PlayerInput m_InputData;
	CNetObj_PlayerInput m_LastData;
	int m_InputDirectionLeft;
	int m_InputDirectionRight;

	CControls();

	void GrenadeKills();

	virtual void OnReset();
	virtual void OnRelease();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
	virtual bool OnMouseMove(float x, float y);
	virtual void OnConsoleInit();
	virtual void OnPlayerDeath();

	int SnapInput(int *pData);
	void ClampMousePos();

	bool FlyCollidingTee(vec2 From, vec2 To);
	int FlybotGetWallPos(vec2 TeePos, vec2 *WallTop, vec2 *WallBot);
	bool GrenadeWillHit(CNetObj_PlayerInput Input, vec2 *pOuts);

	void SetInput();
	void JoystickInput();
	void InputNormal();
	void Swift();
	void Fly();
	void BlockSet();
	void Block();
	void SaveJump();
	void AutoUnfreeze();
	void GrenadeDodge();
	void GrenadeAim();
	void Step();
	void Vibrate();

	void LockedInput();

	bool GetInputLocked() const { return m_InputLocked; }
};
#endif
