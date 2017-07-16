
#include <string.h>

#include <base/system.h>
#include <base/stringseperation.h>

#include <engine/storage.h>
#include <engine/keys.h>
#include <engine/shared/config.h>
#include <engine/shared/linereader.h>

#include <game/client/components/camera.h>
#include <game/client/components/controls.h>

#include "blockscore.h"

#define SCHEMA_NAME "sys"
#define MIN_BLOCK_NUM 30

static int gs_TextureBlockScore = -1;

void ClearClientName(char *pStr)
{
	for(int i = 0; i < str_length(pStr); i++)
	{
		if(pStr[i] == ';')
			pStr[i] = ' ';
	}
}

CBlockScore::CBlockScore()
{
	m_MapWidth = 0;
	m_MapHeight = 0;
	m_pTiles = 0;
	m_DrawMode = 1;
}

void CBlockScore::UpdateScores()
{
	char aClientName[MAX_NAME_LENGTH];
	char aQuery[QUERY_MAX_STR_LEN];

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(m_pClient->m_aClients[i].m_Active == false)
			continue;

		str_copy(aClientName, m_pClient->m_aClients[i].m_aName, sizeof(aClientName));
		ClearClientName(aClientName);

		str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.blockscore WHERE blockscore.name=\"%s\"", SCHEMA_NAME, aClientName);
		int res = Query(aQuery, &WriteBlockScore, &m_pClient->m_aClients[i]);

	}
}

void CBlockScore::AutoStay()
{
	if(Client()->State() == IClient::STATE_OFFLINE)
		Client()->Connect(m_aServerAddress);

	if(Client()->State() == IClient::STATE_ONLINE)
	{
		int LocalID = m_pClient->m_Snap.m_LocalClientID;
		if(LocalID >= 0 && LocalID < MAX_CLIENTS)
		{
			if(m_pClient->m_aClients[LocalID].m_Active == true)
			{
				m_pClient->SendSwitchTeam(TEAM_SPECTATORS);
			}
		}
	}
}

float CBlockScore::GetTotalScore(CGameClient::CClientData *pClient)
{
	int Wins = pClient->m_BlockScore[3][0];
	int Looses = pClient->m_BlockScore[3][1];
	float Rate = Wins;
	if(Looses)
		Rate = Wins/(float)Looses; 

	return Wins * Rate;
}

void CBlockScore::PrintTop3()
{
	/*CChatReplyMsg Msg;
	Msg.m_ClientID = ClientID;
	str_format(Msg.m_aMsg, sizeof(Msg.m_aMsg), "%s: %s's blockprofile in %s is not complete yet because of missing activity. Profile is to %i%% done.", m_pClient->m_aClients[ClientID].m_aName, pName, aTypeNames[Type-1], (int)ProfileDone);

	m_ChatReplyMsgs.add(Msg);*/

	char aQuery[QUERY_MAX_STR_LEN];
	CGameClient::CClientData aClientData[3];
	for(int i = 0; i < 3; i++)
		aClientData[i].Reset();

	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.blockscore", SCHEMA_NAME);
	int res = Query(aQuery, WriteTop3, aClientData);
	
	if(res == -1)
	{//not found
		return;
	}

	CChatReplyMsg Msg;
	Msg.m_ClientID = -1;
	str_format(Msg.m_aMsg, sizeof(Msg.m_aMsg), "Top3 total score: ");
	for(int i = 2; i >= 0; i--)
	{
		float Score = GetTotalScore(&aClientData[i]);
		str_format(Msg.m_aMsg, sizeof(Msg.m_aMsg), "%s%i. %s with %.2f ", Msg.m_aMsg, 3-i, aClientData[i].m_aName, Score);
	}
	m_ChatReplyMsgs.add(Msg);
}

