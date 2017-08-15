
#ifndef GAME_CLIENT_COMPONENTS_MENUS_H
#define GAME_CLIENT_COMPONENTS_MENUS_H

#include <base/vmath.h>
#include <base/tl/sorted_array.h>

#include <engine/demo.h>
#include <engine/friends.h>

#include <game/voting.h>
#include <game/client/component.h>
#include <game/client/ui.h>

// compnent to fetch keypresses, override all other input
class CMenusKeyBinder : public CComponent
{
public:
	bool m_TakeKey;
	bool m_GotKey;
	IInput::CEvent m_Key;
	CMenusKeyBinder();
	virtual bool OnInput(IInput::CEvent Event);
};

class CMenus : public CComponent, public CTextureUser
{
	static vec4 ms_GuiColor;
	static vec4 ms_ColorTabbarInactiveOutgame;
	static vec4 ms_ColorTabbarActiveOutgame;
	static vec4 ms_ColorTabbarInactiveIngame;
	static vec4 ms_ColorTabbarActiveIngame;
	static vec4 ms_ColorTabbarInactive;
	static vec4 ms_ColorTabbarActive;
	static vec4 ms_ColorTabButtonChecked;

	int ms_TextureButton;
	int ms_TextureCopy;


	vec4 GetButtonColor(const void *pID, int Checked);
	vec4 ButtonColorMul(const void *pID);

