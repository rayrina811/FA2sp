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
#include <cctype>
#include <chrono>
#include "../../Miscs/MultiSelection.h"
#include <unordered_set>

namespace LuaFunctions
{
	static long long time = 0;
	static std::map<ppmfc::CString, CINI*> LoadedINIs;
	static std::unordered_set<std::string> UsedINIIndices;

	static void write_lua_console(std::string text)
	{
		text = ">> " + text + "\r\n";;
		int len = GetWindowTextLength(CLuaConsole::hOutputBox);
		SendMessage(CLuaConsole::hOutputBox, EM_SETSEL, len, len);

		SendMessage(CLuaConsole::hOutputBox, EM_REPLACESEL, FALSE, (LPARAM)text.c_str());
		SendMessage(CLuaConsole::hOutputBox, EM_SCROLLCARET, 0, 0);

		auto&& now = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if (now - time > 2000)
		{
			time = now;
			MSG msg;
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	static void clear()
	{
		SendMessage(CLuaConsole::hOutputBox, WM_SETTEXT, 0, (LPARAM)"");
	}

	static void lua_print(sol::variadic_args args)
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
		write_lua_console(output);
	}

	static void avoid_time_out()
	{
		auto&& now = duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		if (now - time > 2000)
		{
			time = now;
			MSG msg;
			while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	}

	static void sleep(int ms)
	{
		Sleep(ms);
	}

	static int message_box(std::string text, std::string title, int format) {
		static const std::unordered_map<int, UINT> formatMap = {
			{0, MB_OK},                       // OK button
			{1, MB_OKCANCEL},                 // OK/Cancel buttons
			{2, MB_YESNO},                    // Yes/No buttons
			{3, MB_YESNOCANCEL},              // Yes/No/Cancel buttons
			{4, MB_RETRYCANCEL},              // Retry/Cancel buttons
			{5, MB_ABORTRETRYIGNORE},         // Abort/Retry/Ignore buttons
			{6, MB_CANCELTRYCONTINUE},        // Cancel/Try Again/Continue buttons

			{10, MB_OK | MB_ICONINFORMATION}, // OK button with Information icon
			{11, MB_OK | MB_ICONWARNING},     // OK button with Warning icon
			{12, MB_OK | MB_ICONERROR},       // OK button with Error icon
			{13, MB_OK | MB_ICONQUESTION},    // OK button with Question icon

			{14, MB_YESNO | MB_ICONQUESTION}, // Yes/No buttons with Question icon
			{15, MB_RETRYCANCEL | MB_ICONERROR}, // Retry/Cancel buttons with Error icon
			{16, MB_YESNO | MB_ICONWARNING},  // Yes/No buttons with Warning icon
			{17, MB_ABORTRETRYIGNORE | MB_ICONERROR} // Abort/Retry/Ignore buttons with Error icon
		};

		auto it = formatMap.find(format);
		UINT style = (it != formatMap.end()) ? it->second : MB_OK;
		return MessageBoxA(CFinalSunDlg::Instance->GetSafeHwnd(), text.c_str(), title.c_str(), style);
	}

	struct TimePoint {
		int year, month, day, hour, minute, second;
		std::chrono::system_clock::time_point time;
	};

	static TimePoint getCurrentTime() {
		auto now = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);
		std::tm local_time = *std::localtime(&now_c);

		return TimePoint{
			local_time.tm_year + 1900,
			local_time.tm_mon + 1,
			local_time.tm_mday,
			local_time.tm_hour,
			local_time.tm_min,
			local_time.tm_sec,
			now
		};
	}

	static std::string formatTime(const TimePoint& time) {
		std::ostringstream oss;
		oss << std::setfill('0')
			<< time.year << "-"
			<< std::setw(2) << time.month << "-"
			<< std::setw(2) << time.day << " "
			<< std::setw(2) << time.hour << ":"
			<< std::setw(2) << time.minute << ":"
			<< std::setw(2) << time.second;
		return oss.str();
	}

	static std::string formatTimeInterval(std::chrono::seconds interval) {
		int hours = interval.count() / 3600;
		int minutes = (interval.count() % 3600) / 60;
		int seconds = interval.count() % 60;

		std::ostringstream oss;
		if (hours > 0) {
			oss << hours << "h ";
		}
		oss << minutes << "m " << seconds << "s";
		return oss.str();
	}

	static std::vector<std::string> split_string(std::string s, std::string delimiter = ",") {
		char deli = delimiter[0];
		std::vector<std::string> tokens;
		std::stringstream ss(s);
		std::string token;
		while (getline(ss, token, deli)) {
			tokens.push_back(token);
		}
		return tokens;
	}

	class infantry
	{
	public:
		infantry(std::string house, std::string type, int y, int x)
			: House(house), TypeID(type), X(x), Y(y) 
		{
			Health = "256";
			Status = "Guard";
			Tag = "None";
			Facing = "64";
			VeterancyPercentage = "0";
			Group = "-1";
			IsAboveGround = "0";
			AutoNORecruitType = "0";
			AutoYESRecruitType = "0";
			SubCell = "-1";
		}
		std::string House;
		std::string TypeID;
		std::string Health;
		int Y;
		int X;
		std::string SubCell;
		std::string Status;
		std::string Facing;
		std::string Tag;
		std::string VeterancyPercentage;
		std::string Group;
		std::string IsAboveGround;
		std::string AutoNORecruitType;
		std::string AutoYESRecruitType;
		bool operator==(const infantry& another)
		{
			return House == another.House &&
				TypeID == another.TypeID &&
				Health == another.Health &&
				X == another.X &&
				Y == another.Y &&
				SubCell == another.SubCell &&
				Status == another.Status &&
				Facing == another.Facing &&
				Tag == another.Tag &&
				VeterancyPercentage == another.VeterancyPercentage &&
				Group == another.Group &&
				IsAboveGround == another.IsAboveGround &&
				AutoNORecruitType == another.AutoNORecruitType &&
				AutoYESRecruitType == another.AutoYESRecruitType;
		};
		void place() const
		{
			if (!CMapData::Instance->IsCoordInMap(X, Y))
				return;
			CInfantryData inf;
			inf.House = this->House.c_str();
			inf.TypeID = this->TypeID.c_str();
			inf.Health = this->Health.c_str();
			inf.Y.Format("%d", this->Y);
			inf.X.Format("%d", this->X);
			inf.SubCell = this->SubCell.c_str();
			inf.Status = this->Status.c_str();
			inf.Facing = this->Facing.c_str();
			inf.Tag = this->Tag.c_str();
			inf.VeterancyPercentage = this->VeterancyPercentage.c_str();
			inf.Group = this->Group.c_str();
			inf.IsAboveGround = this->IsAboveGround.c_str();
			inf.AutoNORecruitType = this->AutoNORecruitType.c_str();
			inf.AutoYESRecruitType = this->AutoYESRecruitType.c_str();
			CMapData::Instance->SetInfantryData(&inf, nullptr, nullptr, 0, -1);
			CLuaConsole::needRedraw = true;
		}
		static infantry convert(CInfantryData& obj)
		{
			infantry ret{ "","" ,0 ,0 };
			ret.House = obj.House.m_pchData;
			ret.TypeID = obj.TypeID.m_pchData;
			ret.Health = obj.Health.m_pchData;
			ret.Y = atoi(obj.Y);
			ret.X = atoi(obj.X);
			ret.SubCell = obj.SubCell.m_pchData;
			ret.Status = obj.Status.m_pchData;
			ret.Facing = obj.Facing.m_pchData;
			ret.Tag = obj.Tag.m_pchData;
			ret.VeterancyPercentage = obj.VeterancyPercentage.m_pchData;
			ret.Group = obj.Group.m_pchData;
			ret.IsAboveGround = obj.IsAboveGround.m_pchData;
			ret.AutoNORecruitType = obj.AutoNORecruitType.m_pchData;
			ret.AutoYESRecruitType = obj.AutoYESRecruitType.m_pchData;
			return ret;
		}
		void remove() const
		{
			for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Infantry"); ++i)
			{
				CInfantryData obj;
				CMapData::Instance->GetInfantryData(i, obj);
				if (*this == convert(obj))
				{
					//CINI::CurrentDocument->DeleteKey("Infantry", CINI::CurrentDocument->GetKeyAt("Infantry", i));
					CMapData::Instance->DeleteInfantryData(i);
					CLuaConsole::needRedraw = true;
					CLuaConsole::updateInfantry = true;
					CLuaConsole::updateMinimap = true;
					break;
				}
			}
		}
	};
	
	class unit
	{
	public:
		unit(std::string house, std::string type, int y, int x)
			: House(house), TypeID(type), X(x), Y(y) 
		{
			Health = "256";
			Status = "Guard";
			Tag = "None";
			Facing = "64";
			VeterancyPercentage = "0";
			Group = "-1";
			IsAboveGround = "0";
			AutoNORecruitType = "0";
			AutoYESRecruitType = "0";
			FollowsIndex = "-1";
		}
		std::string House;
		std::string TypeID;
		std::string Health;
		int Y;
		int X;
		std::string FollowsIndex;
		std::string Status;
		std::string Facing;
		std::string Tag;
		std::string VeterancyPercentage;
		std::string Group;
		std::string IsAboveGround;
		std::string AutoNORecruitType;
		std::string AutoYESRecruitType;

		bool operator==(const unit& another)
		{
			return House == another.House &&
				TypeID == another.TypeID &&
				Health == another.Health &&
				X == another.X &&
				Y == another.Y &&
				FollowsIndex == another.FollowsIndex &&
				Status == another.Status &&
				Facing == another.Facing &&
				Tag == another.Tag &&
				VeterancyPercentage == another.VeterancyPercentage &&
				Group == another.Group &&
				IsAboveGround == another.IsAboveGround &&
				AutoNORecruitType == another.AutoNORecruitType &&
				AutoYESRecruitType == another.AutoYESRecruitType;
		};
		void place() const
		{
			if (!CMapData::Instance->IsCoordInMap(this->X, this->Y))
				return;
			CUnitData unit;
			unit.House = this->House.c_str();
			unit.TypeID = this->TypeID.c_str();
			unit.Health = this->Health.c_str();
			unit.Y.Format("%d", this->Y);
			unit.X.Format("%d", this->X);
			unit.FollowsIndex = this->FollowsIndex.c_str();
			unit.Status = this->Status.c_str();
			unit.Facing = this->Facing.c_str();
			unit.Tag = this->Tag.c_str();
			unit.VeterancyPercentage = this->VeterancyPercentage.c_str();
			unit.Group = this->Group.c_str();
			unit.IsAboveGround = this->IsAboveGround.c_str();
			unit.AutoNORecruitType = this->AutoNORecruitType.c_str();
			unit.AutoYESRecruitType = this->AutoYESRecruitType.c_str();
			CMapData::Instance->SetUnitData(&unit, nullptr, nullptr, 0, "");
			CLuaConsole::needRedraw = true;
		}
		static unit convert(CUnitData& obj)
		{
			unit ret{ "","" ,0 ,0 };
			ret.House = obj.House.m_pchData;
			ret.TypeID = obj.TypeID.m_pchData;
			ret.Health = obj.Health.m_pchData;
			ret.Y = atoi(obj.Y);
			ret.X = atoi(obj.X);
			ret.FollowsIndex = obj.FollowsIndex.m_pchData;
			ret.Status = obj.Status.m_pchData;
			ret.Facing = obj.Facing.m_pchData;
			ret.Tag = obj.Tag.m_pchData;
			ret.VeterancyPercentage = obj.VeterancyPercentage.m_pchData;
			ret.Group = obj.Group.m_pchData;
			ret.IsAboveGround = obj.IsAboveGround.m_pchData;
			ret.AutoNORecruitType = obj.AutoNORecruitType.m_pchData;
			ret.AutoYESRecruitType = obj.AutoYESRecruitType.m_pchData;
			return ret;
		}
		void remove() const
		{
			for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Units"); ++i)
			{
				CUnitData obj;
				CMapData::Instance->GetUnitData(i, obj);
				if (*this == convert(obj))
				{
					CINI::CurrentDocument->DeleteKey("Units", CINI::CurrentDocument->GetKeyAt("Units", i));
					CLuaConsole::needRedraw = true;
					CLuaConsole::updateUnit = true;
					CLuaConsole::updateMinimap = true;
					break;
				}
			}
		}
	};
	
	class aircraft
	{
	public:
		aircraft(std::string house, std::string type, int y, int x)
			: House(house), TypeID(type), X(x), Y(y) 
		{
			Health = "256";
			Status = "Guard";
			Tag = "None";
			Facing = "64";
			VeterancyPercentage = "0";
			Group = "-1";
			AutoNORecruitType = "0";
			AutoYESRecruitType = "0";
		}
		std::string House;
		std::string TypeID;
		std::string Health;
		int Y;
		int X;
		std::string Status;
		std::string Facing;
		std::string Tag;
		std::string VeterancyPercentage;
		std::string Group;
		std::string AutoNORecruitType;
		std::string AutoYESRecruitType;

		bool operator==(const aircraft& another)
		{
			return House == another.House &&
				TypeID == another.TypeID &&
				Health == another.Health &&
				X == another.X &&
				Y == another.Y &&
				Status == another.Status &&
				Facing == another.Facing &&
				Tag == another.Tag &&
				VeterancyPercentage == another.VeterancyPercentage &&
				Group == another.Group &&
				AutoNORecruitType == another.AutoNORecruitType &&
				AutoYESRecruitType == another.AutoYESRecruitType;
		};
		void place() const
		{
			if (!CMapData::Instance->IsCoordInMap(this->X, this->Y))
				return;
			CAircraftData air;
			air.House = this->House.c_str();
			air.TypeID = this->TypeID.c_str();
			air.Health = this->Health.c_str();
			air.Y.Format("%d", this->Y);
			air.X.Format("%d", this->X);
			air.Status = this->Status.c_str();
			air.Facing = this->Facing.c_str();
			air.Tag = this->Tag.c_str();
			air.VeterancyPercentage = this->VeterancyPercentage.c_str();
			air.Group = this->Group.c_str();
			air.AutoNORecruitType = this->AutoNORecruitType.c_str();
			air.AutoYESRecruitType = this->AutoYESRecruitType.c_str();
			CMapData::Instance->SetAircraftData(&air, nullptr, nullptr, 0, "");
			CLuaConsole::needRedraw = true;
		}
		static aircraft convert(CAircraftData& obj)
		{
			aircraft ret{ "","" ,0 ,0 };
			ret.House = obj.House.m_pchData;
			ret.TypeID = obj.TypeID.m_pchData;
			ret.Health = obj.Health.m_pchData;
			ret.Y = atoi(obj.Y);
			ret.X = atoi(obj.X);
			ret.Status = obj.Status.m_pchData;
			ret.Facing = obj.Facing.m_pchData;
			ret.Tag = obj.Tag.m_pchData;
			ret.VeterancyPercentage = obj.VeterancyPercentage.m_pchData;
			ret.Group = obj.Group.m_pchData;
			ret.AutoNORecruitType = obj.AutoNORecruitType.m_pchData;
			ret.AutoYESRecruitType = obj.AutoYESRecruitType.m_pchData;
			return ret;
		}
		void remove() const
		{
			for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Aircraft"); ++i)
			{
				CAircraftData obj;
				CMapData::Instance->GetAircraftData(i, obj);
				if (*this == convert(obj))
				{
					CINI::CurrentDocument->DeleteKey("Aircraft", CINI::CurrentDocument->GetKeyAt("Aircraft", i));
					CLuaConsole::needRedraw = true;
					CLuaConsole::updateAircraft = true;
					CLuaConsole::updateMinimap = true;
					break;
				}
			}
		}
	};
	
	class building
	{
	public:
		building(std::string house, std::string type, int y, int x)
			: House(house), TypeID(type), X(x), Y(y) 
		{
			Tag = "None";
			Facing = "0";
			if (ExtConfigs::AISellableDefaultYes)
				AISellable = "1";
			else
				AISellable = "0";
			AIRebuildable = "0";
			SpotLight = "0";
			if (ExtConfigs::AIRepairDefaultYes)
				AIRepairable = "1";
			else
				AIRepairable = "0";
			Nominal = "0";
			PoweredOn = "1";
			Upgrade1 = "None";
			Upgrade2 = "None";
			Upgrade3 = "None";
			Upgrades = "0";
			Health = "256";
		}
		std::string House;
		std::string TypeID;
		std::string Health;
		int Y;
		int X;
		std::string Facing;
		std::string Tag;
		std::string AISellable;
		std::string AIRebuildable;
		std::string PoweredOn;
		std::string Upgrades;
		std::string SpotLight;
		std::string Upgrade1;
		std::string Upgrade2;
		std::string Upgrade3;
		std::string AIRepairable;
		std::string Nominal;

		bool operator==(const building& another)
		{
			return House == another.House &&
				TypeID == another.TypeID &&
				Health == another.Health &&
				Y == another.Y &&
				X == another.X &&
				Facing == another.Facing &&
				Tag == another.Tag &&
				AISellable == another.AISellable &&
				AIRebuildable == another.AIRebuildable &&
				PoweredOn == another.PoweredOn &&
				Upgrades == another.Upgrades &&
				SpotLight == another.SpotLight &&
				Upgrade1 == another.Upgrade1 &&
				Upgrade2 == another.Upgrade2 &&
				Upgrade3 == another.Upgrade3 &&
				AIRepairable == another.AIRepairable;
		};
		void place() const
		{
			if (!CMapData::Instance->IsCoordInMap(this->X, this->Y))
				return;

			ppmfc::CString id = CINI::GetAvailableKey("Structures");

			ppmfc::CString value;
			value.Format("%s,%s,%s,%d,%d,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s", 
				this->House.c_str(), this->TypeID.c_str(), this->Health.c_str(), this->Y, this->X,
				this->Facing.c_str(), this->Tag.c_str(), this->AISellable.c_str(), this->AIRebuildable.c_str(),
				this->PoweredOn.c_str(), this->Upgrades.c_str(), this->SpotLight.c_str(), this->Upgrade1.c_str(),
				this->Upgrade2.c_str(), this->Upgrade3.c_str(), this->AIRepairable.c_str(), this->Nominal.c_str());
				
			CINI::CurrentDocument->WriteString("Structures", id, value);
			CLuaConsole::needRedraw = true;
			CLuaConsole::updateBuilding = true;
		}
		void place_node(bool deleteBuilding = false) const
		{
			if (this->X == -1 || this->Y == -1)
				return;
			auto cell = CMapData::Instance->TryGetCellAt(this->X, this->Y);
			if (cell->BaseNode.BasenodeID > 0)
				return;
			if (auto pHouse = CINI::CurrentDocument->GetSection(this->House.c_str()))
			{
				for (int i = 0; i < 1000; ++i)
				{
					ppmfc::CString key;
					key.Format("%03d", i);
					if (!CINI::CurrentDocument->KeyExists(this->House.c_str(), key))
					{
						ppmfc::CString count;
						ppmfc::CString value;
						count.Format("%d", i + 1);
						value.Format("%s,%d,%d", this->TypeID.c_str(), this->Y, this->X);
						CINI::CurrentDocument->WriteString(this->House.c_str(), "NodeCount", count);
						CINI::CurrentDocument->WriteString(this->House.c_str(), key, value);
						CLuaConsole::updateNode = true;
						CLuaConsole::needRedraw = true;
						if (deleteBuilding && cell->Structure > -1)
						{
							remove();
						}
						break;
					}
				}
			}

		}
		void remove() const
		{
			for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Structures"); ++i)
			{
				CBuildingData obj;
				CMapData::Instance->GetBuildingData(i, obj);
				if (*this == convert(obj))
				{
					CINI::CurrentDocument->DeleteKey("Structures", CINI::CurrentDocument->GetKeyAt("Structures", i));
					CLuaConsole::needRedraw = true;
					CLuaConsole::updateBuilding = true;
					CLuaConsole::updateMinimap = true;
					break;
				}
			}
		}
		static building convert(CBuildingData& obj)
		{
			building ret{ "","" ,0 ,0 };
			ret.House = obj.House.m_pchData;
			ret.TypeID = obj.TypeID.m_pchData;
			ret.Health = obj.Health.m_pchData;
			ret.Y = atoi(obj.Y);
			ret.X = atoi(obj.X);
			ret.Facing = obj.Facing.m_pchData;
			ret.Tag = obj.Tag.m_pchData;
			ret.AISellable = obj.AISellable.m_pchData;
			ret.AIRebuildable = obj.AIRebuildable.m_pchData;
			ret.PoweredOn = obj.PoweredOn.m_pchData;
			ret.Upgrades = obj.Upgrades.m_pchData;
			ret.SpotLight = obj.SpotLight.m_pchData;
			ret.Upgrade1 = obj.Upgrade1.m_pchData;
			ret.Upgrade2 = obj.Upgrade2.m_pchData;
			ret.Upgrade3 = obj.Upgrade3.m_pchData;
			ret.AIRepairable = obj.AIRepairable.m_pchData;
			ret.Nominal = obj.Nominal.m_pchData;
			return ret;
		}
	};

	class cell
	{
	public:
		bool IsHidden() const
		{
			return IsHiddenCell || (*CTileTypeClass::Instance)[TileIndex == 0xFFFF ? 0 : TileIndex].IsHidden;
		}

		void apply() const
		{
			auto pCell = CMapData::Instance->GetCellAt(this->X, this->Y);
			pCell->Unit = this->Unit;
			pCell->Infantry[0] = this->Infantry[0];
			pCell->Infantry[1] = this->Infantry[1];
			pCell->Infantry[2] = this->Infantry[2];
			pCell->Aircraft = this->Aircraft;
			pCell->Structure = this->Structure;
			pCell->TypeListIndex = this->TypeListIndex;
			pCell->Terrain = this->Terrain;
			pCell->TerrainType = this->TerrainType;
			pCell->Smudge = this->Smudge;
			pCell->SmudgeType = this->SmudgeType;
			pCell->Waypoint = this->Waypoint;
			pCell->BaseNode.BuildingID = this->BuildingID;
			pCell->BaseNode.BasenodeID = this->BasenodeID;
			pCell->BaseNode.House = this->House.c_str();
			if (pCell->Overlay != this->Overlay || pCell->OverlayData != this->OverlayData)
			{
				int olyPos = this->Y + this->X * 512;
				if (olyPos < 262144)
				{
					CMapData::Instance->Overlay[olyPos] = this->Overlay;
					CMapData::Instance->OverlayData[olyPos] = this->OverlayData;
				}
				CLuaConsole::recalculateOre = true;
			}
			pCell->Overlay = this->Overlay;
			pCell->OverlayData = this->OverlayData;
			pCell->TileIndex = this->TileIndex;
			pCell->TileIndexHiPart = this->TileIndexHiPart;
			pCell->TileSubIndex = this->TileSubIndex;
			pCell->Height = this->Height;
			pCell->IceGrowth = this->IceGrowth;
			pCell->CellTag = this->CellTag;
			pCell->Tube = this->Tube;
			pCell->TubeDataIndex = this->TubeDataIndex;
			pCell->StatusFlag = this->StatusFlag;
			pCell->Flag.NotAValidCell = this->NotAValidCell;
			pCell->Flag.IsHiddenCell = this->IsHiddenCell;
			pCell->Flag.RedrawTerrain = this->RedrawTerrain;
			pCell->Flag.CliffHack = this->CliffHack;
			pCell->Flag.AltIndex = this->AltIndex;
			CMapData::Instance->UpdateMapPreviewAt(this->X, this->Y);
			CLuaConsole::needRedraw = true;
		}

		short X;
		short Y;
		short Unit;
		short Infantry[3];
		short Aircraft;
		short Structure;
		short TypeListIndex;
		short Terrain;
		int TerrainType;
		short Smudge;
		int SmudgeType;
		short Waypoint;

		int BuildingID;
		int BasenodeID;
		std::string House;

		unsigned char Overlay;
		unsigned char OverlayData;
		unsigned short TileIndex;
		unsigned short TileIndexHiPart;
		unsigned char TileSubIndex;
		unsigned char Height;
		unsigned char IceGrowth;
		short CellTag;
		short Tube;
		unsigned char TubeDataIndex;
		unsigned char StatusFlag;
		bool NotAValidCell;
		bool IsHiddenCell;
		bool RedrawTerrain;
		bool CliffHack;
		char AltIndex;
	};

	class tile 
	{
	public:
		bool Valid = false;
		short TileIndex;
		short TileSubIndex;
		short Height;
		short AltCount;
		short RelativeX;
		short RelativeY;
		static tile get_tile_block(int tileIdx, int tileSubIdx)
		{
			tile ret{};
			tileIdx = CMapDataExt::GetSafeTileIndex(tileIdx);
			ret.TileIndex = tileIdx;
			ret.TileSubIndex = tileSubIdx;
			if (tileIdx > CMapDataExt::TileDataCount)
				return ret;
			const auto& tileData = CMapDataExt::TileData[tileIdx];
			ret.AltCount = tileData.AltTypeCount;
			if (tileSubIdx > tileData.TileBlockCount)
				return ret;
			const auto& tileBlock = tileData.TileBlockDatas[tileSubIdx];
			if (tileBlock.ImageData == NULL)
				return ret;
			ret.Height = tileBlock.Height;
			ret.RelativeY = tileSubIdx % tileData.Width;
			ret.RelativeX = tileSubIdx / tileData.Width;
			ret.Valid = true;
			return ret;
		}
		static std::vector<tile> get_tile_data(int tileIdx)
		{
			std::vector<tile> ret;
			if (tileIdx > CMapDataExt::TileDataCount)
				return ret;
			const auto& tileData = CMapDataExt::TileData[tileIdx];
			for (int i = 0; i < tileData.Width * tileData.Height; ++i)
			{
				auto&& t = get_tile_block(tileIdx, i);
				if (t.Valid)
				{
					ret.push_back(t);
				}
			}
			return ret;
		}
	};

	class snapshot
	{
	public:
		TimePoint savedTime;
		std::string fileName;
		std::map<ppmfc::CString, std::map<ppmfc::CString, ppmfc::CString>> INI;
		unsigned char Overlay[0x40000];
		unsigned char OverlayData[0x40000];
		std::vector<CellData> CellDatas;
	};
	static std::map<int, snapshot> snapshots;

	static std::string GetAvailableIndex()
	{
		auto& ini = CINI::CurrentDocument;
		int n = 1000000;

		std::unordered_set<std::string> usedIDs;

		for (const auto& sec : { "ScriptTypes", "TaskForces", "TeamTypes" }) {
			if (auto pSection = ini->GetSection(sec)) {
				for (const auto& [k, v] : pSection->GetEntities()) {
					usedIDs.insert(v.m_pchData);
				}
			}
		}
		for (const auto& sec : { "Triggers", "Events", "Tags", "Actions", "AITriggerTypes" }) {
			if (auto pSection = ini->GetSection(sec)) {
				for (const auto& [k, v] : pSection->GetEntities()) {
					usedIDs.insert(k.m_pchData);
				}
			}
		}

		char idBuffer[9];
		while (true)
		{
			std::sprintf(idBuffer, "%08d", n);
			std::string id(idBuffer);

			if (usedIDs.find(id) == usedIDs.end() 
				&& UsedINIIndices.find(id) == UsedINIIndices.end() 
				&& !ini->SectionExists(id.c_str())) {
				return id;
			}

			n++;
		}

		return "";
	}


	struct tag
	{
		std::string ID;
		std::string Name;
		std::string RepeatType;
	};

	class trigger
	{
	public:
		std::string ID;
		std::string Name;
		std::string House;
		std::vector<tag> Tags;
		std::string AttachedTrigger;
		std::string Obsolete;
		bool Disabled;
		bool EasyEnabled;
		bool MediumEnabled;
		bool HardEnabled;
		std::vector<std::string> Events;
		std::vector<std::string> Actions;

		trigger()
		{
			ID = GetAvailableIndex();
			Name = "New Trigger";
			House = "Americans";
			AttachedTrigger = "<none>";
			Obsolete = "0";
			Disabled = false;
			EasyEnabled = true;
			MediumEnabled = true;
			HardEnabled = true;
			UsedINIIndices.insert(ID);
		}
		trigger(std::string id)
			: ID(id)
		{
			Name = "New Trigger";
			House = "Americans";
			AttachedTrigger = "<none>";
			Obsolete = "0";
			Disabled = false;
			EasyEnabled = true;
			MediumEnabled = true;
			HardEnabled = true;
			UsedINIIndices.insert(id);
		}
		void add_tag(std::string id, std::string name, int repeat)
		{
			if (id == "")
				id = GetAvailableIndex();
			if (name == "")
				name = Name + " 1";
			auto& tag = Tags.emplace_back();
			tag.ID = id;
			tag.Name = name;
			tag.RepeatType = repeat == 2 ? "2" : repeat == 1 ? "1" : "0";
			UsedINIIndices.insert(id);
		}
		void add_event(std::string value)
		{
			auto&& splits = split_string(value);
			if (splits.size() < 3 || splits.size() > 4 || (splits.size() == 4 && splits[1] != "2"))
			{
				write_lua_console("Ill-formed event " + value);
				return;
			}
			Events.push_back(value);
		}
		void add_action(std::string value)
		{
			auto&& splits = split_string(value);
			if (splits.size() != 8)
			{
				write_lua_console("Ill-formed action " + value);
				return;
			}
			Actions.push_back(value);
		}
		void delete_tag(int index)
		{
			if (0 <= index && index < Tags.size())
			{
				UsedINIIndices.erase(Tags[index].ID);
				Tags.erase(Tags.begin() + index);
			}
		}
		void delete_event(int index)
		{
			if (0 <= index && index < Events.size())
				Events.erase(Events.begin() + index);
		}
		void delete_action(int index)
		{
			if (0 <= index && index < Actions.size())
				Actions.erase(Actions.begin() + index);
		}
		void change_id(std::string id)
		{
			UsedINIIndices.erase(ID);
			UsedINIIndices.insert(id);
			ID = id;
		}
		void apply()
		{
			ppmfc::CString trigger;
			trigger.Format("%s,%s,%s,%s,%s,%s,%s,%s", House.c_str(), AttachedTrigger.c_str(), Name.c_str(),
				Disabled ? "1" : "0", EasyEnabled ? "1" : "0", MediumEnabled ? "1" : "0", HardEnabled ? "1" : "0", Obsolete.c_str());
			CINI::CurrentDocument->WriteString("Triggers", ID.c_str(), trigger);

			for (const auto& tag : Tags)
			{
				ppmfc::CString tagStr;
				tagStr.Format("%s,%s,%s", tag.RepeatType.c_str(), tag.Name.c_str(), ID.c_str());
				CINI::CurrentDocument->WriteString("Tags", tag.ID.c_str(), tagStr);
			}

			ppmfc::CString events;
			events.Format("%d", Events.size());
			for (const auto& e : Events)
			{
				events += ",";
				events += e.c_str();
			}
			CINI::CurrentDocument->WriteString("Events", ID.c_str(), events);

			ppmfc::CString actions;
			actions.Format("%d", Actions.size());
			for (const auto& a : Actions)
			{
				actions += ",";
				actions += a.c_str();
			}
			CINI::CurrentDocument->WriteString("Actions", ID.c_str(), actions);
			CMapDataExt::AddTrigger(ID.c_str());
			CLuaConsole::updateTrigger = true;
		}
		void delete_trigger_self(bool keepTag)
		{
			CINI::CurrentDocument->DeleteKey("Triggers", ID.c_str());
			CINI::CurrentDocument->DeleteKey("Events", ID.c_str());
			CINI::CurrentDocument->DeleteKey("Actions", ID.c_str());
			if (!keepTag)
			{
				for (const auto& tag : Tags)
				{
					CINI::CurrentDocument->DeleteKey("Tags", tag.ID.c_str());
				}
			}

			CMapDataExt::DeleteTrigger(ID.c_str());
			CLuaConsole::updateTrigger = true;
		}
		static void delete_trigger(std::string ID, bool keepTag)
		{
			CINI::CurrentDocument->DeleteKey("Triggers", ID.c_str());
			CINI::CurrentDocument->DeleteKey("Events", ID.c_str());
			CINI::CurrentDocument->DeleteKey("Actions", ID.c_str());
			if (!keepTag)
			{
				std::vector<ppmfc::CString> keys;
				if (auto pSection = CINI::CurrentDocument->GetSection("Tags"))
				{
					for (const auto& [key, value] : pSection->GetEntities())
					{
						auto&& atoms = STDHelpers::SplitString(value, 2);
						if (atoms[2] == ID.c_str())
							keys.push_back(key);
					}
				}
				for (auto& key : keys)
					CINI::CurrentDocument->DeleteKey("Tags", key);
			}
			CMapDataExt::DeleteTrigger(ID.c_str());
			CLuaConsole::updateTrigger = true;
		}
		static void delete_tag_static(std::string ID, bool keepTrigger)
		{
			if (!keepTrigger)
			{
				auto&& atoms = STDHelpers::SplitString(CINI::CurrentDocument->GetString("Tags", ID.c_str()), 2);
				CINI::CurrentDocument->DeleteKey("Triggers", atoms[2]);
				CINI::CurrentDocument->DeleteKey("Events", atoms[2]);
				CINI::CurrentDocument->DeleteKey("Actions", atoms[2]);
				CMapDataExt::DeleteTrigger(atoms[2]);
			}
			CINI::CurrentDocument->DeleteKey("Tags", ID.c_str());
			CLuaConsole::updateTrigger = true;
		}
		static sol::object get_trigger(std::string ID)
		{
			if (auto t = CMapDataExt::GetTrigger(ID.c_str()))
			{
				trigger ret{ t->ID.m_pchData };
				ret.Name = t->Name.m_pchData;
				ret.House = t->House.m_pchData;
				ret.AttachedTrigger = t->AttachedTrigger.m_pchData;
				ret.Obsolete = "0";
				ret.Disabled = t->Disabled;
				ret.EasyEnabled = t->EasyEnabled;
				ret.MediumEnabled = t->MediumEnabled;
				ret.HardEnabled = t->HardEnabled;
				if (auto pSection = CINI::CurrentDocument->GetSection("Tags"))
				{
					for (const auto& [key, value] : pSection->GetEntities())
					{
						auto&& atoms = STDHelpers::SplitString(value, 2);
						if (atoms[2] == ID.c_str())
						{
							auto& tag = ret.Tags.emplace_back();
							tag.ID = key.m_pchData;
							tag.Name = atoms[1].m_pchData;
							tag.RepeatType = atoms[0].m_pchData;
						}
					}
				}
				for (const auto& eve : t->Events)
				{
					auto& str = ret.Events.emplace_back();
					str += eve.EventNum.m_pchData;
					int loopCount = eve.P3Enabled ? 3 : 2;
					for (int i = 0; i < loopCount; ++i)
					{
						str += ",";
						str += eve.Params[i].m_pchData;
					}
				}
				for (const auto& act : t->Actions)
				{
					auto& str = ret.Actions.emplace_back();
					str += act.ActionNum.m_pchData;
					for (int i = 0; i < 7; ++i)
					{
						str += ",";
						str += act.Params[i].m_pchData;
					}
				}
				return sol::make_object(CLuaConsole::Lua, ret);
			}
			else
			{
				write_lua_console("Cannot find trigger " + ID);
			}
			return sol::make_object(CLuaConsole::Lua, sol::nil);
		}
		void release_id() const {
			UsedINIIndices.erase(ID);
		}

	};

	static void place_terrain(int y, int x, std::string id)
	{
		int pos = CMapData::Instance->GetCoordIndex(x, y);
		if (CMapData::Instance->IsCoordInMap(x, y))
		{
			auto cell = CMapData::Instance->GetCellAt(pos);
			if (cell->Terrain > -1) {
				CMapData::Instance->DeleteTerrainData(cell->Terrain);
			}
			CMapData::Instance->SetTerrainData(id.c_str(), pos);
			CLuaConsole::needRedraw = true;
		}
	}

	static void remove_terrain(int y, int x)
	{
		if (CMapData::Instance->IsCoordInMap(x, y))
		{
			auto cell = CMapData::Instance->GetCellAt(x, y);
			if (cell->Terrain > -1) {
				CMapData::Instance->DeleteTerrainData(cell->Terrain);
				CLuaConsole::needRedraw = true;
			}
		}
	}

	static void place_smudge(int y, int x, std::string id)
	{
		int pos = CMapData::Instance->GetCoordIndex(x, y);
		if (CMapData::Instance->IsCoordInMap(x, y))
		{
			auto cell = CMapData::Instance->GetCellAt(pos);
			if (cell->Smudge > -1) {
				CMapData::Instance->DeleteSmudgeData(cell->Smudge);
			}
			CSmudgeData obj;
			obj.X = y;
			obj.Y = x;
			obj.TypeID = id.c_str();
			CMapData::Instance->SetSmudgeData(&obj);
			CLuaConsole::needRedraw = true;
		}
	}

	static void remove_smudge(int y, int x)
	{
		if (CMapData::Instance->IsCoordInMap(x, y))
		{
			auto cell = CMapData::Instance->GetCellAt(x, y);
			if (cell->Smudge > -1) {
				CMapData::Instance->DeleteSmudgeData(cell->Smudge);
				CLuaConsole::needRedraw = true;
			}
		}
	}

	static void place_overlay(int y, int x, int overlay, int overlayData = -1)
	{		
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;
		if (overlay > 255 || (overlay < 0 && overlayData < 0))
		{
			std::ostringstream oss;
			oss << "Invalid Overlay: " << overlay;
			write_lua_console(oss.str());
			return;
		}
		if (overlayData > 60)
		{
			std::ostringstream oss;
			oss << "Invalid OverlayData: " << overlayData;
			write_lua_console(oss.str());
			return;
		}
		CLuaConsole::needRedraw = true;
		const int ORE_COUNT = 12;
		auto pExt = CMapDataExt::GetExtension();
		int pos = pExt->GetCoordIndex(x, y);
		auto ovr = pExt->GetOverlayAt(pos);
		auto ovrd = pExt->GetOverlayDataAt(pos);
		if (overlay < 0)
			overlay = ovr;
		if (overlayData < 0)
			overlayData = ovr == 255 ? 0 : ovrd;
		
		pExt->SetOverlayAt(pos, overlay);
		pExt->SetOverlayDataAt(pos, overlayData);
	}

	static void remove_overlay(int y, int x)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;

		int pos = CMapData::Instance->GetCoordIndex(x, y);
		CMapData::Instance->SetOverlayAt(pos, 255);
		CMapData::Instance->SetOverlayDataAt(pos, 0);
		CLuaConsole::needRedraw = true;
	}

