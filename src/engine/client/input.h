/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#ifndef ENGINE_CLIENT_INPUT_H
#define ENGINE_CLIENT_INPUT_H

class CInput : public IEngineInput, public CTextureUser
{
	IEngineGraphics *m_pGraphics;

	SDL_Joystick *m_pJoystick;

	int m_InputGrabbed;

	int64 m_LastRelease;
	int64 m_ReleaseDelta;

	void AddEvent(int Unicode, int Key, int Flags);

	IEngineGraphics *Graphics() { return m_pGraphics; }

public:
	CInput();

	virtual void InitTextures();

	virtual void Init();

	virtual void MouseRelative(float *x, float *y);
	virtual void MouseModeAbsolute();
	virtual void MouseModeRelative();
	virtual int MouseDoubleClick();

	virtual float JoystickAxes(int Axe);
	virtual int JoystickButton(int Button);

	void ClearKeyStates();
	int KeyState(int Key);

	int ButtonPressed(int Button) { return m_aInputState[m_InputCurrent][Button]; }

	virtual int Update();
};

#endif
