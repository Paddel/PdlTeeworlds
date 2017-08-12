
#include <base/math.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/gameclient.h>
#include <game/client/animstate.h>
#include "nameplates.h"
#include "controls.h"

static void ConKeyInputState(IConsole::IResult *pResult, void *pUserData)
{
	((int *)pUserData)[0] = pResult->GetInteger(0);
}

void CNamePlates::RenderNameplate(
	const CNetObj_Character *pPrevChar,
	const CNetObj_Character *pPlayerChar,
	const CNetObj_PlayerInfo *pPlayerInfo
	)
{
	float IntraTick = Client()->IntraGameTick();

	CNetObj_Character Char = *pPlayerChar;
	CNetObj_Character PrevChar = *pPrevChar;

	bool ShowIDs = m_pClient->m_pControls->m_InputShowIDs == 1;

	if(pPlayerInfo->m_Local && (g_Config.m_PdlNameplateOwn || ShowIDs))
	{
		m_pClient->m_PredictedChar.Write(&Char);
		m_pClient->m_PredictedPrevChar.Write(&PrevChar);
		IntraTick = Client()->PredIntraGameTick();
	}

	vec2 Position = mix(vec2(PrevChar.m_X, PrevChar.m_Y), vec2(Char.m_X, Char.m_Y), IntraTick);


	float FontSize = 18.0f + 20.0f * g_Config.m_ClNameplatesSize / 100.0f;
	// render name plate
	if(!pPlayerInfo->m_Local || g_Config.m_PdlNameplateOwn)
	{

		float a = 1;
		if(g_Config.m_ClNameplatesAlways == 0)
			a = clamp(1-powerf(distance(m_pClient->m_pControls->m_TargetPos, Position)/200.0f,16.0f), 0.0f, 1.0f);

		const char *pName = m_pClient->m_aClients[pPlayerInfo->m_ClientID].m_aName;
		float tw = TextRender()->TextWidth(0, FontSize, pName, -1);

		TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.5f*a);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, a);
		if(g_Config.m_ClNameplatesTeamcolors && m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
		{
			if(pPlayerInfo->m_Team == TEAM_RED)
				TextRender()->TextColor(1.0f, 0.5f, 0.5f, a);
			else if(pPlayerInfo->m_Team == TEAM_BLUE)
				TextRender()->TextColor(0.7f, 0.7f, 1.0f, a);
		}

		TextRender()->Text(0, Position.x-tw/2.0f, Position.y-FontSize-38.0f, FontSize, pName, -1);

		if(g_Config.m_Debug || ShowIDs) // render client id when in debug aswell
		{
			char aBuf[128];
			str_format(aBuf, sizeof(aBuf),"%02d", pPlayerInfo->m_ClientID);
			float IDTextWidth = TextRender()->TextWidth(0, 19.0f, aBuf, -1);
			TextRender()->Text(0, Position.x - IDTextWidth / 2, Position.y-82.0f, 19.0f, aBuf, -1);
		}

		/*if(g_Config.m_PdlBlockscoreShow)
		{
			int Wins = m_pClient->m_aClients[pPlayerInfo->m_ClientID].m_BlockScore[g_Config.m_PdlBlockscoreShowType][0];
			int Looses = m_pClient->m_aClients[pPlayerInfo->m_ClientID].m_BlockScore[g_Config.m_PdlBlockscoreShowType][1];
			float Rate = Wins;
			if(Looses)
				Rate = Wins/(float)Looses;

			char aBuf[128];
			str_format(aBuf, sizeof(aBuf),"W%i L%i R%.2f", Wins, Looses, Rate);

			TextRender()->Text(0, Position.x, Position.y-90, 28.0f, aBuf, -1);
		}*/

		TextRender()->TextColor(1,1,1,1);
		TextRender()->TextOutlineColor(0.0f, 0.0f, 0.0f, 0.3f);
	}
	else if(ShowIDs)
	{
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "%02d", pPlayerInfo->m_ClientID);
		float IDTextWidth = TextRender()->TextWidth(0, 19.0f, aBuf, -1);
		TextRender()->Text(0, Position.x - IDTextWidth / 2, Position.y - 53.0f, 19.0f, aBuf, -1);
	}
}

void CNamePlates::OnRender()
{
	if (!g_Config.m_ClNameplates)
		return;

	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		// only render active characters
		if(!m_pClient->m_Snap.m_aCharacters[i].m_Active)
			continue;

		const void *pInfo = Client()->SnapFindItem(IClient::SNAP_CURRENT, NETOBJTYPE_PLAYERINFO, i);

		if(pInfo)
		{
			RenderNameplate(
				&m_pClient->m_Snap.m_aCharacters[i].m_Prev,
				&m_pClient->m_Snap.m_aCharacters[i].m_Cur,
				(const CNetObj_PlayerInfo *)pInfo);
		}
	}
}
