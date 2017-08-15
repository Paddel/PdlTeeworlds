
#include <engine/config.h>
#include <engine/textrender.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>
#include <game/client/components/identities.h>
#include <game/client/components/mapinker.h>
#include <game/client/animstate.h>

#include "skins.h"
#include "countryflags.h"
#include "menus.h"

#define PAGE_SIZE_MAX 24 * 3

CUIRect VariableButton;
static int s_VariableIDs[1024] = { };
static int s_VariableCounter = 0;
static CUIRect s_ResetView;
static char *s_pLastName = 0;
static float s_VariableOffset[1024] = { };

static CIdentities::CPlayerItem s_PlayerItems[PAGE_SIZE_MAX];
static int s_NumPlayerItems;
static int s_CopyButtonIDs[PAGE_SIZE_MAX];

void CMenus::RenderExtrasVariableInt(char *pName, char *pScriptName, int &Value, char *pDefault, char *pMin, char *pMax, char *pFlags, char *pDesc, void *pData)
{
	char aBuf[32];
	CMenus *pMenu = (CMenus *)pData;
	if(str_comp_num(pScriptName, "pdl", 3) != 0)
		return;

	pName += 3;

	if(VariableButton.y + VariableButton.h >= s_ResetView.y + s_ResetView.h)
	{
		VariableButton.y = s_ResetView.y;
		VariableButton.x += s_ResetView.w / 3.0f;
	}

	if(s_pLastName != 0 && str_comp_num(s_pLastName, pName, 3) == 0)
		VariableButton.y += VariableButton.h - 3.0f;
	else
		VariableButton.y += VariableButton.h + 4.0f;

	if(str_toint(pMin) == 0 && str_toint(pMax) == 1)
	{
		if(pMenu->DoButton_CheckBox(&s_VariableIDs[s_VariableCounter++], pName, Value, &VariableButton))
			Value = !Value;
	}
	else
	{
		pMenu->UI()->DoLabelScaled(&VariableButton, pName, 15.0f, 0, -1);
		VariableButton.y += VariableButton.h;

		CUIRect TextRect;
		VariableButton.VSplitRight(36.0f, &VariableButton, &TextRect);
		float Span = str_toint(pMax) - str_toint(pMin);
		Value = pMenu->DoScrollbarH(&s_VariableIDs[s_VariableCounter++], &VariableButton, (Value - str_toint(pMin)) / Span ) * Span + str_toint(pMin);
		str_format(aBuf, sizeof(aBuf), "%i", Value);
		TextRect.x += 8.0f;
		pMenu->UI()->DoLabel(&TextRect, aBuf, 12.0f, 1);

		VariableButton.w += 36.0f;
	}

	s_pLastName = pName;
}

void CMenus::RenderExtrasVariableStr(char *pName, char *pScriptName, char *pValue, char *pLength, char *pDefault, char *pFlags, char *pDesc, void *pData)
{
	char aBuf[32];
	CMenus *pMenu = (CMenus *)pData;
	if(str_comp_num(pScriptName, "pdl", 3) != 0)
		return;

	pName += 3;

	if(VariableButton.y + VariableButton.h * 2.0f >= s_ResetView.y + s_ResetView.h)
	{
		VariableButton.y = s_ResetView.y;
		VariableButton.x += s_ResetView.w / 3.0f;
	}

	if(s_pLastName != 0 && str_comp_num(s_pLastName, pName, 3) == 0)
		VariableButton.y += VariableButton.h + 3.0f;
	else
		VariableButton.y += VariableButton.h + 6.0f;

	pMenu->UI()->DoLabel(&VariableButton, pName, 10.0f, -1);
	float TextSize = pMenu->TextRender()->TextWidth(0, 10.0f, pName, -1);
	VariableButton.x += TextSize + 8.0f;
	VariableButton.w -= TextSize + 8.0f;
	pMenu->DoEditBox(pValue, &VariableButton, pValue, str_toint(pLength), 14.0f, &s_VariableOffset[s_VariableCounter++]);
	VariableButton.x -= TextSize + 8.0f;
	VariableButton.w += TextSize + 8.0f;

	s_pLastName = pName;
}

void CMenus::RenderExtrasGeneral(CUIRect MainView)
{
	IConfig *pConfig = Kernel()->RequestInterface<IConfig>();
	s_VariableCounter = 0;
	MainView.HSplitTop(18.0f, &VariableButton, 0x0);
	VariableButton.VSplitLeft(MainView.w / 3.0f, &VariableButton, 0x0);
	VariableButton.VSplitLeft(8.0f, 0x0, &VariableButton);
	s_ResetView = MainView;
	s_pLastName = 0x0;
	pConfig->GetAllConfigVariables(&RenderExtrasVariableInt, &RenderExtrasVariableStr, this);
}