	static void place_wall(int y, int x, int wall, int damageStage = 0)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;
		auto& walls = CViewObjectsExt::WallDamageStages;
		if (walls.find(wall) != walls.end())
		{
			if (damageStage == -1)
				damageStage = STDHelpers::RandomSelectInt(0, walls[wall]);
			else if (damageStage >= walls[wall])
			{
				std::ostringstream oss;
				oss << "Invalid Wall Damage Stage: " << wall << ", " << damageStage;
				write_lua_console(oss.str());
				return;
			}	
		}
		else
		{
			std::ostringstream oss;
			oss << "Invalid Wall: " << wall;
			write_lua_console(oss.str());
			return;
		}
		CLuaConsole::needRedraw = true;
		int pos = CMapData::Instance->GetCoordIndex(x, y);
		CMapDataExt::PlaceWallAt(pos, wall, damageStage);
	}

	static void place_waypoint(int y, int x, int index = -1)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;
		int oldWp = CMapData::Instance->GetCellAt(x, y)->Waypoint;
		if (oldWp > -1)
		{
			write_lua_console(std::format("Waypoint {} already exists at ({},{}), abort.", 
				CINI::CurrentDocument->GetKeyAt("Waypoints", oldWp).m_pchData, x, y));
			return;
		}

		CLuaConsole::needRedraw = true;
		if (index < 0)
		{
			index = atoi(CINI::CurrentDocument->GetAvailableKey("Waypoints"));
		}
		ppmfc::CString key;
		ppmfc::CString value;
		key.Format("%d", index);
		value.Format("%d", x * 1000 + y);
		CINI::CurrentDocument->WriteString("Waypoints", key, value);
		CMapData::Instance->UpdateFieldWaypointData(false);
	}

	static void remove_waypoint(int index)
	{
		CLuaConsole::needRedraw = true;
		if (auto pSection = CINI::CurrentDocument->GetSection("Waypoints"))
		{
			ppmfc::CString key;
			key.Format("%d", index);
			if (CINI::CurrentDocument->KeyExists("Waypoints", key))
			{
				auto&& value = pSection->GetString(key);
				int x = atoi(value) / 1000;
				int y = atoi(value) % 1000;
				CINI::CurrentDocument->DeleteKey(pSection, key);
				CMapData::Instance->UpdateFieldWaypointData(false);

				if (CMapData::Instance->IsMultiOnly())
				{
					int k, l;
					for (k = -1; k < 2; k++)
						for (l = -1; l < 2; l++)
							CMapData::Instance->UpdateMapPreviewAt(x + k, y + l);
				}
			}
		}
	}

	static void remove_waypoint_at(int y, int x)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;
		int index = CMapData::Instance->GetCellAt(x, y)->Waypoint;
		if (index > -1)
			remove_waypoint(atoi(CINI::CurrentDocument->GetKeyAt("Waypoints", index)));
	}

	static std::string get_string(std::string section, std::string key, std::string def = "", std::string loadFrom = "map")
	{
		auto endsWithIni = [](const std::string& str) {
			size_t strLength = str.length();
			size_t iniLength = 4;
			if (strLength < iniLength) {
				return false;
			}

			return (str.compare(strLength - iniLength, iniLength, ".ini") == 0);
			};

		std::transform(loadFrom.begin(), loadFrom.end(), loadFrom.begin(),
			[](unsigned char c) { return std::tolower(c); });
		if (endsWithIni(loadFrom))
		{
			CINI* ini;
			if (LoadedINIs.find(loadFrom.c_str()) != LoadedINIs.end())
			{
				ini = LoadedINIs[loadFrom.c_str()];
			}
			else
			{
				ini = GameCreate<CINI>();
				LoadedINIs[loadFrom.c_str()] = ini;
				CLoading::Instance->LoadTSINI(loadFrom.c_str(), ini, TRUE);
			}
			return ini->GetString(section.c_str(), key.c_str(), def.c_str()).m_pchData;
		}
		
		MultimapHelper mmh;
		ExtraWindow::LoadFrom(mmh, loadFrom.c_str());
		return mmh.GetString(section.c_str(), key.c_str(), def.c_str()).m_pchData;
	}

	static int get_integer(std::string section, std::string key, int def = 0, std::string loadFrom = "map")
	{
		auto result = get_string(section, key, "", loadFrom);
		int ret = 0;
		if (sscanf_s(result.c_str(), "%d", &ret) == 1)
			return ret;
		return def;
	}

	static float get_float(std::string section, std::string key, float def = 0.0f, std::string loadFrom = "map")
	{
		auto result = get_string(section, key, "", loadFrom);
		float ret = 0.0f;
		if (sscanf_s(result.c_str(), "%f", &ret) == 1)
		{
			if (strchr(result.c_str(), '%%'))
				ret *= 0.01f;
			return ret;
		}
		return def;
	}

	static bool get_bool(std::string section, std::string key, bool def = false, std::string loadFrom = "map")
	{
		auto result = get_string(section, key, "", loadFrom);
		switch (toupper(static_cast<unsigned char>(result[0])))
		{
		case '1':
		case '2':
		case '3':
		case '4':
		case '5':
		case '6':
		case '7':
		case '8':
		case '9':
		case 'T':
		case 'Y':
			return true;
		case '0':
		case 'F':
		case 'N':
			return false;
		default:
			return def;
		}
	}

	static std::set<std::string> get_sections(std::string loadFrom = "map")
	{
		std::set<std::string> ret;
		auto endsWithIni = [](const std::string& str) {
			size_t strLength = str.length();
			size_t iniLength = 4;
			if (strLength < iniLength) {
				return false;
			}

			return (str.compare(strLength - iniLength, iniLength, ".ini") == 0);
			};

		std::transform(loadFrom.begin(), loadFrom.end(), loadFrom.begin(),
			[](unsigned char c) { return std::tolower(c); });
		if (endsWithIni(loadFrom))
		{
			CINI* ini;
			if (LoadedINIs.find(loadFrom.c_str()) != LoadedINIs.end())
			{
				ini = LoadedINIs[loadFrom.c_str()];
			}
			else
			{
				ini = GameCreate<CINI>();
				LoadedINIs[loadFrom.c_str()] = ini;
				CLoading::Instance->LoadTSINI(loadFrom.c_str(), ini, TRUE);
			}

			auto itr = ini->Dict.begin();
			for (size_t i = 0, sz = ini->Dict.size(); i < sz; ++i, ++itr)
			{
				ret.insert(itr->first.m_pchData);
			}
			return ret;
		}

		MultimapHelper mmh;
		ExtraWindow::LoadFrom(mmh, loadFrom.c_str());
		for (auto pINI : mmh.GetINIData())
		{
			if (pINI)
			{
				auto itr = pINI->Dict.begin();
				for (size_t i = 0, sz = pINI->Dict.size(); i < sz; ++i, ++itr)
				{
					ret.insert(itr->first.m_pchData);
				}
			}
		}
		return ret;
	}

	static std::vector<std::string> get_keys(std::string section, std::string loadFrom = "map")
	{
		std::vector<std::string> ret;
		auto endsWithIni = [](const std::string& str) {
			size_t strLength = str.length();
			size_t iniLength = 4;
			if (strLength < iniLength) {
				return false;
			}

			return (str.compare(strLength - iniLength, iniLength, ".ini") == 0);
			};

		std::transform(loadFrom.begin(), loadFrom.end(), loadFrom.begin(),
			[](unsigned char c) { return std::tolower(c); });
		if (endsWithIni(loadFrom))
		{
			CINI* ini;
			if (LoadedINIs.find(loadFrom.c_str()) != LoadedINIs.end())
			{
				ini = LoadedINIs[loadFrom.c_str()];
			}
			else
			{
				ini = GameCreate<CINI>();
				LoadedINIs[loadFrom.c_str()] = ini;
				CLoading::Instance->LoadTSINI(loadFrom.c_str(), ini, TRUE);
			}
			if (auto pSection = ini->GetSection(section.c_str()))
			{
				for (auto& [key, value] : pSection->GetEntities())
				{
					ret.push_back(key.m_pchData);
				}
			}
			return ret;
		}

		MultimapHelper mmh;
		ExtraWindow::LoadFrom(mmh, loadFrom.c_str());
		for (auto& [key, value] : mmh.GetUnorderedUnionSection(section.c_str()))
		{
			ret.push_back(key.m_pchData);
		}
		return ret;
	}

	static std::vector<std::pair<std::string, std::string>> get_key_value_pairs(std::string section, std::string loadFrom = "map")
	{
		std::vector<std::pair<std::string, std::string>>  ret;
		auto endsWithIni = [](const std::string& str) {
			size_t strLength = str.length();
			size_t iniLength = 4;
			if (strLength < iniLength) {
				return false;
			}

			return (str.compare(strLength - iniLength, iniLength, ".ini") == 0);
			};

		std::transform(loadFrom.begin(), loadFrom.end(), loadFrom.begin(),
			[](unsigned char c) { return std::tolower(c); });
		if (endsWithIni(loadFrom))
		{
			CINI* ini;
			if (LoadedINIs.find(loadFrom.c_str()) != LoadedINIs.end())
			{
				ini = LoadedINIs[loadFrom.c_str()];
			}
			else
			{
				ini = GameCreate<CINI>();
				LoadedINIs[loadFrom.c_str()] = ini;
				CLoading::Instance->LoadTSINI(loadFrom.c_str(), ini, TRUE);
			}
			if (auto pSection = ini->GetSection(section.c_str()))
			{
				for (auto& [key, value] : pSection->GetEntities())
				{
					ret.push_back(std::make_pair(key.m_pchData, value.m_pchData));
				}
			}
			return ret;
		}

		MultimapHelper mmh;
		ExtraWindow::LoadFrom(mmh, loadFrom.c_str());
		for (auto& [key, value] : mmh.GetUnorderedUnionSection(section.c_str()))
		{
			ret.push_back(std::make_pair(key.m_pchData, value.m_pchData));
		}
		return ret;
	}

	static void write_string(std::string section, std::string key, std::string value)
	{
		CINI::CurrentDocument->WriteString(section.c_str(), key.c_str(), value.c_str());
	}

	static void delete_key(std::string section, std::string key)
	{
		CINI::CurrentDocument->DeleteKey(section.c_str(), key.c_str());
	}

	static void delete_section(std::string section)
	{
		CINI::CurrentDocument->DeleteSection(section.c_str());
	}

	static std::string get_free_waypoint()
	{
		return CINI::CurrentDocument->GetAvailableKey("Waypoints").m_pchData;
	}

	static std::string get_free_key(std::string section)
	{
		return CINI::CurrentDocument->GetAvailableKey(section.c_str()).m_pchData;
	}

	static std::string get_param(std::string section, std::string key, int index, std::string delimiter = ",", std::string loadFrom = "map")
	{
		index--;// to fit lua 
		auto&& str = get_string(section, key, "", loadFrom);
		auto&& atoms = split_string(str, delimiter);
		if (atoms.size() > index) 
		{
			return atoms[index];
		}
		return "";
	}

	static void set_param(std::string section, std::string key, std::string value, int index, std::string delimiter = ",")
	{
		index--;// to fit lua 
		auto&& str = get_string(section, key, "");
		auto&& atoms = split_string(str, delimiter);
		if (atoms.size() > index)
		{
			std::string fullValue;
			for (int i = 0; i < atoms.size(); ++i)
			{
				auto& atom = atoms[i];
				if (i == index)
					atom = value;
				fullValue += atom;
				if (i != atoms.size() - 1)
					fullValue += delimiter[0];
			}
			write_string(section, key, fullValue);
		}
	}

	static void place_infantry(std::string house, std::string type, int y, int x)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;

		int coord = CMapData::Instance->GetCoordIndex(x, y);
		CMapData::Instance->SetInfantryData(NULL, type.c_str(), house.c_str(), coord, -1);
		CLuaConsole::needRedraw = true;
	}

	static void remove_infantry(int indexY, int x = -1, int pos = -1)
	{
		if (x < 0)
		{
			CMapData::Instance->DeleteInfantryData(indexY);
			CLuaConsole::needRedraw = true;
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return;

			int idx = CMapDataExt::GetInfantryAt(CMapData::Instance->GetCoordIndex(x, indexY), pos);
			if (idx > -1)
			{
				CMapData::Instance->DeleteInfantryData(idx);
				CLuaConsole::needRedraw = true;
			}
		}
	}

	static infantry get_infantry(int indexY, int x = -1, int pos = -1)
	{
		infantry ret{ "","" ,-1 ,-1 };
		if (x < 0)
		{
			CInfantryData obj;
			CMapData::Instance->GetInfantryData(indexY, obj);
			return infantry::convert(obj);
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
			{
				return ret;
			}

			int idx = CMapDataExt::GetInfantryAt(CMapData::Instance->GetCoordIndex(x, indexY), pos);
			if (idx > -1)
			{
				CInfantryData obj;
				CMapData::Instance->GetInfantryData(idx, obj);
				return infantry::convert(obj);
			}
		}
		return ret;
	}

	static void place_unit(std::string house, std::string type, int y, int x)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;

		int coord = CMapData::Instance->GetCoordIndex(x, y);
		CMapData::Instance->SetUnitData(NULL, type.c_str(), house.c_str(), coord, "-1");
		CLuaConsole::needRedraw = true;
	}

	static void remove_unit(int indexY, int x = -1)
	{
		if (x < 0)
		{
			CMapData::Instance->DeleteUnitData(indexY);
			CLuaConsole::needRedraw = true;
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return;

			auto idx = CMapData::Instance->GetCellAt(x, indexY)->Unit;
			if (idx > -1)
			{
				CMapData::Instance->DeleteUnitData(idx);
				CLuaConsole::needRedraw = true;
			}
		}
	}
	
	static unit get_unit(int indexY, int x = -1)
	{
		unit ret = { "","",-1,-1 };
		if (x < 0)
		{
			CUnitData obj;
			CMapData::Instance->GetUnitData(indexY, obj);
			return unit::convert(obj);
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return ret;

			auto idx = CMapData::Instance->GetCellAt(x, indexY)->Unit;
			if (idx > -1)
			{
				CUnitData obj;
				CMapData::Instance->GetUnitData(idx, obj);
				return unit::convert(obj);
			}
		}
		return ret;
	}

	static void place_aircraft(std::string house, std::string type, int y, int x)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;

		int coord = CMapData::Instance->GetCoordIndex(x, y);
		CMapData::Instance->SetAircraftData(NULL, type.c_str(), house.c_str(), coord, "-1");
		CLuaConsole::needRedraw = true;
	}

	static void remove_aircraft(int indexY, int x = -1)
	{
		if (x < 0)
		{
			CMapData::Instance->DeleteAircraftData(indexY);
			CLuaConsole::needRedraw = true;
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return;

			auto idx = CMapData::Instance->GetCellAt(x, indexY)->Aircraft;
			if (idx > -1)
			{
				CMapData::Instance->DeleteAircraftData(idx);
				CLuaConsole::needRedraw = true;
			}
		}
	}

	static aircraft get_aircraft(int indexY, int x = -1)
	{
		aircraft ret = { "","",-1,-1 };
		if (x < 0)
		{
			CAircraftData obj;
			CMapData::Instance->GetAircraftData(indexY, obj);
			return aircraft::convert(obj);
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return ret;

			auto idx = CMapData::Instance->GetCellAt(x, indexY)->Aircraft;
			if (idx > -1)
			{
				CAircraftData obj;
				CMapData::Instance->GetAircraftData(idx, obj);
				return aircraft::convert(obj);
			}
		}
		return ret;
	}

	static void place_building(std::string house, std::string type, int y, int x, bool ignoreOverlap = false)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y))
			return;

		int coord = CMapData::Instance->GetCoordIndex(x, y);
		auto oldCheck = ExtConfigs::PlaceStructureOverlappingCheck;
		if (ignoreOverlap)
		{
			ExtConfigs::PlaceStructureOverlappingCheck = false;
		}
		CMapData::Instance->SetBuildingData(NULL, type.c_str(), house.c_str(), coord, "-1");
		ExtConfigs::PlaceStructureOverlappingCheck = oldCheck;

		CLuaConsole::needRedraw = true; 
		CLuaConsole::updateBuilding = true;
	}

	static void remove_building(int indexY, int x = -1)
	{
		if (x < 0)
		{
			CMapData::Instance->DeleteBuildingData(indexY);
			CLuaConsole::needRedraw = true;
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return;

			auto idx = CMapData::Instance->GetCellAt(x, indexY)->Structure;
			if (idx > -1)
			{
				CMapData::Instance->DeleteBuildingData(idx);
				CLuaConsole::needRedraw = true;
			}
		}
	}

	static building get_building(int indexY, int x = -1)
	{
		building ret = { "","",-1,-1 };
		if (x < 0)
		{
			CBuildingData obj;
			CMapData::Instance->GetBuildingData(indexY, obj);
			return building::convert(obj);
		}
		else
		{
			if (!CMapData::Instance->IsCoordInMap(x, indexY))
				return ret;

			auto idx = CMapData::Instance->GetCellAt(x, indexY)->Structure;
			if (idx > -1)
			{
				CBuildingData obj;
				CMapData::Instance->GetBuildingData(idx, obj);
				return building::convert(obj);
			}
		}
		return ret;
	}

	static std::vector<building> get_buildings()
	{
		std::vector<building> ret;
		for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Structures"); ++i)
		{
			ret.push_back(get_building(i));
		}
		return ret;
	}

	static std::vector<aircraft> get_aircrafts()
	{
		std::vector<aircraft> ret;
		for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Aircraft"); ++i)
		{
			ret.push_back(get_aircraft(i));
		}
		return ret;
	}

	static std::vector<infantry> get_infantries()
	{
		std::vector<infantry> ret;
		for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Infantry"); ++i)
		{
			ret.push_back(get_infantry(i));
		}
		return ret;
	}

	static std::vector<unit> get_units()
	{
		std::vector<unit> ret;
		for (int i = 0; i < CINI::CurrentDocument->GetKeyCount("Units"); ++i)
		{
			ret.push_back(get_unit(i));
		}
		return ret;
	}

	static cell get_cell(int y, int x)
	{
		cell c;

		if (!CMapData::Instance->IsCoordInMap(x, y))
		{
			c.X = -1;
			c.Y = -1;
			c.X = -1;
			c.Y = -1;
			c.Unit = -1;
			c.Infantry[0] = -1;
			c.Infantry[1] = -1;
			c.Infantry[2] = -1;
			c.Aircraft = -1;
			c.Structure = -1;
			c.TypeListIndex = -1;
			c.Terrain = -1;
			c.TerrainType = -1;
			c.Smudge = -1;
			c.SmudgeType = -1;
			c.Waypoint = -1;
			c.BuildingID = -1;
			c.BasenodeID = -1;
			c.House = "";
			c.Overlay = -1;
			c.OverlayData = -1;
			c.TileIndex = -1;
			c.TileIndexHiPart = -1;
			c.TileSubIndex = -1;
			c.Height = -1;
			c.IceGrowth = -1;
			c.CellTag = -1;
			c.Tube = -1;
			c.TubeDataIndex = -1;
			c.StatusFlag = -1;
			c.NotAValidCell = false;
			c.IsHiddenCell = false;
			c.RedrawTerrain = false;
			c.CliffHack = false;
			c.AltIndex = false;
			return c;
		}

		auto pCell = CMapData::Instance->GetCellAt(x, y);
		c.X = x;
		c.Y = y;
		c.Unit = pCell->Unit;
		c.Infantry[0] = pCell->Infantry[0];
		c.Infantry[1] = pCell->Infantry[1];
		c.Infantry[2] = pCell->Infantry[2];
		c.Aircraft = pCell->Aircraft;
		c.Structure = pCell->Structure;
		c.TypeListIndex = pCell->TypeListIndex;
		c.Terrain = pCell->Terrain;
		c.TerrainType = pCell->TerrainType;
		c.Smudge = pCell->Smudge;
		c.SmudgeType = pCell->SmudgeType;
		c.Waypoint = pCell->Waypoint;
		c.BuildingID = pCell->BaseNode.BuildingID;
		c.BasenodeID = pCell->BaseNode.BasenodeID;
		c.House = pCell->BaseNode.House.m_pchData;
		c.Overlay = pCell->Overlay;
		c.OverlayData = pCell->OverlayData;
		c.TileIndex = pCell->TileIndex;
		c.TileIndexHiPart = pCell->TileIndexHiPart;
		c.TileSubIndex = pCell->TileSubIndex;
		c.Height = pCell->Height;
		c.IceGrowth = pCell->IceGrowth;
		c.CellTag = pCell->CellTag;
		c.Tube = pCell->Tube;
		c.TubeDataIndex = pCell->TubeDataIndex;
		c.StatusFlag = pCell->StatusFlag;
		c.NotAValidCell = pCell->Flag.NotAValidCell > 0 ? true : false;
		c.IsHiddenCell = pCell->Flag.IsHiddenCell > 0 ? true : false;
		c.RedrawTerrain = pCell->Flag.RedrawTerrain > 0 ? true : false;
		c.CliffHack = pCell->Flag.CliffHack > 0 ? true : false;
		c.AltIndex = pCell->Flag.AltIndex;
		return c;
	}

	static std::vector<cell> get_cells()
	{
		std::vector<cell> ret;
		for (int x = 0; x < CMapData::Instance->MapWidthPlusHeight; ++x)
		{
			for (int y = 0; y < CMapData::Instance->MapWidthPlusHeight; ++y)
			{
				if (CMapData::Instance->IsCoordInMap(x, y))
				{
					ret.push_back(get_cell(x, y));
				}
			}
		}
		return ret;
	}

	static void place_whole_tile(int y, int x, int tileIdx)
	{
		CMapDataExt::GetExtension()->PlaceTileAt(x, y, tileIdx);
		CLuaConsole::needRedraw = true;
	}

	static void place_tile(int y, int x, tile tile, int height = -1, int altType = -1)
	{
		if (!tile.Valid) return;
		if (!CMapData::Instance->IsCoordInMap(x, y)) return;
		auto pExt = CMapDataExt::GetExtension();
		auto cell = pExt->GetCellAt(x, y);
		cell->TileIndex = tile.TileIndex;
		cell->TileSubIndex = tile.TileSubIndex;
		if (altType < 0)
			altType = STDHelpers::RandomSelectInt(0, pExt->TileData[tile.TileIndex].AltTypeCount + 1);
		else if (altType > pExt->TileData[tile.TileIndex].AltTypeCount)
			altType = pExt->TileData[tile.TileIndex].AltTypeCount;
		cell->Flag.AltIndex = altType;
		if (height == -1)
			height = cell->Height + tile.Height;
		pExt->SetHeightAt(x, y, height);
		pExt->UpdateMapPreviewAt(x, y);
		CLuaConsole::needRedraw = true;
	}

	static void set_height(int y, int x, int height)
	{
		CMapDataExt::GetExtension()->SetHeightAt(x, y, height);
		CLuaConsole::needRedraw = true;
	}

	static void hide_tile(int y, int x, int type = 0)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y)) return;
		auto cell = CMapData::Instance->GetCellAt(x, y);
		if (type == 0)
			cell->Flag.IsHiddenCell = true;
		else if (type == 1)
			cell->Flag.IsHiddenCell = false;
		else if (type == 2)
			cell->Flag.IsHiddenCell ^= 1;
		CLuaConsole::needRedraw = true;
	}

	static void hide_tile_set(int index, int type = 0)
	{
		if (index >= 0 && index < CMapDataExt::TileSet_starts.size() - 1) {
			for (int i = CMapDataExt::TileSet_starts[index]; i < CMapDataExt::TileSet_starts[index + 1]; i++) {
				if (type == 0) {
					(*CTileTypeClass::Instance)[i].IsHidden = 1;
				}
				else if (type == 1) {
					(*CTileTypeClass::Instance)[i].IsHidden = 0;
				}
				else if (type == 2) {
					(*CTileTypeClass::Instance)[i].IsHidden ^= 1;
				}
			}
			CLuaConsole::needRedraw = true;
		}
	}
	

	static void multi_select_tile(int y, int x, int type = 0)
	{
		if (!CMapData::Instance->IsCoordInMap(x, y)) return;
		if (type == 0) {
			MultiSelection::AddCoord(x, y);
		}
		else if (type == 1) {
			MultiSelection::RemoveCoord(x, y);
		}
		else if (type == 2) {
			MultiSelection::ReverseStatus(x, y);
		}
		CLuaConsole::needRedraw = true;
	}

	static void multi_select_tile_set(int index, int type = 0)
	{
		if (index >= 0 && index < CMapDataExt::TileSet_starts.size() - 1) {
			for (int x = 0; x < CMapData::Instance->MapWidthPlusHeight; x++) {
				for (int y = 0; y < CMapData::Instance->MapWidthPlusHeight; y++) {
					if (!CMapData::Instance->IsCoordInMap(x, y)) continue;
					auto cell = CMapData::Instance->GetCellAt(x, y);
					int tileIdx = CMapDataExt::GetSafeTileIndex(cell->TileIndex);
					if (tileIdx >= CMapDataExt::TileSet_starts[index] && tileIdx < CMapDataExt::TileSet_starts[index + 1]) {
						if (type == 0) {
							MultiSelection::AddCoord(x, y);
						}
						else if (type == 1) {
							MultiSelection::RemoveCoord(x, y);
						}
						else if (type == 2) {
							MultiSelection::ReverseStatus(x, y);
						}
					}
				}
			}
			CLuaConsole::needRedraw = true;
		}
	}

	static void update_minimap(int Y = -1, int X = -1)
	{
		if (X < 0 || Y < 0)
		{
			for (int x = 0; x < CMapData::Instance->MapWidthPlusHeight; ++x)
			{
				for (int y = 0; y < CMapData::Instance->MapWidthPlusHeight; ++y)
				{
					if (CMapData::Instance->IsCoordInMap(x, y))
					{
						CMapData::Instance->UpdateMapPreviewAt(x, y);
					}
				}
			}
		}
		else
		{
			CMapData::Instance->UpdateMapPreviewAt(X, Y);
		}
	}

	static int create_snapshot()
	{
		CMapData::Instance->UpdateINIFile(SaveMapFlag::UpdateMapFieldData);
		auto ini = &CMapData::Instance->INI;
		int version = 1;
		while (snapshots.find(version) != snapshots.end())
			version++;

		auto& snapshot = snapshots[version];
		auto itr = ini->Dict.begin();
		for (size_t i = 0, sz = ini->Dict.size(); i < sz; ++i, ++itr)
		{
			auto& section = snapshot.INI[itr->first];
			for (const auto& [key, value] : itr->second.GetEntities())
			{
				section[key] = value;
			}
		}
		memcpy(snapshot.Overlay, CMapData::Instance->Overlay, sizeof(CMapData::Instance->Overlay));
		memcpy(snapshot.OverlayData, CMapData::Instance->OverlayData, sizeof(CMapData::Instance->OverlayData));

		for (int i = 0; i < CMapData::Instance->CellDataCount; ++i)
		{
			snapshot.CellDatas.push_back(CMapData::Instance->CellDatas[i]);
		}
		CMapData::Instance->UpdateINIFile(SaveMapFlag::LoadFromINI);
		snapshot.savedTime = getCurrentTime();
		snapshot.fileName = CFinalSunApp::MapPath();
		if (snapshot.fileName == "")
			snapshot.fileName = "New map";
		write_lua_console(std::format("Create snapshot #{}", version));
		return version;
	}

	static void restore_snapshot(int version)
	{
		if (snapshots.find(version) == snapshots.end())
		{
			write_lua_console(std::format("Cannot find snapshot #{}", version));
			return;
		}
		auto& snapshot = snapshots[version];
		Logger::Debug("Restoring map snapshot #%d, time point: %s\n", version, formatTime(snapshot.savedTime).c_str());
		if (CINI::CurrentDocument->GetString("Map", "Theater") != snapshot.INI["Map"]["Theater"])
		{
			write_lua_console(std::format("Cannot restore snapshot #{}, theater dismatch.", version));
			return;
		}

		auto timeSpan = std::chrono::system_clock::now() - snapshot.savedTime.time;
		std::string title = Translations::TranslateOrDefault("LuaConsole.SnapshotConfirmTitle", "Are you sure to restore the snapshot?");
		ppmfc::CString text; 
		text.Format(Translations::TranslateOrDefault("LuaConsole.SnapshotConfirmMessage"
				, "Snapshot timepoint: %s\nTime span: %s\nMap file name: %s")
			, formatTime(snapshot.savedTime).c_str()
			, formatTimeInterval(std::chrono::duration_cast<std::chrono::seconds>(timeSpan)).c_str()
				, snapshot.fileName.c_str());
	
		if (message_box(text.m_pchData, title, 1) == IDCANCEL)
			return;

		auto ini = &CMapData::Instance->INI;

		RECT newSize = { 0,0,0,0 };
		auto&& ns = STDHelpers::SplitString(snapshot.INI["Map"]["Size"], 3);
		newSize.right = atoi(ns[2]);
		newSize.bottom = atoi(ns[3]);
		RECT oldSize = { 0,0,0,0 };
		auto&& os = STDHelpers::SplitString(ini->GetString("Map", "Size"), 3);
		oldSize.right = atoi(os[2]);
		oldSize.bottom = atoi(os[3]);
		if (newSize.right != oldSize.right || newSize.bottom != oldSize.bottom)
		{
			MapRect rect{ 0, 0, newSize.right, newSize.bottom };
			CMapDataExt::GetExtension()->ResizeMapExt(&rect);
		}

		std::vector<ppmfc::CString> sections;
		auto itr = ini->Dict.begin();
		for (size_t i = 0, sz = ini->Dict.size(); i < sz; ++i, ++itr)
		{
			sections.push_back(itr->first);
		}
		
		for (auto& sec : sections)
			ini->DeleteSection(sec);
		for (const auto& section : snapshot.INI)
		{
			auto pSection = ini->AddSection(section.first);
			for (const auto& [key, value] : section.second)
			{
				ini->WriteString(pSection, key, value);
			}
		}
		memcpy(CMapData::Instance->Overlay, snapshot.Overlay, sizeof(snapshot.Overlay));
		memcpy(CMapData::Instance->OverlayData, snapshot.OverlayData, sizeof(snapshot.OverlayData));

		for (int i = 0; i < CMapData::Instance->CellDataCount; ++i)
		{
			CMapData::Instance->CellDatas[i] = snapshot.CellDatas[i];
		}

		CMapDataExt::UpdateFieldStructureData_Optimized(-1);
		CMapData::Instance->UpdateFieldOverlayData(false);
		CMapData::Instance->UpdateINIFile(SaveMapFlag::LoadFromINI);
		// load objects to avoid weird palette issue
		CIsoView::GetInstance()->PrimarySurfaceLost();
		CFinalSunDlg::Instance->MyViewFrame.Minimap.Update();
		CLuaConsole::needRedraw = true;
		CLuaConsole::updateMinimap = true;
	}

	static void clear_snapshot()
	{
		snapshots.clear();
	}

	static void redraw_window()
	{
		::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
	}


}