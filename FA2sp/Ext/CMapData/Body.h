#pragma once

#include <CMapData.h>

#include <unordered_map>
#include <vector>
#include "../../ExtraWindow/CNewTrigger/CNewTrigger.h"

struct TerrainGeneratorOverlay
{
    unsigned char Overlay;
    std::vector<int> AvailableOverlayData;
};

struct TerrainGeneratorGroup
{
    double Chance = 0.0;
    std::vector<ppmfc::CString> Items;
    std::vector<int> AvailableTiles;
    bool HasExtraIndex = false;
    std::vector<TerrainGeneratorOverlay> Overlays;
    std::vector<ppmfc::CString> OverlayItems;
};

struct BuildingPowers
{
    int TotalPower;
    int Output;
    int Drain;
};

struct OverlayTypeData
{
    bool Rock;
    bool Wall;
    bool TerrainRock;
    ppmfc::CString WallPaletteName;
    RGBClass RadarColor;
};

struct BuildingDataExt
{
    ~BuildingDataExt()
    {
        if (Foundations)
            delete Foundations;
        if (LinesToDraw)
            delete LinesToDraw;
    }

    bool IsCustomFoundation() const
    {
        return Foundations != nullptr;
    }

    int Width{ 0 };
    int Height{ 0 };
    std::vector<MapCoord>* Foundations{ nullptr };
    std::vector<std::pair<MapCoord, MapCoord>>* LinesToDraw{ nullptr };
};

struct BuildingRenderData
{
    unsigned int HouseColor;
    ppmfc::CString ID;
    short X;
    short Y;
    short Facing;
    short Strength;
    unsigned char PowerUpCount;
    ppmfc::CString PowerUp1;
    ppmfc::CString PowerUp2;
    ppmfc::CString PowerUp3;
};

struct LightingSource
{
    float CenterX;
    float CenterY;
    int LightVisibility;
    float LightIntensity;
    float LightRedTint;
    float LightGreenTint;
    float LightBlueTint;
};

struct LightingSourcePosition
{
    int X;
    int Y;
    ppmfc::CString BuildingType;
    bool operator==(const LightingSourcePosition& another) const
    {
        return
            X == another.X &&
            Y == another.Y &&
            BuildingType == another.BuildingType;
    }
};

struct CellDataExt
{
    short X;
    short Y;

    // for preview
    bool AroundPlayerLocation = false;
    bool AroundHighBridge = false;

    // for locate cell
    bool drawCell = false;

    // for copy paste
    CBuildingData BuildingData;
    CAircraftData AircraftData;
    CInfantryData InfantryData[3];
    CUnitData UnitData;
    ppmfc::CString TerrainData;
    ppmfc::CString SmudgeData;

    // for smooth water
    bool IsWater = false;
    bool Processed = false;

    // for raise ground
    bool Adjusted = false;
    bool CreateSlope = false;

    // for create shore
    bool ShoreProcessed = false;
    bool ShoreLATNeeded = false;

    // for terrain generation
    bool AddRandomTile = false;
};

class CMapDataExt : public CMapData
{
public:
    static CMapDataExt* GetExtension()
    {
        return reinterpret_cast<CMapDataExt*>(&CMapData::Instance());
    }
    enum OverlayCreditsType
    {
        OverlayCredits_Riparius = 0,
        OverlayCredits_Cruentus = 1,
        OverlayCredits_Vinifera = 2,
        OverlayCredits_Aboreus = 3,
        OverlayCredits_NumOf
    };

    void PackExt(bool UpdatePreview, bool Description);
    //bool ResizeMapExt(MapRect* const pRect);
    
    enum OreType { Riparius = 0, Cruentus, Vinifera, Aboreus };
    int GetOreValue(unsigned char nOverlay, unsigned char nOverlayData);
    int GetOreValueAt(CellData& cell);
    void InitOreValue();
    static bool IsOre(unsigned char nOverlay);

    bool IsTileIntact(int x, int y, int startX = -1, int startY = -1, int right = -1, int bottom = -1);
    std::vector<MapCoord> GetIntactTileCoords(int x, int y, bool oriIntact);
    LandType GetAltLandType(int tileIndex, int TileSubIndex);
    static inline LandType GetLandType(int tileIndex, int TileSubIndex)
    {
        if (tileIndex == 0xFFFF)
            tileIndex = 0;

        return CMapDataExt::TileData[tileIndex].TileBlockDatas[TileSubIndex].TerrainType;
    }
    void PlaceTileAt(int X, int Y, int index, int callType = -1);
    void SetHeightAt(int X, int Y, int height);

