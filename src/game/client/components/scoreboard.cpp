
#include <engine/demo.h>
#include <engine/friends.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <game/localization.h>
#include <game/client/animstate.h>
#include <game/client/gameclient.h>
#include <game/client/render.h>
#include <game/client/components/countryflags.h>
#include <game/client/components/motd.h>
#include <game/client/components/controls.h>

#include "scoreboard.h"

const int MIN_PING = 0, MAX_PING = 300;

bool CScoreboard::CPlayerItem::operator<(const CPlayerItem &Other)
{
	const CNetObj_PlayerInfo *pInfoOwn = m_pClient->m_Snap.m_paPlayerInfos[m_ID];
	const CNetObj_PlayerInfo *pInfoOther = m_pClient->m_Snap.m_paPlayerInfos[Other.m_ID];
	if (!pInfoOwn || !pInfoOther)//should not happen
			return true;

	//int IDOwn = pInfoOwn->m_ClientID;
	//int IDOther = pInfoOther->m_ClientID;

	if (pInfoOwn->m_Team == TEAM_SPECTATORS && pInfoOther->m_Team != TEAM_SPECTATORS)
		return false;
	else if (pInfoOwn->m_Team != TEAM_SPECTATORS && pInfoOther->m_Team == TEAM_SPECTATORS)
		return true;

	if(m_pClient->Client()->IsDDRace() == false)
		return pInfoOwn->m_Score > pInfoOther->m_Score;

	int Sortion = m_pClient->m_pScoreboard->GetSortion();

	switch (Sortion)
	{
	case SORTION_ID: return m_ID < Other.m_ID;
	case SORTION_NAME: return str_comp_nocase(m_pClient->m_aClients[m_ID].m_aName, m_pClient->m_aClients[Other.m_ID].m_aName) < 0;
	case SORTION_SCORE: return pInfoOwn->m_Score > pInfoOther->m_Score;
	case SORTION_CLAN: return str_comp_nocase(m_pClient->m_aClients[m_ID].m_aClan, m_pClient->m_aClients[Other.m_ID].m_aClan) < 0;
	case SORTION_LATENCY: return pInfoOwn->m_Latency < pInfoOther->m_Latency;
	case SORTION_COUNTRY: return m_pClient->m_aClients[m_ID].m_Country < m_pClient->m_aClients[Other.m_ID].m_Country;
	}

	return m_ID < Other.m_ID;
		
}

CScoreboard::CScoreboard()
{
	OnReset();
	m_Sortion = SORTION_NAME;
}

void CScoreboard::ConKeyScoreboard(IConsole::IResult *pResult, void *pUserData)
{
	((CScoreboard *)pUserData)->m_Active = pResult->GetInteger(0) != 0;
}

void CScoreboard::OnReset()
{
	m_Active = false;
}

void CScoreboard::OnRelease()
{
	m_Active = false;
}

void CScoreboard::OnInit()
{
	m_Sortion = GetSortion(g_Config.m_PdlScoreboardSortion);
}

void CScoreboard::OnConsoleInit()
{
	Console()->Register("+scoreboard", "", CFGFLAG_CLIENT, ConKeyScoreboard, this, "Show scoreboard");
}

void CScoreboard::RenderGoals(float x, float y, float w)
{
	float h = 50.0f;

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x, y, w, h, 10.0f);
	Graphics()->QuadsEnd();

	// render goals
	y += 10.0f;
	if(m_pClient->m_Snap.m_pGameInfoObj)
	{
		if(m_pClient->m_Snap.m_pGameInfoObj->m_ScoreLimit)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%s: %d", Localize("Score limit"), m_pClient->m_Snap.m_pGameInfoObj->m_ScoreLimit);
			TextRender()->Text(0, x+10.0f, y, 20.0f, aBuf, -1);
		}
		if(m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), Localize("Time limit: %d min"), m_pClient->m_Snap.m_pGameInfoObj->m_TimeLimit);
			TextRender()->Text(0, x+230.0f, y, 20.0f, aBuf, -1);
		}
		if(m_pClient->m_Snap.m_pGameInfoObj->m_RoundNum && m_pClient->m_Snap.m_pGameInfoObj->m_RoundCurrent)
		{
			char aBuf[64];
			str_format(aBuf, sizeof(aBuf), "%s %d/%d", Localize("Round"), m_pClient->m_Snap.m_pGameInfoObj->m_RoundCurrent, m_pClient->m_Snap.m_pGameInfoObj->m_RoundNum);
			float tw = TextRender()->TextWidth(0, 20.0f, aBuf, -1);
			TextRender()->Text(0, x+w-tw-10.0f, y, 20.0f, aBuf, -1);
		}
	}
}