void CBlockScore::WriteTop3(int index, char *pResult, int pResultSize, void *pData)
{
	CGameClient::CClientData *pClient = (CGameClient::CClientData *)pData;

	static CGameClient::CClientData ClientData;
	if(index == 0)
		ClientData.Reset();

	switch(index)
	{
		case 0: str_copy(ClientData.m_aName, pResult?pResult:"", sizeof(ClientData.m_aName));
		case 1: ClientData.m_BlockScore[3][0] = atoi(pResult); break;
		case 2: ClientData.m_BlockScore[0][0] = atoi(pResult); break;
		case 3: ClientData.m_BlockScore[1][0] = atoi(pResult); break;
		case 4: ClientData.m_BlockScore[2][0] = atoi(pResult); break;
		case 5: ClientData.m_BlockScore[3][1] = atoi(pResult); break;
		case 6: ClientData.m_BlockScore[0][1] = atoi(pResult); break;
		case 7: ClientData.m_BlockScore[1][1] = atoi(pResult); break;
		case 8: ClientData.m_BlockScore[2][1] = atoi(pResult); break;
	}
	
	if(index != 8)
		return;

	float CurScore = GetTotalScore(&ClientData);

	for(int i = 2; i >= 0; i--)
	{
		float Score = GetTotalScore(&pClient[i]);
		if(CurScore > Score)
		{
			if(i == 2)
			{
				pClient[0] = pClient[1];
				pClient[1] = pClient[2];
			}
			else if(i == 1)
			{
				pClient[0] = pClient[1];
			}

			pClient[i] = ClientData;

			break;
		}
	}
}

int CBlockScore::GetIndex(vec2 Pos)
{
	int Nx = clamp(round(Pos.x)/32, 0, m_MapWidth-1);
	int Ny = clamp(round(Pos.y)/32, 0, m_MapHeight-1);
	int Tile = Ny*m_MapWidth+Nx;
	return m_pTiles[Tile].m_Index;
}

int CBlockScore::GetHooedBy(int ClientID)
{
	int Hooked = -1;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(ClientID == i)
			continue;

		CNetObj_Character Player = m_pClient->m_Snap.m_aCharacters[i].m_Cur;

		if(Player.m_HookState == HOOK_GRABBED && Player.m_HookedPlayer == ClientID)
		{
			if(Hooked == -1)
				Hooked = i;
			else
			{
				Hooked = -1;
				break;//hooked by more ppl
			}
		}
	}

	return Hooked;
}

void CBlockScore::DoBlockScores()
{
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		CGameClient::CClientData *pClient = &m_pClient->m_aClients[i];
		CNetObj_Character Player = m_pClient->m_Snap.m_aCharacters[i].m_Cur;

		if(pClient->m_Active == false)
			continue;

		int HookedBy = GetHooedBy(i);
		if(HookedBy != -1 && pClient->m_Blocked == false)
		{
			pClient->m_BlockLastInteraction = HookedBy;
			pClient->m_BlockInteractionTime = time_get() + time_freq() * 2.0f;
		}

		if(pClient->m_Blocked == false && pClient->m_BlockLastInteraction != -1 && pClient->m_BlockInteractionTime < time_get())
			pClient->m_BlockLastInteraction = -1;


		int Tile = Collision()->GetCollisionAt(vec2(Player.m_X, Player.m_Y));
		int BlockScoreTile = GetIndex(vec2(Player.m_X, Player.m_Y));
		if(pClient->m_Blocked == false)
		{
			if(Player.m_Weapon == WEAPON_NINJA && BlockScoreTile > BLOCKSCORE_TILE_EMPTY && BlockScoreTile < NUM_BLOCKSCORE_TILES)
			{
				pClient->m_BlockType = BlockScoreTile;
				pClient->m_BlockScoreTime = time_get() + time_freq()*3;
				pClient->m_Blocked = true;
			}
		}
		else
		{
			if(pClient->m_BlockScoreTime < time_get() && pClient->m_BlockLastInteraction != -1)
			{
				IncreaseScore(pClient->m_BlockLastInteraction, pClient->m_BlockType, false);
				IncreaseScore(i, pClient->m_BlockType, true);
				pClient->m_BlockLastInteraction = -1;
			}

			if(Player.m_Weapon != WEAPON_NINJA && Tile != 9)
			{
				pClient->m_Blocked = false;
				pClient->m_BlockLastInteraction = -1;
			}
		}
	}
}

void CBlockScore::WriteExplenation(int ClientID, char *pAdd)
{
	CChatReplyMsg Msg;
	Msg.m_ClientID = ClientID;
	str_format(Msg.m_aMsg, sizeof(Msg.m_aMsg), "%s: %s.", m_pClient->m_aClients[ClientID].m_aName, pAdd);
	m_ChatReplyMsgs.add(Msg);
	str_format(Msg.m_aMsg, sizeof(Msg.m_aMsg), "%s: To see blockscores write .Blockscore <type> <name> examples: \".Blockscore starblock nameless tee\", \".Blockscore v3\"", m_pClient->m_aClients[ClientID].m_aName);
	m_ChatReplyMsgs.add(Msg);
}

