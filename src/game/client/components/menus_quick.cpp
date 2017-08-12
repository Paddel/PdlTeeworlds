
#include <engine/shared/config.h>

#include <game/client/components/controls.h>

#include "menus.h"

void CMenus::RenderQuickMenu(CUIRect MainView)
{
	CUIRect Top, Bottom;
	MainView.HSplitTop(100.0f, &Top, 0x0);
	MainView.HSplitBottom(64.0f, 0x0, &Bottom);

	RenderQuickMenuTop(Top);
	RenderQuickMenuBottom(Bottom);

	if(UI()->MouseInside(&Top) || UI()->MouseInside(&Bottom))
		m_QuickMenuLock = true;
	else
		m_QuickMenuLock = false;
}

void CMenus::RenderQuickMenuTop(CUIRect MainView)
{
	CUIRect Button;

	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.5f);
	IGraphics::CQuadItem QuadItem(MainView.x, MainView.y, MainView.w, MainView.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	MainView.Margin(10.0f, &MainView);
	MainView.HSplitTop(30.0f, &Button, 0x0);
	Button.VSplitLeft(128.0, &Button, 0x0);

	static int s_DummyToggle;
	if(DoButton_Menu(&s_DummyToggle, Client()->GetDummyActive(0)?"Disconnect":"Connect", Client()->GetDummyActive(0), &Button))
		Client()->DummyToggle(0);
}

void CMenus::RenderQuickMenuBottom(CUIRect MainView)
{
	CUIRect Button;

	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(0.0f, 0.0f, 0.0f, 0.5f);
	IGraphics::CQuadItem QuadItem(MainView.x, MainView.y, MainView.w, MainView.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	MainView.Margin(10.0f, &MainView);

	MainView.HSplitTop(30.0f, &Button, 0x0);
	static int s_Gamelayer = -1;
	if(DoButton_CheckBox(&s_Gamelayer, "Gamelayer", g_Config.m_PdlGamelayer, &Button))
		g_Config.m_PdlGamelayer ^= 1;

	Button.HSplitTop(26.0f, 0x0, &Button);
	Button.HSplitTop(26.0f, &Button, 0x0);

	static int s_GamelayerBack = -1;
	if(g_Config.m_PdlGamelayer)
	{
		if(DoButton_CheckBox(&s_GamelayerBack, "Own Background", g_Config.m_PdlGamelayerBack, &Button))
			g_Config.m_PdlGamelayerBack ^= 1;
	}

	//move right
	MainView.VSplitLeft(256.0f, 0x0, &MainView);
	MainView.HSplitTop(26.0f, &Button, 0x0);

	static int s_SaveJmp = -1;
	if(DoButton_CheckBox(&s_SaveJmp, "Save jump", m_pClient->m_pControls->m_FakeInput == FAKEINPUT_SAVEJUMP, &Button))
	{
		if(m_pClient->m_pControls->m_FakeInput != FAKEINPUT_SAVEJUMP)
			m_pClient->m_pControls->m_FakeInput = FAKEINPUT_SAVEJUMP;
		else
			m_pClient->m_pControls->m_FakeInput = FAKEINPUT_NONE;
	}

	Button.HSplitTop(26.0f, 0x0, &Button);
	Button.HSplitTop(26.0f, &Button, 0x0);

	if(m_pClient->m_pControls->m_FakeInput == FAKEINPUT_SAVEJUMP)
	{
		static int s_SaveJmpShoot = -1;
		if(DoButton_CheckBox(&s_SaveJmpShoot, "Shoot", g_Config.m_PdlSaveJumpShoot, &Button))
			g_Config.m_PdlSaveJumpShoot ^= 1;
	}

	//move right
	MainView.VSplitLeft(256.0f, 0x0, &MainView);
	MainView.HSplitTop(26.0f, &Button, 0x0);

	static int s_Autounfreeze = -1;
	if(DoButton_CheckBox(&s_Autounfreeze, "Autounfreeze", m_pClient->m_pControls->m_FakeInput == FAKEINPUT_AUTOUNFREEZE, &Button))
	{
		if(m_pClient->m_pControls->m_FakeInput != FAKEINPUT_AUTOUNFREEZE)
			m_pClient->m_pControls->m_FakeInput = FAKEINPUT_AUTOUNFREEZE;
		else
			m_pClient->m_pControls->m_FakeInput = FAKEINPUT_NONE;
	}

	Button.HSplitTop(26.0f, 0x0, &Button);
	Button.HSplitTop(26.0f, &Button, 0x0);
}