void CScoreboard::RenderSpectators(float x, float y, float w)
{
	float h = 140.0f;

	// background
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0,0,0,0.5f);
	RenderTools()->DrawRoundRect(x, y, w, h, 10.0f);
	Graphics()->QuadsEnd();

	// Headline
	y += 10.0f;
	TextRender()->Text(0, x+10.0f, y, 28.0f, Localize("Spectators"), w-20.0f);

	// spectator names
	y += 30.0f;
	char aBuffer[1024*4];
	aBuffer[0] = 0;
	bool Multiple = false;
	for(int i = 0; i < MAX_CLIENTS; ++i)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[i];
		if(!pInfo || pInfo->m_Team != TEAM_SPECTATORS)
			continue;

		if(Multiple)
			str_append(aBuffer, ", ", sizeof(aBuffer));
		str_append(aBuffer, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, sizeof(aBuffer));
		Multiple = true;
	}
	CTextCursor Cursor;
	TextRender()->SetCursor(&Cursor, x+10.0f, y, 22.0f, TEXTFLAG_RENDER);
	Cursor.m_LineWidth = w-20.0f;
	Cursor.m_MaxLines = 4;
	TextRender()->TextEx(&Cursor, aBuffer, -1);
}

void CScoreboard::RenderScoreboard64(float x, float y, float w, float h, const char *pTitle)
{
	bool isTeam = false;

	if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
		isTeam = true;

	if(m_NumPlayers > 32)
		h += 140.0f;
	else
		h += 60.0f;

	// background
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.5f);
	RenderTools()->DrawRoundRectExt(x, y + 50, w, h - 50, 17.0f, CUI::CORNER_B);

	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f);
	RenderTools()->DrawRoundRectExt(x, y, w, 50, 17.0f, CUI::CORNER_T);
	Graphics()->QuadsEnd();

	// render title
	float TitleFontsize = 40.0f;
	if(!pTitle)
	{
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
			pTitle = Localize("Game over");
		else
			pTitle = Localize("Score board");
	}
	TextRender()->Text(0, x + 20.0f, y, TitleFontsize, pTitle, -1);

	char aBuf[128] = { 0 };
	if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW &&
		m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID])
	{
		int Score = m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID]->m_Score;
		str_format(aBuf, sizeof(aBuf), "%d", Score);
	}
	else if(m_pClient->m_Snap.m_pLocalInfo)
	{
		int Score = m_pClient->m_Snap.m_pLocalInfo->m_Score;
		str_format(aBuf, sizeof(aBuf), "%d", Score);
	}
	
	float tw = TextRender()->TextWidth(0, TitleFontsize, aBuf, -1);
	TextRender()->Text(0, x + w - tw - 20.0f, y, TitleFontsize, aBuf, -1);

	// calculate measurements
	x += 10.0f;
	float LineHeight = (h-y)/(m_NumPlayers / 2 + (m_NumPlayers % 2 == 1 ? 1 : 0));
	//float LineHeight = 35.0f;
	float TeeSizeMod = 0.7f;
	float Spacing = 4.0f;
	float FontSize = 24.0f;

	if(m_NumPlayers > 40)
	{
		
		TeeSizeMod = 0.5f;
		Spacing = 3.0f;
		FontSize = 16.0f;
	}

	float ScoreOffset = x + 10.0f, ScoreLength = 60.0f;
	float TeeOffset = ScoreOffset + ScoreLength, TeeLength = 60 * TeeSizeMod;
	float NameOffset = TeeOffset + TeeLength, NameLength = 300.0f - TeeLength;
	float PingOffset = x + 610.0f, PingLength = 65.0f;
	float CountryOffset = PingOffset - (LineHeight - Spacing - TeeSizeMod*5.0f)*2.0f, CountryLength = (LineHeight - Spacing - TeeSizeMod*5.0f)*2.0f;
	float ClanOffset = x + 370.0f, ClanLength = 230.0f - CountryLength;

	bool ShowIDs = m_pClient->m_pControls->m_InputShowIDs == 1;

	// render headlines
	y += 50.0f;
	float HeadlineFontsize = 22.0f;
	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Score"), -1);
	if(ShowIDs == false)
		TextRender()->Text(0, ScoreOffset + ScoreLength - tw, y, HeadlineFontsize, Localize("Score"), -1);
	else
		TextRender()->Text(0, ScoreOffset + ScoreLength - tw, y, HeadlineFontsize, Localize("ID"), 0);
	TextRender()->Text(0, NameOffset, y, HeadlineFontsize, Localize("Name"), -1);
	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Clan"), -1);
	TextRender()->Text(0, ClanOffset + ClanLength / 2 - tw / 2, y, HeadlineFontsize, Localize("Clan"), -1);
	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Ping"), -1);
	TextRender()->Text(0, PingOffset + PingLength - tw, y, HeadlineFontsize, Localize("Ping"), -1);

	// render again on right side
	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Score"), -1);
	if (ShowIDs == false)
		TextRender()->Text(0, w / 2 + ScoreOffset + ScoreLength - tw, y, HeadlineFontsize, Localize("Score"), -1);
	else
		TextRender()->Text(0, w / 2 + ScoreOffset + ScoreLength - tw, y, HeadlineFontsize, Localize("ID"), 0);
	TextRender()->Text(0, w / 2 + NameOffset, y, HeadlineFontsize, Localize("Name"), -1);
	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Clan"), -1);
	TextRender()->Text(0, w / 2 + ClanOffset + ClanLength / 2 - tw / 2, y, HeadlineFontsize, Localize("Clan"), -1);
	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Ping"), -1);
	TextRender()->Text(0, w / 2 + PingOffset + PingLength - tw, y, HeadlineFontsize, Localize("Ping"), -1);

	// render player entries
	y += HeadlineFontsize*2.0f;
	CTextCursor Cursor;
	int Found = 0;
	int Max = m_NumPlayers - m_NumSpectators;
	int numEntries = m_NumPlayers/2 + (m_NumPlayers % 2 == 1 ? 1 : 0);// num entries per side
	bool Grid = false;
	bool Switched = false;

	//for(int j = 0; j < 2; j++) // render active players first, then spectators
	{
		for (int i = 0; i < m_lPlayers.size(); i++)
		{
			int ID = m_lPlayers[i].m_ID;
			// make sure that we render the correct team
			const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[ID];
			if(!pInfo /*|| 
				(j == 0 & pInfo->m_Team == TEAM_SPECTATORS) ||
				(j == 1 & pInfo->m_Team != TEAM_SPECTATORS)*/)
				continue;

			// left side is full, continue on right side
			if(Found == numEntries && !Switched)  
			{
				x += w / 2;
				y -= numEntries * (LineHeight + Spacing);
				ScoreOffset = x + 10.0f, ScoreLength = 60.0f;
				TeeOffset = ScoreOffset + ScoreLength, TeeLength = 60 * TeeSizeMod;
				NameOffset = TeeOffset + TeeLength, NameLength = 300.0f - TeeLength;
				PingOffset = x + 610.0f, PingLength = 65.0f;
				CountryOffset = PingOffset - (LineHeight - Spacing - TeeSizeMod*5.0f)*2.0f, CountryLength = (LineHeight - Spacing - TeeSizeMod*5.0f)*2.0f;
				ClanOffset = x + 370.0f, ClanLength = 230.0f - CountryLength;
				Switched = true;
				Grid = true;
			}

			// background so it's easy to find the local or tagged player
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			if(pInfo->m_Team == TEAM_SPECTATORS)
				Graphics()->SetColor(0.4f, 0.2f, 0.4f, 0.5f);
			else if(pInfo->m_Local || (m_pClient->m_Snap.m_SpecInfo.m_Active && pInfo->m_ClientID == m_pClient->m_Snap.m_SpecInfo.m_SpectatorID))
				Graphics()->SetColor(1.0f, 0.6f, 0.2f, 0.5f);
			else if(Client()->IsDDRace() && m_pClient->m_pControls->m_BlockingPlayer == pInfo->m_ClientID)
				Graphics()->SetColor(0.6f, 0.0f, 0.0f, 0.5f);
			else if(m_pClient->m_aClients[pInfo->m_ClientID].m_Friend)
				Graphics()->SetColor(0.2f, 1.0f, 0.2f, 0.25f);
			//else if(m_pClient->m_aClients[pInfo->m_ClientID].m_Friend == 2)
				//Graphics()->SetColor(1.0f, 0.2f, 0.2f, 0.25f);
			else
			{
				if(Grid)
					Graphics()->SetColor(0.25f, 0.25f, 0.25f, 0.25f);
				else
					Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.25f);
			}
			RenderTools()->DrawRoundRect(x - 10.0f, y, w / 2, LineHeight + Spacing, 0.0f);
			Graphics()->QuadsEnd();
			Grid ^= 1;

			// score
			if (ShowIDs == false)
				str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Score, -999, 999));
			else
				str_format(aBuf, sizeof(aBuf), "%02d", pInfo->m_ClientID);

			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->SetCursor(&Cursor, ScoreOffset - tw + (ShowIDs ? ScoreLength * 0.5f : ScoreLength), y + Spacing, FontSize, TEXTFLAG_RENDER | TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = ScoreLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);

			// flag
			if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS &&
				m_pClient->m_Snap.m_pGameDataObj && (m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierRed == pInfo->m_ClientID ||
				m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierBlue == pInfo->m_ClientID))
			{
				Graphics()->BlendNormal();
				Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
				Graphics()->QuadsBegin();

				RenderTools()->SelectSprite(pInfo->m_Team == TEAM_RED ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

				float Size = LineHeight;
				IGraphics::CQuadItem QuadItem(TeeOffset + 0.0f, y - 5.0f - Spacing / 2.0f, Size / 2.0f, Size);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
				Graphics()->QuadsEnd();
			}

			// avatar
			CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientID].m_RenderInfo;
			TeeInfo.m_Size *= TeeSizeMod;
			RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(TeeOffset + TeeLength / 2, y + (LineHeight + Spacing) / 2));

			// name
			TextRender()->SetCursor(&Cursor, NameOffset, y - FontSize * 0.7f + LineHeight / 2, FontSize, TEXTFLAG_RENDER | TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = NameLength;
			TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);

			// clan
			tw = TextRender()->TextWidth(0, FontSize, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);
			TextRender()->SetCursor(&Cursor, ClanOffset + ClanLength / 2 - tw / 2, y - FontSize * 0.7f + LineHeight / 2, FontSize, TEXTFLAG_RENDER | TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = ClanLength;
			TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);

			// country flag
			vec4 Color(1.0f, 1.0f, 1.0f, 0.5f);
			m_pClient->m_pCountryFlags->Render(m_pClient->m_aClients[pInfo->m_ClientID].m_Country, &Color,
				CountryOffset, y + (Spacing + TeeSizeMod*5.0f) / 2.0f, CountryLength, LineHeight - Spacing - TeeSizeMod*5.0f);

			// ping
			if(pInfo->m_Team != TEAM_SPECTATORS)
			{
				float PingFactor = 1.0f - (clamp(pInfo->m_Latency, MIN_PING, MAX_PING) / (float)MAX_PING);
				float Hue = PingFactor * (120.0f / 360.0f);
				float Saturation = 0.45f;
				vec3 PingColor = HslToRgb(vec3(Hue, Saturation, 0.75f));
				TextRender()->TextColor(PingColor.r, PingColor.g, PingColor.b, 1.0f);

				str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Latency, 0, 9999));
				tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
				TextRender()->SetCursor(&Cursor, PingOffset + PingLength - tw, y + Spacing, FontSize, TEXTFLAG_RENDER | TEXTFLAG_STOP_AT_END);
				Cursor.m_LineWidth = PingLength;
				TextRender()->TextEx(&Cursor, aBuf, -1);

				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
			}
			Found++;
			y += LineHeight + Spacing;
		}
		Max = Found + m_NumSpectators;
		Grid ^= 1;
	}


}