void CBlockScore::WriteUncompleteProfile(char *pTo, int ClientID, char *pName, int Type, int Activity)
{
	CChatReplyMsg Msg;
	Msg.m_ClientID = ClientID;
	float ProfileDone = 0;
	if(Activity > 0)
		ProfileDone = ((Activity)/(float)MIN_BLOCK_NUM)*100.0f;

	str_format(Msg.m_aMsg, sizeof(Msg.m_aMsg), "%s: %s's blockprofile in %s is not complete yet because of missing activity. Profile is to %i%% done.", pTo, pName, aTypeNames[Type-1], (int)ProfileDone);

	m_ChatReplyMsgs.add(Msg);
}

void CBlockScore::WriteProfile(int ClientID, char *pName, int Type)
{
	char aClientName[MAX_NAME_LENGTH];
	char aQuery[QUERY_MAX_STR_LEN];
	char aToName[MAX_NAME_LENGTH+4];
	CGameClient::CClientData aClientData;
	aClientData.Reset();

	if(Type <= BLOCKSCORE_TILE_EMPTY || Type >= NUM_BLOCKSCORE_TILES)
		return;

	str_format(aToName, sizeof(aToName), "%s%s", m_pClient->m_aClients[ClientID].m_aName[0]=='/'?" ": "", m_pClient->m_aClients[ClientID].m_aName);
	str_copy(aClientName, pName, sizeof(aClientName));
	ClearClientName(aClientName);

	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.blockscore WHERE blockscore.name=\"%s\"", SCHEMA_NAME, aClientName);
	int res = Query(aQuery, &WriteBlockScore, &aClientData);
	
	if(res == -1)
	{//not found. create a new acc
		WriteUncompleteProfile(aToName, ClientID, pName, Type, 0);
		return;
	}

	int Activity = aClientData.m_BlockScore[Type-1][0] + aClientData.m_BlockScore[Type-1][1];
	if(Activity < (int)MIN_BLOCK_NUM)
	{
		WriteUncompleteProfile(aToName, ClientID, pName, Type, Activity);
		return;
	}

	int Wins = aClientData.m_BlockScore[Type-1][0];
	int Looses = aClientData.m_BlockScore[Type-1][1];
	float Rate = Wins;
	if(Looses)
		Rate = Wins/(float)Looses;

	CChatReplyMsg Msg;
	Msg.m_ClientID = ClientID;
	str_format(Msg.m_aMsg, sizeof(Msg.m_aMsg), "%s: %s's blockprofile for %s: WINS=%i, LOOSES=%i, WINRATE=%.2f, SCORE=%.2f", aToName, pName, aTypeNames[Type-1], Wins, Looses, Rate, Wins*Rate);
	m_ChatReplyMsgs.add(Msg);
}

void CBlockScore::AnalyseMessage(int ClientID, const char *pMsgOrg)
{
	char aMsg[512];
	char aBeginning[12];
	char *pMsg = aMsg;

	if(ClientID == -1)
		return;

	for(int i = 0; i < m_ChatReplyMsgs.size(); i++)
		if(m_ChatReplyMsgs[i].m_ClientID == ClientID)
			return;

	str_copy(aMsg, pMsgOrg, sizeof(aMsg));
	str_copy(aBeginning, ".Blockscore", sizeof(aBeginning));

	if(str_comp_nocase_num(aMsg, aBeginning, str_length(aBeginning)) == 0)
	{
		pMsg += str_length(aBeginning);

		if(pMsg[0] == ':')
			pMsg += 2;
		else
			pMsg++;

		if(str_length(pMsg) == 0)//default
		{
			WriteProfile(ClientID, m_pClient->m_aClients[ClientID].m_aName, BLOCKSCORE_TILE_STARBLOCK);
			return;
		}
		else
		{
			char *pFirstArg = GetSepStr(' ', &pMsg);
			if(pMsg)
			{//more than one argmunent

				for(int i = 0; i < NUM_BLOCKSCORE_TILES-1; i++)
				{
					if(str_comp_nocase(aTypeNames[i], pFirstArg) == 0)
					{
						WriteProfile(ClientID, pMsg, i+1);
						return;
					}
				}

				char aFail[256];
				str_format(aFail, sizeof(aFail), "%s is not a type of this map. Possible types are", pFirstArg);

				for(int i = 0; i < NUM_BLOCKSCORE_TILES-1; i++)
					str_format(aFail, sizeof(aFail), "%s%s%s", aFail, i!=0?", ":" ", aTypeNames[i]);

				WriteExplenation(ClientID, aFail);
			}
			else
			{//one argmunent

				for(int i = 0; i < NUM_BLOCKSCORE_TILES-1; i++)
				{
					if(str_comp_nocase(aTypeNames[i], pFirstArg) == 0)
					{
						WriteProfile(ClientID, m_pClient->m_aClients[ClientID].m_aName, i+1);
						return;
					}
				}

				//wants score from another player
				WriteProfile(ClientID, pFirstArg, BLOCKSCORE_TILE_STARBLOCK);
			}
		}
	}
}

