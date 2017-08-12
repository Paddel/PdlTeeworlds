

#include <base/system.h>
#include <base/math.h>
#include <base/vmath.h>

#include <engine/config.h>
#include <engine/editor.h>
#include <engine/engine.h>
#include <engine/friends.h>
#include <engine/graphics.h>
#include <engine/keys.h>
#include <engine/serverbrowser.h>
#include <engine/storage.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/version.h>
#include <game/generated/protocol.h>

#include <game/generated/client_data.h>
#include <game/client/components/sounds.h>
#include <game/client/gameclient.h>
#include <game/client/lineinput.h>
#include <game/localization.h>
#include <mastersrv/mastersrv.h>

#include "countryflags.h"
#include "menus.h"
#include "skins.h"

vec4 CMenus::ms_GuiColor;
vec4 CMenus::ms_ColorTabbarInactiveOutgame;
vec4 CMenus::ms_ColorTabbarActiveOutgame;
vec4 CMenus::ms_ColorTabbarInactive;
vec4 CMenus::ms_ColorTabbarActive = vec4(0,0,0,0.5f);
vec4 CMenus::ms_ColorTabbarInactiveIngame;
vec4 CMenus::ms_ColorTabbarActiveIngame;

float CMenus::ms_ButtonHeight = 25.0f;
float CMenus::ms_ListheaderHeight = 17.0f;
float CMenus::ms_FontmodHeight = 0.8f;

IInput::CEvent CMenus::m_aInputEvents[MAX_INPUTEVENTS];
int CMenus::m_NumInputEvents;

static char *m_aMainButtonText[] = { "Play", "Extras", "Demos", "Editor", "Settings", "Quit" };
static char CircleChar[] = { -30, -100, -109 };

static int gs_TextureBlob = -1;
static int gs_TextureBackInv = -1;
static int gs_TextureSpark = -1;
static int gs_TextureAurora = -1;
static int gs_TextureMainButton = -1;
static int gs_TexturePaddel = -1;

static void ConKeyInputState(IConsole::IResult *pResult, void *pUserData)
{
	((int *)pUserData)[0] = pResult->GetInteger(0);
}

CMenus::CMenus()
{
	m_Popup = POPUP_NONE;
	m_ActivePage = PAGE_INTERNET;
	m_GamePage = PAGE_GAME;

	m_NeedRestartGraphics = false;
	m_NeedRestartSound = false;
	m_NeedSendinfo = false;
	m_MenuActive = true;
	m_UseMouseButtons = true;

	m_EscapePressed = false;
	m_EnterPressed = false;
	m_DeletePressed = false;
	m_NumInputEvents = 0;

	m_LastInput = time_get();

	str_copy(m_aCurrentDemoFolder, "demos", sizeof(m_aCurrentDemoFolder));
	m_aCallvoteReason[0] = 0;

	m_FriendlistSelectedIndex = -1;
}

vec4 CMenus::GetButtonColor(const void *pID, int Checked)
{
	if(Checked < 0)
		return vec4(0,0,0,0.5f);

	if(Checked > 0)
	{
		if(UI()->HotItem() == pID)
			return vec4(1,0,0,0.75f);
		return vec4(1,0,0,0.5f);
	}

	if(UI()->ActiveItem() == pID)
		return vec4(45/255.0f, 254/255.0f, 184/255.0f,0.25f);
	else if(UI()->HotItem() == pID)
		return vec4(45/255.0f, 254/255.0f, 184/255.0f,0.75f);
	return vec4(45/255.0f, 254/255.0f, 184/255.0f,0.5f);
}

vec4 CMenus::ButtonColorMul(const void *pID)
{
	if(UI()->ActiveItem() == pID)
		return vec4(1,1,1,0.5f);
	else if(UI()->HotItem() == pID)
		return vec4(1,1,1,1.5f);
	return vec4(1,1,1,1);
}

int CMenus::DoButton_Icon(int ImageId, int SpriteId, const CUIRect *pRect)
{
	Graphics()->TextureSet(g_pData->m_aImages[ImageId].m_Id);

	Graphics()->QuadsBegin();
	RenderTools()->SelectSprite(SpriteId);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	return 0;
}

int CMenus::DoButton_Texture(const void *pID, int Texture, int Checked, const CUIRect *pRect)
{
	float SizeFac = 1.0f;
	if(UI()->ActiveItem() == pID)
		SizeFac = 0.95f;
	else if(UI()->HotItem() == pID)
		SizeFac = 1.05f;

	Graphics()->TextureSet(Texture);

	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);

	float GrowW = pRect->w - pRect->w * SizeFac, GrowH = pRect->h - pRect->h * SizeFac;
	IGraphics::CQuadItem QuadItem(pRect->x + GrowW * 0.5f, pRect->y + GrowH * 0.5f, pRect->w * SizeFac, pRect->h * SizeFac);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	return UI()->DoButtonLogic(pID, "", Checked, pRect);
}

int CMenus::DoButton_Toggle(const void *pID, int Checked, const CUIRect *pRect, bool Active)
{
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_GUIBUTTONS].m_Id);
	Graphics()->QuadsBegin();
	if(!Active)
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.5f);
	RenderTools()->SelectSprite(Checked?SPRITE_GUIBUTTON_ON:SPRITE_GUIBUTTON_OFF);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	if(UI()->HotItem() == pID && Active)
	{
		RenderTools()->SelectSprite(SPRITE_GUIBUTTON_HOVER);
		IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	}
	Graphics()->QuadsEnd();

	return Active ? UI()->DoButtonLogic(pID, "", Checked, pRect) : 0;
}