	int DoButton_DemoPlayer(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoButton_Sprite(const void *pID, int ImageID, int SpriteID, int Checked, const CUIRect *pRect, int Corners);
	int DoButton_Texture(const void *pID, int Texture, int Checked, const CUIRect *pRect);
	int DoButton_Toggle(const void *pID, int Checked, const CUIRect *pRect, bool Active);
	int DoButton_Menu(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoButton_MenuTab(const void *pID, const char *pText, int Checked, const CUIRect *pRect, int Corners);

	int DoButton_CheckBox_Common(const void *pID, const char *pText, const char *pBoxText, const CUIRect *pRect);
	int DoButton_CheckBox(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoButton_CheckBox_Number(const void *pID, const char *pText, int Checked, const CUIRect *pRect);

	/*static void ui_draw_menu_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_keyselect_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_menu_tab_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_settings_tab_button(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	*/

	int DoButton_Icon(int ImageId, int SpriteId, const CUIRect *pRect);
	int DoButton_GridHeader(const void *pID, const char *pText, int Checked, const CUIRect *pRect);

	//static void ui_draw_browse_icon(int what, const CUIRect *r);
	//static void ui_draw_grid_header(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);

	/*static void ui_draw_checkbox_common(const void *id, const char *text, const char *boxtext, const CUIRect *r, const void *extra);
	static void ui_draw_checkbox(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	static void ui_draw_checkbox_number(const void *id, const char *text, int checked, const CUIRect *r, const void *extra);
	*/
	int DoEditBox(void *pID, const CUIRect *pRect, char *pStr, unsigned StrSize, float FontSize, float *Offset, bool Hidden=false, int Corners=CUI::CORNER_ALL);
	//static int ui_do_edit_box(void *id, const CUIRect *rect, char *str, unsigned str_size, float font_size, bool hidden=false);

	float DoScrollbarV(const void *pID, const CUIRect *pRect, float Current);
	float DoScrollbarH(const void *pID, const CUIRect *pRect, float Current);
	void DoButton_KeySelect(const void *pID, const char *pText, int Checked, const CUIRect *pRect);
	int DoKeyReader(void *pID, const CUIRect *pRect, int Key);

	//static int ui_do_key_reader(void *id, const CUIRect *rect, int key);
	void UiDoGetButtons(int Start, int Stop, CUIRect View);

	struct CListboxItem
	{
		int m_Visible;
		int m_Selected;
		CUIRect m_Rect;
		CUIRect m_HitRect;
	};

	void UiDoListboxStart(const void *pID, const CUIRect *pRect, float RowHeight, const char *pTitle, const char *pBottomText, int NumItems,
						int ItemsPerRow, int SelectedIndex, float ScrollValue, float FontSize = 0.0f);
	CListboxItem UiDoListboxNextItem(const void *pID, bool Selected = false);
	CListboxItem UiDoListboxNextRow();
	int UiDoListboxEnd(float *pScrollValue, bool *pItemActivated);

	//static void demolist_listdir_callback(const char *name, int is_dir, void *user);
	//static void demolist_list_callback(const CUIRect *rect, int index, void *user);

	//bool Grow(float *pSrc, float To, float Speed);

	enum
	{
		POPUP_NONE=0,
		POPUP_FIRST_LAUNCH,
		POPUP_CONNECTING,
		POPUP_MESSAGE,
		POPUP_DISCONNECTED,
		POPUP_PURE,
		POPUP_LANGUAGE,
		POPUP_COUNTRY,
		POPUP_DELETE_DEMO,
		POPUP_RENAME_DEMO,
		POPUP_REMOVE_FRIEND,
		POPUP_SOUNDERROR,
		POPUP_PASSWORD,
		POPUP_QUIT,
	};

	enum
	{
		PAGE_PADDEL,
		PAGE_SELECT,
		PAGE_GAME,
		PAGE_PLAYERS,
		PAGE_SERVER_INFO,
		PAGE_CALLVOTE,
		PAGE_INTERNET,
		PAGE_LAN,
		PAGE_FAVORITES,
		PAGE_DEMOS,
		PAGE_EXTRAS,
		PAGE_SETTINGS,
		PAGE_SYSTEM,
	};

	int m_GamePage;
	int m_Popup;
	int m_ActivePage;
	bool m_MenuActive;
	bool m_UseMouseButtons;
	vec2 m_MousePos;

	int64 m_LastInput;

	int m_QuickActive;

	// loading
	int m_LoadCurrent;
	int m_LoadTotal;

	//
	char m_aMessageTopic[512];
	char m_aMessageBody[512];
	char m_aMessageButton[512];

	void PopupMessage(const char *pTopic, const char *pBody, const char *pButton);

	// TODO: this is a bit ugly but.. well.. yeah
	enum { MAX_INPUTEVENTS = 32 };
	static IInput::CEvent m_aInputEvents[MAX_INPUTEVENTS];
	static int m_NumInputEvents;

	// some settings
	static float ms_ButtonHeight;
	static float ms_ListheaderHeight;
	static float ms_FontmodHeight;

	// for settings
	bool m_NeedRestartGraphics;
	bool m_NeedRestartSound;
	bool m_NeedSendinfo;
	int m_SettingPlayerPage;

	//
	bool m_EscapePressed;
	bool m_EnterPressed;
	bool m_DeletePressed;

	// for map download popup
	int64 m_DownloadLastCheckTime;
	int m_DownloadLastCheckSize;
	float m_DownloadSpeed;

	// for call vote
	int m_CallvoteSelectedOption;
	int m_CallvoteSelectedPlayer;
	char m_aCallvoteReason[VOTE_REASON_LENGTH];

	// demo
	struct CDemoItem
	{
		char m_aFilename[128];
		char m_aName[128];
		bool m_IsDir;
		int m_StorageType;

		bool m_InfosLoaded;
		bool m_Valid;
		CDemoHeader m_Info;

		bool operator<(const CDemoItem &Other) { return !str_comp(m_aFilename, "..") ? true : !str_comp(Other.m_aFilename, "..") ? false :
														m_IsDir && !Other.m_IsDir ? true : !m_IsDir && Other.m_IsDir ? false :
														str_comp_filenames(m_aFilename, Other.m_aFilename) < 0; }
	};

	sorted_array<CDemoItem> m_lDemos;
	char m_aCurrentDemoFolder[256];
	char m_aCurrentDemoFile[64];
	int m_DemolistSelectedIndex;
	bool m_DemolistSelectedIsDir;
	int m_DemolistStorageType;

	void DemolistOnUpdate(bool Reset);
	void DemolistPopulate();
	static int DemolistFetchCallback(const char *pName, int IsDir, int StorageType, void *pUser);

	enum
	{
		QUICKITEM_COMMAND = 0,
		QUICKITEM_VARINT,
		QUICKITEM_VARSTR,
	};

	struct CQuickItem
	{
		char m_aScriptName[64];
		char m_aName[64];
		char m_aHelp[256];
		int m_QuickItemType;
	};

	struct CQuickItemVariableStr : CQuickItem
	{
		int m_Length;
		char *m_pDefault;
		char *m_pValue;
		~CQuickItemVariableStr() { delete m_pDefault; };
	};

	struct CQuickItemVariableInt : CQuickItem
	{
		int m_Default;
		int m_Mininum;
		int m_Maximum;
		int *m_pValue;
	};

	struct CQuickItemCommand : CQuickItem
	{
		char m_aValue[256];
	};

	array<CQuickItem *> m_lQuickItem;

	static void AddQuickUtemVariableInt(char *pName, char *pScriptName, int &Value, char *pDefault, char *pMin, char *pMax, char *pFlags, char *pDesc, void *pData);
	static void AddQuickItemVariableStr(char *pName, char *pScriptName, char *pValue, char *pLength, char *pDefault, char *pFlags, char *pDesc, void *pData);
	static void ConAddQuickitem(IConsole::IResult *pResult, void *pUserData);
	static void ConRemoveQuickitem(IConsole::IResult *pResult, void *pUserData);
	static void ConListQuickitem(IConsole::IResult *pResult, void *pUserData);
	static void ConfigSaveCallback(class IConfig *pConfig, void *pUserData);

	// friends
	struct CFriendItem
	{
		const CFriendInfo *m_pFriendInfo;
		int m_NumFound;

		bool operator<(const CFriendItem &Other)
		{
			if(m_NumFound && !Other.m_NumFound)
				return true;
			else if(!m_NumFound && Other.m_NumFound)
				return false;
			else
			{
				int Result = str_comp(m_pFriendInfo->m_aName, Other.m_pFriendInfo->m_aName);
				if(Result)
					return Result < 0;
				else
					return str_comp(m_pFriendInfo->m_aClan, Other.m_pFriendInfo->m_aClan) < 0;
			}
		}
	};

	sorted_array<CFriendItem> m_lFriends;
	int m_FriendlistSelectedIndex;

	void FriendlistOnUpdate();

	// found in menus.cpp
	int Render();
	//void render_background();
	//void render_loading(float percent);
	int RenderMenubar(CUIRect r);

	// found in menus_demo.cpp
	void RenderDemoPlayer(CUIRect MainView);
	void RenderDemoList(CUIRect MainView);

	// found in menus_ingame.cpp
	void RenderGame(CUIRect MainView);
	void RenderPlayers(CUIRect MainView);
	void RenderServerInfo(CUIRect MainView);
	void RenderServerControl(CUIRect MainView);
	void RenderServerControlKick(CUIRect MainView, bool FilterSpectators);
	void RenderServerControlServer(CUIRect MainView);

	// found in menus_browser.cpp
	int m_SelectedIndex;
	int m_ScrollOffset;
	void RenderServerbrowserServerList(CUIRect View);
	void RenderServerbrowserServerDetail(CUIRect View);
	void RenderServerbrowserFilters(CUIRect View);
	void RenderServerbrowserFriends(CUIRect View);
	void RenderServerbrowser(CUIRect MainView);
	static void ConchainFriendlistUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);
	static void ConchainServerbrowserUpdate(IConsole::IResult *pResult, void *pUserData, IConsole::FCommandCallback pfnCallback, void *pCallbackUserData);

	// found in menus_settings.cpp
	void RenderLanguageSelection(CUIRect MainView);
	void RenderSettingsGeneral(CUIRect MainView);
	void RenderSettingsPlayer(CUIRect MainView);
	void RenderSettingsTee(CUIRect MainView);
	void RenderSettingsControls(CUIRect MainView);
	void RenderSettingsGraphics(CUIRect MainView);
	void RenderSettingsSound(CUIRect MainView);
	void RenderSettings(CUIRect MainView);

	static void RenderExtrasVariableInt(char *pName, char *pScriptName, int &Value, char *pDefault, char *pMin, char *pMax, char *pFlags, char *pDesc, void *pData);
	static void RenderExtrasVariableStr(char *pName, char *pScriptName, char *pValue, char *pLength, char *pDefault, char *pFlags, char *pDesc, void *pData);
	void RenderVariable(int index, char *pResult, int pResultSize, void *pData);
	void RenderExtrasGeneral(CUIRect MainView);
	static void GetMenuIdentityResult(int Index, char *pResult, int pResultSize, void *pData, int Row, int MaxRows);
	void RenderExtrasIdentities(CUIRect MainView);
	void RenderExtrasDummies(CUIRect MainView);
	void RenderExtrasMapinker(CUIRect MainView);
	void RenderExtras(CUIRect MainView);

	void SetActive(bool Active);
public:
	void RenderBackground();
	void RenderBackgroundStandart();
	void RenderBackgroundPaddel();
	void RenderSparks();
	void RenderSunrays();

	void InitMainButtons();
	void DoMainButtons(bool Init = false);

	enum
	{
		MAINBUTTON_PLAY=0,
		MAINBUTTON_EXTRAS,
		MAINBUTTON_DEMOS,
		MAINBUTTON_EDITOR,
		MAINBUTTON_SETTINGS,
		MAINBUTTON_QUIT,
		NUM_MAINBUTTONS,
	};
	struct
	{
		vec2 m_Pos;
		vec2 m_WantedPos;
		float m_Scale;
		int m_ButtonID;
	}m_aMainButtons[NUM_MAINBUTTONS];

	void InitPaddel();
	void DoPaddel();
	vec2 m_PaddelPos;
	vec2 m_PaddelWantedPos;
	int m_PaddelState;
	float m_PaddelSize;
	float m_PaddelWantedSize;
	float m_PaddelRotation;
	float m_PaddelAlpha;
	bool m_PaddelVisible;
	int64 m_AnimationTime;
	int m_AnimationState;

	int m_LastMouseAction;

	class CBackTile
	{
		private:
		CMenus *m_pMenu;
		int m_LifeTime;
		vec2 m_Pos;
		vec2 m_Dir;
		float m_Speed;
		int m_Width;
		int m_Height;
		vec3 m_Color;
		float m_Alpha;
		int64 m_MoveTime;

		void Render();
		
	public:
		CBackTile(CMenus *pMenu, int LifeTime, vec2 Pos, vec2 Dir, float Speed, int Width, int Height, vec3 Color);

		bool Tick();
	};

	bool m_QuickMenuLock;
	void RenderQuickMenu(CUIRect MainView);
	void RenderQuickMenuItems(CUIRect MainView);
	bool QuickMenuLockInput() { return m_QuickMenuLock; }

	void UseMouseButtons(bool Use) { m_UseMouseButtons = Use; }

	static CMenusKeyBinder m_Binder;

	CMenus();

	void RenderLoading();

	bool IsActive() const { return m_MenuActive; }

	virtual void OnInit();
	virtual void InitTextures();

	void JoystickInput(int *pButtons);

	virtual void OnStateChange(int NewState, int OldState);
	virtual void OnReset();
	virtual void OnRender();
	virtual bool OnInput(IInput::CEvent Event);
	virtual bool OnMouseMove(float x, float y);
	virtual void OnConsoleInit();
};
#endif
