
#include <base/string_seperation.h>

#include <engine/config.h>
#include <engine/textrender.h>
#include <engine/shared/config.h>

#include <game/client/components/controls.h>

#include "menus.h"

static int gs_VariableIDs[1024] = {};
static float gs_VariableOffset[1024] = {};
static int gs_StartItem = 0;

void CMenus::RenderQuickMenu(CUIRect MainView)
{
	CUIRect Top, Bottom;
	MainView.HSplitTop(100.0f, &Top, 0x0);
	MainView.HSplitBottom(100.0f, 0x0, &Bottom);

	gs_StartItem = 0;
	RenderQuickMenuItems(Top);
	RenderQuickMenuItems(Bottom);

	if(UI()->MouseInside(&Top) || UI()->MouseInside(&Bottom))
		m_QuickMenuLock = true;
	else
		m_QuickMenuLock = false;
}

void CMenus::RenderQuickMenuItems(CUIRect MainView)
{
	CUIRect VariableButton;

	RenderTools()->DrawUIRect(&MainView, vec4(0, 0, 0, 0.5f), 0, 0.0f);

	MainView.HSplitTop(MainView.h/3.0f, &VariableButton, 0x0);
	VariableButton.VSplitLeft(160.0f, &VariableButton, 0x0);

	char aBuf[256];
	bool ResetLine = false;
	for (int i = gs_StartItem; i < m_lQuickItem.size(); i++)
	{
		CQuickItem *pQuickItem = m_lQuickItem[i];

		if (ResetLine)
		{
			VariableButton.y += VariableButton.h;
			VariableButton.x = MainView.x;
			ResetLine = false;
			gs_StartItem--;
			if (VariableButton.y + VariableButton.h * 0.5f > MainView.h)
				break; // window filled
		}
		gs_StartItem++;

		if (pQuickItem->m_QuickItemType == QUICKITEM_VARINT)
		{
			CQuickItemVariableInt *pQuickItemVariableInt = (CQuickItemVariableInt *)pQuickItem;

			if (pQuickItemVariableInt->m_Mininum == 0 && pQuickItemVariableInt->m_Maximum == 1)
			{
				CUIRect CheckBox = VariableButton;
				CheckBox.h *= 0.5f;
				CheckBox.y += CheckBox.h  *0.5f;
				float wt = TextRender()->TextWidth(0, CheckBox.h * 0.75f, pQuickItemVariableInt->m_aName, -1);

				if (VariableButton.x + wt + 16.0f >= MainView.w)
				{
					ResetLine = true;
					i--;
					continue;
				}

				if (DoButton_CheckBox(&gs_VariableIDs[i], pQuickItemVariableInt->m_aName, *pQuickItemVariableInt->m_pValue, &CheckBox))
					*pQuickItemVariableInt->m_pValue = !*pQuickItemVariableInt->m_pValue;

				VariableButton.x += wt + 16.0f;
			}
			else
			{
				if (VariableButton.x + VariableButton.w * 2.0f >= MainView.w)
				{
					//break;
					ResetLine = true;
					i--;
					continue;
				}

				CUIRect Slider = VariableButton;

				Slider.h *= 0.5f;
				str_format(aBuf, sizeof(aBuf), "%s(%i)", pQuickItemVariableInt->m_aName, *pQuickItemVariableInt->m_pValue);
				UI()->DoLabelScaled(&Slider, aBuf, 12.f, 0, -1);
				Slider.y += Slider.h;

				CUIRect TextRect;
				Slider.VSplitRight(36.0f, &Slider, &TextRect);
				float Span = pQuickItemVariableInt->m_Maximum - pQuickItemVariableInt->m_Mininum;
				*pQuickItemVariableInt->m_pValue = DoScrollbarH(&gs_VariableIDs[i], &Slider, (*pQuickItemVariableInt->m_pValue - pQuickItemVariableInt->m_Mininum) / Span) * Span + pQuickItemVariableInt->m_Mininum;
				*pQuickItemVariableInt->m_pValue = clamp(*pQuickItemVariableInt->m_pValue, pQuickItemVariableInt->m_Mininum, pQuickItemVariableInt->m_Maximum);

				VariableButton.x += Slider.w + 8.0f;
			}
		}
		else if (pQuickItem->m_QuickItemType == QUICKITEM_VARSTR)
		{
			CQuickItemVariableStr *pQuickItemVariableStr = (CQuickItemVariableStr *)pQuickItem;

			if (VariableButton.x + VariableButton.w >= MainView.w)
			{
				//break;
				ResetLine = true;
				i--;
				continue;
			}

			CUIRect TextBox = VariableButton;
			TextBox.h *= 0.5f;
			TextBox.y += TextBox.h * 0.5f;
			UI()->DoLabel(&TextBox, pQuickItemVariableStr->m_aName, 8.0f, -1);
			float TextSize = TextRender()->TextWidth(0, 8.0f, pQuickItemVariableStr->m_aName, -1);
			TextBox.x += TextSize + 8.0f;
			TextBox.w -= TextSize + 8.0f;
			
			DoEditBox(pQuickItemVariableStr->m_pValue, &TextBox, pQuickItemVariableStr->m_pValue, pQuickItemVariableStr->m_Length, 14.0f, &gs_VariableOffset[i]);

			VariableButton.x += VariableButton.w + 8.0f;
		}
		else if (pQuickItem->m_QuickItemType == QUICKITEM_COMMAND)
		{
			CQuickItemCommand *pQuickItemVariableCommand = (CQuickItemCommand *)pQuickItem;

			if (VariableButton.x + VariableButton.w >= MainView.w)
			{
				//break;
				ResetLine = true;
				i--;
				continue;
			}

			char *pName = pQuickItemVariableCommand->m_aName;
			if (str_comp_num(pName, "pdl", 3) == 0)
				pName += 4;

			CUIRect Button = VariableButton;
			Button.h *= 0.5f;
			Button.y += Button.h * 0.5f;
			if (DoButton_Menu(&gs_VariableOffset[i], Localize(pName), 0, &Button))
				Console()->ExecuteLine(pQuickItemVariableCommand->m_aValue);

			VariableButton.x += VariableButton.w + 8.0f;
		}
	}
}