int CMenus::DoButton_Menu(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	vec4 Color = GetButtonColor(pID, Checked);

	{
		CUIRect ButtonRect = *pRect;

		const int n = 8;
		ButtonRect.x += n;
		ButtonRect.w -= n*2;

		Graphics()->TextureSet(-1);
		Graphics()->TriangleBegin();

		Graphics()->SetColor(Color.r,Color.g,Color.b,Color.a);
		IGraphics::CTriangleItem TriangleLeft(ButtonRect.x, ButtonRect.y, ButtonRect.x, ButtonRect.y + ButtonRect.h, ButtonRect.x - n, ButtonRect.y + (ButtonRect.h*2.0f/3.0f));
		Graphics()->TriangleDraw(&TriangleLeft, 1);

		IGraphics::CTriangleItem TriangleRight(ButtonRect.x+ButtonRect.w, ButtonRect.y, ButtonRect.x+ButtonRect.w, ButtonRect.y+ButtonRect.h, ButtonRect.x+ButtonRect.w + n, ButtonRect.y + (ButtonRect.h*1.0f/3.0f));
		Graphics()->TriangleDraw(&TriangleRight, 1);

		Graphics()->TriangleEnd();


		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();

		Graphics()->SetColor(Color.r,Color.g,Color.b,Color.a);
		IGraphics::CQuadItem QuadItemButton(ButtonRect.x+ButtonRect.w/2, ButtonRect.y+ButtonRect.h/2, ButtonRect.w, ButtonRect.h);
		Graphics()->QuadsDraw(&QuadItemButton, 1);

		Graphics()->QuadsEnd();

		Graphics()->TextureSet(-1);	
		Graphics()->LinesBegin();

		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		IGraphics::CLineItem Lines[6];
		Lines[0] = IGraphics::CLineItem(ButtonRect.x, ButtonRect.y, ButtonRect.x+ButtonRect.w, ButtonRect.y);//Top
		Lines[1] = IGraphics::CLineItem(ButtonRect.x, ButtonRect.y+ButtonRect.h, ButtonRect.x+ButtonRect.w, ButtonRect.y+ButtonRect.h);//Bottom
		Lines[2] = IGraphics::CLineItem(ButtonRect.x,  ButtonRect.y, ButtonRect.x - n, ButtonRect.y + (ButtonRect.h*2/3));//Left1
		Lines[3] = IGraphics::CLineItem(ButtonRect.x - n, ButtonRect.y + (ButtonRect.h*2.0f/3.0f), ButtonRect.x, ButtonRect.y+ButtonRect.h);//Left2
		Lines[4] = IGraphics::CLineItem(ButtonRect.x+ButtonRect.w, ButtonRect.y, ButtonRect.x+ButtonRect.w + n, ButtonRect.y + (ButtonRect.h*1.0f/3.0f));//Right1
		Lines[5] = IGraphics::CLineItem(ButtonRect.x+ButtonRect.w + n, ButtonRect.y + (ButtonRect.h*1.0f/3.0f), ButtonRect.x+ButtonRect.w, ButtonRect.y+ButtonRect.h);//Right2
		Graphics()->LinesDraw(Lines, 6);

		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		Graphics()->LinesEnd();
	}

	Graphics()->TextureSet(m_TextureButton);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(Color.r,Color.g,Color.b,Color.a);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	CUIRect Temp;
	pRect->HMargin(pRect->h>=20.0f?2.0f:1.0f, &Temp);
	TextRender()->Bubble();
	UI()->DoLabel(&Temp, pText, Temp.h*ms_FontmodHeight, 0);
	TextRender()->Bubble();
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

void CMenus::DoButton_KeySelect(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	vec4 Color = GetButtonColor(pID, Checked);
	RenderTools()->DrawUIRect(pRect, Color, CUI::CORNER_ALL, 2.0f);

	Graphics()->TextureSet(m_TextureButton);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(Color.r,Color.g,Color.b,Color.a);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	CUIRect Temp;
	pRect->HMargin(1.0f, &Temp);
	TextRender()->Bubble();
	UI()->DoLabel(&Temp, pText, Temp.h*ms_FontmodHeight, 0);
	TextRender()->Bubble();
}

int CMenus::DoButton_MenuTab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners)
{
	vec4 Color = ms_ColorTabbarInactive;
	if(Checked)
		Color = ms_ColorTabbarActive;

	RenderTools()->DrawUIRect(pRect, Color, Corners, 5.0f);

	Graphics()->TextureSet(m_TextureButton);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(Color.r,Color.g,Color.b,Color.a);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	CUIRect Temp;
	pRect->HMargin(2.0f, &Temp);
	TextRender()->Bubble();
	UI()->DoLabel(&Temp, pText, Temp.h*ms_FontmodHeight, 0);
	TextRender()->Bubble();

	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int CMenus::DoButton_GridHeader(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
//void CMenus::ui_draw_grid_header(const void *id, const char *text, int checked, const CUIRect *r, const void *extra)
{
	if(Checked)
		RenderTools()->DrawUIRect(pRect, vec4(1,1,1,0.5f), CUI::CORNER_T, 5.0f);
	CUIRect t;
	pRect->VSplitLeft(5.0f, 0, &t);
	UI()->DoLabel(&t, pText, pRect->h*ms_FontmodHeight, -1);
	return UI()->DoButtonLogic(pID, pText, Checked, pRect);
}

int CMenus::DoButton_CheckBox_Common(const void *pID, const char *pText, const char *pBoxText, const CUIRect *pRect)
//void CMenus::ui_draw_checkbox_common(const void *id, const char *text, const char *boxtext, const CUIRect *r, const void *extra)
{
	CUIRect c = *pRect;
	CUIRect t = *pRect;
	c.w = c.h;
	t.x += c.w;
	t.w -= c.w;
	t.VSplitLeft(5.0f, 0, &t);

	c.Margin(2.0f, &c);
	vec4 Color = vec4(1,1,1,0.25f)*ButtonColorMul(pID);
	RenderTools()->DrawUIRect(&c, Color, CUI::CORNER_ALL, 3.0f);

	Graphics()->TextureSet(m_TextureButton);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(Color.r,Color.g,Color.b,Color.a);
	IGraphics::CQuadItem QuadItem(c.x, c.y, c.w, c.h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	float Size = pRect->h*ms_FontmodHeight*0.6f;
	if(str_comp(pBoxText, CircleChar) == 0)
	{
		c.y -= 8;
		Size = pRect->h*ms_FontmodHeight*1.6f;
	}
	else c.y += 2;

	UI()->DoLabel(&c, pBoxText, Size, 0);
	UI()->DoLabel(&t, pText, pRect->h*ms_FontmodHeight*0.8f, -1);
	return UI()->DoButtonLogic(pID, pText, 0, pRect);
}

int CMenus::DoButton_CheckBox(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	return DoButton_CheckBox_Common(pID, pText, Checked?CircleChar:"", pRect);
}


int CMenus::DoButton_CheckBox_Number(const void *pID, const char *pText, int Checked, const CUIRect *pRect)
{
	char aBuf[16];
	str_format(aBuf, sizeof(aBuf), "%d", Checked);
	return DoButton_CheckBox_Common(pID, pText, aBuf, pRect);
}

int CMenus::DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *Offset, bool Hidden, int Corners)
{
	int Inside = UI()->MouseInside(pRect);
	bool ReturnValue = false;
	bool UpdateOffset = false;
	static int s_AtIndex = 0;
	static bool s_DoScroll = false;
	static float s_ScrollStart = 0.0f;

	FontSize *= UI()->Scale();

	if(UI()->LastActiveItem() == pID)
	{
		int Len = str_length(pStr);
		if(Len == 0)
			s_AtIndex = 0;

		if(Inside && UI()->MouseButton(0))
		{
			s_DoScroll = true;
			s_ScrollStart = UI()->MouseX();
			int MxRel = (int)(UI()->MouseX() - pRect->x);

			for(int i = 1; i <= Len; i++)
			{
				if(TextRender()->TextWidth(0, FontSize, pStr, i) - *Offset > MxRel)
				{
					s_AtIndex = i - 1;
					break;
				}

				if(i == Len)
					s_AtIndex = Len;
			}
		}
		else if(!UI()->MouseButton(0))
			s_DoScroll = false;
		else if(s_DoScroll)
		{
			// do scrolling
			if(UI()->MouseX() < pRect->x && s_ScrollStart-UI()->MouseX() > 10.0f)
			{
				s_AtIndex = max(0, s_AtIndex-1);
				s_ScrollStart = UI()->MouseX();
				UpdateOffset = true;
			}
			else if(UI()->MouseX() > pRect->x+pRect->w && UI()->MouseX()-s_ScrollStart > 10.0f)
			{
				s_AtIndex = min(Len, s_AtIndex+1);
				s_ScrollStart = UI()->MouseX();
				UpdateOffset = true;
			}
		}

		for(int i = 0; i < m_NumInputEvents; i++)
		{
			Len = str_length(pStr);
			int NumChars = Len;
			ReturnValue |= CLineInput::Manipulate(m_aInputEvents[i], pStr, StrSize, StrSize, &Len, &s_AtIndex, &NumChars);
		}
	}

	bool JustGotActive = false;

	if(UI()->ActiveItem() == pID)
	{
		if(!UI()->MouseButton(0))
		{
			s_AtIndex = min(s_AtIndex, str_length(pStr));
			s_DoScroll = false;
			UI()->SetActiveItem(0);
		}
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			if (UI()->LastActiveItem() != pID)
				JustGotActive = true;
			UI()->SetActiveItem(pID);
		}
	}

	if(Inside)
		UI()->SetHotItem(pID);

	CUIRect Textbox = *pRect;
	vec4 Color = vec4(1/255.0f, 204/255.0f, 140/255.0f, 0.5f);
	RenderTools()->DrawUIRect(&Textbox, Color, Corners, 2.0f);

	Graphics()->TextureSet(m_TextureButton);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(Color.r,Color.g,Color.b,Color.a);
	IGraphics::CQuadItem QuadItem(pRect->x, pRect->y, pRect->w, pRect->h);
	Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	Textbox.VMargin(2.0f, &Textbox);
	Textbox.HMargin(2.0f, &Textbox);

	const char *pDisplayStr = pStr;
	char aStars[128];

	if(Hidden)
	{
		unsigned s = str_length(pStr);
		if(s >= sizeof(aStars))
			s = sizeof(aStars)-1;
		for(unsigned int i = 0; i < s; ++i)
			aStars[i] = '*';
		aStars[s] = 0;
		pDisplayStr = aStars;
	}

	// check if the text has to be moved
	if(UI()->LastActiveItem() == pID && !JustGotActive && (UpdateOffset || m_NumInputEvents))
	{
		float w = TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex);
		if(w-*Offset > Textbox.w)
		{
			// move to the left
			float wt = TextRender()->TextWidth(0, FontSize, pDisplayStr, -1);
			do
			{
				*Offset += min(wt-*Offset-Textbox.w, Textbox.w/3);
			}
			while(w-*Offset > Textbox.w);
		}
		else if(w-*Offset < 0.0f)
		{
			// move to the right
			do
			{
				*Offset = max(0.0f, *Offset-Textbox.w/3);
			}
			while(w-*Offset < 0.0f);
		}
	}
	UI()->ClipEnable(pRect);
	Textbox.x -= *Offset;

	CUIRect Shadow = Textbox; Shadow.x += 3; Shadow.y += 3;
	vec4 OldColor;
	TextRender()->GetTextColor(&OldColor.r, &OldColor.g, &OldColor.b, &OldColor.a);
	TextRender()->TextColor(0.0f, 0.0f, 0.0f, 0.4f);
	UI()->DoLabel(&Shadow, pDisplayStr, FontSize, -1);
	TextRender()->TextColor(OldColor.r, OldColor.g, OldColor.b, OldColor.a);
	UI()->DoLabel(&Textbox, pDisplayStr, FontSize, -1);

	// render the cursor
	if(UI()->LastActiveItem() == pID && !JustGotActive)
	{
		float w = TextRender()->TextWidth(0, FontSize, pDisplayStr, s_AtIndex);
		Textbox = *pRect;
		Textbox.VSplitLeft(2.0f, 0, &Textbox);
		Textbox.x += (w-*Offset-TextRender()->TextWidth(0, FontSize, "|", -1)/2);

		if((2*time_get()/time_freq()) % 2)	// make it blink
		{
			CUIRect Shadow = Textbox; Shadow.x += 3; Shadow.y += 3;
			vec4 OldColor;
			TextRender()->GetTextColor(&OldColor.r, &OldColor.g, &OldColor.b, &OldColor.a);
			TextRender()->TextColor(0.0f, 0.0f, 0.0f, 0.4f);
			UI()->DoLabel(&Shadow, "|", FontSize, -1);
			TextRender()->TextColor(OldColor.r, OldColor.g, OldColor.b, OldColor.a);
			UI()->DoLabel(&Textbox, "|", FontSize, -1);
		}
	}
	UI()->ClipDisable();

	return ReturnValue;
}

float CMenus::DoScrollbarV(const void *pID, const CUIRect *pRect, float Current)
{
	CUIRect Handle;
	static float OffsetY;
	pRect->HSplitTop(33, &Handle, 0);

	Handle.y += (pRect->h-Handle.h)*Current;

	// logic
	float ReturnValue = Current;
	int Inside = UI()->MouseInside(&Handle);

	if(UI()->ActiveItem() == pID)
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);

		float Min = pRect->y;
		float Max = pRect->h-Handle.h;
		float Cur = UI()->MouseY()-OffsetY;
		ReturnValue = (Cur-Min)/Max;
		if(ReturnValue < 0.0f) ReturnValue = 0.0f;
		if(ReturnValue > 1.0f) ReturnValue = 1.0f;
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(pID);
			OffsetY = UI()->MouseY()-Handle.y;
		}
	}

	if(Inside)
		UI()->SetHotItem(pID);

	// render
	CUIRect Rail;
	pRect->VMargin(5.0f, &Rail);
	RenderTools()->DrawUIRect(&Rail, vec4(1,1,1,0.25f), 0, 0.0f);

	CUIRect Slider = Handle;
	Slider.w = Rail.x-Slider.x;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_L, 2.5f);
	Slider.x = Rail.x+Rail.w;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_R, 2.5f);

	Slider = Handle;
	Slider.Margin(5.0f, &Slider);
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f)*ButtonColorMul(pID), CUI::CORNER_ALL, 2.5f);

	return ReturnValue;
}



float CMenus::DoScrollbarH(const void *pID, const CUIRect *pRect, float Current)
{
	CUIRect Handle;
	static float OffsetX;
	pRect->VSplitLeft(33, &Handle, 0);

	Handle.x += (pRect->w-Handle.w)*Current;

	// logic
	float ReturnValue = Current;
	int Inside = UI()->MouseInside(&Handle);

	if(UI()->ActiveItem() == pID)
	{
		if(!UI()->MouseButton(0))
			UI()->SetActiveItem(0);

		float Min = pRect->x;
		float Max = pRect->w-Handle.w;
		float Cur = UI()->MouseX()-OffsetX;
		ReturnValue = (Cur-Min)/Max;
		if(ReturnValue < 0.0f) ReturnValue = 0.0f;
		if(ReturnValue > 1.0f) ReturnValue = 1.0f;
	}
	else if(UI()->HotItem() == pID)
	{
		if(UI()->MouseButton(0))
		{
			UI()->SetActiveItem(pID);
			OffsetX = UI()->MouseX()-Handle.x;
		}
	}

	if(Inside)
		UI()->SetHotItem(pID);

	// render
	CUIRect Rail;
	pRect->HMargin(5.0f, &Rail);
	RenderTools()->DrawUIRect(&Rail, vec4(1,1,1,0.25f), 0, 0.0f);

	CUIRect Slider = Handle;
	Slider.h = Rail.y-Slider.y;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_T, 2.5f);
	Slider.y = Rail.y+Rail.h;
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f), CUI::CORNER_B, 2.5f);

	Slider = Handle;
	Slider.Margin(5.0f, &Slider);
	RenderTools()->DrawUIRect(&Slider, vec4(1,1,1,0.25f)*ButtonColorMul(pID), CUI::CORNER_ALL, 2.5f);

	return ReturnValue;
}

int CMenus::DoKeyReader(void *pID, const CUIRect *pRect, int Key)
{
	// process
	static void *pGrabbedID = 0;
	static bool MouseReleased = true;
	static int ButtonUsed = 0;
	int Inside = UI()->MouseInside(pRect);
	int NewKey = Key;

	if(!UI()->MouseButton(0) && !UI()->MouseButton(1) && pGrabbedID == pID)
		MouseReleased = true;

	if(UI()->ActiveItem() == pID)
	{
		if(m_Binder.m_GotKey)
		{
			// abort with escape key
			if(m_Binder.m_Key.m_Key != KEY_ESCAPE)
				NewKey = m_Binder.m_Key.m_Key;
			m_Binder.m_GotKey = false;
			UI()->SetActiveItem(0);
			MouseReleased = false;
			pGrabbedID = pID;
		}

		if(ButtonUsed == 1 && !UI()->MouseButton(1))
		{
			if(Inside)
				NewKey = 0;
			UI()->SetActiveItem(0);
		}
	}
	else if(UI()->HotItem() == pID)
	{
		if(MouseReleased)
		{
			if(UI()->MouseButton(0))
			{
				m_Binder.m_TakeKey = true;
				m_Binder.m_GotKey = false;
				UI()->SetActiveItem(pID);
				ButtonUsed = 0;
			}

			if(UI()->MouseButton(1))
			{
				UI()->SetActiveItem(pID);
				ButtonUsed = 1;
			}
		}
	}

	if(Inside)
		UI()->SetHotItem(pID);

	// draw
	if (UI()->ActiveItem() == pID && ButtonUsed == 0)
		DoButton_KeySelect(pID, "???", 0, pRect);
	else
	{
		if(Key == 0)
			DoButton_KeySelect(pID, "", 0, pRect);
		else
			DoButton_KeySelect(pID, Input()->KeyName(Key), 0, pRect);
	}
	return NewKey;
}


