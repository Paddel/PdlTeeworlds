
#ifndef GAME_CLIENT_COMPONENTS_BROADCAST_H
#define GAME_CLIENT_COMPONENTS_BROADCAST_H
#include <game/client/component.h>

class CBroadcast : public CComponent
{
	// broadcasts
	char m_aShownText[1024];
	char m_aBroadcastText[1024];
	int64 m_BroadcastTime;
	int64 m_AnimTime;
	float m_BroadcastRenderOffset;

public:
	virtual void OnReset();
	virtual void OnRender();
	virtual void OnMessage(int MsgType, void *pRawMsg);
};

#endif