void CMenus::GetMenuIdentityResult(int Index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows)
{
	if (Row == 0)
		s_NumPlayerItems = 0;

	CIdentities::CPlayerItem *pCurItem = &s_PlayerItems[s_NumPlayerItems];

	switch(Index)
	{
	case 0: str_copy(pCurItem->m_Info.m_aName, pResult, sizeof(pCurItem->m_Info.m_aName)); break;
	case 1: str_copy(pCurItem->m_Info.m_aClan, pResult, sizeof(pCurItem->m_Info.m_aClan)); break;
	case 2: pCurItem->m_Info.m_Country = str_toint(pResult); break;
	case 3: str_copy(pCurItem->m_Info.m_aSkin, pResult, sizeof(pCurItem->m_Info.m_aSkin)); break;
	case 4: pCurItem->m_Info.m_UseCustomColor = str_toint(pResult); break;
	case 5: pCurItem->m_Info.m_ColorBody = str_toint(pResult); break;
	case 6: pCurItem->m_Info.m_ColorFeet = str_toint(pResult); break;
	case 7: pCurItem->m_Latency = str_toint(pResult); break;
	}

	if(Index == 7)
		s_NumPlayerItems++;
}

void CMenus::RenderExtrasIdentities(CUIRect MainView)
{
	CUIRect Button, Line, Top, Slider, CountText;
	CTeeRenderInfo SkinInfo;
	static int s_Count = m_pClient->m_pIdentities->GetMenuCount("");
	static bool s_Refresh = true;
	static int s_LastSliderValue = 0;
	static int s_SliderID = 0;
	static char s_ConditionText[256] = { };

	int Columns = (int)(MainView.w / 256.0f);
	int PageSize = 24 * Columns;

	MainView.HSplitTop(50.0f, &Top, &Button);
	Button.VSplitLeft(8.0f, 0x0, &Button);

	Top.HSplitTop(30, &Top, &Slider);
	Top.HSplitTop(8.0f, 0x0, &Top);
	Top.VSplitLeft(256.0f, &Top, &CountText);
	Top.VSplitLeft(8.0f, 0x0, &Top);
	static float s_ContionOffset = 0;
	if(DoEditBox(s_ConditionText, &Top, s_ConditionText, sizeof(s_ConditionText), 14.0f, &s_ContionOffset))
	{
		s_Count = m_pClient->m_pIdentities->GetMenuCount(s_ConditionText);
		s_LastSliderValue = 0;
		s_Refresh = true;
	}

	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "Page:%i/%i", s_LastSliderValue, s_Count/ PageSize);
	UI()->DoLabel(&CountText, aBuf, 10.0f, 1);

	int Value = 0;
	static int64 s_RefreshTime = time_get();
	if(s_Count > 0 && s_Count > PageSize)
	{
		float Span = s_Count / PageSize;
		Value = DoScrollbarH(&s_SliderID, &Slider, s_LastSliderValue / Span ) * Span + 0.1f;

		if (Value != s_LastSliderValue)
		{
			s_RefreshTime = time_get() + time_freq() * 0.05f;
			s_Refresh = true;
			s_LastSliderValue = Value;
		}
	}

	static int s_ReinitWindowCount = Client()->ReinitWindowCount();
	if (Client()->ReinitWindowCount() != s_ReinitWindowCount)
	{
		s_Refresh = true;
		s_ReinitWindowCount = Client()->ReinitWindowCount();
	}

	if(s_Refresh && s_RefreshTime < time_get())
	{
		s_NumPlayerItems = 0;
		m_pClient->m_pIdentities->GetMenuIdentity(&GetMenuIdentityResult, this, Value * PageSize, PageSize, s_ConditionText);
		s_Refresh = false;
	}

	for(int i = 0; i < s_NumPlayerItems; i++)
	{
		Line = Button;

		if(m_pClient->PlayerOnline(s_PlayerItems[i].m_Info))
		{
			Graphics()->TextureSet(g_pData->m_aImages[IMAGE_BROWSEICONS].m_Id);
			Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
			RenderTools()->SelectSprite(SPRITE_BROWSE_EPSILON);
			IGraphics::CQuadItem QuadItem(Line.x, Line.y + 2.0f, 12.0f, 12.0f);
			Graphics()->QuadsDrawTL(&QuadItem, 1);
			Graphics()->QuadsEnd();
		}

		Line.VSplitLeft(10.0f, 0x0, &Line);

		int SkinID = m_pClient->m_pSkins->Find(s_PlayerItems[i].m_Info.m_aSkin);
		if(SkinID == -1) SkinID = m_pClient->m_pSkins->Find("default");
		const CSkins::CSkin *pSkin = m_pClient->m_pSkins->Get(SkinID);
		if(s_PlayerItems[i].m_Info.m_UseCustomColor)
		{
			SkinInfo.m_Texture = pSkin->m_ColorTexture;
			SkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(s_PlayerItems[i].m_Info.m_ColorBody);
			SkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(s_PlayerItems[i].m_Info.m_ColorFeet);
		}
		else
		{
			SkinInfo.m_Texture = pSkin->m_OrgTexture;
			SkinInfo.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			SkinInfo.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}

		SkinInfo.m_Size = 20.0f*UI()->Scale();

		RenderTools()->RenderTee(CAnimState::GetIdle(), &SkinInfo, 0, vec2(1, 0), vec2(Line.x + SkinInfo.m_Size / 2, Line.y + SkinInfo.m_Size / 2));

		Line.VSplitLeft(SkinInfo.m_Size, 0x0, &Line);
		UI()->DoLabel(&Line, s_PlayerItems[i].m_Info.m_aName, 10.0f, -1);

		Line.VSplitLeft(110.0f, 0x0, &Line);
		UI()->DoLabel(&Line, s_PlayerItems[i].m_Info.m_aClan, 10.0f, -1);

		Line.VSplitLeft(70.0f, 0x0, &Line);

		const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_pCountryFlags->GetByCountryCode(s_PlayerItems[i].m_Info.m_Country);

		vec4 Color(1.0f, 1.0f, 1.0f, 1.0f);
		m_pClient->m_pCountryFlags->Render(pEntry->m_CountryCode, &Color, Line.x, Line.y, 28.0f, 14.0f);
			//UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, 0);

		Line.VSplitLeft(34.0f, 0x0, &Line);
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%i", s_PlayerItems[i].m_Latency);
		UI()->DoLabel(&Line, aBuf, 10.0f, -1);

		Line.VSplitLeft(24.0f, 0x0, &Line);
		Line.h = 17.0f; Line.w = 14.0f;
		static int s_PaddelButton = 0;
		if(DoButton_Texture((void*)&s_CopyButtonIDs[i], ms_TextureCopy, 0, &Line))
		{
			char aBuf[512];
			m_pClient->m_pIdentities->SerializeIdentity(aBuf, sizeof(aBuf), s_PlayerItems[i]);
			clipboard_set(aBuf, str_length(aBuf));
		}


		Button.HSplitTop(20.0f, 0x0, &Button);

		if(Button.y + 20.0f >= MainView.y + MainView.h)
		{
			Button.y = MainView.y + 50.0f;
			Button.x += MainView.w / Columns;
		}
	}
}