static const char *gs_pCommandName = 0x0;
static bool gs_VariableFound = false;

void CMenus::AddQuickUtemVariableInt(char *pName, char *pScriptName, int &Value, char *pDefault, char *pMin, char *pMax, char *pFlags, char *pDesc, void *pData)
{
	CMenus *pThis = (CMenus *)pData;
	if (str_comp(pScriptName, gs_pCommandName) != 0 || gs_VariableFound == true)
		return;

	CQuickItemVariableInt *pQuickItem = new CQuickItemVariableInt();
	pQuickItem->m_Default = str_toint(pDefault);
	pQuickItem->m_Mininum = str_toint(pMin);
	pQuickItem->m_Maximum = str_toint(pMax);
	pQuickItem->m_pValue = &Value;

	str_copy(pQuickItem->m_aScriptName, pScriptName, sizeof(pQuickItem->m_aScriptName));
	str_copy(pQuickItem->m_aName, pName, sizeof(pQuickItem->m_aName));
	str_copy(pQuickItem->m_aHelp, pDesc, sizeof(pQuickItem->m_aHelp));
	pQuickItem->m_QuickItemType = QUICKITEM_VARINT;

	pThis->m_lQuickItem.add(pQuickItem);
	gs_VariableFound = true;
}

void CMenus::AddQuickItemVariableStr(char *pName, char *pScriptName, char *pValue, char *pLength, char *pDefault, char *pFlags, char *pDesc, void *pData)
{
	CMenus *pThis = (CMenus *)pData;
	if (str_comp(pScriptName, gs_pCommandName) != 0 || gs_VariableFound == true)
		return;

	CQuickItemVariableStr *pQuickItem = new CQuickItemVariableStr();
	pQuickItem->m_Length = str_toint(pLength);
	pQuickItem->m_pDefault = new char[pQuickItem->m_Length + 1];
	str_copy(pQuickItem->m_pDefault, pDefault, pQuickItem->m_Length);
	pQuickItem->m_pValue = pValue;

	str_copy(pQuickItem->m_aScriptName, pScriptName, sizeof(pQuickItem->m_aScriptName));
	str_copy(pQuickItem->m_aName, pName, sizeof(pQuickItem->m_aName));
	str_copy(pQuickItem->m_aHelp, pDesc, sizeof(pQuickItem->m_aHelp));
	pQuickItem->m_QuickItemType = QUICKITEM_VARSTR;

	pThis->m_lQuickItem.add(pQuickItem);
	gs_VariableFound = true;
}