int CMenus::RenderMenubar(CUIRect r)
{
	CUIRect Box = r;
	CUIRect Button;

	m_ActivePage = g_Config.m_UiPage;
	int NewPage = -1;

	if(Client()->State() != IClient::STATE_OFFLINE)
		m_ActivePage = m_GamePage;

	if(Client()->State() == IClient::STATE_OFFLINE)
	{
		Box.VSplitLeft(100.0f, &Button, &Box);
		static int s_InternetButton=0;
		if(DoButton_MenuTab(&s_InternetButton, Localize("Internet"), m_ActivePage==PAGE_INTERNET, &Button, CUI::CORNER_TL))
		{
			ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
			NewPage = PAGE_INTERNET;
		}

		//Box.VSplitLeft(4.0f, 0, &Box);
		Box.VSplitLeft(80.0f, &Button, &Box);
		static int s_LanButton=0;
		if(DoButton_MenuTab(&s_LanButton, Localize("LAN"), m_ActivePage==PAGE_LAN, &Button, 0))
		{
			ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
			NewPage = PAGE_LAN;
		}

		//box.VSplitLeft(4.0f, 0, &box);
		Box.VSplitLeft(110.0f, &Button, &Box);
		static int s_FavoritesButton=0;
		if(DoButton_MenuTab(&s_FavoritesButton, Localize("Favorites"), m_ActivePage==PAGE_FAVORITES, &Button, CUI::CORNER_TR))
		{
			ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
			NewPage = PAGE_FAVORITES;
		}
	}
	else
	{
		// online menus
		Box.VSplitLeft(90.0f, &Button, &Box);
		static int s_GameButton=0;
		if(DoButton_MenuTab(&s_GameButton, Localize("Game"), m_ActivePage==PAGE_GAME, &Button, CUI::CORNER_TL))
			NewPage = PAGE_GAME;

		Box.VSplitLeft(90.0f, &Button, &Box);
		static int s_PlayersButton=0;
		if(DoButton_MenuTab(&s_PlayersButton, Localize("Players"), m_ActivePage==PAGE_PLAYERS, &Button, 0))
			NewPage = PAGE_PLAYERS;

		Box.VSplitLeft(130.0f, &Button, &Box);
		static int s_ServerInfoButton=0;
		if(DoButton_MenuTab(&s_ServerInfoButton, Localize("Server info"), m_ActivePage==PAGE_SERVER_INFO, &Button, 0))
			NewPage = PAGE_SERVER_INFO;

		Box.VSplitLeft(130.0f, &Button, &Box);
		static int s_CallVoteButton=0;
		if(DoButton_MenuTab(&s_CallVoteButton, Localize("Call vote"), m_ActivePage==PAGE_CALLVOTE, &Button, CUI::CORNER_TR))
			NewPage = PAGE_CALLVOTE;

		Box.VSplitRight(10.0f, &Box, &Button);
		Box.VSplitRight(130.0f, &Box, &Button);
		static int s_SettingsButton=0;
		if(DoButton_MenuTab(&s_SettingsButton, Localize("Settings"), m_ActivePage==PAGE_SETTINGS, &Button, CUI::CORNER_TR))
			NewPage = PAGE_SETTINGS;

		Box.VSplitRight(130.0f, &Box, &Button);
		static int s_ExtrasButton=0;
		if(DoButton_MenuTab(&s_ExtrasButton, Localize("Extras"), m_ActivePage==PAGE_EXTRAS, &Button, CUI::CORNER_TL))
			NewPage = PAGE_EXTRAS;
	}

	/*
	box.VSplitRight(110.0f, &box, &button);
	static int system_button=0;
	if (UI()->DoButton(&system_button, "System", g_Config.m_UiPage==PAGE_SYSTEM, &button))
		g_Config.m_UiPage = PAGE_SYSTEM;

	box.VSplitRight(30.0f, &box, 0);
	*/

	if(NewPage != -1)
	{
		if(Client()->State() == IClient::STATE_OFFLINE)
			g_Config.m_UiPage = NewPage;
		else
			m_GamePage = NewPage;
	}

	return 0;
}

bool CMenus::Grow(float *pSrc, float To, float Speed)
{
	float Dist = (float)fabsolute(*pSrc - To);
	if (Dist >= Speed*0.03f)
	{
		*pSrc += (Dist*Speed) * (To > *pSrc ? 1 : -1);
		return true;
	}
	else
	{
		*pSrc = To;
		return false;
	}
}