    //void InitializeBuildingTypesExt(const char* ID);
    static void InitializeAllHdmEdition(bool updateMinimap = true);
    static void UpdateTriggers();
    static ppmfc::CString AddTrigger(std::shared_ptr<Trigger> trigger);
    static ppmfc::CString AddTrigger(ppmfc::CString id);
    static std::shared_ptr<Trigger> GetTrigger(ppmfc::CString id);
    static void DeleteTrigger(ppmfc::CString id);
    static void CreateRandomGround(int TopX, int TopY, int BottomX, int BottomY, int scale, std::vector<std::pair<std::vector<int>, float>> tiles, bool override, bool multiSelection, bool onlyClear = false);
    static void CreateRandomOverlay(int TopX, int TopY, int BottomX, int BottomY, std::vector<std::pair<std::vector<TerrainGeneratorOverlay>, float>> overlays, bool override, bool multiSelection, bool onlyClear = false);
    static void CreateRandomTerrain(int TopX, int TopY, int BottomX, int BottomY, std::vector<std::pair<std::vector<ppmfc::CString>, float>> terrains, bool override, bool multiSelection, bool onlyClear = false);
    static void CreateRandomSmudge(int TopX, int TopY, int BottomX, int BottomY, std::vector<std::pair<std::vector<ppmfc::CString>, float>> smudges, bool override, bool multiSelection, bool onlyClear = false);

    static unsigned short CurrentRenderBuildingStrength;
    static std::unordered_map<int, BuildingDataExt> BuildingDataExts;
    static std::map<int, BuildingRenderData>  BuildingRenderDatasFix;
    static std::vector<OverlayTypeData> OverlayTypeDatas;
    static void UpdateFieldStructureData_Optimized(int ID, bool add = true, ppmfc::CString oldStructure = "");
    static void SmoothAll();
    static void SmoothTileAt(int X, int Y, bool gameLAT = false);
    static void SmoothWater();
    static BuildingPowers GetStructurePower(CBuildingData object);
    static BuildingPowers GetStructurePower(ppmfc::CString value);
    static inline int GetSafeTileIndex(int idx)
    {
        if (idx == 0xFFFF)
            idx = 0;
        return idx;
    }
    // damageStage = -1 means read the target cell overlayData to determine
    static void PlaceWallAt(int dwPos, int overlay, int damageStage = -1, bool firstRun = true);
    static int GetInfantryAt(int dwPos, int dwSubPos = 0xFFFFFFFF);
    static std::vector<int> GetStructureSize(ppmfc::CString structure);
    static ppmfc::CString GetFacing(MapCoord oldMapCoord, MapCoord newMapCoord, ppmfc::CString currentFacing);
    static int GetFacing(MapCoord oldMapCoord, MapCoord newMapCoord);
    static int GetFacing4(MapCoord oldMapCoord, MapCoord newMapCoord);
    static bool IsValidTileSet(int tileset);

    static int OreValue[4];
    static bool SkipUpdateBuildingInfo;
    static std::vector<int> deletedKeys;
    static std::vector<std::vector<ppmfc::CString>> Tile_to_lat;
    static std::vector<int> TileSet_starts;

    static CellDataExt CellDataExt_FindCell;
    static std::vector<CellDataExt> CellDataExts;
    //static MapCoord CurrentMapCoord;
    static MapCoord CurrentMapCoordPaste;

    static CTileTypeClass* TileData;
    static int TileDataCount;
    static int CurrentTheaterIndex;

    static int PaveTile;
    static int GreenTile;
    static int MiscPaveTile;
    static int Medians;
    static int PavedRoads;
    static int ShorePieces;
    static int WaterBridge;
    static int BridgeSet;
    static int WoodBridgeSet;
    static Palette Palette_ISO;
    static Palette Palette_Shadow;
    static Palette Palette_AlphaImage;
    static std::vector<std::pair<LightingSourcePosition, LightingSource>> LightingSources;

    static int AutoShore_ShoreTileSet;
    static int AutoShore_GreenTileSet;
    static std::vector<int> ShoreTileSets;
    static std::map<int, bool> SoftTileSets; // soft = affected by shore logic
    static ppmfc::CString BitmapImporterTheater;
    static float ConditionYellow;
    static std::map<int, bool> TileSetCumstomPalette;

    static std::map<ppmfc::CString, std::shared_ptr<Trigger>> Triggers;
};