
#include <engine/storage.h>
#include <engine/shared/config.h>

#include "mapinker.h"

CMapInker::CMapInker()
{
	m_pMapInkerLayers = 0x0;
	m_NumLayers = 0;
	m_LoadedMaps = 0;
}

void CMapInker::UpdateTextureID(bool Night, int Index)
{
	if (Index < 0 || Index >= m_NumLayers)
		return;

	int RealIndex = CalcIndex(Night, Index);

	if (m_pTextureIDs[RealIndex] != -1)
		Graphics()->UnloadTexture(m_pTextureIDs[RealIndex]);

	if (m_pMapInkerLayers[RealIndex].m_aExternalTexture[0] != '\0')
	{
		char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "mapres/%s.png", m_pMapInkerLayers[RealIndex].m_aExternalTexture);
		m_pTextureIDs[RealIndex] = Graphics()->LoadTexture(aBuf, IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
		if (m_pTextureIDs[RealIndex] == 0)
			m_pTextureIDs[RealIndex] = -1;
	}
}

bool CMapInker::NightTime()
{
	int64 ts = time_timestamp();
	int TimeFrom = g_Config.m_PdlMapinkerTimeFrom;
	int TimeTo = g_Config.m_PdlMapinkerTimeTo;
	int TimeCur = time_hour(ts);

	if (g_Config.m_PdlMapinkerTimeEnable == 0 || TimeFrom == TimeTo)
		return false;
	else if (TimeFrom > TimeTo)
	{
		TimeTo += 24;
		if (TimeCur < TimeFrom)
			TimeCur += 24;
	}

	return TimeCur >= TimeFrom && TimeCur < TimeTo;
}

void CMapInker::Save()
{
	char aBuf[256];
	IOHANDLE MapInkerFile;

	if (Client()->State() != IClient::STATE_ONLINE)
		return;

	str_format(aBuf, sizeof(aBuf), "mapinker/%s_%08x.mi", Client()->CurrentMapName(), Client()->CurrentMapCrc());
	MapInkerFile = Storage()->OpenFile(aBuf, IOFLAG_WRITE, IStorage::TYPE_SAVE);

	if (!MapInkerFile)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_ERROR, "game", "Could not write MapInker file!");
		return;
	}

	io_write(MapInkerFile, (const char *)m_pMapInkerLayers, sizeof(CMapInkerLayer) * 2 * m_NumLayers);
	io_close(MapInkerFile);
}

void CMapInker::Load()
{
	char aBuf[256];
	IOHANDLE MapInkerFile;

	if (Client()->State() <= IClient::STATE_OFFLINE)
		return;

	str_format(aBuf, sizeof(aBuf), "mapinker/%s_%08x.mi", Client()->CurrentMapName(), Client()->CurrentMapCrc());
	MapInkerFile = Storage()->OpenFile(aBuf, IOFLAG_READ, IStorage::TYPE_SAVE);

	if (!MapInkerFile)
		return;

	int Length = io_length(MapInkerFile);
	if (Length != sizeof(CMapInkerLayer) * 2 * m_NumLayers)
	{
		Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_ERROR, "game", "Outdated file!");
		io_close(MapInkerFile);
		return;
	}

	io_read(MapInkerFile, m_pMapInkerLayers, Length);
	io_close(MapInkerFile);
	for (int j = 0; j < 2; j++)
		for (int i = 0; i < m_NumLayers; i++)
			UpdateTextureID((bool)j, i);
}

void CMapInker::OnMapLoad()
{
	m_NumLayers = m_pClient->Layers()->NumLayers();

	if (m_pMapInkerLayers != 0x0)
		delete m_pMapInkerLayers;

	m_pMapInkerLayers = new CMapInkerLayer[m_NumLayers * 2];
	m_pTextureIDs = new int[m_NumLayers * 2];

	for (int j = 0; j < 2; j++)
	{
		for (int i = 0; i < m_NumLayers; i++)
		{
			int Index = CalcIndex((bool)j, i);
			m_pMapInkerLayers[Index].m_Used = false;
			m_pMapInkerLayers[Index].m_Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			m_pMapInkerLayers[Index].m_Interpolation = 0.0f;
			mem_zero(m_pMapInkerLayers[Index].m_aExternalTexture, sizeof(m_pMapInkerLayers[Index].m_aExternalTexture));
			m_pTextureIDs[Index] = -1;
		}
	}

	Load();

	m_LoadedMaps++;
}

void CMapInker::OnInit()
{
	Graphics()->AddTextureUser(this);
}

void CMapInker::InitTextures()
{
	if (m_NumLayers <= 0)
		return;

	for (int j = 0; j < 2; j++)
		for(int i = 0; i < m_NumLayers; i++)
		UpdateTextureID(j, i);
}

CMapInkerLayer *CMapInker::MapInkerLayer(bool Night, int Index)
{
	if (Index < 0 || Index >= m_NumLayers)
		return 0x0;

	return &m_pMapInkerLayers[CalcIndex(Night, Index)];
};