void CBlockScore::ChatSupport()
{
	if(m_ChatTime < time_get() && m_ChatReplyMsgs.size() > 0)
	{
		CNetMsg_Cl_Say Msg;
		Msg.m_Team = 0;
		Msg.m_pMessage = m_ChatReplyMsgs[0].m_aMsg;
		Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);

		m_ChatReplyMsgs.remove_index(0);
		m_ChatTime = time_get() + time_freq() *5.1f;
	}

	if(m_ExplenationTime < time_get())
	{
		CChatReplyMsg Msg;
		Msg.m_ClientID = -1;
		str_format(Msg.m_aMsg, sizeof(Msg.m_aMsg), "I am a blockscore counter. To see your presonal blockscore write \".Blockscore\".");
		m_ChatReplyMsgs.add(Msg);
		str_format(Msg.m_aMsg, sizeof(Msg.m_aMsg), "You can also see other blockscores. \".Blockscore <type> <name>\". An example would be \".Blockscore starblock nameless tee\"");
		m_ChatReplyMsgs.add(Msg);
		
		PrintTop3();

		m_ExplenationTime = time_get() + time_freq() * 180;
	}

	if(m_ChatReplyMsgs.size() > 30)
		m_ChatReplyMsgs.clear();
}

void CBlockScore::OnRender()
{
	if(g_Config.m_PdlBlockscoreActive == 0)
		return;

	if(m_UpdateScoreTime < time_get())
	{
		UpdateScores();
		m_UpdateScoreTime = time_get() + time_freq() * 15.0f;
	}

	if(g_Config.m_PdlBlockscoreStay)
		AutoStay();

	if(g_Config.m_PdlBlockscoreCollect)
		DoBlockScores();

	if(g_Config.m_PdlBlockscoreChat)
		ChatSupport();

	if(Client()->State() == IClient::STATE_ONLINE && g_Config.m_PdlBlockscoreDraw)
	{
		vec2 Center = m_pClient->m_pCamera->m_Center;
		float Points[4];
		CUIRect Screen;
		Graphics()->GetScreen(&Screen.x, &Screen.y, &Screen.w, &Screen.h);

		RenderTools()->MapscreenToWorld(Center.x, Center.y, 1.0f, 1.0f,
			0, 0, Graphics()->ScreenAspect(), g_Config.m_PdlZoom, Points);
		Graphics()->MapScreen(Points[0], Points[1], Points[2], Points[3]);

		Graphics()->TextureSet(gs_TextureBlockScore);
	
		Graphics()->BlendNone();
		vec4 Color = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		RenderTools()->RenderTilemap(m_pTiles, m_MapWidth, m_MapHeight, 32.0f, Color, TILERENDERFLAG_EXTEND|LAYERRENDERFLAG_OPAQUE,
										NULL, this, -1, 0);
		Graphics()->BlendNormal();
		RenderTools()->RenderTilemap(m_pTiles, m_MapWidth, m_MapHeight, 32.0f, Color, TILERENDERFLAG_EXTEND|LAYERRENDERFLAG_TRANSPARENT,
										NULL, this, -1, 0);

		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);
	}
}