void CMenus::RenderExtrasDummies(CUIRect MainView)
{
	CUIRect Top, Left, Right, Button;
	static int s_DummyID = 0;
	MainView.HSplitTop(24.0f, &Top, &MainView);

	{//Top
		Top.HSplitTop(8.0f, 0x0, &Top);
		Top.VSplitRight(128.0f, &Top, &Button);
		static int s_ScrollbarID = -1;
		s_DummyID = DoScrollbarH(&s_ScrollbarID, &Top, s_DummyID / (float)MAX_DUMMIES ) * (float)MAX_DUMMIES;

		static char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "Dummy: %i/%i", s_DummyID, MAX_DUMMIES);
		UI()->DoLabel(&Button, aBuf, 15.0f, 0);
	}

	MainView.VSplitMid(&Left, &Right);

	Left.VSplitLeft(8.0f, 0x0, &Left);
	Left.VSplitRight(8.0f, &Left, 0x0);
	Right.VSplitLeft(8.0f, 0x0, &Right);
	Right.VSplitRight(8.0f, &Right, 0x0);
	IGameClient::CPlayerInfo *pDummyInfo = m_pClient->GetDummyPlayerInfo(s_DummyID);

	{//name
		Left.HSplitTop(20.0f, &Button, &Left);
		UI()->DoLabel(&Button, "Name", 15.0f, -1);
		float TextSize = TextRender()->TextWidth(0, 15.0f, "Name", -1);
		Button.VSplitLeft(TextSize + 8.0f, 0x0, &Button);
		static float s_NameOffset = 0.0f;
		DoEditBox(pDummyInfo->m_aName, &Button, pDummyInfo->m_aName, sizeof(pDummyInfo->m_aName), 14.0f, &s_NameOffset);
	}

	{//clan
		Left.HSplitTop(8.0f, 0x0, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		UI()->DoLabel(&Button, "Clan", 15.0f, -1);
		float TextSize = TextRender()->TextWidth(0, 15.0f, "Clan", -1);
		Button.VSplitLeft(TextSize + 8.0f, 0x0, &Button);
		static float s_ClanOffset = 0.0f;
		DoEditBox(pDummyInfo->m_aClan, &Button, pDummyInfo->m_aClan, sizeof(pDummyInfo->m_aClan), 14.0f, &s_ClanOffset);
	}

	{//Country
		Left.HSplitTop(8.0f, 0x0, &Left);

		static float s_ScrollValue = 0.0f;
		int OldSelected = -1;
		UiDoListboxStart(&s_ScrollValue, &Left, 50.0f, Localize("Country"), "", m_pClient->m_pCountryFlags->Num(), 6, OldSelected, s_ScrollValue);

		for(int i = 0; i < m_pClient->m_pCountryFlags->Num(); ++i)
		{
			const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_pCountryFlags->GetByIndex(i);
			if(pEntry->m_CountryCode == pDummyInfo->m_Country)
				OldSelected = i;

			CListboxItem Item = UiDoListboxNextItem(&pEntry->m_CountryCode, OldSelected == i);
			if(Item.m_Visible)
			{
				CUIRect Label;
				Item.m_Rect.Margin(5.0f, &Item.m_Rect);
				Item.m_Rect.HSplitBottom(10.0f, &Item.m_Rect, &Label);
				float OldWidth = Item.m_Rect.w;
				Item.m_Rect.w = Item.m_Rect.h*2;
				Item.m_Rect.x += (OldWidth-Item.m_Rect.w)/ 2.0f;
				vec4 Color(1.0f, 1.0f, 1.0f, 1.0f);
				m_pClient->m_pCountryFlags->Render(pEntry->m_CountryCode, &Color, Item.m_Rect.x, Item.m_Rect.y, Item.m_Rect.w, Item.m_Rect.h);
				if(pEntry->m_Texture != -1)
					UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, 0);
			}
		}

		const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
		if(OldSelected != NewSelected)
		{
			pDummyInfo->m_Country = m_pClient->m_pCountryFlags->GetByIndex(NewSelected)->m_CountryCode;
		}
	}

	{//skin picker
		CUIRect Button, Label;

		// skin info
		const CSkins::CSkin *pOwnSkin = m_pClient->m_pSkins->Get(m_pClient->m_pSkins->Find(pDummyInfo->m_aSkin));
		CTeeRenderInfo OwnSkinInfo;
		if(pDummyInfo->m_UseCustomColor)
		{
			OwnSkinInfo.m_Texture = pOwnSkin->m_ColorTexture;
			OwnSkinInfo.m_ColorBody = m_pClient->m_pSkins->GetColorV4(pDummyInfo->m_ColorBody);
			OwnSkinInfo.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(pDummyInfo->m_ColorFeet);
		}
		else
		{
			OwnSkinInfo.m_Texture = pOwnSkin->m_OrgTexture;
			OwnSkinInfo.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
			OwnSkinInfo.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
		}
		OwnSkinInfo.m_Size = 50.0f*UI()->Scale();

		Right.HSplitTop(20.0f, &Label, &Right);
		Label.VSplitLeft(230.0f, &Label, 0);
		char aBuf[128];
		str_format(aBuf, sizeof(aBuf), "%s:", Localize("Your skin"));
		UI()->DoLabelScaled(&Label, aBuf, 14.0f, -1);

		Right.HSplitTop(50.0f, &Label, &Right);
		Label.VSplitLeft(230.0f, &Label, 0);
		RenderTools()->DrawUIRect(&Label, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 10.0f);
		RenderTools()->RenderTee(CAnimState::GetIdle(), &OwnSkinInfo, 0, vec2(1, 0), vec2(Label.x+30.0f, Label.y+28.0f));
		Label.HSplitTop(15.0f, 0, &Label);;
		Label.VSplitLeft(70.0f, 0, &Label);
		UI()->DoLabelScaled(&Label, pDummyInfo->m_aSkin, 14.0f, -1, 150.0f);

		Label.VSplitLeft(Label.w+8, 0, &Label);
		Label.VSplitLeft(64.0f, &Label, 0);
		Label.HSplitTop(16.0f, &Label, 0x0);
		static int s_PaddelButton = 0;
		if(DoButton_Menu((void*)&s_PaddelButton, Localize("Paddel"), 0, &Label))
		{
			str_copy(pDummyInfo->m_aSkin, "limekitty", sizeof(pDummyInfo->m_aSkin));
			pDummyInfo->m_UseCustomColor = 1;
			pDummyInfo->m_ColorBody = 7424000;
			pDummyInfo->m_ColorFeet = 7339830;
		}

		static int64 s_ClipboardCheck = time_get();
		static bool s_PasteAble = false;
		static char s_aClipBoard[512] = { };

		if(s_ClipboardCheck < time_get())
		{
			char *pClipboard = clipboard_get();
			if(str_comp_num(pClipboard, "Identity\r\n", str_length("Identity\r\n"))  == 0 && str_length(pClipboard) < 512)
			{
				s_PasteAble = true;
				str_copy(s_aClipBoard, pClipboard, sizeof(s_aClipBoard));
			}
			else
				s_PasteAble = false;
			s_ClipboardCheck = time_get() + time_freq();
		}
		
		if(s_PasteAble)
		{
			Label.HSplitTop(38.0f, 0x0, &Label);
			Label.HSplitTop(32.0f, &Label, 0x0);
			static int s_PasteButton = 0;
			if(DoButton_Menu((void*)&s_PasteButton, Localize("Paste"), 0, &Label))
			{
				CIdentities::CPlayerItem Temp;
				m_pClient->m_pIdentities->WriteIdentity(s_aClipBoard, sizeof(s_aClipBoard), Temp);
				mem_copy(pDummyInfo, &Temp.m_Info, sizeof(IGameClient::CPlayerInfo));
			}
		}

		// custom colour selector
		Right.HSplitTop(20.0f, 0, &Right);
		Right.HSplitTop(20.0f, &Button, &Right);
		Button.VSplitLeft(230.0f, &Button, 0);
		if(DoButton_CheckBox(&pDummyInfo->m_ColorBody, Localize("Custom colors"), pDummyInfo->m_UseCustomColor, &Button))
		{
			pDummyInfo->m_UseCustomColor = pDummyInfo->m_UseCustomColor?0:1;
		}

		Right.HSplitTop(5.0f, 0, &Right);
		Right.HSplitTop(82.5f, &Label, &Right);
		if(pDummyInfo->m_UseCustomColor)
		{
			CUIRect aRects[2];
			Label.VSplitMid(&aRects[0], &aRects[1]);
			aRects[0].VSplitRight(10.0f, &aRects[0], 0);
			aRects[1].VSplitLeft(10.0f, 0, &aRects[1]);

			int *paColors[2];
			paColors[0] = &pDummyInfo->m_ColorBody;
			paColors[1] = &pDummyInfo->m_ColorFeet;

			const char *paParts[] = {
				Localize("Body"),
				Localize("Feet")};
			const char *paLabels[] = {
				Localize("Hue"),
				Localize("Sat."),
				Localize("Lht.")};
			static int s_aColorSlider[2][3] = {{0}};

			for(int i = 0; i < 2; i++)
			{
				aRects[i].HSplitTop(20.0f, &Label, &aRects[i]);
				UI()->DoLabelScaled(&Label, paParts[i], 14.0f, -1);
				aRects[i].VSplitLeft(20.0f, 0, &aRects[i]);
				aRects[i].HSplitTop(2.5f, 0, &aRects[i]);

				int PrevColor = *paColors[i];
				int Color = 0;
				for(int s = 0; s < 3; s++)
				{
					aRects[i].HSplitTop(20.0f, &Label, &aRects[i]);
					Label.VSplitLeft(100.0f, &Label, &Button);
					Button.HMargin(2.0f, &Button);

					float k = ((PrevColor>>((2-s)*8))&0xff) / 255.0f;
					k = DoScrollbarH(&s_aColorSlider[i][s], &Button, k);
					Color <<= 8;
					Color += clamp((int)(k*255), 0, 255);
					UI()->DoLabelScaled(&Label, paLabels[s], 14.0f, -1);
				}

				if(PrevColor != Color)
					m_NeedSendinfo = true;

				*paColors[i] = Color;
			}
		}

		// skin selector
		Right.HSplitTop(20.0f, 0, &Right);
		static bool s_InitSkinlist = true;
		static sorted_array<const CSkins::CSkin *> s_paSkinList;
		static float s_ScrollValue = 0.0f;
		if(s_InitSkinlist)
		{
			s_paSkinList.clear();
			for(int i = 0; i < m_pClient->m_pSkins->Num(); ++i)
			{
				const CSkins::CSkin *s = m_pClient->m_pSkins->Get(i);
				// no special skins
				if(s->m_aName[0] == 'x' && s->m_aName[1] == '_')
					continue;
				s_paSkinList.add(s);
			}
			s_InitSkinlist = false;
		}

		int OldSelected = -1;
		UiDoListboxStart(&s_InitSkinlist, &Right, 50.0f, Localize("Skins"), "", s_paSkinList.size(), 4, OldSelected, s_ScrollValue);

		for(int i = 0; i < s_paSkinList.size(); ++i)
		{
			const CSkins::CSkin *s = s_paSkinList[i];
			if(s == 0)
				continue;

			if(str_comp(s->m_aName, pDummyInfo->m_aSkin) == 0)
				OldSelected = i;

			CListboxItem Item = UiDoListboxNextItem(&s_paSkinList[i], OldSelected == i);
			if(Item.m_Visible)
			{
				CTeeRenderInfo Info;
				if(pDummyInfo->m_UseCustomColor)
				{
					Info.m_Texture = s->m_ColorTexture;
					Info.m_ColorBody = m_pClient->m_pSkins->GetColorV4(pDummyInfo->m_ColorBody);
					Info.m_ColorFeet = m_pClient->m_pSkins->GetColorV4(pDummyInfo->m_ColorFeet);
				}
				else
				{
					Info.m_Texture = s->m_OrgTexture;
					Info.m_ColorBody = vec4(1.0f, 1.0f, 1.0f, 1.0f);
					Info.m_ColorFeet = vec4(1.0f, 1.0f, 1.0f, 1.0f);
				}

				Info.m_Size = UI()->Scale()*50.0f;
				Item.m_Rect.HSplitTop(5.0f, 0, &Item.m_Rect); // some margin from the top
				RenderTools()->RenderTee(CAnimState::GetIdle(), &Info, 0, vec2(1.0f, 0.0f), vec2(Item.m_Rect.x+Item.m_Rect.w/2, Item.m_Rect.y+Item.m_Rect.h/2));

				if(g_Config.m_Debug)
				{
					vec3 BloodColor = pDummyInfo->m_UseCustomColor ? m_pClient->m_pSkins->GetColorV3(pDummyInfo->m_ColorBody) : s->m_BloodColor;
					Graphics()->TextureSet(-1);
					Graphics()->QuadsBegin();
					Graphics()->SetColor(BloodColor.r, BloodColor.g, BloodColor.b, 1.0f);
					IGraphics::CQuadItem QuadItem(Item.m_Rect.x, Item.m_Rect.y, 12.0f, 12.0f);
					Graphics()->QuadsDrawTL(&QuadItem, 1);
					Graphics()->QuadsEnd();
				}
			}
		}

		const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
		if(OldSelected != NewSelected)
		{
			mem_copy(pDummyInfo->m_aSkin, s_paSkinList[NewSelected]->m_aName, sizeof(pDummyInfo->m_aSkin));
			m_NeedSendinfo = true;
		}
	}
}

