
#ifndef GAME_CLIENT_COMPONENTS_MAPLAYERS_H
#define GAME_CLIENT_COMPONENTS_MAPLAYERS_H
#include <game/client/component.h>

class CMapLayers : public CComponent, public CTextureUser
{
	CLayers *m_pLayers;	// todo refactor: maybe remove it and access it through client*
	int m_Type;
	int m_CurrentLocalTick;
	int m_LastLocalTick;
	bool m_EnvelopeUpdate;

	void MapScreenToGroup(float CenterX, float CenterY, CMapItemGroup *pGroup, float Zoom);
	static void EnvelopeEval(float TimeOffset, int Env, float *pChannels, void *pUser);

	void Render(vec2 Center, float Zoom, bool UseClip);

	void RenderGamelayer(vec2 Center);

public:
	enum
	{
		TYPE_BACKGROUND=0,
		TYPE_FOREGROUND,
	};

	CMapLayers(int Type);
	virtual void OnInit();
	virtual void OnRender();
	virtual void InitTextures();

	
	void Render(vec2 Center);
	void Render(vec2 Center, CUIRect Clip, float Zoom);

	void EnvelopeUpdate();
};

#endif
