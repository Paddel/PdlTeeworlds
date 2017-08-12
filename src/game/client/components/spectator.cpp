
#include <engine/demo.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>
#include <game/generated/protocol.h>

#include <game/client/animstate.h>
#include <game/client/render.h>

#include <game/client/components/chat.h>

#include "spectator.h"


void CSpectator::ConKeySpectator(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_Active &&
		(pSelf->Client()->State() != IClient::STATE_DEMOPLAYBACK || pSelf->DemoPlayer()->GetDemoType() == IDemoPlayer::DEMOTYPE_SERVER))
		pSelf->m_Active = pResult->GetInteger(0) != 0;
}

void CSpectator::ConSpectate(IConsole::IResult *pResult, void *pUserData)
{
	((CSpectator *)pUserData)->Spectate(pResult->GetInteger(0));
}

void CSpectator::ConSpectateNext(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	int NewSpectatorID;
	bool GotNewSpectatorID = false;

	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
	{
		for(int i = 0; i < MAX_CLIENTS; i++)
		{
			if(!pSelf->m_pClient->m_Snap.m_paPlayerInfos[i] || pSelf->m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
		}
	}
	else
	{
		for(int i = pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID + 1; i < MAX_CLIENTS; i++)
		{
			if(!pSelf->m_pClient->m_Snap.m_paPlayerInfos[i] || pSelf->m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
		}

		if(!GotNewSpectatorID)
		{
			for(int i = 0; i < pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID; i++)
			{
				if(!pSelf->m_pClient->m_Snap.m_paPlayerInfos[i] || pSelf->m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
					continue;

				NewSpectatorID = i;
				GotNewSpectatorID = true;
				break;
			}
		}
	}
	if(GotNewSpectatorID)
		pSelf->Spectate(NewSpectatorID);
}

void CSpectator::ConSpectatePrevious(IConsole::IResult *pResult, void *pUserData)
{
	CSpectator *pSelf = (CSpectator *)pUserData;
	int NewSpectatorID;
	bool GotNewSpectatorID = false;

	if(pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
	{
		for(int i = MAX_CLIENTS -1; i > -1; i--)
		{
			if(!pSelf->m_pClient->m_Snap.m_paPlayerInfos[i] || pSelf->m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
		}
	}
	else
	{
		for(int i = pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID - 1; i > -1; i--)
		{
			if(!pSelf->m_pClient->m_Snap.m_paPlayerInfos[i] || pSelf->m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
				continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
			break;
		}

		if(!GotNewSpectatorID)
		{
			for(int i = MAX_CLIENTS - 1; i > pSelf->m_pClient->m_Snap.m_SpecInfo.m_SpectatorID; i--)
			{
				if(!pSelf->m_pClient->m_Snap.m_paPlayerInfos[i] || pSelf->m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
					continue;

			NewSpectatorID = i;
			GotNewSpectatorID = true;
				break;
			}
		}
	}
	if(GotNewSpectatorID)
		pSelf->Spectate(NewSpectatorID);
}

CSpectator::CSpectator()
{
	OnReset();
}

void CSpectator::OnConsoleInit()
{
	Console()->Register("+spectate", "", CFGFLAG_CLIENT, ConKeySpectator, this, "Open spectator mode selector");
	Console()->Register("spectate", "i", CFGFLAG_CLIENT, ConSpectate, this, "Switch spectator mode");
	Console()->Register("spectate_next", "", CFGFLAG_CLIENT, ConSpectateNext, this, "Spectate the next player");
	Console()->Register("spectate_previous", "", CFGFLAG_CLIENT, ConSpectatePrevious, this, "Spectate the previous player");
}

bool CSpectator::OnMouseMove(float x, float y)
{
	if(!m_Active)
		return false;

	UI()->ConvertMouseMove(&x, &y);
	m_SelectorMouse += vec2(x,y);
	return true;
}

void CSpectator::OnRelease()
{
	OnReset();
}

/*void CSpectator::OnRender()
{
	if(!m_Active)
	{
		if(m_WasActive)
		{
			if(m_SelectedSpectatorID != NO_SELECTION)
				Spectate(m_SelectedSpectatorID);
			m_WasActive = false;
		}
		return;
	}

	if(!m_pClient->m_Snap.m_SpecInfo.m_Active)
	{
		m_Active = false;
		m_WasActive = false;
		return;
	}

	m_WasActive = true;
	m_SelectedSpectatorID = NO_SELECTION;

	// draw background
	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;

	Graphics()->MapScreen(0, 0, Width, Height);

	Graphics()->BlendNormal();
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.3f);
	RenderTools()->DrawRoundRect(Width/2.0f-300.0f, Height/2.0f-300.0f, 600.0f, 600.0f, 20.0f);
	Graphics()->QuadsEnd();

	// clamp mouse position to selector area
	m_SelectorMouse.x = clamp(m_SelectorMouse.x, -280.0f, 280.0f);
	m_SelectorMouse.y = clamp(m_SelectorMouse.y, -280.0f, 280.0f);

	// draw selections
	float FontSize = 20.0f;
	float StartY = -190.0f;
	float LineHeight = 60.0f;
	bool Selected = false;

	if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SPEC_FREEVIEW)
	{
		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f);
		RenderTools()->DrawRoundRect(Width/2.0f-280.0f, Height/2.0f-280.0f, 270.0f, 60.0f, 20.0f);
		Graphics()->QuadsEnd();
	}

	if(m_SelectorMouse.x >= -280.0f && m_SelectorMouse.x <= -10.0f &&
		m_SelectorMouse.y >= -280.0f && m_SelectorMouse.y <= -220.0f)
	{
		m_SelectedSpectatorID = SPEC_FREEVIEW;
		Selected = true;
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
	TextRender()->Text(0, Width/2.0f-240.0f, Height/2.0f-265.0f, FontSize, Localize("Free-View"), -1);

	float x = -270.0f, y = StartY;
	for(int i = 0, Count = 0; i < MAX_CLIENTS; ++i)
	{
		if(!m_pClient->m_Snap.m_paPlayerInfos[i] || m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
			continue;

		if(++Count%9 == 0)
		{
			x += 290.0f;
			y = StartY;
		}

		if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == i)
		{
			Graphics()->TextureSet(-1);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.25f);
			RenderTools()->DrawRoundRect(Width/2.0f+x-10.0f, Height/2.0f+y-10.0f, 270.0f, 60.0f, 20.0f);
			Graphics()->QuadsEnd();
		}

		Selected = false;
		if(m_SelectorMouse.x >= x-10.0f && m_SelectorMouse.x <= x+260.0f &&
			m_SelectorMouse.y >= y-10.0f && m_SelectorMouse.y <= y+50.0f)
		{
			m_SelectedSpectatorID = i;
			Selected = true;
		}
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
		TextRender()->Text(0, Width/2.0f+x+50.0f, Height/2.0f+y+5.0f, FontSize, m_pClient->m_aClients[i].m_aName, 220.0f);

		// flag
		if(m_pClient->m_Snap.m_pGameInfoObj->m_GameFlags&GAMEFLAG_FLAGS &&
			m_pClient->m_Snap.m_pGameDataObj && (m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierRed == m_pClient->m_Snap.m_paPlayerInfos[i]->m_ClientID ||
			m_pClient->m_Snap.m_pGameDataObj->m_FlagCarrierBlue == m_pClient->m_Snap.m_paPlayerInfos[i]->m_ClientID))
		{
			Graphics()->BlendNormal();
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GAME].m_Id);
			Graphics()->QuadsBegin();

			RenderTools()->SelectSprite(m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team==TEAM_RED ? SPRITE_FLAG_BLUE : SPRITE_FLAG_RED, SPRITE_FLAG_FLIP_X);

			float Size = LineHeight;
			IGraphics::CQuadItem QuadItem(Width/2.0f+x-LineHeight/5.0f, Height/2.0f+y-LineHeight/3.0f, Size/2.0f, Size);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[i].m_RenderInfo;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(Width/2.0f+x+20.0f, Height/2.0f+y+20.0f));

		y += LineHeight;
	}
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

	// draw cursor
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	IGraphics::CQuadItem QuadItem(m_SelectorMouse.x+Width/2.0f, m_SelectorMouse.y+Height/2.0f, 48.0f, 48.0f);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}*/

void CSpectator::OnRender()
{
	static int showHud = -1;

	if(!m_Active)
	{
		if(m_WasActive)
		{
			if(m_SelectedSpectatorID != NO_SELECTION)
				Spectate(m_SelectedSpectatorID);
			m_WasActive = false;
			
			if(showHud != -1)
			{
				g_Config.m_ClShowhud=showHud;
				showHud = -1;
			}
			//m_pClient->m_pChat->m_Render = 1;
		}
		return;
	}
		
	if(g_Config.m_ClShowhud && showHud == -1)
		showHud = 1;
	else if(showHud == -1)
		showHud = 0;

	g_Config.m_ClShowhud = 0;
	//m_pClient->m_pChat->m_Render = 0;

	if(!m_pClient->m_Snap.m_SpecInfo.m_Active)
	{
		m_Active = false;
		m_WasActive = false;
		return;
	}

	m_WasActive = true;
	m_SelectedSpectatorID = NO_SELECTION;

	// draw background
	float Width = 400*3.0f*Graphics()->ScreenAspect();
	float Height = 400*3.0f;
	Graphics()->MapScreen(0, 0, Width, Height);

	CUIRect View, Selection, Row, Col[4], Freeview, Temp;
	View.x=0;
	View.y=0;
	View.w=Width;
	View.h=Height;
	View.HMargin(120.0f, &View);
	View.VMargin(300.0f, &View);
	View.HSplitTop(60.0f, &Freeview, &Selection);
	Selection.Margin(25.0f, &Selection);

	m_SelectorMouse.x = clamp(m_SelectorMouse.x, View.x, View.x+View.w-48.0f);
	m_SelectorMouse.y = clamp(m_SelectorMouse.y, View.y, View.y+View.h-48.0f);

	RenderTools()->DrawUIRect(&View, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 10.0f);
	
	static int SelectionHight = Selection.h;

	bool Selected = false;

	// free view
	{
		Freeview.HSplitTop(10.0f, 0, &Freeview);
		Freeview.VMargin(25.0f, &Freeview);
		RenderTools()->DrawUIRect(&Freeview, vec4(0.75f, 0.75f, 0.75f, 0.25f), CUI::CORNER_ALL, 5.0f);
		Selected = false;
		if(UI()->MouseInside(&Freeview, m_SelectorMouse))
		{
			m_SelectedSpectatorID = SPEC_FREEVIEW;
			Selected = true;
		}

		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
		UI()->DoLabel(&Freeview, "Free-View", 40.0f*UI()->Scale(), 0);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
	}

	// player selection
	int NumRender = 0;	
	for(int i = 0; i < MAX_CLIENTS; i++)
	{
		if(!m_pClient->m_Snap.m_paPlayerInfos[i] || m_pClient->m_Snap.m_paPlayerInfos[i]->m_Team == TEAM_SPECTATORS)
			continue;

		if(NumRender%4==0)
		{
			Selection.HSplitTop((int)SelectionHight/16.0f, &Row, &Selection);
			Row.VSplitMid(&Temp, &Row);
			Temp.VSplitMid(&Col[0], &Col[1]);
			Row.VSplitMid(&Col[2], &Col[3]);
		}

		Selected = false;
		if(UI()->MouseInside(&Col[NumRender%4], m_SelectorMouse))
		{
			m_SelectedSpectatorID = i;
			Selected = true;
		}
		
		vec4 BackColor(0, 0, 0, 0);
		if((NumRender%4)%2 == ((int)NumRender/4)%2)
			BackColor = vec4(0.75f, 0.75f, 0.75f, 0.25f);
			
		RenderTools()->DrawUIRect(&Col[NumRender%4], BackColor, CUI::CORNER_ALL, 5.0f);
		Col[NumRender%4].VSplitLeft(80.0f, &Temp, &Col[NumRender%4]);
		Col[NumRender%4].Margin(10.0f, &Col[NumRender%4]);

		CTeeRenderInfo TeeInfo = m_pClient->m_aClients[i].m_RenderInfo;
		
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, Selected?1.0f:0.5f);
		UI()->DoLabel(&Col[NumRender%4], m_pClient->m_aClients[i].m_aName, 20.0f*UI()->Scale(), -1);
		TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);

		TeeInfo.m_Size*=0.8f;
		RenderTools()->RenderTee(CAnimState::GetIdle(), &TeeInfo, EMOTE_NORMAL, vec2(1.0f, 0.0f), vec2(Temp.x+TeeInfo.m_Size/2.0f, Temp.y+TeeInfo.m_Size/2.0f));
		
		NumRender++;
	}

	// draw cursor
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	IGraphics::CQuadItem QuadItem(m_SelectorMouse.x, m_SelectorMouse.y, 48.0f, 48.0f);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();
}

void CSpectator::OnReset()
{
	m_WasActive = false;
	m_Active = false;
	m_SelectedSpectatorID = NO_SELECTION;
}

void CSpectator::Spectate(int SpectatorID)
{
	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		m_pClient->m_DemoSpecID = clamp(SpectatorID, (int)SPEC_FREEVIEW, MAX_CLIENTS-1);
		return;
	}

	if(m_pClient->m_Snap.m_SpecInfo.m_SpectatorID == SpectatorID)
		return;

	CNetMsg_Cl_SetSpectatorMode Msg;
	Msg.m_SpectatorID = SpectatorID;
	Client()->SendPackMsg(&Msg, MSGFLAG_VITAL);
}