void CMenus::RenderLoading()
{
	// TODO: not supported right now due to separate render thread

	static int64 LastLoadRender = 0;
	float Percent = m_LoadCurrent++/(float)m_LoadTotal;
	/*static float a = 0.0f;
	a += 0.001f;
	if (a >= 1.0f)
		a = 0.0f;
	float Percent = a;*/

	//static float Percent = 0;
	//Grow(&Percent, PercentStatic, 0.05);

	// make sure that we don't render for each little thing we load
	// because that will slow down loading if we have vsync
	if(time_get()-LastLoadRender < time_freq()/60)
		return;

	LastLoadRender = time_get();

	// need up date this here to get correct
	vec3 Rgb = HslToRgb(vec3(g_Config.m_UiColorHue/255.0f, g_Config.m_UiColorSat/255.0f, g_Config.m_UiColorLht/255.0f));
	ms_GuiColor = vec4(Rgb.r, Rgb.g, Rgb.b, g_Config.m_UiColorAlpha/255.0f);

	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	RenderBackground();

	static int s_PaddelSize = 256.0f;

	Graphics()->BlendNormal();

	CUIRect AvatarRect; AvatarRect.x = Screen.w*0.5f-s_PaddelSize*0.5f; AvatarRect.y = Screen.h*0.5f-s_PaddelSize*0.5f; AvatarRect.w = s_PaddelSize; AvatarRect.h = s_PaddelSize;

	/*if (Graphics()->UseShader())
	{
		float aVar[4];
		Graphics()->FrameBufferBegin(IGraphics::FBO_LOADGREY);

		Graphics()->ShaderBegin(IGraphics::SHADER_LOADGREY);

		aVar[0] = Graphics()->ScreenWidth();
		aVar[1] = Graphics()->ScreenHeight();
		Graphics()->ShaderUniformSet("u_Resolution", aVar, 2);

		aVar[0] = Percent;
		Graphics()->ShaderUniformSet("u_Percent", aVar, 1);

		aVar[0] = Graphics()->ScreenHeight();
		Graphics()->ShaderUniformSet("u_Radius", aVar, 1);

		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		Graphics()->QuadsDrawTL(&IGraphics::CQuadItem(0, 0, Screen.w, Screen.h), 1);
		Graphics()->QuadsEnd();

		Graphics()->TextureSet(gs_TexturePaddel);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		IGraphics::CQuadItem QuadItem = IGraphics::CQuadItem(0, 0, 10, 10);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();

		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		Graphics()->QuadsDrawTL(&IGraphics::CQuadItem(0, 0, Screen.w, Screen.h), 1);
		Graphics()->QuadsEnd();

		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		Graphics()->QuadsDrawTL(&IGraphics::CQuadItem(0, 0, Screen.w, Screen.h), 1);
		Graphics()->QuadsEnd();

		Graphics()->FrameBufferEnd();
		Graphics()->ShaderEnd();
	}
	else*/
	{
		float SPercent = Percent * 0.91f;
		CUIRect ClipRect = AvatarRect;
		ClipRect.x += ClipRect.w*SPercent;
		ClipRect.w -= ClipRect.w*SPercent;
		UI()->ClipEnable(&ClipRect);

		Graphics()->TextureSet(gs_TexturePaddel);
		Graphics()->QuadsBegin();

		Graphics()->SetColor(0.5f, 0.5f, 0.5f, 1.0f);
		IGraphics::CQuadItem QuadItem(AvatarRect.x, AvatarRect.y, AvatarRect.w, AvatarRect.h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
		UI()->ClipDisable();

		ClipRect = AvatarRect;
		ClipRect.w *= SPercent;
		UI()->ClipEnable(&ClipRect);

		Graphics()->TextureSet(gs_TexturePaddel);
		Graphics()->QuadsBegin();

		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
		QuadItem = IGraphics::CQuadItem(AvatarRect.x, AvatarRect.y, AvatarRect.w, AvatarRect.h);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
		UI()->ClipDisable();

		Graphics()->TextureSet(m_TextureButton);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(1.0f, 1.0f, 1.0f, 0.7f);
		Graphics()->QuadsSetRotation(pi * 0.5f);
		static const float Height = 18.0f;
		QuadItem = IGraphics::CQuadItem(AvatarRect.x + AvatarRect.w * SPercent - (AvatarRect.w - Height) * 0.5f, AvatarRect.y + AvatarRect.w * 0.5f, AvatarRect.w, Height);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsSetRotation(pi * 1.5f);
		QuadItem = IGraphics::CQuadItem(AvatarRect.x + AvatarRect.w * SPercent - (AvatarRect.w + Height - 2.0f) * 0.5f, AvatarRect.y + AvatarRect.w * 0.5f, AvatarRect.w, Height);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
		Graphics()->QuadsEnd();
	}

	Graphics()->Swap();
}

void CMenus::InitTextures()
{
	gs_TextureBlob = Graphics()->LoadTexture("blob.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	gs_TextureBackInv = Graphics()->LoadTexture("pdl_background_inv.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	gs_TextureSpark = Graphics()->LoadTexture("pdl_spark.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	gs_TextureAurora = Graphics()->LoadTexture("pdl_aurora.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	gs_TextureMainButton = Graphics()->LoadTexture("pdl_mainbutton.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	m_TextureButton = Graphics()->LoadTexture("pdl_button.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);
	gs_TexturePaddel = Graphics()->LoadTexture("pdl_Paddel.png", IStorage::TYPE_ALL, CImageInfo::FORMAT_AUTO, 0);

	m_LoadCurrent = 0;
}

void CMenus::OnInit()
{
	Graphics()->AddTextureUser(this);
	InitTextures();

	if(g_Config.m_ClShowWelcome)
		m_Popup = POPUP_LANGUAGE;
	g_Config.m_ClShowWelcome = 0;

	Console()->Chain("add_favorite", ConchainServerbrowserUpdate, this);
	Console()->Chain("remove_favorite", ConchainServerbrowserUpdate, this);
	Console()->Chain("add_friend", ConchainFriendlistUpdate, this);
	Console()->Chain("remove_friend", ConchainFriendlistUpdate, this);

	// setup load amount
	m_LoadCurrent = 0;
	m_LoadTotal = g_pData->m_NumImages;
	if(!g_Config.m_ClThreadsoundloading)
		m_LoadTotal += g_pData->m_NumSounds;

	InitPaddel();
	InitMainButtons();
}

void CMenus::PopupMessage(const char *pTopic, const char *pBody, const char *pButton)
{
	// reset active item
	UI()->SetActiveItem(0);

	str_copy(m_aMessageTopic, pTopic, sizeof(m_aMessageTopic));
	str_copy(m_aMessageBody, pBody, sizeof(m_aMessageBody));
	str_copy(m_aMessageButton, pButton, sizeof(m_aMessageButton));
	m_Popup = POPUP_MESSAGE;
}


int CMenus::Render()
{
	CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	static bool s_First = true;
	if(s_First)
	{
		if(g_Config.m_UiPage == PAGE_INTERNET)
			ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
		else if(g_Config.m_UiPage == PAGE_LAN)
			ServerBrowser()->Refresh(IServerBrowser::TYPE_LAN);
		else if(g_Config.m_UiPage == PAGE_FAVORITES)
			ServerBrowser()->Refresh(IServerBrowser::TYPE_FAVORITES);
		m_pClient->m_pSounds->Enqueue(CSounds::CHN_MUSIC, SOUND_MENU_ALT);
		s_First = false;
	}

	if(Client()->State() == IClient::STATE_ONLINE)
	{
		ms_ColorTabbarInactive = ms_ColorTabbarInactiveIngame;
		ms_ColorTabbarActive = ms_ColorTabbarActiveIngame;
	}
	else
	{
		RenderBackground();
		ms_ColorTabbarInactive = ms_ColorTabbarInactiveOutgame;
		ms_ColorTabbarActive = ms_ColorTabbarActiveOutgame;
	}

	CUIRect TabBar;
	CUIRect MainView;

	if(m_QuickActive && m_Popup == POPUP_NONE)
	{
		RenderQuickMenu(Screen);
	}

	// some margin around the screen
	Screen.Margin(10.0f, &Screen);

	static bool s_SoundCheck = false;
	if(!s_SoundCheck && m_Popup == POPUP_NONE)
	{
		if(Client()->SoundInitFailed())
			m_Popup = POPUP_SOUNDERROR;
		s_SoundCheck = true;
	}

	if(m_Popup == POPUP_NONE && !m_QuickActive)
	{
		Screen.HSplitTop(24.0f, &TabBar, &MainView);

		if(g_Config.m_UiPage >= PAGE_INTERNET && g_Config.m_UiPage <= PAGE_FAVORITES)
		{
			// do tab bar
			TabBar.VMargin(20.0f, &TabBar);
			RenderMenubar(TabBar);
		}

		// news is not implemented yet
		if(g_Config.m_UiPage < PAGE_PADDEL || g_Config.m_UiPage > PAGE_SETTINGS || (Client()->State() == IClient::STATE_OFFLINE && g_Config.m_UiPage >= PAGE_GAME && g_Config.m_UiPage <= PAGE_CALLVOTE))
		{
			ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
			g_Config.m_UiPage = PAGE_INTERNET;
		}

		// render current page
		if(Client()->State() != IClient::STATE_OFFLINE)
		{
			if(m_GamePage == PAGE_GAME)
				RenderGame(MainView);
			else if(m_GamePage == PAGE_PLAYERS)
				RenderPlayers(MainView);
			else if(m_GamePage == PAGE_SERVER_INFO)
				RenderServerInfo(MainView);
			else if(m_GamePage == PAGE_CALLVOTE)
				RenderServerControl(MainView);
			else if(m_GamePage == PAGE_SETTINGS)
				RenderSettings(MainView);
			else if(m_GamePage == PAGE_EXTRAS)
				RenderExtras(MainView);
		}
		else if(g_Config.m_UiPage == PAGE_INTERNET)
			RenderServerbrowser(MainView);
		else if(g_Config.m_UiPage == PAGE_LAN)
			RenderServerbrowser(MainView);
		else if(g_Config.m_UiPage == PAGE_DEMOS)
			RenderDemoList(MainView);
		else if(g_Config.m_UiPage == PAGE_FAVORITES)
			RenderServerbrowser(MainView);
		else if(g_Config.m_UiPage == PAGE_SETTINGS)
			RenderSettings(MainView);
		else if(g_Config.m_UiPage == PAGE_EXTRAS)
			RenderExtras(MainView);
	}

	DoPaddel();
	DoMainButtons();

	if(m_Popup != POPUP_NONE)
	{
		// make sure that other windows doesn't do anything funnay!
		//UI()->SetHotItem(0);
		//UI()->SetActiveItem(0);
		char aBuf[128];
		const char *pTitle = "";
		const char *pExtraText = "";
		const char *pButtonText = "";
		int ExtraAlign = 0;

		if(m_Popup == POPUP_MESSAGE)
		{
			pTitle = m_aMessageTopic;
			pExtraText = m_aMessageBody;
			pButtonText = m_aMessageButton;
		}
		else if(m_Popup == POPUP_CONNECTING)
		{
			pTitle = Localize("Connecting to");
			pExtraText = g_Config.m_UiServerAddress; // TODO: query the client about the address
			pButtonText = Localize("Abort");
			if(Client()->MapDownloadTotalsize() > 0)
			{
				pTitle = Localize("Downloading map");
				pExtraText = "";
			}
		}
		else if(m_Popup == POPUP_DISCONNECTED)
		{
			pTitle = Localize("Disconnected");
			pExtraText = Client()->ErrorString();
			pButtonText = Localize("Ok");
			ExtraAlign = -1;
		}
		else if(m_Popup == POPUP_PURE)
		{
			pTitle = Localize("Disconnected");
			pExtraText = Localize("The server is running a non-standard tuning on a pure game type.");
			pButtonText = Localize("Ok");
			ExtraAlign = -1;
		}
		else if(m_Popup == POPUP_DELETE_DEMO)
		{
			pTitle = Localize("Delete demo");
			pExtraText = Localize("Are you sure that you want to delete the demo?");
			ExtraAlign = -1;
		}
		else if(m_Popup == POPUP_RENAME_DEMO)
		{
			pTitle = Localize("Rename demo");
			pExtraText = "";
			ExtraAlign = -1;
		}
		else if(m_Popup == POPUP_REMOVE_FRIEND)
		{
			pTitle = Localize("Remove friend");
			pExtraText = Localize("Are you sure that you want to remove the player from your friends list?");
			ExtraAlign = -1;
		}
		else if(m_Popup == POPUP_SOUNDERROR)
		{
			pTitle = Localize("Sound error");
			pExtraText = Localize("The audio device couldn't be initialised.");
			pButtonText = Localize("Ok");
			ExtraAlign = -1;
		}
		else if(m_Popup == POPUP_PASSWORD)
		{
			pTitle = Localize("Password incorrect");
			pExtraText = "";
			pButtonText = Localize("Try again");
		}
		else if(m_Popup == POPUP_QUIT)
		{
			pTitle = Localize("Quit");
			pExtraText = Localize("Are you sure that you want to quit?");
			ExtraAlign = -1;
		}
		else if(m_Popup == POPUP_FIRST_LAUNCH)
		{
			pTitle = Localize("Welcome to Teeworlds");
			pExtraText = Localize("As this is the first time you launch the game, please enter your nick name below. It's recommended that you check the settings to adjust them to your liking before joining a server.");
			pButtonText = Localize("Ok");
			ExtraAlign = -1;
		}

		CUIRect Box, Part;
		Box = Screen;
		Box.VMargin(150.0f/UI()->Scale(), &Box);
		Box.HMargin(150.0f/UI()->Scale(), &Box);

		// render the box
		RenderTools()->DrawUIRect(&Box, vec4(0,0,0,0.5f), CUI::CORNER_ALL, 5.0f);

		Box.HSplitTop(20.f/UI()->Scale(), &Part, &Box);
		Box.HSplitTop(24.f/UI()->Scale(), &Part, &Box);
		TextRender()->Bubble();
		UI()->DoLabelScaled(&Part, pTitle, 24.f, 0);
		TextRender()->Bubble();
		Box.HSplitTop(20.f/UI()->Scale(), &Part, &Box);
		Box.HSplitTop(24.f/UI()->Scale(), &Part, &Box);
		Part.VMargin(20.f/UI()->Scale(), &Part);

		if(ExtraAlign == -1)
			UI()->DoLabelScaled(&Part, pExtraText, 20.f, -1, (int)Part.w);
		else
			UI()->DoLabelScaled(&Part, pExtraText, 20.f, 0, -1);

		if(m_Popup == POPUP_QUIT)
		{
			CUIRect Yes, No;
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);

			// additional info
			Box.HSplitTop(10.0f, 0, &Box);
			Box.VMargin(20.f/UI()->Scale(), &Box);
			if(m_pClient->Editor()->HasUnsavedData())
			{
				char aBuf[256];
				str_format(aBuf, sizeof(aBuf), "%s\n%s", Localize("There's an unsaved map in the editor, you might want to save it before you quit the game."), Localize("Quit anyway?"));
				UI()->DoLabelScaled(&Box, aBuf, 20.f, -1, Part.w-20.0f);
			}

			// buttons
			Part.VMargin(80.0f, &Part);
			Part.VSplitMid(&No, &Yes);
			Yes.VMargin(20.0f, &Yes);
			No.VMargin(20.0f, &No);

			static int s_ButtonAbort = 0;
			if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static int s_ButtonTryAgain = 0;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), 0, &Yes) || m_EnterPressed)
				Client()->Quit();
		}
		else if(m_Popup == POPUP_PASSWORD)
		{
			CUIRect Label, TextBox, TryAgain, Abort;

			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(80.0f, &Part);

			Part.VSplitMid(&Abort, &TryAgain);

			TryAgain.VMargin(20.0f, &TryAgain);
			Abort.VMargin(20.0f, &Abort);

			static int s_ButtonAbort = 0;
			if(DoButton_Menu(&s_ButtonAbort, Localize("Abort"), 0, &Abort) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static int s_ButtonTryAgain = 0;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Try again"), 0, &TryAgain) || m_EnterPressed)
			{
				Client()->Connect(g_Config.m_UiServerAddress);
			}

			Box.HSplitBottom(60.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);

			Part.VSplitLeft(60.0f, 0, &Label);
			Label.VSplitLeft(100.0f, 0, &TextBox);
			TextBox.VSplitLeft(20.0f, 0, &TextBox);
			TextBox.VSplitRight(60.0f, &TextBox, 0);
			UI()->DoLabel(&Label, Localize("Password"), 18.0f, -1);
			static float Offset = 0.0f;
			DoEditBox(&g_Config.m_Password, &TextBox, g_Config.m_Password, sizeof(g_Config.m_Password), 12.0f, &Offset, true);
		}
		else if(m_Popup == POPUP_CONNECTING)
		{
			Box = Screen;
			Box.VMargin(150.0f, &Box);
			Box.HMargin(150.0f, &Box);
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(120.0f, &Part);

			static int s_Button = 0;
			if(DoButton_Menu(&s_Button, pButtonText, 0, &Part) || m_EscapePressed || m_EnterPressed)
			{
				Client()->Disconnect();
				m_Popup = POPUP_NONE;
			}

			if(Client()->MapDownloadTotalsize() > 0)
			{
				int64 Now = time_get();
				if(Now-m_DownloadLastCheckTime >= time_freq())
				{
					if(m_DownloadLastCheckSize > Client()->MapDownloadAmount())
					{
						// map downloaded restarted
						m_DownloadLastCheckSize = 0;
					}

					// update download speed
					float Diff = (Client()->MapDownloadAmount()-m_DownloadLastCheckSize)/((int)((Now-m_DownloadLastCheckTime)/time_freq()));
					float StartDiff = m_DownloadLastCheckSize-0.0f;
					if(StartDiff+Diff > 0.0f)
						m_DownloadSpeed = (Diff/(StartDiff+Diff))*(Diff/1.0f) + (StartDiff/(Diff+StartDiff))*m_DownloadSpeed;
					else
						m_DownloadSpeed = 0.0f;
					m_DownloadLastCheckTime = Now;
					m_DownloadLastCheckSize = Client()->MapDownloadAmount();
				}

				Box.HSplitTop(64.f, 0, &Box);
				Box.HSplitTop(24.f, &Part, &Box);
				str_format(aBuf, sizeof(aBuf), "%d/%d KiB (%.1f KiB/s)", Client()->MapDownloadAmount()/1024, Client()->MapDownloadTotalsize()/1024,	m_DownloadSpeed/1024.0f);
				UI()->DoLabel(&Part, aBuf, 20.f, 0, -1);

				// time left
				const char *pTimeLeftString;
				int TimeLeft = max(1, m_DownloadSpeed > 0.0f ? static_cast<int>((Client()->MapDownloadTotalsize()-Client()->MapDownloadAmount())/m_DownloadSpeed) : 1);
				if(TimeLeft >= 60)
				{
					TimeLeft /= 60;
					pTimeLeftString = TimeLeft == 1 ? Localize("%i minute left") : Localize("%i minutes left");
				}
				else
					pTimeLeftString = TimeLeft == 1 ? Localize("%i second left") : Localize("%i seconds left");
				Box.HSplitTop(20.f, 0, &Box);
				Box.HSplitTop(24.f, &Part, &Box);
				str_format(aBuf, sizeof(aBuf), pTimeLeftString, TimeLeft);
				UI()->DoLabel(&Part, aBuf, 20.f, 0, -1);

				// progress bar
				Box.HSplitTop(20.f, 0, &Box);
				Box.HSplitTop(24.f, &Part, &Box);
				Part.VMargin(40.0f, &Part);
				RenderTools()->DrawUIRect(&Part, vec4(1.0f, 1.0f, 1.0f, 0.25f), CUI::CORNER_ALL, 5.0f);
				Part.w = max(10.0f, (Part.w*Client()->MapDownloadAmount())/Client()->MapDownloadTotalsize());
				RenderTools()->DrawUIRect(&Part, vec4(1.0f, 1.0f, 1.0f, 0.5f), CUI::CORNER_ALL, 5.0f);
			}
		}
		else if(m_Popup == POPUP_LANGUAGE)
		{
			Box = Screen;
			Box.VMargin(150.0f, &Box);
			Box.HMargin(150.0f, &Box);
			Box.HSplitTop(20.f, &Part, &Box);
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Box.HSplitBottom(20.f, &Box, 0);
			Box.VMargin(20.0f, &Box);
			RenderLanguageSelection(Box);
			Part.VMargin(120.0f, &Part);

			static int s_Button = 0;
			if(DoButton_Menu(&s_Button, Localize("Ok"), 0, &Part) || m_EscapePressed || m_EnterPressed)
				m_Popup = POPUP_FIRST_LAUNCH;
		}
		else if(m_Popup == POPUP_COUNTRY)
		{
			Box = Screen;
			Box.VMargin(150.0f, &Box);
			Box.HMargin(150.0f, &Box);
			Box.HSplitTop(20.f, &Part, &Box);
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Box.HSplitBottom(20.f, &Box, 0);
			Box.VMargin(20.0f, &Box);

			static int ActSelection = -2;
			if(ActSelection == -2)
				ActSelection = g_Config.m_BrFilterCountryIndex;
			static float s_ScrollValue = 0.0f;
			int OldSelected = -1;
			UiDoListboxStart(&s_ScrollValue, &Box, 50.0f, Localize("Country"), "", m_pClient->m_pCountryFlags->Num(), 6, OldSelected, s_ScrollValue);

			for(int i = 0; i < m_pClient->m_pCountryFlags->Num(); ++i)
			{
				const CCountryFlags::CCountryFlag *pEntry = m_pClient->m_pCountryFlags->GetByIndex(i);
				if(pEntry->m_CountryCode == ActSelection)
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
					UI()->DoLabel(&Label, pEntry->m_aCountryCodeString, 10.0f, 0);
				}
			}

			const int NewSelected = UiDoListboxEnd(&s_ScrollValue, 0);
			if(OldSelected != NewSelected)
				ActSelection = m_pClient->m_pCountryFlags->GetByIndex(NewSelected)->m_CountryCode;

			Part.VMargin(120.0f, &Part);

			static int s_Button = 0;
			if(DoButton_Menu(&s_Button, Localize("Ok"), 0, &Part) || m_EnterPressed)
			{
				g_Config.m_BrFilterCountryIndex = ActSelection;
				Client()->ServerBrowserUpdate();
				m_Popup = POPUP_NONE;
			}

			if(m_EscapePressed)
			{
				ActSelection = g_Config.m_BrFilterCountryIndex;
				m_Popup = POPUP_NONE;
			}
		}
		else if(m_Popup == POPUP_DELETE_DEMO)
		{
			CUIRect Yes, No;
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(80.0f, &Part);

			Part.VSplitMid(&No, &Yes);

			Yes.VMargin(20.0f, &Yes);
			No.VMargin(20.0f, &No);

			static int s_ButtonAbort = 0;
			if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static int s_ButtonTryAgain = 0;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), 0, &Yes) || m_EnterPressed)
			{
				m_Popup = POPUP_NONE;
				// delete demo
				if(m_DemolistSelectedIndex >= 0 && !m_DemolistSelectedIsDir)
				{
					char aBuf[512];
					str_format(aBuf, sizeof(aBuf), "%s/%s", m_aCurrentDemoFolder, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
					if(Storage()->RemoveFile(aBuf, m_lDemos[m_DemolistSelectedIndex].m_StorageType))
					{
						DemolistPopulate();
						DemolistOnUpdate(false);
					}
					else
						PopupMessage(Localize("Error"), Localize("Unable to delete the demo"), Localize("Ok"));
				}
			}
		}
		else if(m_Popup == POPUP_RENAME_DEMO)
		{
			CUIRect Label, TextBox, Ok, Abort;

			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(80.0f, &Part);

			Part.VSplitMid(&Abort, &Ok);

			Ok.VMargin(20.0f, &Ok);
			Abort.VMargin(20.0f, &Abort);

			static int s_ButtonAbort = 0;
			if(DoButton_Menu(&s_ButtonAbort, Localize("Abort"), 0, &Abort) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static int s_ButtonOk = 0;
			if(DoButton_Menu(&s_ButtonOk, Localize("Ok"), 0, &Ok) || m_EnterPressed)
			{
				m_Popup = POPUP_NONE;
				// rename demo
				if(m_DemolistSelectedIndex >= 0 && !m_DemolistSelectedIsDir)
				{
					char aBufOld[512];
					str_format(aBufOld, sizeof(aBufOld), "%s/%s", m_aCurrentDemoFolder, m_lDemos[m_DemolistSelectedIndex].m_aFilename);
					int Length = str_length(m_aCurrentDemoFile);
					char aBufNew[512];
					if(Length <= 4 || m_aCurrentDemoFile[Length-5] != '.' || str_comp_nocase(m_aCurrentDemoFile+Length-4, "demo"))
						str_format(aBufNew, sizeof(aBufNew), "%s/%s.demo", m_aCurrentDemoFolder, m_aCurrentDemoFile);
					else
						str_format(aBufNew, sizeof(aBufNew), "%s/%s", m_aCurrentDemoFolder, m_aCurrentDemoFile);
					if(Storage()->RenameFile(aBufOld, aBufNew, m_lDemos[m_DemolistSelectedIndex].m_StorageType))
					{
						DemolistPopulate();
						DemolistOnUpdate(false);
					}
					else
						PopupMessage(Localize("Error"), Localize("Unable to rename the demo"), Localize("Ok"));
				}
			}

			Box.HSplitBottom(60.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);

			Part.VSplitLeft(60.0f, 0, &Label);
			Label.VSplitLeft(120.0f, 0, &TextBox);
			TextBox.VSplitLeft(20.0f, 0, &TextBox);
			TextBox.VSplitRight(60.0f, &TextBox, 0);
			UI()->DoLabel(&Label, Localize("New name:"), 18.0f, -1);
			static float Offset = 0.0f;
			DoEditBox(&Offset, &TextBox, m_aCurrentDemoFile, sizeof(m_aCurrentDemoFile), 12.0f, &Offset);
		}
		else if(m_Popup == POPUP_REMOVE_FRIEND)
		{
			CUIRect Yes, No;
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(80.0f, &Part);

			Part.VSplitMid(&No, &Yes);

			Yes.VMargin(20.0f, &Yes);
			No.VMargin(20.0f, &No);

			static int s_ButtonAbort = 0;
			if(DoButton_Menu(&s_ButtonAbort, Localize("No"), 0, &No) || m_EscapePressed)
				m_Popup = POPUP_NONE;

			static int s_ButtonTryAgain = 0;
			if(DoButton_Menu(&s_ButtonTryAgain, Localize("Yes"), 0, &Yes) || m_EnterPressed)
			{
				m_Popup = POPUP_NONE;
				// remove friend
				if(m_FriendlistSelectedIndex >= 0)
				{
					m_pClient->Friends()->RemoveFriend(m_lFriends[m_FriendlistSelectedIndex].m_pFriendInfo->m_aName,
						m_lFriends[m_FriendlistSelectedIndex].m_pFriendInfo->m_aClan);
					FriendlistOnUpdate();
					Client()->ServerBrowserUpdate();
				}
			}
		}
		else if(m_Popup == POPUP_FIRST_LAUNCH)
		{
			CUIRect Label, TextBox;

			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(80.0f, &Part);

			static int s_EnterButton = 0;
			if(DoButton_Menu(&s_EnterButton, Localize("Enter"), 0, &Part) || m_EnterPressed)
				m_Popup = POPUP_NONE;

			Box.HSplitBottom(40.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);

			Part.VSplitLeft(60.0f, 0, &Label);
			Label.VSplitLeft(100.0f, 0, &TextBox);
			TextBox.VSplitLeft(20.0f, 0, &TextBox);
			TextBox.VSplitRight(60.0f, &TextBox, 0);
			UI()->DoLabel(&Label, Localize("Nickname"), 18.0f, -1);
			static float Offset = 0.0f;
			DoEditBox(&g_Config.m_PlayerName, &TextBox, g_Config.m_PlayerName, sizeof(g_Config.m_PlayerName), 12.0f, &Offset);
		}
		else
		{
			Box.HSplitBottom(20.f, &Box, &Part);
			Box.HSplitBottom(24.f, &Box, &Part);
			Part.VMargin(120.0f, &Part);

			static int s_Button = 0;
			if(DoButton_Menu(&s_Button, pButtonText, 0, &Part) || m_EscapePressed || m_EnterPressed)
				m_Popup = POPUP_NONE;
		}

		if(m_Popup == POPUP_NONE)
			UI()->SetActiveItem(0);
	}

	return 0;
}


