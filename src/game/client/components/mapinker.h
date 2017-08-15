#pragma once

#include <game/client/component.h>

struct CMapInkerLayer
{
	bool m_Used;
	float m_Interpolation;
	char m_aExternalTexture[48];
	vec4 m_Color;
};

class CMapInker : public CComponent, public CTextureUser
{
private:
	CMapInkerLayer *m_pMapInkerLayers;
	int *m_pTextureIDs;
	int m_NumLayers;
	int m_LoadedMaps;

	int CalcIndex(bool Night, int Index) const { return Index + (int)Night * m_NumLayers; };

public:
	CMapInker();

	void UpdateTextureID(bool Night, int Index);
	bool NightTime();
	void Save();
	void Load();

	virtual void OnMapLoad();
	virtual void OnInit();

	virtual void InitTextures();

	CMapInkerLayer *MapInkerLayer(bool Night, int Index);
	int LayerTextureID(bool Night, int Index) { return m_pTextureIDs[CalcIndex(Night, Index)]; };
	int GetLoadedMaps() const { return m_LoadedMaps; };
};