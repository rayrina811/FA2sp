#pragma once

#include <FA2PP.h>

#include <CIsoView.h>
#include "../FA2Expand.h"
#include <unordered_map>
#include <unordered_set>

struct CellData;

struct Cell3DLocation
{
    short X;
    short Y;
    short Height;
};

struct DDBoundary
{
    unsigned dwWidth{};
    unsigned dwHeight{};
    long dpitch{};
};

struct DrawBuildings
{
    short index;
    short x;
    short y;
    short buildingIndex;
};

struct DrawVeterancies
{
    int X;
    int Y;
    int VP;
};

class NOVTABLE CIsoViewExt : public CIsoView
{
public:
    static void ProgramStartupInit();

    BOOL PreTranslateMessageExt(MSG* pMsg);

    BOOL OnMouseWheelExt(UINT Flags, short zDelta, CPoint pt);

    void DrawLockedCellOutline(int X, int Y, int W, int H, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc, bool s1 = true, bool s2 = true, bool s3 = true, bool s4 = true);
    void DrawLockedCellOutlineX(int X, int Y, int W, int H, COLORREF color, COLORREF colorX, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc);
    void DrawTopRealBorder(int x1, int y1, int x2, int y2, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc);
    void DrawLockedLines(const std::vector<std::pair<MapCoord, MapCoord>>& lines, int X, int Y, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc);
    void DrawCelltag(int X, int Y);
    void DrawBitmap(ppmfc::CString filename, int X, int Y);
    void DrawWaypointFlag(int X, int Y);
    void DrawTube(CellData* pData, int X, int Y);

    void AddTube(int EnterX, int EnterY, int ExitX, int ExitY);

    void DrawLockedCellOutlinePaint(int X, int Y, int W, int H, COLORREF color, bool bUseDot, HDC hdc, HWND hwnd, bool s1 = true, bool s2 = true, bool s3 = true, bool s4 = true);
    void DrawLockedCellOutlinePaintCursor(int X, int Y, int height, COLORREF color, HDC hdc, HWND hwnd, bool useHeightColor);
    static int GetSelectedSubcellInfantryIdx(int X = -1, int Y = -1, bool getSubcell = false);
    static void FillArea(int X, int Y, int ID, int Subtile);
    static IDirectDrawSurface7* BitmapToSurface(IDirectDraw7* pDD, const CBitmap& bitmap);
    static void BlitTransparent(LPDIRECTDRAWSURFACE7 pic, int x, int y, int width = -1, int height = -1, BYTE alpha = 255);
    static void BlitSHPTransparent(LPDDSURFACEDESC2 lpDesc, int x, int y, ImageDataClass* pd, Palette* newPal = NULL, BYTE alpha = 255, int houseColor = -1);
    static void BlitSHPTransparent(CIsoView* pThis, void* dst, const RECT& window,
        const DDBoundary& boundary, int x, int y, ImageDataClass* pd, Palette* newPal = NULL, BYTE alpha = 255, int houseColor = -1);
    static void BlitSHPTransparent_Building(CIsoView* pThis, void* dst, const RECT& window,
        const DDBoundary& boundary, int x, int y, ImageDataClass* pd, Palette* newPal = NULL, BYTE alpha = 255, int houseColor = -1, int addOnColor = -1);
    static void BlitSHPTransparent_AlphaImage(CIsoView* pThis, void* dst, const RECT& window,
        const DDBoundary& boundary, int x, int y, ImageDataClass* pd);
    static void BlitTerrain(CIsoView* pThis, void* dst, const RECT& window,
        const DDBoundary& boundary, int x, int y, CTileBlockClass* subTile, Palette* pal, BYTE alpha = 255);
    static void BlitText(const std::wstring& text, COLORREF textColor, COLORREF bgColor,
        CIsoView* pThis, void* dst, const RECT& window, const DDBoundary& boundary,
        int x, int y, int fontSize = 20, BYTE alpha = 255, bool bold = false);

    static bool DrawStructures;
    static bool DrawInfantries;
    static bool DrawUnits;
    static bool DrawAircrafts;
    static bool DrawBasenodes;
    static bool DrawWaypoints;
    static bool DrawCelltags;
    static bool DrawMoneyOnMap;
    static bool DrawOverlays;
    static bool DrawTerrains;
    static bool DrawSmudges;
    static bool DrawTubes;
    static bool DrawBounds;
    static bool DrawVeterancy;
    static bool DrawShadows;
    static bool DrawAlphaImages;
    static bool DrawBaseNodeIndex;
    static bool RockCells;

    static bool PasteStructures;
    static bool PasteInfantries;
    static bool PasteUnits;
    static bool PasteAircrafts;
    static bool PasteOverlays;
    static bool PasteTerrains;
    static bool PasteSmudges;
    static bool PasteGround;
    static bool PasteOverriding;

    static bool DrawStructuresFilter;
    static bool DrawInfantriesFilter;
    static bool DrawUnitsFilter;
    static bool DrawAircraftsFilter;
    static bool DrawBasenodesFilter;
    static bool DrawCellTagsFilter;

    static bool AutoPropertyBrush[4];

    static COLORREF _cell_hilight_colors[16];
    static float drawOffsetX;
    static float drawOffsetY;
    static Cell3DLocation CurrentDrawCellLocation;

    static std::vector<std::pair<MapCoord, ppmfc::CString>> WaypointsToDraw;
    static std::vector<std::pair<MapCoord, DrawBuildings>> BuildingsToDraw;
    static std::unordered_set<short> VisibleStructures;
    static std::unordered_set<short> VisibleInfantries;
    static std::unordered_set<short> VisibleUnits;
    static std::unordered_set<short> VisibleAircrafts;
    static std::vector<DrawVeterancies> DrawVeterancies;

    static bool IsPressingALT;
};