void CMenus::SetActive(bool Active)
{
	m_MenuActive = Active;
	if(!m_MenuActive)
	{
		if(m_NeedSendinfo)
		{
			m_pClient->SendInfo(false);
			m_NeedSendinfo = false;
		}

		if(Client()->State() == IClient::STATE_ONLINE)
		{
			m_pClient->OnRelease();
		}
	}
	else if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		m_pClient->OnRelease();
	}
}

void CMenus::OnReset()
{
	m_LoadCurrent = 0;
}

void CMenus::JoystickInput(int *pButtons)
{
	static bool s_MenuPressed = false;
	if(Input()->JoystickButton(7))
	{
		if(s_MenuPressed == false)
		{
			SetActive(!IsActive());
			s_MenuPressed = true;

			m_LastInput = time_get();
		}
	}
	else
		s_MenuPressed = false;

	if(!m_MenuActive && !m_QuickActive)
		return;

	float MouseAxeX = Input()->JoystickAxes(0);
	float MouseAxeY = Input()->JoystickAxes(1);
	float MouseAxeSX = Input()->JoystickAxes(5);
	float MouseAxeSY = Input()->JoystickAxes(4);

	if(fabsolute(MouseAxeX) > 0.2f)
	{
		m_MousePos.x += MouseAxeX*5.5f;
		m_LastInput = time_get();
	}

	if(fabsolute(MouseAxeY) > 0.2f)
	{
		m_MousePos.y += MouseAxeY*5.5f;
		m_LastInput = time_get();
	}

	if(fabsolute(MouseAxeSX) > 0.2f)
	{
		m_MousePos.x += MouseAxeSX;
		m_LastInput = time_get();
	}

	if(fabsolute(MouseAxeSY) > 0.2f)
	{
		m_MousePos.y += MouseAxeSY;
		m_LastInput = time_get();
	}

	if(m_MousePos.x < 0) m_MousePos.x = 0;
	if(m_MousePos.y < 0) m_MousePos.y = 0;
	if(m_MousePos.x > Graphics()->ScreenWidth()) m_MousePos.x = Graphics()->ScreenWidth();
	if(m_MousePos.y > Graphics()->ScreenHeight()) m_MousePos.y = Graphics()->ScreenHeight();

	if(Input()->JoystickButton(0))
	{
		*pButtons |= 1;
		m_LastInput = time_get();
	}

	if(Input()->JoystickButton(3))
	{
		*pButtons |= 2;
		m_LastInput = time_get();
	}

	static bool s_EscapePressed = false;
	if(Input()->JoystickButton(4))
	{
		if(s_EscapePressed == false)
		{
			m_EscapePressed = true;
			s_EscapePressed = true;

			m_LastInput = time_get();
		}
	}
	else
		s_EscapePressed = false;

	static bool s_EnterPressed = false;
	if(Input()->JoystickButton(2))
	{
		if(s_EnterPressed == false)
		{
			m_EnterPressed = true;
			s_EnterPressed = true;

			m_LastInput = time_get();
		}
	}
	else
		s_EnterPressed = false;
}