void CMenus::RenderExtrasMapinker(CUIRect MainView)
{
	CUIRect LayerSelection, Right, Button;
	static const int MAX_GROUPS = 100;
	static bool s_GroupExtrended[MAX_GROUPS] = {};
	CLayers *pLayers = m_pClient->Layers();
	static bool s_NightTime = m_pClient->m_pMapInker->NightTime() && g_Config.m_PdlMapinkerTimeEnable == 1;
	bool LoadFromInker = false;

	bool UpdateMapInker = false;
	static float MapR = 255.0f, MapG = 255.0f, MapB = 255.0f,
		MapH = 360.0f, MapS = 255.0f, MapL = 255.0f,
		MapA = 255.0f, MapI = 255.0f;
	static int s_SliderR = 0, s_SliderG = 0, s_SliderB = 0,
		s_SliderH = 0, s_SliderS = 0, s_SliderL = 0,
		s_SliderA = 0, s_SliderI = 0;

	if (Client()->State() != IClient::STATE_ONLINE)
	{
		TextRender()->Text(0, MainView.x + 12.0f, MainView.y + 12.0f, 14.0f*UI()->Scale(), "Go ingame to use Mapinker", -1);
		return;
	}

	if (m_pClient->Layers()->NumGroups() > MAX_GROUPS)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "ERROR: More than %i groups", MAX_GROUPS);
		TextRender()->Text(0, MainView.x + 12.0f, MainView.y + 12.0f, 14.0f*UI()->Scale(), aBuf, -1);
		return;
	}

	MainView.VSplitLeft(170.0f, &LayerSelection, &Right);

	static float s_ScrollValue = 0.0f;
	static int SelectedLayer = -1;
	int SelectedItem = -1;

	int NumItems = 0;
	for (int g = 0; g < pLayers->NumGroups(); g++)
	{
		CMapItemGroup *pGroup = pLayers->GetGroup(g);
		NumItems++;
		if (s_GroupExtrended[g])
		{
			for (int l = 0; l < pGroup->m_NumLayers; l++)
			{
				int Index = pGroup->m_StartLayer + l;
				if (SelectedLayer == Index)
					SelectedItem = NumItems;

				NumItems++;
			}
		}
	}


	UiDoListboxStart(&s_ScrollValue, &LayerSelection, 20.0f, Localize("Layers"), "", NumItems, 1, SelectedItem, s_ScrollValue);

	for (int g = 0; g < pLayers->NumGroups(); g++)
	{
		CMapItemGroup *pGroup = pLayers->GetGroup(g);

		char aGroupName[32];

		CListboxItem Item = UiDoListboxNextItem(pGroup, 0);
		if (Item.m_Visible)
		{
			char aBuf[64];
			if (pGroup->m_Version >= 3)
			{
				IntsToStr(pGroup->m_aName, sizeof(aGroupName) / sizeof(int), aGroupName);
				str_format(aBuf, sizeof(aBuf), "#%i %s", g, aGroupName);
			}
			else
				str_format(aBuf, sizeof(aBuf), "#%i", g);

			str_sanitize_strong(aBuf);
			TextRender()->TextColor(0.7f, 0.7f, 0.7f, 1.0f);
			UI()->DoLabelScaled(&Item.m_Rect, aBuf, 14.0f, -1);

			if (g != 0)
			{
				Graphics()->TextureSet(-1);
				Graphics()->LinesBegin();
				IGraphics::CLineItem Line = IGraphics::CLineItem(Item.m_Rect.x, Item.m_Rect.y, Item.m_Rect.x+Item.m_Rect.w, Item.m_Rect.y);
				Graphics()->LinesDraw(&Line, 1);
				Graphics()->LinesEnd();
			}

			Graphics()->TextureSet(-1);
			Graphics()->LinesBegin();
			IGraphics::CLineItem Line = IGraphics::CLineItem(Item.m_Rect.x, Item.m_Rect.y + Item.m_Rect.h, Item.m_Rect.x + Item.m_Rect.w, Item.m_Rect.y + Item.m_Rect.h);
			Graphics()->LinesDraw(&Line, 1);
			Graphics()->LinesEnd();
		}

		if (s_GroupExtrended[g] == false)
			continue;

		for (int l = 0; l < pGroup->m_NumLayers; l++)
		{
			int Index = pGroup->m_StartLayer + l;
			CMapItemLayer *pLayer = pLayers->GetLayer(Index);
			
		
			CListboxItem Item = UiDoListboxNextItem(pLayer, SelectedLayer == Index);
			if (Item.m_Visible)
			{
				TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
				char aLayerName[32] = {};
				if (pLayer->m_Type == LAYERTYPE_TILES)
				{
					if (((CMapItemLayerTilemap *)pLayer)->m_Version >= 3)
						IntsToStr(((CMapItemLayerTilemap *)pLayer)->m_aName, sizeof(aLayerName) / sizeof(int), aLayerName);
					else
						str_format(aLayerName, sizeof(aLayerName), "<%i>", l);

					CMapItemLayerTilemap *pTilemap = reinterpret_cast<CMapItemLayerTilemap *>(pLayer);
					if (pTilemap->m_Flags != 0)
						TextRender()->TextColor(0.2f, 0.0f, 0.0f, 1.0f);
				}
				else if (pLayer->m_Type == LAYERTYPE_QUADS)
				{
					if (((CMapItemLayerQuads *)pLayer)->m_Version >= 2)
						IntsToStr(((CMapItemLayerQuads *)pLayer)->m_aName, sizeof(aLayerName) / sizeof(int), aLayerName);
					else
						str_format(aLayerName, sizeof(aLayerName), "<%i>", l);
				}

				str_sanitize_strong(aLayerName);

				
				UI()->DoLabelScaled(&Item.m_Rect, aLayerName, 14.0f, -1);
			}
		}
	}

	int Selected = UiDoListboxEnd(&s_ScrollValue, 0);
	if (Selected != SelectedItem)
	{
		for (int g = 0; g < pLayers->NumGroups(); g++)
		{
			CMapItemGroup *pGroup = pLayers->GetGroup(g);

			if (Selected <= 0)
			{
				if (Selected == 0)
					s_GroupExtrended[g] = !s_GroupExtrended[g];

				break;
			}

			Selected--;

			if (s_GroupExtrended[g])
			{
				for (int l = 0; l < pGroup->m_NumLayers; l++)
				{
					if (Selected == 0)
					{
						bool Game = false;

						CMapItemLayer *pLayer = pLayers->GetLayer(pGroup->m_StartLayer + l);
						if (pLayer->m_Type == LAYERTYPE_TILES)
							if ((reinterpret_cast<CMapItemLayerTilemap *>(pLayer))->m_Flags != 0)
								Game = true;

						if (Game == false)
						{
							SelectedLayer = pGroup->m_StartLayer + l;
							Selected = -1;
							LoadFromInker = true;
						}
						break;
					}

					Selected--;
				}
			}
		}
	}

	if (SelectedItem != -1)
	{


		Right.HSplitTop(16.0f, &Button, &Right);

		static int s_NightButton = 0;
		if (DoButton_CheckBox(&s_NightButton, "Night-Design", s_NightTime, &Button))
		{
			s_NightTime = !s_NightTime;
			LoadFromInker = true;
		}

		CMapInkerLayer *pMapInkerLayer = m_pClient->m_pMapInker->MapInkerLayer(s_NightTime, SelectedLayer);
		if (pMapInkerLayer == 0x0)
			return;

		Right.HSplitTop(8.0f, 0x0, &Right);
		Right.HSplitTop(16.0f, &Button, &Right);
		static int s_UsingButton = 0;
		if (DoButton_CheckBox(&s_UsingButton, "Use Layer", pMapInkerLayer->m_Used, &Button))
			pMapInkerLayer->m_Used = !pMapInkerLayer->m_Used;

		Right.HSplitTop(8.0f, 0x0, &Right);
		Right.HSplitTop(16.0f, &Button, &Right);
		UI()->DoLabel(&Button, "Red", 12.f, -1);
		Button.VSplitLeft(46.0f, 0x0, &Button);
		float MapRN = DoScrollbarH(&s_SliderR, &Button, MapR / 255.0f) * 255.0f;
		Right.HSplitTop(8.0f, 0x0, &Right);
		Right.HSplitTop(16.0f, &Button, &Right);
		UI()->DoLabel(&Button, "Green", 12.f, -1);
		Button.VSplitLeft(46.0f, 0x0, &Button);
		float MapGN = DoScrollbarH(&s_SliderG, &Button, MapG / 255.0f) * 255.0f;
		Right.HSplitTop(8.0f, 0x0, &Right);
		Right.HSplitTop(16.0f, &Button, &Right);
		UI()->DoLabel(&Button, "Blue", 12.f, -1);
		Button.VSplitLeft(46.0f, 0x0, &Button);
		float MapBN = DoScrollbarH(&s_SliderB, &Button, MapB / 255.0f) * 255.0f;
		if (fabsolute(MapRN - MapR) > 1.0f || fabsolute(MapGN - MapG) > 1.0f || fabsolute(MapBN - MapB) > 1.0f)
		{
			MapR = MapRN; MapG = MapGN; MapB = MapBN;
			vec3 HSL = RgbToHsl(vec3(MapR / 255.0f, MapG / 255.0f, MapB / 255.0f));
			MapH = HSL.h * 360.0f; MapS = HSL.s * 255.0f; MapL = HSL.l * 255.0f;
			UpdateMapInker = true;
		}

		Right.HSplitTop(18.0f, 0x0, &Right);
		Right.HSplitTop(16.0f, &Button, &Right);
		UI()->DoLabel(&Button, "Hue", 12.f, -1);
		Button.VSplitLeft(46.0f, 0x0, &Button);
		float MapHN = DoScrollbarH(&s_SliderH, &Button, MapH / 360.0f) * 360.0f;
		Right.HSplitTop(8.0f, 0x0, &Right);
		Right.HSplitTop(16.0f, &Button, &Right);
		UI()->DoLabel(&Button, "Sat.", 12.f, -1);
		Button.VSplitLeft(46.0f, 0x0, &Button);
		float MapSN = DoScrollbarH(&s_SliderS, &Button, MapS / 255.0f) * 255.0f;
		Right.HSplitTop(8.0f, 0x0, &Right);
		Right.HSplitTop(16.0f, &Button, &Right);
		UI()->DoLabel(&Button, "Lht.", 12.f, -1);
		Button.VSplitLeft(46.0f, 0x0, &Button);
		float MapLN = DoScrollbarH(&s_SliderL, &Button, MapL / 255.0f) * 255.0f;
		if (fabsolute(MapHN - MapH) > 1.0f || fabsolute(MapSN - MapS) > 1.0f || fabsolute(MapLN - MapL) > 1.0f)
		{
			MapH = MapHN; MapS = MapSN; MapL = MapLN;
			vec3 RGB = HslToRgb(vec3(MapH / 360.0f, MapS / 255.0f, MapL / 255.0f));
			MapR = RGB.r * 255.0f; MapG = RGB.g * 255.0f; MapB = RGB.b * 255.0f;
			UpdateMapInker = true;
		}

		Right.HSplitTop(18.0f, 0x0, &Right);
		Right.HSplitTop(16.0f, &Button, &Right);
		UI()->DoLabel(&Button, "Alpha", 12.f, -1);
		Button.VSplitLeft(46.0f, 0x0, &Button);
		float MapAN = DoScrollbarH(&s_SliderA, &Button, MapA / 255.0f) * 255.0f;
		Right.HSplitTop(8.0f, 0x0, &Right);
		Right.HSplitTop(16.0f, &Button, &Right);
		UI()->DoLabel(&Button, "Int", 12.f, -1);
		Button.VSplitLeft(46.0f, 0x0, &Button);
		float MapIN = DoScrollbarH(&s_SliderI, &Button, MapI / 255.0f) * 255.0f;
		if (fabsolute(MapAN - MapA) > 1.0f || fabsolute(MapIN - MapI) > 1.0f)
		{
			MapA = MapAN;
			MapI = MapIN;
			UpdateMapInker = true;
		}

		Right.HSplitTop(8.0f, 0x0, &Right);
		Right.HSplitTop(16.0f, &Button, &Right);
		UI()->DoLabel(&Button, Localize("Texture"), 10.0f, -1);
		Button.VSplitLeft(46.0f, 0x0, &Button);
		static float s_TextureOffset = 0.0f;
		if (DoEditBox(pMapInkerLayer->m_aExternalTexture, &Button, pMapInkerLayer->m_aExternalTexture, sizeof(pMapInkerLayer->m_aExternalTexture), 14.0f, &s_TextureOffset))
		{
			m_pClient->m_pMapInker->UpdateTextureID(s_NightTime, SelectedLayer);
			pMapInkerLayer->m_Used = true;
		}

		Right.HSplitTop(8.0f, 0x0, &Right);
		Right.HSplitTop(16.0f, &Button, &Right);

		Button.VSplitLeft(Right.w * 0.5f - 8.0f, &Button, 0x0);
		static int s_ButtonSave = 0;
		if (DoButton_Menu(&s_ButtonSave, Localize("Save"), 0, &Button))
			m_pClient->m_pMapInker->Save();

		Button.VSplitLeft(Right.w * 0.5f + 16.0f, 0x0, &Button);
		Button.VSplitLeft(Right.w * 0.5f - 8.0f, &Button, 0x0);
		static int s_ButtonLoad = 0;
		if (DoButton_Menu(&s_ButtonLoad, Localize("Load"), 0, &Button))
		{
			m_pClient->m_pMapInker->Load();
			LoadFromInker = true;
		}

		if (UpdateMapInker)
		{
			pMapInkerLayer->m_Color = vec4(MapR / 255.0f, MapG / 255.0f, MapB / 255.0f, MapA / 255.0f);
			pMapInkerLayer->m_Interpolation = MapI / 255.0f;
			pMapInkerLayer->m_Used = true;
		}
	}

	static int s_LoadedMaps = 0;
	if (s_LoadedMaps != m_pClient->m_pMapInker->GetLoadedMaps())
	{
		LoadFromInker = true;
		mem_zero(s_GroupExtrended, sizeof(s_GroupExtrended));
		s_LoadedMaps = m_pClient->m_pMapInker->GetLoadedMaps();
	}

	if (LoadFromInker)
	{
		CMapInkerLayer *pMapInkerLayer = m_pClient->m_pMapInker->MapInkerLayer(s_NightTime, SelectedLayer);
		if (pMapInkerLayer != 0x0)
		{
			vec4 Color = pMapInkerLayer->m_Color;
			MapR = Color.r*255.0f; MapG = Color.g*255.0f; MapB = Color.b*255.0f;
			vec3 HSL = RgbToHsl(vec3(Color.r, Color.g, Color.b));
			MapH = HSL.h * 360.0f; MapS = HSL.s * 255.0f; MapL = HSL.l * 255.0f;
			MapA = Color.a*255.0f; MapI = pMapInkerLayer->m_Interpolation*255.0f;
		}
	}
}

