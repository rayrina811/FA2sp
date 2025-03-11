#include "CLuaConsole.h"
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/MultimapHelper.h"
#include "../Common.h"

#include <CLoading.h>
#include <CFinalSunDlg.h>
#include <CObjectDatas.h>
#include <CMapData.h>
#include <CIsoView.h>
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../../Ext/CMapData/Body.h"
#include "../CObjectSearch/CObjectSearch.h"

#include "../../Sol/sol.hpp"

namespace LuaFunctions
{
	static void writeLuaConsole(std::string text)
	{
		text = ">> " + text;
		int len = GetWindowTextLength(CLuaConsole::hOutputBox);
		SendMessage(CLuaConsole::hOutputBox, EM_SETSEL, len, len);

		SendMessage(CLuaConsole::hOutputBox, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
		SendMessage(CLuaConsole::hOutputBox, EM_SCROLLCARET, 0, 0);
	}

	static void clear()
	{
		SendMessage(CLuaConsole::hOutputBox, WM_SETTEXT, 0, (LPARAM)"");
	}

	static void luaPrint(sol::variadic_args args)
	{
		std::string output = "";
		for (auto arg : args) {
			switch (arg.get_type()) {
			case sol::type::nil:
				output += "nil ";
				break;
			case sol::type::string:
			case sol::type::number:
				output += arg.get<std::string>() + " ";
				break;
			case sol::type::boolean:
				output += (arg.as<bool>() ? "true " : "false ");
				break;
			default:
				output += "[unsupported type] ";
				break;
			}
		}
		output += "\r\n";
		writeLuaConsole(output);
	}

	static void placeTerrain(int y, int x, std::string id) 
	{
		int pos = CMapData::Instance->GetCoordIndex(x, y);
		if (CMapData::Instance->IsCoordInMap(x, y))
		{
			auto cell = CMapData::Instance->GetCellAt(pos);
			if (cell->Terrain > -1) {
				CMapData::Instance->DeleteTerrainData(cell->Terrain);
			}
			CMapData::Instance->SetTerrainData(id.c_str(), pos);
		}
	}

	static void redrawFA2Window() 
	{
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
	}
}