bool CMenus::OnMouseMove(float x, float y)
{
	m_LastInput = time_get();

	if(!m_MenuActive && !m_QuickActive)
		return false;

	UI()->ConvertMouseMove(&x, &y);
	m_MousePos.x += x;
	m_MousePos.y += y;

	if(m_MousePos.x < 0) m_MousePos.x = 0;
	if(m_MousePos.y < 0) m_MousePos.y = 0;
	if(m_MousePos.x > Graphics()->ScreenWidth()) m_MousePos.x = Graphics()->ScreenWidth();
	if(m_MousePos.y > Graphics()->ScreenHeight()) m_MousePos.y = Graphics()->ScreenHeight();

	return true;
}

void CMenus::OnConsoleInit()
{
	Console()->Register("+quick", "", CFGFLAG_CLIENT, ConKeyInputState, &m_QuickActive, "Opens the Quickmenu");
}

bool CMenus::OnInput(IInput::CEvent e)
{
	m_LastInput = time_get();

	// special handle esc and enter for popup purposes
	if(e.m_Flags&IInput::FLAG_PRESS)
	{
		if(e.m_Key == KEY_ESCAPE)
		{
			m_EscapePressed = true;
			SetActive(!IsActive());
			return true;
		}
	}

	if(IsActive())
	{
		if(e.m_Flags&IInput::FLAG_PRESS)
		{
			// special for popups
			if(e.m_Key == KEY_RETURN || e.m_Key == KEY_KP_ENTER)
				m_EnterPressed = true;
			else if(e.m_Key == KEY_DELETE)
				m_DeletePressed = true;
		}

		if(m_NumInputEvents < MAX_INPUTEVENTS)
			m_aInputEvents[m_NumInputEvents++] = e;
		return true;
	}


	if(e.m_Flags&IInput::FLAG_PRESS)
	{
		if(e.m_Key == KEY_MOUSE_1 || e.m_Key == KEY_MOUSE_2)
		{
			if(m_QuickActive && QuickMenuLockInput())
				return true;
		}
	}

	return false;
}

void CMenus::OnStateChange(int NewState, int OldState)
{
	// reset active item
	UI()->SetActiveItem(0);

	if(NewState == IClient::STATE_OFFLINE)
	{
		if(OldState >= IClient::STATE_ONLINE && NewState < IClient::STATE_QUITING)
			m_pClient->m_pSounds->Play(CSounds::CHN_MUSIC, SOUND_MENU_ALT, 1.0f);
		m_Popup = POPUP_NONE;
		if(Client()->ErrorString() && Client()->ErrorString()[0] != 0)
		{
			if(str_find(Client()->ErrorString(), "password"))
			{
				m_Popup = POPUP_PASSWORD;
				UI()->SetHotItem(&g_Config.m_Password);
				UI()->SetActiveItem(&g_Config.m_Password);
			}
			else
				m_Popup = POPUP_DISCONNECTED;
		}
	}
	else if(NewState == IClient::STATE_LOADING)
	{
		m_Popup = POPUP_CONNECTING;
		m_DownloadLastCheckTime = time_get();
		m_DownloadLastCheckSize = 0;
		m_DownloadSpeed = 0.0f;
		//client_serverinfo_request();
	}
	else if(NewState == IClient::STATE_CONNECTING)
		m_Popup = POPUP_CONNECTING;
	else if (NewState == IClient::STATE_ONLINE || NewState == IClient::STATE_DEMOPLAYBACK)
	{
		m_Popup = POPUP_NONE;
		SetActive(false);
	}
}

extern "C" void font_debug_render();

void CMenus::OnRender()
{
	/*
	// text rendering test stuff
	render_background();

	CTextCursor cursor;
	TextRender()->SetCursor(&cursor, 10, 10, 20, TEXTFLAG_RENDER);
	TextRender()->TextEx(&cursor, "ようこそ - ガイド", -1);

	TextRender()->SetCursor(&cursor, 10, 30, 15, TEXTFLAG_RENDER);
	TextRender()->TextEx(&cursor, "ようこそ - ガイド", -1);

	//Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
	Graphics()->QuadsDrawTL(60, 60, 5000, 5000);
	Graphics()->QuadsEnd();
	return;*/

	int Buttons = 0;

	if(g_Config.m_Josystick)
		JoystickInput(&Buttons);

	if(Client()->State() != IClient::STATE_ONLINE && Client()->State() != IClient::STATE_DEMOPLAYBACK)
		SetActive(true);

	if(Client()->State() == IClient::STATE_DEMOPLAYBACK)
	{
		CUIRect Screen = *UI()->Screen();
		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);
		RenderDemoPlayer(Screen);
	}

	if(Client()->State() == IClient::STATE_ONLINE && m_pClient->m_ServerMode == m_pClient->SERVERMODE_PUREMOD)
	{
		Client()->Disconnect();
		SetActive(true);
		m_Popup = POPUP_PURE;
	}

	if(!IsActive() && !m_QuickActive)
	{
		m_EscapePressed = false;
		m_EnterPressed = false;
		m_DeletePressed = false;
		m_NumInputEvents = 0;
		return;
	}

	// update colors
	vec3 Rgb = HslToRgb(vec3(g_Config.m_UiColorHue/255.0f, g_Config.m_UiColorSat/255.0f, g_Config.m_UiColorLht/255.0f));
	ms_GuiColor = vec4(Rgb.r, Rgb.g, Rgb.b, g_Config.m_UiColorAlpha/255.0f);

	ms_ColorTabbarInactiveOutgame = vec4(0,0,0,0.25f);
	ms_ColorTabbarActiveOutgame = vec4(0,0,0,0.5f);

	float ColorIngameScaleI = 0.5f;
	float ColorIngameAcaleA = 0.2f;
	ms_ColorTabbarInactiveIngame = vec4(
		ms_GuiColor.r*ColorIngameScaleI,
		ms_GuiColor.g*ColorIngameScaleI,
		ms_GuiColor.b*ColorIngameScaleI,
		ms_GuiColor.a*0.8f);

	ms_ColorTabbarActiveIngame = vec4(
		ms_GuiColor.r*ColorIngameAcaleA,
		ms_GuiColor.g*ColorIngameAcaleA,
		ms_GuiColor.b*ColorIngameAcaleA,
		ms_GuiColor.a);

	// update the ui
	CUIRect *pScreen = UI()->Screen();
	float mx = (m_MousePos.x/(float)Graphics()->ScreenWidth())*pScreen->w;
	float my = (m_MousePos.y/(float)Graphics()->ScreenHeight())*pScreen->h;

	if(m_UseMouseButtons)
	{
		if(Input()->KeyPressed(KEY_MOUSE_1)) Buttons |= 1;
		if(Input()->KeyPressed(KEY_MOUSE_2)) Buttons |= 2;
		if(Input()->KeyPressed(KEY_MOUSE_3)) Buttons |= 4;
	}

	UI()->Update(mx,my,mx*3.0f,my*3.0f,Buttons);

	// render
	if(Client()->State() != IClient::STATE_DEMOPLAYBACK)
		Render();

	// render cursor
	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_CURSOR].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1,1,1,1);
	IGraphics::CQuadItem QuadItem(mx, my, 27, 27);
	if(m_LastInput + time_freq() * 15.0f > time_get())
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// render debug information
	if(g_Config.m_Debug)
	{
		CUIRect Screen = *UI()->Screen();
		Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

		char aBuf[512];
		str_format(aBuf, sizeof(aBuf), "%p %p %p", UI()->HotItem(), UI()->ActiveItem(), UI()->LastActiveItem());
		CTextCursor Cursor;
		TextRender()->SetCursor(&Cursor, 10, 10, 10, TEXTFLAG_RENDER);
		TextRender()->TextEx(&Cursor, aBuf, -1);
	}

	m_EscapePressed = false;
	m_EnterPressed = false;
	m_DeletePressed = false;
	m_NumInputEvents = 0;
}

void CMenus::InitMainButtons()
{
	DoMainButtons(true);

	for(int i = 0; i < NUM_MAINBUTTONS; i++)
		m_aMainButtons[i].m_Pos = m_aMainButtons[i].m_WantedPos;
}