void CMenus::RenderExtras(CUIRect MainView)
{
	CUIRect TabBar, Button, Temp;
	static int s_ExtrasPage = 0;

	MainView.VSplitRight(120.0f, &MainView, &TabBar);
	RenderTools()->DrawUIRect(&MainView, ms_ColorTabbarActive, CUI::CORNER_B|CUI::CORNER_TL, 10.0f);
	TabBar.HSplitTop(50.0f, &Temp, &TabBar);
	RenderTools()->DrawUIRect(&Temp, ms_ColorTabbarActive, CUI::CORNER_R, 10.0f);

	const char *aTabs[] = {
		Localize("General"),
		"Identities",
		"Dummies",
		"Mapinker"};

	int NumTabs = (int)(sizeof(aTabs)/sizeof(*aTabs));

	for(int i = 0; i < NumTabs; i++)
	{
		TabBar.HSplitTop(10, &Button, &TabBar);
		TabBar.HSplitTop(26, &Button, &TabBar);
		if(DoButton_MenuTab(aTabs[i], aTabs[i], s_ExtrasPage == i, &Button, CUI::CORNER_R))
			s_ExtrasPage = i;
	}

	MainView.VSplitLeft(8.0f, 0x0, &MainView);
	MainView.VSplitRight(16.0f, &MainView, 0x0);
	MainView.HSplitTop(8.0f, 0x0, &MainView);
	MainView.HSplitBottom(16.0f, &MainView, 0x0);

	if(s_ExtrasPage == 0)
		RenderExtrasGeneral(MainView);
	else if(s_ExtrasPage == 1)
		RenderExtrasIdentities(MainView);
	else if(s_ExtrasPage == 2)
		RenderExtrasDummies(MainView);
	else if (s_ExtrasPage == 3)
		RenderExtrasMapinker(MainView);
}