void CMenus::ConAddQuickitem(IConsole::IResult *pResult, void *pUserData)
{
	char aBuf[128];
	CMenus *pThis = (CMenus *)pUserData;
	const char *pCommand = pResult->GetString(0);
	IConfig *pConfig = pThis->Kernel()->RequestInterface<IConfig>();

	if (pThis->Console()->LineIsValid(pCommand) == false)
	{
		char aBuf[256];
		str_format(aBuf, sizeof(aBuf), "'%s' is not a valid command line", pCommand);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_ERROR, "console", aBuf);
		return;
	}

	gs_pCommandName = pCommand;
	gs_VariableFound = false;

	pConfig->GetAllConfigVariables(&AddQuickUtemVariableInt, &AddQuickItemVariableStr, pThis);

	if (gs_VariableFound == false)
	{
		CQuickItemCommand *pQuickItem = new CQuickItemCommand();
		str_copy(pQuickItem->m_aValue, pCommand, sizeof(pQuickItem->m_aValue));

		str_copy(pQuickItem->m_aScriptName, gs_pCommandName, sizeof(pQuickItem->m_aScriptName));
		str_copy(pQuickItem->m_aName, gs_pCommandName, sizeof(pQuickItem->m_aName));
		str_copy(pQuickItem->m_aHelp, pCommand, sizeof(pQuickItem->m_aHelp));
		pQuickItem->m_QuickItemType = QUICKITEM_COMMAND;

		pThis->m_lQuickItem.add(pQuickItem);
	}

	if (pThis->m_lQuickItem.size() > 0)
	{
		CQuickItem *pQuickItem = pThis->m_lQuickItem[pThis->m_lQuickItem.size() - 1];
		str_format(aBuf, sizeof(aBuf), "'%s' added to quickitems ", pCommand);
		if (pQuickItem->m_QuickItemType == QUICKITEM_COMMAND)
			str_append(aBuf, "a command", sizeof(aBuf));
		else
			str_append(aBuf, "a variable", sizeof(aBuf));

		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "console", aBuf);
	}
}

void CMenus::ConRemoveQuickitem(IConsole::IResult *pResult, void *pUserData)
{
	CMenus *pThis = (CMenus *)pUserData;
	int Index = pResult->GetInteger(0);

	if (pThis->m_lQuickItem.size() == 0)
	{
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "console", "Quickitem list empty");
		return;
	}

	if (Index < 0 || Index >= pThis->m_lQuickItem.size())
	{
		char aBuf[32];
		str_format(aBuf, sizeof(aBuf), "%i is not a valid Index", Index);
		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "console", aBuf);
		ConListQuickitem(0x0, pThis);
		return;
	}

	CQuickItem *pQuickItem = pThis->m_lQuickItem[Index];
	pThis->m_lQuickItem.remove_index(Index);
	delete pQuickItem;
}

void CMenus::ConListQuickitem(IConsole::IResult *pResult, void *pUserData)
{
	CMenus *pThis = (CMenus *)pUserData;
	char aBuf[256];

	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "console", "--- List of Quickitems ---");

	for (int i = 0; i < pThis->m_lQuickItem.size(); i++)
	{
		str_format(aBuf, sizeof(aBuf), "[%i]: ", i);

		if (pThis->m_lQuickItem[i]->m_QuickItemType == QUICKITEM_COMMAND)
			str_append(aBuf, ((CQuickItemCommand *)pThis->m_lQuickItem[i])->m_aValue, sizeof(aBuf));
		else
			str_append(aBuf, pThis->m_lQuickItem[i]->m_aScriptName, sizeof(aBuf));

		pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "console", aBuf);
	}

	pThis->Console()->Print(IConsole::OUTPUT_LEVEL_STANDARD, IConsole::OUTPUTTYPE_STANDARD, "console", "------------------------------");
}

void CMenus::ConfigSaveCallback(IConfig *pConfig, void *pUserData)
{
	CMenus *pThis = (CMenus *)pUserData;
	char aBuf[256];

	for (int i = 0; i < pThis->m_lQuickItem.size(); i++)
	{
		if(pThis->m_lQuickItem[i]->m_QuickItemType == QUICKITEM_COMMAND)
			str_format(aBuf, sizeof(aBuf), "add_quickitem %s", ((CQuickItemCommand *)pThis->m_lQuickItem[i])->m_aValue);
		else
			str_format(aBuf, sizeof(aBuf), "add_quickitem %s", pThis->m_lQuickItem[i]->m_aScriptName);
		pConfig->WriteLine(aBuf);
	}
}