void CMenus::DoMainButtons(bool Init)
{

	CUIRect Screen = *UI()->Screen();
	float sw = Screen.w;
	float sh = Screen.h;
	//Graphics()->MapScreen(0, 0, sw, sh);

	float ButtonW = 256.0f;
	float ButtonH = 64.0f;

	//moving
	bool Moving = false;
	static int64 s_LastMove = 0;
	if(s_LastMove < time_get())
	{
		for(int i = 0; i < NUM_MAINBUTTONS; i++)
		{
			float PosDist = distance(m_aMainButtons[i].m_Pos, m_aMainButtons[i].m_WantedPos);
			if(PosDist > 0)
			{
				vec2 Dir = normalize(m_aMainButtons[i].m_WantedPos-m_aMainButtons[i].m_Pos);
				m_aMainButtons[i].m_Pos += Dir*min(10.0f, PosDist);
				s_LastMove = time_get() + time_freq()*0.005f;
				Moving = true;
			}
		}

	}

	if(Moving)
	{

	}
	else if(g_Config.m_UiPage == PAGE_SELECT && Client()->State() == IClient::STATE_OFFLINE)
	{
		for(int i = 0; i < NUM_MAINBUTTONS; i++)
		{
			float Border = 32.0f;
			float PosY = ((sh-Border - 20.0f)/NUM_MAINBUTTONS) * i + Border;
			m_aMainButtons[i].m_WantedPos = vec2(100, PosY);
			m_aMainButtons[i].m_Scale = 1.0f;

			float t = Client()->LocalTime();
			m_aMainButtons[i].m_WantedPos += vec2(sinusf(t+0.7f*i), cosinusf(t+0.7f*i))*2.0f;

			if(m_Popup == POPUP_NONE)
			{
				if(UI()->ActiveItem() == &m_aMainButtons[i].m_ButtonID)
					m_aMainButtons[i].m_Scale = 0.95f;
				else if(UI()->HotItem() == &m_aMainButtons[i].m_ButtonID)
					m_aMainButtons[i].m_Scale = 1.05f;
			}
		}
	}
	else
	{
		for(int i = 0; i < NUM_MAINBUTTONS; i++)
		{
			float Border = 32.0f;
			float PosY = ((sh-Border - ButtonH*0.5f)/NUM_MAINBUTTONS) * i + Border;
			m_aMainButtons[i].m_WantedPos = vec2(-(ButtonW+32), PosY);
			m_aMainButtons[i].m_Scale = 1.0f;
		}
	}

	if (Init == true)
		return;

	Graphics()->TextureSet(gs_TextureMainButton);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	IGraphics::CQuadItem aQuadItem[NUM_MAINBUTTONS];
	for(int i = 0; i < NUM_MAINBUTTONS; i++)
	{
		int ScaleSizeW = ButtonW*m_aMainButtons[i].m_Scale-ButtonW;
		int ScaleSizeH = ButtonH*m_aMainButtons[i].m_Scale-ButtonH;
		aQuadItem[i] = IGraphics::CQuadItem(m_aMainButtons[i].m_Pos.x-ScaleSizeW*0.5f, m_aMainButtons[i].m_Pos.y-ScaleSizeH*0.5f, ButtonW+ScaleSizeW, ButtonH+ScaleSizeH*2.0f);
	}
	Graphics()->QuadsDrawTL(aQuadItem, NUM_MAINBUTTONS);
	Graphics()->QuadsEnd();

	//for(int i = 0; i < NUM_MAINBUTTONS; i++)
	{
		/*int i = 0;
		int ScaleSizeW = ButtonW*m_aMainButtons[i].m_Scale-ButtonW;
		int ScaleSizeH = ButtonH*m_aMainButtons[i].m_Scale-ButtonH;
		float ButtonWidth = ButtonW+ScaleSizeW;
		float ButtonHeight = ButtonH+ScaleSizeH*2.0f;
		float aVar[4];
		Graphics()->ShaderBegin(IGraphics::SHADER_BUTTON);

		aVar[0] = 1000;
		aVar[1] = 1000;
		Graphics()->ShaderUniformSet("u_Resolution", aVar, 2);

		static float s_Move;
		s_Move += 0.005f;
		aVar[0] = s_Move;
		Graphics()->ShaderUniformSet("u_Time", aVar, 1);

		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
			Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
			Graphics()->QuadsDrawTL(&IGraphics::CQuadItem(m_aMainButtons[i].m_Pos.x-ScaleSizeW*0.5f, m_aMainButtons[i].m_Pos.y-ScaleSizeH*0.5f, ButtonWidth, ButtonHeight), 1);
		Graphics()->QuadsEnd();

		Graphics()->ShaderEnd();*/
	}


	//Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);

	CUIRect Text;
	TextRender()->Bubble();
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 0.82f);
	for(int i = 0; i < NUM_MAINBUTTONS; i++)
	{
		int ScaleSizeH = ButtonH*m_aMainButtons[i].m_Scale-ButtonH;
		Text.x = m_aMainButtons[i].m_Pos.x; Text.y = m_aMainButtons[i].m_Pos.y + (ButtonH*0.5f-(32.0f+ScaleSizeH)*0.5f); Text.w = ButtonW; Text.h = ButtonH;
		UI()->DoLabel(&Text, Localize(m_aMainButtonText[i]), 28.0f+ScaleSizeH, 0);

		Text.y = m_aMainButtons[i].m_Pos.y;
		if(UI()->DoButtonLogic(&m_aMainButtons[i].m_ButtonID, "", 0, &Text) && m_Popup == POPUP_NONE)
		{
			switch(i)
			{
			case MAINBUTTON_PLAY: g_Config.m_UiPage = PAGE_INTERNET; break;
			case MAINBUTTON_EXTRAS: g_Config.m_UiPage = PAGE_EXTRAS; break;
			case MAINBUTTON_DEMOS: g_Config.m_UiPage = PAGE_DEMOS; break;
			case MAINBUTTON_EDITOR: g_Config.m_ClEditor = 1; break;
			case MAINBUTTON_SETTINGS: g_Config.m_UiPage = PAGE_SETTINGS; break;
			case MAINBUTTON_QUIT: m_Popup = POPUP_QUIT; break;
			}

			if(i == MAINBUTTON_PLAY)
				ServerBrowser()->Refresh(IServerBrowser::TYPE_INTERNET);
		}
	}
	TextRender()->Bubble();
	TextRender()->TextColor(1.0f, 1.0f, 1.0f, 1.0f);
}

void CMenus::InitPaddel()
{
	m_AnimationState = SPRITE_PADDEL_1-1;
	g_Config.m_UiPage = PAGE_PADDEL;

	DoPaddel();

	m_PaddelSize = m_PaddelWantedSize;
	m_PaddelPos = m_PaddelWantedPos;
	m_PaddelAlpha = m_PaddelVisible?1.0f:0.0f;
}

void CMenus::DoPaddel()
{
	CUIRect Screen = *UI()->Screen();

	float sw = 300*Graphics()->ScreenAspect();
	float sh = 300;
	Graphics()->MapScreen(0, 0, sw, sh);

	if(m_AnimationTime < time_get())
	{
		m_AnimationState++;

		if(m_AnimationState > SPRITE_PADDEL_9)
		{
			float NextAnim = random()%20/20.0f+2.0f;
			m_AnimationState = SPRITE_PADDEL_1-1;
			m_AnimationTime = time_get() + time_freq()*NextAnim; 
		}
		else
			m_AnimationTime = time_get() + time_freq()*0.035f;
	}
	
	//move
	bool Moving = false;
	float PosDist = distance(m_PaddelPos, m_PaddelWantedPos);
	if(PosDist > 0)
	{
		static int64 s_MoveTime = 0;
		if(s_MoveTime < time_get())
		{
			vec2 Dir = normalize(m_PaddelWantedPos-m_PaddelPos);
			m_PaddelPos += Dir*min(4.0f, PosDist);
			s_MoveTime = time_get() + time_freq()*0.01f;
		}
		Moving = true;
	}

	static int64 s_SizeTime = 0;
	if(s_SizeTime < time_get() && m_PaddelWantedSize != m_PaddelSize)
	{
		m_PaddelSize = absolute(m_PaddelWantedSize-m_PaddelSize)<2?m_PaddelWantedSize:(m_PaddelWantedSize>m_PaddelSize?m_PaddelSize+2:m_PaddelSize-2);
		s_SizeTime = time_get() + time_freq()*0.005f;
	}

	static int64 s_AlphaTime = 0;
	if(s_AlphaTime < time_get())
	{
		if(m_PaddelVisible)
			m_PaddelAlpha = clamp(m_PaddelAlpha + 0.1f, 0.0f, 1.0f);
		else
			m_PaddelAlpha = clamp(m_PaddelAlpha - 0.1f, 0.0f, 1.0f);

		s_AlphaTime = time_get() + time_freq()*0.01f;
	}

	//set state
	if(Moving)
	{

	}
	else if(g_Config.m_UiPage == PAGE_PADDEL && Client()->State() == IClient::STATE_OFFLINE)
	{
		m_PaddelWantedPos = vec2(sw*0.5f, sh*0.5f);
		m_PaddelWantedSize = 129+sinusf(Client()->LocalTime()*2.5f)*4.0f;
		m_PaddelRotation = 0.0f;
		m_PaddelVisible = true;

		float mx = (m_MousePos.x/(float)Graphics()->ScreenWidth())*300*Graphics()->ScreenAspect();
		float my = (m_MousePos.y/(float)Graphics()->ScreenHeight())*300;
		if(m_Popup == POPUP_NONE && distance(m_PaddelPos, vec2(mx, my)) < m_PaddelSize*0.5f)
		{
			m_PaddelWantedSize += 16;

			if(UI()->MouseButtonClicked(0))
				g_Config.m_UiPage = PAGE_SELECT;
		}

		if(m_Popup == POPUP_NONE && m_EnterPressed)
			g_Config.m_UiPage = PAGE_SELECT;
	}
	else if(g_Config.m_UiPage == PAGE_SELECT && Client()->State() == IClient::STATE_OFFLINE)
	{
		m_PaddelWantedPos = vec2(sw*0.5f+128.0f, sh*0.5f);
		m_PaddelWantedSize = 128.0f;
		m_PaddelRotation = sinusf(time_get()/(time_freq()*0.25f))*0.03f;
		m_PaddelVisible = true;

		if(m_LastInput + time_freq() * 15.0f < time_get())
			g_Config.m_UiPage = PAGE_PADDEL;
	}
	else if(Client()->State() == IClient::STATE_OFFLINE)
	{
		m_PaddelWantedPos = vec2(sw-16.0f, 16.0f);
		m_PaddelWantedSize = 32.0f;
		m_PaddelRotation = 0.0f;
		m_PaddelVisible = true;

		float mx = (m_MousePos.x/(float)Graphics()->ScreenWidth())*300*Graphics()->ScreenAspect();
		float my = (m_MousePos.y/(float)Graphics()->ScreenHeight())*300;

		if(distance(m_PaddelPos, vec2(mx, my)) < m_PaddelSize*0.5f)
		{
			m_PaddelWantedSize += 8;

			if(UI()->MouseButtonClicked(0))
				g_Config.m_UiPage = PAGE_SELECT;
		}
	}
	else
	{
		m_PaddelWantedPos = vec2(sw-16.0f, 16.0f);
		m_PaddelWantedSize = 32.0f;
		m_PaddelRotation = 0.0f;
		m_PaddelVisible = false;
	}

	//render

	Graphics()->TextureSet(gs_TexturePaddel);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, m_PaddelAlpha);
	Graphics()->QuadsSetRotation(m_PaddelRotation);
	Graphics()->QuadsDrawTL(&IGraphics::CQuadItem(m_PaddelPos.x-m_PaddelSize*0.5f, m_PaddelPos.y-m_PaddelSize*0.5f, m_PaddelSize, m_PaddelSize), 1);
	Graphics()->QuadsEnd();

	Graphics()->TextureSet(g_pData->m_aImages[IMAGE_PADDEL].m_Id);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, m_PaddelAlpha);
	Graphics()->QuadsSetRotation(m_PaddelRotation);
	if(m_AnimationState >= SPRITE_PADDEL_1)
	{
		RenderTools()->SelectSprite(m_AnimationState);
		Graphics()->QuadsDrawTL(&IGraphics::CQuadItem(m_PaddelPos.x-m_PaddelSize*0.5f, m_PaddelPos.y-m_PaddelSize*0.5f, m_PaddelSize, m_PaddelSize), 1);
	}
	Graphics()->QuadsEnd();

	if(g_Config.m_UiPage == PAGE_PADDEL || g_Config.m_UiPage == PAGE_SELECT)
	{
		float mx = (m_MousePos.x/(float)Graphics()->ScreenWidth())*300*Graphics()->ScreenAspect();
		float my = (m_MousePos.y/(float)Graphics()->ScreenHeight())*300;
		if(m_Popup == POPUP_NONE && distance(m_PaddelPos, vec2(mx, my)) < m_PaddelSize*0.5f)
		{
			//m_PaddelWantedSize += 16;

			if(UI()->MouseButtonClicked(0))
				m_pClient->m_pSounds->Play(CSounds::CHN_GUI, SOUND_PLAYER_SPAWN, 1.0f);
		}
	}

	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);
}

