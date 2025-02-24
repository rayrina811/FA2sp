#pragma once

#include <FA2PP.h>

#include <CIsoView.h>
#include "../FA2Expand.h"

struct CellData;

struct Cell3DLocation
{
    short X;
    short Y;
    short Height;
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
    static int drawOffsetX;
    static int drawOffsetY;
    static Cell3DLocation CurrentDrawCellLocation;

    static bool IsPressingALT;
};