void CScoreboard::RenderScoreboard(float x, float y, float w, float h, int Team, const char *pTitle, int Corners)
{
	//if(Team == TEAM_SPECTATORS)
	//	return;

	bool isTeam = false;

	if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
		isTeam = true;

	if(m_NumPlayers > 16 && !isTeam)
	{

		RenderScoreboard64(x - w / 2, y, w*2, h, pTitle);
		return;
	}

	// background
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.5f);
	RenderTools()->DrawRoundRectExt(x, y+50, w, h-50, 17.0f, Team == TEAM_SPECTATORS ? CUI::CORNER_B : 0);
	
	if(isTeam)
	{
		if(Team == TEAM_RED)
			Graphics()->SetColor(1.0f, 0.2f, 0.2f, 0.5f);
		else if(Team == TEAM_BLUE)
			Graphics()->SetColor(0.2f, 0.2f, 1.0f, 0.5f);
		else
			Graphics()->SetColor(0.4f, 0.2f, 0.4f, 0.5f);
		RenderTools()->DrawRoundRectExt(x, y, w, 50, 17.0f, Corners);
	}
	else
	{
		if(Team == TEAM_SPECTATORS)
		{
			Graphics()->SetColor(0.4f, 0.2f, 0.4f, 0.5f);
			RenderTools()->DrawRoundRectExt(x, y, w, 50, 17.0f, Corners);
		}
		else
		{
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
			RenderTools()->DrawRoundRectExt(x, y, w, 50, 17.0f, CUI::CORNER_T);
		}
	}
	Graphics()->QuadsEnd();

	// render title
	float TitleFontsize = 40.0f;
	if(!pTitle)
	{
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
			pTitle = Localize("Game over");
		else
			pTitle = Localize("Score board");
	}
	TextRender()->Text(0, x+20.0f, y, TitleFontsize, pTitle, -1);

	char aBuf[128] = {0};
	if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS)
	{
		if(m_pClient->m_Snap.m_pGameDataObj)
		{
			int Score = Team == TEAM_RED ? m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed : m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue;
			str_format(aBuf, sizeof(aBuf), "%d", Score);
		}
	}
	else
	{
		if(m_pClient->m_Snap.m_SpecInfo.m_Active && m_pClient->m_Snap.m_SpecInfo.m_SpectatorID != SPEC_FREEVIEW &&
			m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID])
		{
			int Score = m_pClient->m_Snap.m_paPlayerInfos[m_pClient->m_Snap.m_SpecInfo.m_SpectatorID]->m_Score;
			str_format(aBuf, sizeof(aBuf), "%d", Score);
		}
		else if(m_pClient->m_Snap.m_pLocalInfo)
		{
			int Score = m_pClient->m_Snap.m_pLocalInfo->m_Score;
			str_format(aBuf, sizeof(aBuf), "%d", Score);
		}
	}
	float tw = TextRender()->TextWidth(0, TitleFontsize, aBuf, -1);
	if(Team != TEAM_SPECTATORS)
		TextRender()->Text(0, x+w-tw-20.0f, y, TitleFontsize, aBuf, -1);

	// calculate measurements
	x += 10.0f;
	float LineHeight = 60.0f;
	float TeeSizeMod = 0.0f;
	float Spacing = 16.0f;
	if(m_pClient->m_Snap.m_aTeamSize[Team] > 12)
	{
		LineHeight = 40.0f;
		TeeSizeMod = 0.8f;
		Spacing = 0.0f;
	}
	else if(m_pClient->m_Snap.m_aTeamSize[Team] > 8)
	{
		LineHeight = 50.0f;
		TeeSizeMod = 0.9f;
		Spacing = 8.0f;
	}

	//test
		LineHeight = 35.0f;
		TeeSizeMod = 0.7f;
		Spacing = 4.0f;

	float ScoreOffset = x+10.0f, ScoreLength = 60.0f;
	float TeeOffset = ScoreOffset+ScoreLength, TeeLength = 60*TeeSizeMod;
	float NameOffset = TeeOffset+TeeLength, NameLength = 300.0f-TeeLength;
	float PingOffset = x+610.0f, PingLength = 65.0f;
	float CountryOffset = PingOffset-(LineHeight-Spacing-TeeSizeMod*5.0f)*2.0f, CountryLength = (LineHeight-Spacing-TeeSizeMod*5.0f)*2.0f;
	float ClanOffset = x+370.0f, ClanLength = 230.0f-CountryLength;

	bool ShowIDs = m_pClient->m_pControls->m_InputShowIDs == 1;

	// render headlines
	y += 50.0f;
	float HeadlineFontsize = 22.0f;
	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Score"), -1);

	if(ShowIDs == false)
		TextRender()->Text(0, ScoreOffset+ScoreLength-tw, y, HeadlineFontsize, Localize("Score"), -1);
	else
		TextRender()->Text(0, ScoreOffset + ScoreLength - tw, y, HeadlineFontsize, Localize("ID"), 0);

	TextRender()->Text(0, NameOffset, y, HeadlineFontsize, Localize("Name"), -1);

	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Clan"), -1);
	TextRender()->Text(0, ClanOffset+ClanLength/2-tw/2, y, HeadlineFontsize, Localize("Clan"), -1);

	tw = TextRender()->TextWidth(0, HeadlineFontsize, Localize("Ping"), -1);
	if(Team != TEAM_SPECTATORS)
		TextRender()->Text(0, PingOffset+PingLength-tw, y, HeadlineFontsize, Localize("Ping"), -1);

	// render player entries
	y += HeadlineFontsize*2.0f;
	float FontSize = 24.0f;
	CTextCursor Cursor;

	for(int i = 0; i < m_lPlayers.size(); i++)
	{
		int ID = m_lPlayers[i].m_ID;
		// make sure that we render the correct team
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[ID];
		if(!pInfo || pInfo->m_Team != Team)
			continue;

		// background for local or tagged players
		if(pInfo->m_Local || (m_pClient->m_Snap.m_SpecInfo.m_Active && pInfo->m_ClientID == m_pClient->m_Snap.m_SpecInfo.m_SpectatorID) || m_pClient->m_aClients[pInfo->m_ClientID].m_Friend)
		{
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			if(pInfo->m_Local || (m_pClient->m_Snap.m_SpecInfo.m_Active && pInfo->m_ClientID == m_pClient->m_Snap.m_SpecInfo.m_SpectatorID))
				Graphics()->SetColor(1.0f, 0.6f, 0.2f, 0.5f);
			else if (Client()->IsDDRace() && m_pClient->m_pControls->m_BlockingPlayer == pInfo->m_ClientID)
				Graphics()->SetColor(0.6f, 0.0f, 0.0f, 0.5f);
			else if(m_pClient->m_aClients[pInfo->m_ClientID].m_Friend)
				Graphics()->SetColor(0.2f, 1.0f, 0.2f, 0.25f);
			//else if(m_pClient->m_aClients[pInfo->m_ClientID].m_Friend == 2)
				//Graphics()->SetColor(1.0f, 0.2f, 0.2f, 0.25f);
			RenderTools()->DrawRoundRect(x, y, w - 20.0f, LineHeight, 15.0f);
			Graphics()->QuadsEnd();
		}


		// score
		if (ShowIDs == false)
			str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Score, -999, 999));
		else
			str_format(aBuf, sizeof(aBuf), "%02d", pInfo->m_ClientID);
		tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
		TextRender()->SetCursor(&Cursor, ScoreOffset-tw + (ShowIDs ? ScoreLength * 0.5f : ScoreLength), y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = ScoreLength;
		TextRender()->TextEx(&Cursor, aBuf, -1);

		// flag
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS &&
			m_pClient->m_Snap.m_pGameDataObj && (m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierRed == pInfo->m_ClientID ||
			m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierBlue == pInfo->m_ClientID))
		{
			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

			RenderTools()->SelectSprite(pInfo->m_Team==TEAM_RED ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

			float Size = LineHeight;
			IGraphics::CQuadItem QuadItem(TeeOffset+0.0f, y-5.0f-Spacing/2.0f, Size/2.0f, Size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		// avatar
		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[pInfo->m_ClientID].m_RenderInfo;
		TeeInfo.m_Size *= TeeSizeMod;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(TeeOffset+TeeLength/2, y+(LineHeight+Spacing)/2));

		// name
		TextRender()->SetCursor(&Cursor, NameOffset, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = NameLength;
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aName, -1);

		// clan
		tw = TextRender()->TextWidth(0, FontSize, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);
		TextRender()->SetCursor(&Cursor, ClanOffset+ClanLength/2-tw/2, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = ClanLength;
		TextRender()->TextEx(&Cursor, m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, -1);

		// country flag
		vec4 Color(1.0f, 1.0f, 1.0f, 0.5f);
		m_pClient->m_pCountryFlags->Render(m_pClient->m_aClients[pInfo->m_ClientID].m_Country, &Color,
											CountryOffset, y+(Spacing+TeeSizeMod*5.0f)/2.0f, CountryLength, LineHeight-Spacing-TeeSizeMod*5.0f);

		// ping
		if(Team != TEAM_SPECTATORS)
		{
			float PingFactor = 1.0f - (clamp(pInfo->m_Latency, MIN_PING, MAX_PING) / (float)MAX_PING);
			float Hue = PingFactor * (120.0f / 360.0f);
			float Saturation = 0.45f;
			vec3 PingColor = HslToRgb(vec3(Hue, Saturation, 0.75f));
			TextRender()->TextColor(PingColor.r, PingColor.g, PingColor.b, 1.0f);

			str_format(aBuf, sizeof(aBuf), "%d", clamp(pInfo->m_Latency, 0, 9999));
			tw = TextRender()->TextWidth(0, FontSize, aBuf, -1);
			TextRender()->SetCursor(&Cursor, PingOffset+PingLength-tw, y+Spacing, FontSize, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
			Cursor.m_LineWidth = PingLength;
			TextRender()->TextEx(&Cursor, aBuf, -1);

			TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
		}

		y += LineHeight+Spacing;
	}
}

void CScoreboard::RenderRecordingNotification(float x)
{
	if(!m_pClient->DemoRecorder()->IsRecording())
		return;

	//draw the box
	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.4f);
	RenderTools()->DrawRoundRectExt(x, 0.0f, 180.0f, 50.0f, 15.0f, CUI::CORNER_B);
	Graphics()->QuadsEnd();

	//draw the red dot
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 0.0f, 0.0f, 1.0f);
	RenderTools()->DrawRoundRect(x+20, 15.0f, 20.0f, 20.0f, 10.0f);
	Graphics()->QuadsEnd();

	//draw the text
	char aBuf[64];
	int Seconds = m_pClient->DemoRecorder()->Length();
	str_format(aBuf, sizeof(aBuf), Localize("REC %3d:%02d"), Seconds/60, Seconds%60);
	TextRender()->Text(0, x+50.0f, 10.0f, 20.0f, aBuf, -1);
}