void CMenus::RenderBackground()
{
	RenderBackgroundPaddel();
}

void CMenus::RenderBackgroundPaddel()
{
	CUIRect Screen = *UI()->Screen();

	int MouseOffset = 1;
	vec2 MouseFromMidFac = vec2(UI()->MouseX()/Screen.w*2.0f-1.0f, UI()->MouseY()/Screen.h*2.0f-1.0f);

	float sw = 300*Graphics()->ScreenAspect();
	float sh = 300;
	Graphics()->MapScreen(0, 0, sw, sh);

	bool RenderRipple = Graphics()->UseShader() && (g_Config.m_PdlBackgroundRipple == 1);

	if(RenderRipple)
	{
		float aVar[4];
		Graphics()->FrameBufferBegin(IGraphics::FBO_RIPPLE);

		Graphics()->ShaderBegin(IGraphics::SHADER_RIPPLE);

		aVar[0] = Graphics()->ScreenWidth();
		aVar[1] = Graphics()->ScreenHeight();
		Graphics()->ShaderUniformSet("u_Resolution", aVar, 2);

		static float s_Move;
		s_Move += 0.005f;
		aVar[0] = s_Move;
		Graphics()->ShaderUniformSet("u_Time", aVar, 1);
	}

	Graphics()->TextureSet(gs_TextureBackInv);
	Graphics()->QuadsBegin();
	Graphics()->SetColor(1.0f, 1.0f, 1.0f, 1.0f);
	if(RenderRipple)
		Graphics()->QuadsDrawTL(&IGraphics::CQuadItem(0, 0, sw, sh), 1);
	else
	{
		const float qx = MouseFromMidFac.x*MouseOffset - MouseOffset, qy = MouseFromMidFac.y*MouseOffset - MouseOffset, qw = sw + MouseOffset * 2, qh = sh + MouseOffset * 2;
		Graphics()->QuadsDrawTL(&IGraphics::CQuadItem(qx, qy + qh, qw, -qh), 1);
	}
	Graphics()->QuadsEnd();

	if (RenderRipple)
	{
		Graphics()->FrameBufferEnd();
		Graphics()->ShaderEnd();
	}

	RenderSparks();

	if (Graphics()->UseShader() && g_Config.m_PdlBackgroundRays)
	{
		float aVar[4];
		Graphics()->ShaderBegin(IGraphics::SHADER_RAYS);

		aVar[0] = Graphics()->ScreenWidth();
		aVar[1] = Graphics()->ScreenHeight();
		Graphics()->ShaderUniformSet("u_Resolution", aVar, 2);

		static float s_Move;
		s_Move += 0.005f;
		aVar[0] = s_Move;
		Graphics()->ShaderUniformSet("u_Time", aVar, 1);

		Graphics()->TextureSet(-1);
		Graphics()->QuadsBegin();
		Graphics()->SetColor(0.0f, 0.0f, 0.0f, 1.0f);
		Graphics()->QuadsDrawTL(&IGraphics::CQuadItem(0, 0, sw, sh), 1);
		Graphics()->QuadsEnd();

		Graphics()->ShaderEnd();
	}
	else
		RenderSunrays();

	// restore screen
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);
}

void CMenus::RenderBackgroundStandart()
{
	float sw = 300*Graphics()->ScreenAspect();
	float sh = 300;
	Graphics()->MapScreen(0, 0, sw, sh);

	// render background color
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
		//vec4 bottom(gui_color.r*0.3f, gui_color.g*0.3f, gui_color.b*0.3f, 1.0f);
		//vec4 bottom(0, 0, 0, 1.0f);
		vec4 Bottom(ms_GuiColor.r, ms_GuiColor.g, ms_GuiColor.b, 1.0f);
		vec4 Top(ms_GuiColor.r, ms_GuiColor.g, ms_GuiColor.b, 1.0f);
		IGraphics::CColorVertex Array[4] = {
			IGraphics::CColorVertex(0, Top.r, Top.g, Top.b, Top.a),
			IGraphics::CColorVertex(1, Top.r, Top.g, Top.b, Top.a),
			IGraphics::CColorVertex(2, Bottom.r, Bottom.g, Bottom.b, Bottom.a),
			IGraphics::CColorVertex(3, Bottom.r, Bottom.g, Bottom.b, Bottom.a)};
		Graphics()->SetColorVertex(Array, 4);
		IGraphics::CQuadItem QuadItem(0, 0, sw, sh);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// render the tiles
	Graphics()->TextureSet(-1);
	Graphics()->QuadsBegin();
		float Size = 15.0f;
		float OffsetTime = fmodulu(Client()->LocalTime()*0.15f, 2.0f);
		for(int y = -2; y < (int)(sw/Size); y++)
			for(int x = -2; x < (int)(sh/Size); x++)
			{
				Graphics()->SetColor(0,0,0,0.045f);
				IGraphics::CQuadItem QuadItem((x-OffsetTime)*Size*2+(y&1)*Size, (y+OffsetTime)*Size, Size, Size);
				Graphics()->QuadsDrawTL(&QuadItem, 1);
			}
	Graphics()->QuadsEnd();

	// render border fade
	Graphics()->TextureSet(gs_TextureBlob);
	Graphics()->QuadsBegin();
		Graphics()->SetColor(0,0,0,0.5f);
		QuadItem = IGraphics::CQuadItem(-100, -100, sw+200, sh+200);
		Graphics()->QuadsDrawTL(&QuadItem, 1);
	Graphics()->QuadsEnd();

	// restore screen
	{CUIRect Screen = *UI()->Screen();
	Graphics()->MapScreen(Screen.x, Screen.y, Screen.w, Screen.h);}
}

CMenus::CBackTile::CBackTile(CMenus *pMenu, int LifeTime, vec2 Pos, vec2 Dir, float Speed, int Width, int Height, vec3 Color)
{
	m_pMenu = pMenu;
	m_LifeTime = LifeTime;
	m_Pos = Pos;
	m_Dir = Dir;
	m_Speed = Speed;
	m_Width = Width;
	m_Height = Height;
	m_Alpha = 0;
	m_MoveTime = 0;
	m_Color = Color;
}

void CMenus::CBackTile::Render()
{
	CUIRect Screen = *m_pMenu->UI()->Screen();
	m_pMenu->Graphics()->SetColor(m_Color.r, m_Color.g, m_Color.b, m_Alpha);
	//m_pMenu->Graphics()->SetColor(Rgb.r, Rgb.g, Rgb.b, m_Alpha);
	int MouseOffset = 1;
	vec2 MouseFromMidFac = vec2(m_pMenu->UI()->MouseX()/Screen.w*2.0f-1.0f, m_pMenu->UI()->MouseY()/Screen.h*2.0f-1.0f);
	IGraphics::CQuadItem QuadItem = IGraphics::CQuadItem(m_Pos.x+MouseFromMidFac.x*MouseOffset, m_Pos.y+MouseFromMidFac.y*MouseOffset, m_Width, m_Height);
	m_pMenu->Graphics()->QuadsDraw(&QuadItem, 1);
}

bool CMenus::CBackTile::Tick()
{
	if(m_LifeTime)
	{
		Render();

		if(m_MoveTime < time_get())
		{
			//m_Dir.y += 0.03f;

			{//Apprall in englisch(bounce)
				if(m_Pos.x >= 300*m_pMenu->Graphics()->ScreenAspect() || m_Pos.x <= 0)
					m_Dir.x *= -1;

				if(m_Pos.y >= 300 || m_Pos.y <= 0)
					m_Dir.y *= -1;
			}

			m_Pos += m_Dir * m_Speed;

			if(m_LifeTime <= 40)
				m_Alpha = clamp(m_Alpha-0.01f, 0.0f, 1.0f);
			else if(m_Alpha < 0.4f)
				m_Alpha += 0.01f;

			m_LifeTime--;

			m_MoveTime = time_get()+time_freq()*0.01f;
		}

		return true;
	}

	return false;
}

void CMenus::RenderSparks()
{
	static const int Num = 280;
	static CBackTile *pSparks[Num];

	Graphics()->TextureSet(gs_TextureSpark);
	Graphics()->QuadsBegin();

	for(int i = 0; i < Num; i++)
	{
		if(pSparks[i])
		{
			if (!pSparks[i]->Tick())
			{
				delete pSparks[i];
				pSparks[i] = 0x0;
			}
		}
		else
		{
			/*float mx = (m_MousePos.x/(float)Graphics()->ScreenWidth())*300*Graphics()->ScreenAspect();
			float my = (m_MousePos.y/(float)Graphics()->ScreenHeight())*300;*/

			int LifeTime = (random()%500)+500;//20 <-> 69
			//vec2 Pos = vec2(mx, my);//vec2(0, 310);
			vec2 Pos = vec2(random()%(int)(300*Graphics()->ScreenAspect()), random()%300);
			vec2 Dir = vec2((random()%21/10.0f)-1.0f, (random()%21/10.0f)-1.0f);
			float Speed = (random()%2)/10.0f+0.01f;//1.1 <-> 4.1
			int Size = (random()%6)+2;//8 <-> 15

			vec3 Color;
			switch(random()%4)
			{
				case 0: Color = vec3(164/255.0f, 238/255.0f, 219/255.0f); break;
				case 1: Color = vec3(117/255.0f, 117/255.0f, 53/255.0f); break;
				case 2: Color = vec3(86/255.0f, 138/255.0f, 53/255.0f); break;
				case 3: Color = vec3(99/255.0f, 178/255.0f, 112/255.0f); break;
			}

			pSparks[i] = new CBackTile(this, LifeTime, Pos, Dir, Speed, Size, Size, Color);
		}
	}

	Graphics()->QuadsEnd();
	
}

void CMenus::RenderSunrays()
{
	static const int Num = 30;
	static CBackTile *pSunrays[Num];

	Graphics()->TextureSet(gs_TextureAurora);
	Graphics()->QuadsBegin();

	for(int i = 0; i < Num; i++)
	{
		if(pSunrays[i])
		{
			if(!pSunrays[i]->Tick())
				pSunrays[i] = 0x0;
		}
		else
		{
			int LifeTime = (random()%500)+500;//20 <-> 69
			//vec2 Pos = vec2(mx, my);//vec2(0, 310);
			vec2 Pos = vec2(random()%(int)(300*Graphics()->ScreenAspect()), random()%300);
			vec2 Dir = vec2(random()%2?-1:1, 0);
			float Speed = (random()%20)/700.0f+0.005f;//1.1 <-> 4.1
			//int Size = (random()%6)+2;//8 <-> 15

			pSunrays[i] = new CBackTile(this, LifeTime, Pos, Dir, Speed, 32, 64, vec3(1.0f, 1.0f, 1.0f));
		}
	}

	Graphics()->QuadsEnd();
}