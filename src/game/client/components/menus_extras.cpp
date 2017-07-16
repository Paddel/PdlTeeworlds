
#include <engine/config.h>
#include <engine/textrender.h>
#include <engine/storage.h>
#include <engine/shared/config.h>

#include <game/generated/client_data.h>
#include <game/client/components/identities.h>
#include <game/client/animstate.h>

#include "skins.h"
#include "countryflags.h"
#include "menus.h"

#define PAGE_SIZE 75

CUIRect VariableButton;
static int s_VariableIDs[1024] = { };
static int s_VariableCounter = 0;
static CUIRect s_ResetView;
static char *s_pLastName = 0;
static float s_VariableOffset[1024] = { };

static CIdentities::CPlayerItem s_PlayerItems[PAGE_SIZE];
static int s_NumPlayerItems;
static int s_CopyButtonIDs[PAGE_SIZE];

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
		pMenu->UI()->DoLabel(&TextRect, aBuf, 20.f, 1);

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
	MainView.HSplitTop(18.0f, &VariableButton, NULL);
	VariableButton.VSplitLeft(MainView.w / 3.0f, &VariableButton, NULL);
	VariableButton.VSplitLeft(8.0f, NULL, &VariableButton);
	s_ResetView = MainView;
	s_pLastName = 0x0;
	pConfig->GetAllConfigVariables(&RenderExtrasVariableInt, &RenderExtrasVariableStr, this);
}