void CScoreboard::UpdatePlayerList()
{
	static CNetObj_PlayerInfo s_ScoreboardList[MAX_CLIENTS] = {};

	if (mem_comp(s_ScoreboardList, m_pClient->m_Snap.m_paPlayerInfos, sizeof(s_ScoreboardList) || m_Sortion != GetSortion(g_Config.m_PdlScoreboardSortion)) != 0)
	{
		mem_copy(s_ScoreboardList, m_pClient->m_Snap.m_paPlayerInfos, sizeof(s_ScoreboardList));//new scoreboard
		m_Sortion = GetSortion(g_Config.m_PdlScoreboardSortion);
		m_lPlayers.clear();

		for (int i = 0; i < MAX_CLIENTS; i++)
		{
			if (m_pClient->m_Snap.m_paPlayerInfos[i] == 0x0)
				continue;

			CPlayerItem PlayerItem;
			PlayerItem.m_ID = i;
			PlayerItem.m_pClient = m_pClient;
			m_lPlayers.add(PlayerItem);
		}
	}
}

void CScoreboard::OnRender()
{
	if(!Active())
		return;

	UpdatePlayerList();

	// if the score board is active, then we should clear the motd message aswell
	if(m_pClient->m_pMotd->IsActive())
		m_pClient->m_pMotd->Clear();


	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width, Height);

	float w = 700.0f;
	float h = 724.0f;
	float TeamHeight = 0;
	int TeamSize = 0;
	m_NumSpectators = 0;
	m_NumPlayers = 0;
	

	for(int n = 0, j = TEAM_SPECTATORS; j < 2; j++)
	{
		n = 0;
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[i];
			if(!pInfo || pInfo->m_Team != j)
				continue;
			n++;
			m_NumPlayers++;
		}
		if(j == TEAM_SPECTATORS)
			m_NumSpectators = n;
		else if(n > TeamSize)
			TeamSize = n;
		
	}

	if(TeamSize <= 8 && m_NumSpectators <= 8)
		TeamHeight = 8 * 39.0f;
	else
		TeamHeight = (/*MAX_CLIENTS*/ 16-TeamSize) * 39.0f;

	if(m_pClient->m_Snap.m_pGameInfoObj)
	{
		if(!(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_TEAMS))
		{
			if(m_NumPlayers <= 16)
			{
				RenderScoreboard(Width / 2 - w / 2, 200.0f, w, h - TeamHeight, 0, 0, CUI::CORNER_ALL);
				RenderScoreboard(Width / 2 - w / 2, 200.0f + (h - TeamHeight), w, TeamHeight + 94.0f, TEAM_SPECTATORS, Localize("Spectators"), 0);
			}
			else
				RenderScoreboard(Width / 2 - w / 2, 200.0f, w, h, 0, 0, CUI::CORNER_ALL);
		}
		else
		{
			const char *pRedClanName = GetClanName(TEAM_RED);
			const char *pBlueClanName = GetClanName(TEAM_BLUE);

			if(m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER && m_pClient->m_Snap.m_pGameDataObj)
			{
				char aText[256];
				str_copy(aText, Localize("Draw!"), sizeof(aText));

				if(m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed > m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue)
				{
					if(pRedClanName)
						str_format(aText, sizeof(aText), Localize("%s wins!"), pRedClanName);
					else
						str_copy(aText, Localize("Red team wins!"), sizeof(aText));
				}
				else if(m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreBlue > m_pClient->m_Snap.m_pGameDataObj->m_TeamscoreRed)
				{
					if(pBlueClanName)
						str_format(aText, sizeof(aText), Localize("%s wins!"), pBlueClanName);
					else
						str_copy(aText, Localize("Blue team wins!"), sizeof(aText));
				}

				float w = TextRender()->TextWidth(0, 86.0f, aText, -1);
				TextRender()->Text(0, Width/2-w/2, 39, 86.0f, aText, -1);
			}
			RenderScoreboard(Width/2-w, 200.0f, w, h-TeamHeight, TEAM_RED, pRedClanName ? pRedClanName : Localize("Red team"), CUI::CORNER_TL);
			RenderScoreboard(Width/2, 200.0f, w, h-TeamHeight, TEAM_BLUE, pBlueClanName ? pBlueClanName : Localize("Blue team"), CUI::CORNER_TR);
			RenderScoreboard(Width/2-w, 200.0f + (h-TeamHeight), w*2, TeamHeight + 94.0f, TEAM_SPECTATORS, Localize("Spectators"), 0);
		}
		
	}

	
	//RenderGoals(Width/2-w/2, 150+760+10, w);
	//RenderSpectators(Width/2-w/2, 150+760+10+50+10, w);
	RenderRecordingNotification((Width/7)*4);
}