bool CBlockScore::OnInput(IInput::CEvent e)
{
	if(Client()->State() != IClient::STATE_ONLINE || g_Config.m_PdlBlockscoreDraw == 0)
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
		else if(e.m_Key == '1')
		{
			m_DrawMode = BLOCKSCORE_TILE_EMPTY;
			return true;
		}
		else if(e.m_Key == '2')
		{
			m_DrawMode = BLOCKSCORE_TILE_V3;
			return true;
		}
		else if(e.m_Key == '3')
		{
			m_DrawMode = BLOCKSCORE_TILE_WAYBLOCK;
			return true;
		}
		else if(e.m_Key == '4')
		{
			m_DrawMode = BLOCKSCORE_TILE_RACEBLOCK;
			return true;
		}
		else if(e.m_Key == '5')
		{
			m_DrawMode = BLOCKSCORE_TILE_STARBLOCK;
			return true;
		}
	}

	return false;
}

void CBlockScore::OnMapLoad()
{
	m_MapWidth = Layers()->GameLayer()->m_Width;
	m_MapHeight = Layers()->GameLayer()->m_Height;
	m_pTiles = new CTile[m_MapWidth*m_MapHeight];
	for(int i = 0; i < m_MapWidth*m_MapHeight; i++)
	{
		m_pTiles[i].m_Index = 0;
		m_pTiles[i].m_Flags = 0;
		m_pTiles[i].m_Skip = 0;
		m_pTiles[i].m_Reserved = 0;
	}
}

void CBlockScore::OnConsoleInit()
{
	Console()->Register("pdl_blockscore_map_save", "s", CFGFLAG_CLIENT, ConMapSave, this, "");
	Console()->Register("pdl_blockscore_map_load", "s", CFGFLAG_CLIENT, ConMapLoad, this, "");
}

void CBlockScore::OnInit()
{
	InitTextures();
	Graphics()->AddTextureUser(this);

	CDatabase::Init("localhost", "root", "poseidon", SCHEMA_NAME);
}

void CBlockScore::OnMessage(int MsgType, void *pRawMsg)
{
	if(g_Config.m_PdlBlockscoreActive == 0)
		return;

	if(MsgType == NETMSGTYPE_SV_KILLMSG)
	{
		CNetMsg_Sv_KillMsg *pMsg = (CNetMsg_Sv_KillMsg *)pRawMsg;

		int Victim = pMsg->m_Victim;
		if(Victim < 0 || Victim >= MAX_CLIENTS)
			return;

		CGameClient::CClientData *pClient = &m_pClient->m_aClients[Victim];

		if(m_pClient->m_aClients[Victim].m_Blocked && pClient->m_BlockLastInteraction != -1)
		{
			IncreaseScore(pClient->m_BlockLastInteraction, pClient->m_BlockType, false);
			IncreaseScore(Victim, pClient->m_BlockType, true);
			pClient->m_BlockLastInteraction = -1;
		}
	}
	else if(MsgType == NETMSGTYPE_SV_CHAT)
	{
		CNetMsg_Sv_Chat *pMsg = (CNetMsg_Sv_Chat *)pRawMsg;
		if(g_Config.m_PdlBlockscoreChat)
			AnalyseMessage(pMsg->m_ClientID, pMsg->m_pMessage);
	}
}

void CBlockScore::OnStateChange(int NewState, int OldState)
{
	if(NewState != IClient::STATE_ONLINE)
		return;

	str_copy(m_aServerAddress, Client()->GetServerAddress(), sizeof(m_aServerAddress));

	m_ChatTime = time_get() + time_freq() * 15.0f;

	if(g_Config.m_PdlBlockscoreStay)
	{
		CNetMsg_Cl_Say Msg;
		Msg.m_Team = 0;
		Msg.m_pMessage = "/showall";
		Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);

		char aCommand[300];
		str_format(aCommand, sizeof(aCommand), "pdl_blockscore_map_load %s", m_aLastLoadedMap);
		Console()->ExecuteLine(aCommand);
	}
}

