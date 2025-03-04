#pragma once

#include <CMapData.h>
#include "..\Ext\CMapData\Body.h"

#include <set>
#include <concepts>

class MultiSelection
{
public:
    static bool AddCoord(int X, int Y);
    static bool RemoveCoord(int X, int Y);
    inline static size_t GetCount();
    inline static size_t GetCount2();
    inline static void Clear();
    inline static void ClearT();
    static void Clear2();
    static void ReverseStatus(int X, int Y);
    inline static bool IsSelected(int X, int Y);

    struct MyClipboardData
    {
        int X;
        int Y;
        unsigned char Overlay;
        unsigned char OverlayData;
        unsigned short TileIndex;
        unsigned short TileIndexHiPart;
        unsigned char TileSubIndex;
        unsigned char Height;
        unsigned char IceGrowth;
        CellData::CellDataFlag Flag;
    };
    static void Copy();
    static void Paste(int X, int Y, int nBaseHeight, MyClipboardData* data, size_t length, bool obj);

    template<typename _Fn> requires std::invocable<_Fn, CellData&>
    static void ApplyForEach(_Fn _Func)
    {
        for (auto& coord : SelectedCoords)
        {
            auto pCell = CMapData::Instance->GetCellAt(coord.X, coord.Y);
            _Func(*pCell);
        }
    }

    static bool ShiftKeyIsDown;
    static bool IsPasting;
    static BGRStruct ColorHolder[0x1000];
    static std::set<MapCoord> SelectedCoords;
    static std::set<MapCoord> SelectedCoordsTemp;
    static MapCoord LastAddedCoord;

    static std::vector<CellDataExt> CopiedCells;
    static int CopiedX;
    static int CopiedY;
    static bool AddBuildingOptimize;
    static bool SelectCellsChanged;

    //static std::map<int, int> CopiedCellsInfantry;
    //static std::map<int, int> CopiedCellsAircraft;
    static std::map<int, int> CopiedCellsBuilding;
    //static std::map<int, int> CopiedCellsUnit;
    //static std::map<int, ppmfc::CString> CopiedCellsTerrain;
    //static std::map<int, ppmfc::CString> CopiedCellsSmudge;

};

