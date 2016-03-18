/* (c) Magnus Auvinen. See licence.txt in the root of the distribution for more information. */
/* If you are missing that file, acquire a complete release at teeworlds.com.                */
#include <engine/shared/config.h>
#include <engine/graphics.h>
#include <engine/textrender.h>
#include <game/generated/protocol.h>
#include <game/generated/client_data.h>

#include <game/client/gameclient.h>

#include <game/client/components/motd.h>
#include <game/client/components/scoreboard.h>

#include "broadcast.h"

void CBroadcast::OnReset()
{
	m_BroadcastTime = 0;
}

void CBroadcast::OnRender()
{
	if(m_pClient->m_pScoreboard->Active() || m_pClient->m_pMotd->IsActive())
		return;

	Graphics()->MapScreen(0, 0, 300*Graphics()->ScreenAspect(), 300);

	if(time_get() < m_BroadcastTime)
	{
		mem_copy(m_aShownText, m_aBroadcastText, sizeof(m_aShownText));
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, m_BroadcastRenderOffset, 40.0f, 12.0f, TEXTFLAG_RENDER|TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = 300*Graphics()->ScreenAspect()-m_BroadcastRenderOffset;

		float ms = (time_get() - m_AnimTime) / (float)time_freq() * 1000.0f;
		int Pos = clamp((int)(ms / 10), 0, 1024);
		if (Pos < str_length(m_aBroadcastText))
			m_aShownText[Pos] = '\0';

		TextRender()->TextEx(&Cursor, m_aShownText, -1);
	}
	else if (m_aShownText[0])
	{
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, m_BroadcastRenderOffset, 40.0f, 12.0f, TEXTFLAG_RENDER | TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = 300 * Graphics()->ScreenAspect() - m_BroadcastRenderOffset;

		float ms = (time_get() - m_BroadcastTime) / (float)time_freq() * 1000.0f;
		int Pos = str_length(m_aBroadcastText) - clamp((int)(ms / 10), 0, 1024);
		if (Pos > 0)
		{
			m_aShownText[Pos] = '\0';
			TextRender()->TextEx(&Cursor, m_aShownText, -1);
		}
		else
			mem_zero(&m_aShownText, sizeof(m_aShownText));
	}
}

void CBroadcast::OnMessage(int MsgType, void *pRawMsg)
{
	if(MsgType == NETMSGTYPE_SV_BROADCAST)
	{
		CNetMsg_Sv_Broadcast *pMsg = (CNetMsg_Sv_Broadcast *)pRawMsg;
		str_copy(m_aBroadcastText, pMsg->m_pMessage, sizeof(m_aBroadcastText));
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, 0, 0, 12.0f, TEXTFLAG_STOP_AT_END);
		Cursor.m_LineWidth = 300*Graphics()->ScreenAspect();
		TextRender()->TextEx(&Cursor, m_aBroadcastText, -1);
		m_BroadcastRenderOffset = 150*Graphics()->ScreenAspect()-Cursor.m_X/2;

		if (time_get() > m_BroadcastTime)
			m_AnimTime = time_get();

		m_BroadcastTime = time_get() + time_freq() * 10;
	}
}