void CBlockScore::InitTextures()
{
	gs_TextureBlockScore = Graphics()->LoadTexture("pdl_blockscore.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
}

void CBlockScore::ConMapSave(IConsole::IResult *pResult, void *pUserData)
{
	CBlockScore *pThis = (CBlockScore*)pUserData;
	const char *pName = pResult->GetString(0);
	char aFilename[256];
	char aBuf[32];

	str_format(aFilename, sizeof(aFilename), "Paddel/%s.bs", pName);
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

void CBlockScore::ConMapLoad(IConsole::IResult *pResult, void *pUserData)
{
	CBlockScore *pThis = (CBlockScore*)pUserData;
	const char *pName = pResult->GetString(0);
	char aFilename[256];
	char aBuf[32];
	CTile *pTiles = pThis->MapTiles();
	CLineReader LineReader;
	int Tile = 0;
	

	str_format(aFilename, sizeof(aFilename), "Paddel/%s.bs", pName);
	IOHANDLE File = pThis->Storage()->OpenFile(aFilename, IOFLAG_READ, IStorage::TYPE_ALL);
	if(!File)
	{
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_ERROR, "WayBlock", "File not found.");
		return;
	}

	str_copy(pThis->m_aLastLoadedMap, pName, sizeof(pThis->m_aLastLoadedMap));

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
			pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_ERROR, "WayBlock", "Map file does not fit!");
			break;
		}
	}
}

void CBlockScore::WriteBlockScore(int index, char *pResult, int pResultSize, void *pData)
{
	CGameClient::CClientData *pClient = (CGameClient::CClientData *)pData;
	switch(index)
	{
		case 1: pClient->m_BlockScore[3][0] = atoi(pResult); break;
		case 2: pClient->m_BlockScore[0][0] = atoi(pResult); break;
		case 3: pClient->m_BlockScore[1][0] = atoi(pResult); break;
		case 4: pClient->m_BlockScore[2][0] = atoi(pResult); break;
		case 5: pClient->m_BlockScore[3][1] = atoi(pResult); break;
		case 6: pClient->m_BlockScore[0][1] = atoi(pResult); break;
		case 7: pClient->m_BlockScore[1][1] = atoi(pResult); break;
		case 8: pClient->m_BlockScore[2][1] = atoi(pResult); break;
	}
}

void CBlockScore::IncreaseScore(int ClientID, int Type, bool Loose)
{
	char aClientName[MAX_NAME_LENGTH];
	char aQuery[QUERY_MAX_STR_LEN];
	CGameClient::CClientData *pClient = &m_pClient->m_aClients[ClientID];

	if(ClientID < 0 || ClientID >= MAX_CLIENTS || Type <= BLOCKSCORE_TILE_EMPTY || Type >= NUM_BLOCKSCORE_TILES)
		return;

	str_copy(aClientName, m_pClient->m_aClients[ClientID].m_aName, sizeof(aClientName));
	ClearClientName(aClientName);

	str_format(aQuery, sizeof(aQuery), "SELECT * FROM %s.blockscore WHERE blockscore.name=\"%s\"", SCHEMA_NAME, aClientName);
	int res = Query(aQuery, WriteBlockScore, pClient);
	
	if(res == -1)
	{//not found. create a new acc
		mem_zero(&aQuery, sizeof(aQuery));
		str_copy(aQuery, "INSERT INTO ", sizeof(aQuery));
		FillBlockerInformation(aQuery, sizeof(aQuery), ClientID, aClientName);

		res = Query(aQuery, NULL, NULL);


		if(res > 0)
		{
			dbg_msg("BlockScore", "Register error %i", res);
			return;
		}
	}


	//increase
	m_pClient->m_aClients[ClientID].m_BlockScore[Type-1][(int)Loose]++;


	//save
	mem_zero(&aQuery, sizeof(aQuery));
	str_copy(aQuery, "REPLACE INTO ", sizeof(aQuery));
	FillBlockerInformation(aQuery, sizeof(aQuery), ClientID, aClientName);
	Query(aQuery, NULL, NULL);
}

void CBlockScore::FillBlockerInformation(char *pStr, int StringSize, int ClientID, char *pName)
{
	CGameClient::CClientData *pClient = &m_pClient->m_aClients[ClientID];

	strcat(pStr, "blockscore(name, win_starblock, win_v3, win_wayblock, win_raceblock, loose_starblock, loose_v3, loose_wayblock, loose_raceblock) VALUES (");
	AddQueryStr(pStr, pName, StringSize);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pClient->m_BlockScore[3][0], StringSize);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pClient->m_BlockScore[0][0], StringSize);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pClient->m_BlockScore[1][0], StringSize);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pClient->m_BlockScore[2][0], StringSize);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pClient->m_BlockScore[3][1], StringSize);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pClient->m_BlockScore[0][1], StringSize);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pClient->m_BlockScore[1][1], StringSize);
	strcat(pStr, ", ");
	AddQueryInt(pStr, pClient->m_BlockScore[2][1], StringSize);
	strcat(pStr, ");");
}