bool CScoreboard::Active()
{
	if(g_Config.m_Josystick && Input()->JoystickButton(6))
		return true;

	// if we activly wanna look on the scoreboard
	if(m_Active)
		return true;

	if(m_pClient->m_Snap.m_pLocalInfo && m_pClient->m_Snap.m_pLocalInfo->m_Team != TEAM_SPECTATORS)
	{
		// we are not a spectator, check if we are dead
		if(!m_pClient->m_Snap.m_pLocalCharacter && g_Config.m_PdlScoreboardDeadActive == 1)
			return true;
	}

	// if the game is over
	if(m_pClient->m_Snap.m_pGameInfoObj && m_pClient->m_Snap.m_pGameInfoObj->m_GameStateFlags&GAMESTATEFLAG_GAMEOVER)
		return true;

	return false;
}

const char *CScoreboard::GetClanName(int Team)
{
	int ClanPlayers = 0;
	const char *pClanName = 0;
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		const CNetObj_PlayerInfo *pInfo = m_pClient->m_Snap.m_paPlayerInfos[i];
		if(!pInfo || pInfo->m_Team != Team)
			continue;

		if(!pClanName)
		{
			pClanName = m_pClient->m_aClients[pInfo->m_ClientID].m_aClan;
			ClanPlayers++;
		}
		else
		{
			if(str_comp(m_pClient->m_aClients[pInfo->m_ClientID].m_aClan, pClanName) == 0)
				ClanPlayers++;
			else
				return 0;
		}
	}

	if(ClanPlayers > 1 && pClanName[0])
		return pClanName;
	else
		return 0;
}

int CScoreboard::GetSortion(const char *pSort)
{
	if (str_comp_nocase(pSort, "id") == 0)
		return SORTION_ID;
	else if (str_comp_nocase(pSort, "name") == 0)
		return SORTION_NAME;
	else if (str_comp_nocase(pSort, "score") == 0)
		return SORTION_SCORE;
	else if (str_comp_nocase(pSort, "clan") == 0)
		return SORTION_CLAN;
	else if (str_comp_nocase(pSort, "latency") == 0)
		return SORTION_LATENCY;
	else if (str_comp_nocase(pSort, "country") == 0)
		return SORTION_COUNTRY;
	else
		return SORTION_ID;
}