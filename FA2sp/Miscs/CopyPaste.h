#pragma once

#include <CMapData.h>
#include "..\Ext\CMapData\Body.h"
#include <set>
#include <concepts>

struct MyClipboardData
{
    int X;
    int Y;
    unsigned short Overlay;
    unsigned char OverlayData;
    unsigned short TileIndex;
    unsigned short TileIndexHiPart;
    unsigned char TileSubIndex;
    unsigned char Height;
    unsigned char IceGrowth;
    unsigned short TileSet;
    unsigned short TileSetSubIndex;
    CellData::CellDataFlag Flag;
    char TerrainData[32]{ "" };
    char SmudgeData[32]{ "" };
    char BuildingData[128]{ "" };
    char AircraftData[128]{ "" };
    char InfantryData_1[128]{ "" };
    char InfantryData_2[128]{ "" };
    char InfantryData_3[128]{ "" };
    char UnitData[128]{ "" };
};

struct TileRule {
    std::vector<int> sourceTiles;
    std::vector<int> destinationTiles;
    bool isRandom = false;
    bool hasHeightOverride = false;
    int heightOverride = -1; 
    bool hasSubIndexOverride = false;
    int subIndexOverride = -1; 
};

class CopyPaste
{
public:
    static std::set<MapCoord> PastedCoords;
    static bool CopyWholeMap;
    static bool OnLButtonDownPasted;
    static std::vector<TileRule> TileConvertRules;
    static void Copy(const std::set<MapCoord>& coords);
    static void Paste(int X, int Y, int nBaseHeight, MyClipboardData* data, size_t length);
    static void LoadTileConvertRule(char sourceTheater);
    static void ConvertTile(CellData& cell);
};