void CMenus::GetMenuIdentityResult(int Index, char *pResult, int pResultSize, void *pData)
{
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

	MainView.HSplitTop(50.0f, &Top, &Button);
	Button.VSplitLeft(8.0f, NULL, &Button);

	Top.HSplitTop(30, &Top, &Slider);
	Top.HSplitTop(8.0f, NULL, &Top);
	Top.VSplitLeft(256.0f, &Top, &CountText);
	Top.VSplitLeft(8.0f, NULL, &Top);
	static float s_ContionOffset = 0;
	if(DoEditBox(s_ConditionText, &Top, s_ConditionText, sizeof(s_ConditionText), 14.0f, &s_ContionOffset))
	{
		s_Count = m_pClient->m_pIdentities->GetMenuCount(s_ConditionText);
		s_LastSliderValue = 0;
		s_Refresh = true;
	}

	char aBuf[64];
	str_format(aBuf, sizeof(aBuf), "Page:%i/%i", s_LastSliderValue, s_Count/PAGE_SIZE);
	UI()->DoLabel(&CountText, aBuf, 10.0f, 1);

	int Value = 0;
	if(s_Count > 0 && s_Count > PAGE_SIZE)
	{
		float Span = s_Count / PAGE_SIZE;
		Value = DoScrollbarH(&s_SliderID, &Slider, s_LastSliderValue / Span ) * Span + 0.1f;

		if(Value != s_LastSliderValue)
			s_Refresh = true;

		s_LastSliderValue = Value;
	}

	if(s_Refresh)
	{
		s_NumPlayerItems = 0;
		m_pClient->m_pIdentities->GetMenuIdentity(&GetMenuIdentityResult, this, Value * PAGE_SIZE, PAGE_SIZE, s_ConditionText);
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

		Line.VSplitLeft(10.0f, NULL, &Line);

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

		Line.VSplitLeft(SkinInfo.m_Size, NULL, &Line);
		UI()->DoLabel(&Line, s_PlayerItems[i].m_Info.m_aName, 10.0f, -1);

		Line.VSplitLeft(110.0f, NULL, &Line);
		UI()->DoLabel(&Line, s_PlayerItems[i].m_Info.m_aClan, 10.0f, -1);

		Line.VSplitLeft(70.0f, NULL, &Line);

		const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_pCountryFlags->GetByCountryCode(s_PlayerItems[i].m_Info.m_Country);

		vec4 Color(1.0f, 1.0f, 1.0f, 1.0f);
		m_pClient->m_pCountryFlags->Render(pEntry->m_CountryCode, &Color, Line.x, Line.y, 28.0f, 14.0f);
			//UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, 0);

		Line.VSplitLeft(34.0f, NULL, &Line);
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%i", s_PlayerItems[i].m_Latency);
		UI()->DoLabel(&Line, aBuf, 10.0f, -1);

		Line.VSplitLeft(24.0f, NULL, &Line);
		Line.h = 17.0f; Line.w = 14.0f;
		static int s_PaddelButton = 0;
		static int s_TextureCopy = Graphics()->LoadTexture("pdl_copy.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
		if(DoButton_Texture((void*)&s_CopyButtonIDs[i], s_TextureCopy, 0, &Line))
		{
			char aBuf[512];
			m_pClient->m_pIdentities->SerializeIdentity(aBuf, sizeof(aBuf), s_PlayerItems[i]);
			ClipboardSet(aBuf, str_length(aBuf));
		}


		Button.HSplitTop(20.0f, NULL, &Button);

		if(Button.y + 20.0f >= MainView.y + MainView.h)
		{
			Button.y = MainView.y + 50.0f;
			Button.x += MainView.w / 3;
		}
	}
}

void CMenus::RenderExtrasDummies(CUIRect MainView)
{
	CUIRect Top, Left, Right, Button;
	static int s_DummyID = 0;
	MainView.HSplitTop(24.0f, &Top, &MainView);

	{//Top
		Top.HSplitTop(8.0f, NULL, &Top);
		Top.VSplitRight(128.0f, &Top, &Button);
		static int s_ScrollbarID = -1;
		s_DummyID = DoScrollbarH(&s_ScrollbarID, &Top, s_DummyID / (float)MAX_DUMMIES ) * (float)MAX_DUMMIES;

		static char aBuf[64];
		str_format(aBuf, sizeof(aBuf), "Dummy: %i/%i", s_DummyID, MAX_DUMMIES);
		UI()->DoLabel(&Button, aBuf, 15.0f, 0);
	}

	MainView.VSplitMid(&Left, &Right);

	Left.VSplitLeft(8.0f, NULL, &Left);
	Left.VSplitRight(8.0f, &Left, NULL);
	Right.VSplitLeft(8.0f, NULL, &Right);
	Right.VSplitRight(8.0f, &Right, NULL);
	IGameClient::CPlayerInfo *pDummyInfo = m_pClient->GetDummyPlayerInfo(s_DummyID);

	{//name
		Left.HSplitTop(20.0f, &Button, &Left);
		UI()->DoLabel(&Button, "Name", 15.0f, -1);
		float TextSize = TextRender()->TextWidth(0, 15.0f, "Name", -1);
		Button.VSplitLeft(TextSize + 8.0f, NULL, &Button);
		static float s_NameOffset = 0.0f;
		DoEditBox(pDummyInfo->m_aName, &Button, pDummyInfo->m_aName, sizeof(pDummyInfo->m_aName), 14.0f, &s_NameOffset);
	}

	{//clan
		Left.HSplitTop(8.0f, NULL, &Left);
		Left.HSplitTop(20.0f, &Button, &Left);
		UI()->DoLabel(&Button, "Clan", 15.0f, -1);
		float TextSize = TextRender()->TextWidth(0, 15.0f, "Clan", -1);
		Button.VSplitLeft(TextSize + 8.0f, NULL, &Button);
		static float s_ClanOffset = 0.0f;
		DoEditBox(pDummyInfo->m_aClan, &Button, pDummyInfo->m_aClan, sizeof(pDummyInfo->m_aClan), 14.0f, &s_ClanOffset);
	}

	{//Country
		Left.HSplitTop(8.0f, NULL, &Left);

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
		Label.VSplitLeft(126.0f, &Label, 0);
		Label.HSplitTop(32.0f, &Label, NULL);
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
			char *pClipboard = ClipboardGet();
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
			Label.HSplitTop(38.0f, NULL, &Label);
			Label.HSplitTop(32.0f, &Label, NULL);
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
		"Dummies"};

	int NumTabs = (int)(sizeof(aTabs)/sizeof(*aTabs));

	for(int i = 0; i < NumTabs; i++)
	{
		TabBar.HSplitTop(10, &Button, &TabBar);
		TabBar.HSplitTop(26, &Button, &TabBar);
		if(DoButton_MenuTab(aTabs[i], aTabs[i], s_ExtrasPage == i, &Button, CUI::CORNER_R))
			s_ExtrasPage = i;
	}

	if(s_ExtrasPage == 0)
		RenderExtrasGeneral(MainView);
	else if(s_ExtrasPage == 1)
		RenderExtrasIdentities(MainView);
	else if(s_ExtrasPage == 2)
		RenderExtrasDummies(MainView);
}