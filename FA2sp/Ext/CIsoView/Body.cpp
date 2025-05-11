#include "Body.h"

#include "../../FA2sp.h"

#include <WindowsX.h>
#include <CPalette.h>

#include <CMapData.h>
#include <Drawing.h>
#include <CINI.h>

#include <ranges>
#include "../../Helpers/STDHelpers.h"
#include "../CMapData/Body.h"
#include "../../Helpers/Translations.h"
#include "../../Miscs/MultiSelection.h"
#include "../../Miscs/Palettes.h"
#include "../CLoading/Body.h"

bool CIsoViewExt::DrawStructures = true;
bool CIsoViewExt::DrawInfantries = true;
bool CIsoViewExt::DrawUnits = true;
bool CIsoViewExt::DrawAircrafts = true;
bool CIsoViewExt::DrawBasenodes = true;
bool CIsoViewExt::DrawWaypoints = true;
bool CIsoViewExt::DrawCelltags = true;
bool CIsoViewExt::DrawMoneyOnMap = true;
bool CIsoViewExt::DrawOverlays = true;
bool CIsoViewExt::DrawTerrains = true;
bool CIsoViewExt::DrawSmudges = true;
bool CIsoViewExt::DrawTubes = true;
bool CIsoViewExt::DrawBounds = true;
bool CIsoViewExt::DrawVeterancy = true;
bool CIsoViewExt::DrawShadows = true;
bool CIsoViewExt::DrawAlphaImages = true;
bool CIsoViewExt::DrawBaseNodeIndex = true;
bool CIsoViewExt::RockCells = false;

bool CIsoViewExt::PasteStructures = false;
bool CIsoViewExt::PasteInfantries = false;
bool CIsoViewExt::PasteUnits = false;
bool CIsoViewExt::PasteAircrafts = false;
bool CIsoViewExt::PasteOverlays = true;
bool CIsoViewExt::PasteTerrains = false;
bool CIsoViewExt::PasteSmudges = false;
bool CIsoViewExt::PasteGround = true;
bool CIsoViewExt::PasteOverriding = false;
bool CIsoViewExt::PasteShowOutline = false;

bool CIsoViewExt::DrawStructuresFilter = false;
bool CIsoViewExt::DrawInfantriesFilter = false;
bool CIsoViewExt::DrawUnitsFilter = false;
bool CIsoViewExt::DrawAircraftsFilter = false;
bool CIsoViewExt::DrawBasenodesFilter = false;
bool CIsoViewExt::DrawCellTagsFilter = false;

bool CIsoViewExt::AutoPropertyBrush[4] = { false };
bool CIsoViewExt::IsPressingALT = false;
bool CIsoViewExt::IsPressingTube = false;
MapCoord CIsoViewExt::CopyStart = { -1,-1 };
MapCoord CIsoViewExt::CopyEnd = { -1,-1 };
std::vector<MapCoord> CIsoViewExt::TubeNodes;
ppmfc::CString CIsoViewExt::CurrentCellObjectHouse = "";
int CIsoViewExt::EXTRA_BORDER_BOTTOM = 25;
Cell3DLocation CIsoViewExt::CurrentDrawCellLocation;

float CIsoViewExt::drawOffsetX;
float CIsoViewExt::drawOffsetY;

LPDIRECTDRAWSURFACE7 CIsoViewExt::lpDDBackBufferZoomSurface;
double CIsoViewExt::ScaledFactor = 1.0;
double CIsoViewExt::ScaledMax = 1.5;
double CIsoViewExt::ScaledMin = 0.25;

COLORREF CIsoViewExt::CellHilightColors[16] = {
    RGB(255, 255, 255),	// level 0
    RGB(170, 0, 170),	// level 1
    RGB(0, 170, 170),	// level 2
    RGB(0, 170, 0),		// level 3
    RGB(90, 255, 90),	// level 4
    RGB(255, 255, 90),	// level 5
    RGB(255, 50, 50),	// level 6
    RGB(170, 85, 0),	// level 7
    RGB(170, 0, 0),		// level 8
    RGB(85, 255, 255),	// level 9
    RGB(80, 80, 255),	// level 10
    RGB(0, 0, 170),		// level 11
    RGB(0, 0, 0),		// level 12
    RGB(85,85 ,85),		// level 13
    RGB(170, 170, 170),	// level 14
    RGB(255, 255, 255)	// level 15
};

static bool TilePixels[1800] =
{
    false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false,
        false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false,
        false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false,
        true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true,
        false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false,
        false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false,
        false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, true, true, true, true, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
        false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false, false,
};

void CIsoViewExt::ProgramStartupInit()
{
    // RunTime::ResetMemoryContentAt(0x594518, CIsoViewExt::PreTranslateMessageExt);
}

void CIsoViewExt::ConfirmTube(bool addReverse)
{
    auto& nodes = CIsoViewExt::TubeNodes;
    if (nodes.size() < 2)
        return;

    auto GetOppositeFacing = [](int nFacing) -> char
        {
            switch (nFacing)
            {
            case FACING_NORTHEAST:
                return FACING_SOUTHWEST;

            case FACING_EAST:
                return FACING_WEST;

            case FACING_SOUTHEAST:
                return FACING_NORTHWEST;

            case FACING_SOUTH:
                return FACING_NORTH;

            case FACING_SOUTHWEST:
                return FACING_NORTHEAST;

            case FACING_WEST:
                return FACING_EAST;

            case FACING_NORTHWEST:
                return FACING_SOUTHEAST;

            case FACING_NORTH:
                return FACING_SOUTH;

            default:
            case FACING_INVALID:
                return FACING_INVALID;
            }
        };

    std::vector<int> AllDirections;
    for (int j = 0; j < nodes.size() - 1; ++j)
    {
        int x1, x2, y1, y2;
        x1 = nodes[j].X;
        y1 = nodes[j].Y;
        x2 = nodes[j + 1].X;
        y2 = nodes[j + 1].Y;
        auto path = CIsoViewExt::GetTubePath(x1, y1, x2, y2, j == 0);
        auto directions = CIsoViewExt::GetTubeDirections(path);
        AllDirections.insert(AllDirections.end(), directions.begin(), directions.end());
    }
    if (AllDirections.size() > 99)
    {
        ppmfc::CString pMessage = Translations::TranslateOrDefault("ErrorTooLongTube",
            "Cannot generate a too long tube!");
        ::MessageBox(NULL, pMessage, Translations::TranslateOrDefault("Error", "Error"), MB_OK);
        return;
    }
    auto key = CINI::GetAvailableKey("Tubes");
    ppmfc::CString value;
    value.Format("%d,%d,%d,%d,%d", nodes.front().Y, nodes.front().X, AllDirections.front(), nodes.back().Y, nodes.back().X);
    for (int i = 0; i < AllDirections.size(); ++i)
    {
        ppmfc::CString direc;
        direc.Format(",%d", AllDirections[i]);
        value += direc;
    }
    value += ",-1";
    CINI::CurrentDocument->WriteString("Tubes", key, value);

    if (addReverse)
    {
        key = CINI::GetAvailableKey("Tubes");
        value.Format("%d,%d,%d,%d,%d", nodes.back().Y, nodes.back().X, GetOppositeFacing(AllDirections.back()), nodes.front().Y, nodes.front().X);
        for (int i = AllDirections.size() - 1; i >= 0; --i)
        {
            ppmfc::CString direc;
            direc.Format(",%d", GetOppositeFacing(AllDirections[i]));
            value += direc;
        }
        value += ",-1";
        CINI::CurrentDocument->WriteString("Tubes", key, value);
    }

    CMapData::Instance->UpdateFieldTubeData(false);
}

void CIsoViewExt::DrawLockedCellOutlineX(int X, int Y, int W, int H, COLORREF color, COLORREF colorX, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc, bool onlyX)
{
    X += 3;
    Y += 1;
    if (lpDesc->lpSurface == nullptr)
        return;

    RECT rect = CIsoViewExt::GetScaledWindowRect();

    auto lPitch = lpDesc->lPitch;
    auto nBytesPerPixel = *(int*)0x72A8C0;

    auto pRGB = (ColorStruct*)&color;
    BGRStruct ddColor;
    ddColor.R = pRGB->red;
    ddColor.G = pRGB->green;
    ddColor.B = pRGB->blue;

    auto pRGB2 = (ColorStruct*)&colorX;
    BGRStruct ddColor2;
    ddColor2.R = pRGB2->red;
    ddColor2.G = pRGB2->green;
    ddColor2.B = pRGB2->blue;

    auto DrawLine = [lPitch, nBytesPerPixel, ddColor, lpDesc, &rect](int X1, int Y1, int X2, int Y2)
        {
            int color = *(int*)&ddColor;

            if (X1 > X2)
            {
                std::swap(X1, X2);
                std::swap(Y1, Y2);
            }

            int dx = X2 - X1;
            int dy = Y2 - Y1;

            auto ptr = (unsigned char*)lpDesc->lpSurface + lPitch * Y1 + X1 * nBytesPerPixel;

            if (dy == 0)
            {
                for (int i = 0; i <= dx; ++i)
                {
                    memcpy(ptr, &ddColor, nBytesPerPixel);
                    ptr += nBytesPerPixel;
                }
            }
            else if (dx == 0)
            {
                int pitch = lPitch;
                if (dy < 0)
                {
                    pitch = -pitch;
                    dy = -dy;
                }

                for (int i = 0; i <= dy; ++i)
                {
                    memcpy(ptr, &ddColor, nBytesPerPixel);
                    ptr += pitch;
                }
            }
            else
            {
                int pitch = lPitch;
                if (dy < 0)
                {
                    pitch = -pitch;
                    dy = -dy;
                }

                int dx2 = 2 * dx;
                int dy2 = 2 * dy;

                if (dx > dy)
                {
                    int delta = dy2 - dx;
                    for (int i = 0; i < dx; ++i)
                    {
                        memcpy(ptr + i * nBytesPerPixel, &ddColor, nBytesPerPixel);
                        if (delta > 0)
                        {
                            ptr += pitch;
                            delta -= dx2;
                        }
                        delta += dy2;
                    }
                }
                else
                {
                    int delta = dx2 - dy;
                    int k = 0;

                    for (int i = 0; i < dy; ++i)
                    {
                        memcpy(ptr + k * nBytesPerPixel, &ddColor, nBytesPerPixel);
                        if (delta > 0)
                        {
                            ++k;
                            delta -= dy2;
                        }
                        delta += dx2;
                        ptr += pitch;
                    }
                }
            }
        };

    auto DrawLine2 = [lPitch, nBytesPerPixel, ddColor2, lpDesc, &rect](int X1, int Y1, int X2, int Y2)
        {
            int color = *(int*)&ddColor2;

            if (X1 > X2)
            {
                std::swap(X1, X2);
                std::swap(Y1, Y2);
            }

            int dx = X2 - X1;
            int dy = Y2 - Y1;

            auto ptr = (unsigned char*)lpDesc->lpSurface + lPitch * Y1 + X1 * nBytesPerPixel;

            if (dy == 0)
            {
                for (int i = 0; i <= dx; ++i)
                {
                    memcpy(ptr, &ddColor2, nBytesPerPixel);
                    ptr += nBytesPerPixel;
                }
            }
            else if (dx == 0)
            {
                int pitch = lPitch;
                if (dy < 0)
                {
                    pitch = -pitch;
                    dy = -dy;
                }

                for (int i = 0; i <= dy; ++i)
                {
                    memcpy(ptr, &ddColor2, nBytesPerPixel);
                    ptr += pitch;
                }
            }
            else
            {
                int pitch = lPitch;
                if (dy < 0)
                {
                    pitch = -pitch;
                    dy = -dy;
                }

                int dx2 = 2 * dx;
                int dy2 = 2 * dy;

                if (dx > dy)
                {
                    int delta = dy2 - dx;
                    for (int i = 0; i < dx; ++i)
                    {
                        memcpy(ptr + i * nBytesPerPixel, &ddColor2, nBytesPerPixel);
                        if (delta > 0)
                        {
                            ptr += pitch;
                            delta -= dx2;
                        }
                        delta += dy2;
                    }
                }
                else
                {
                    int delta = dx2 - dy;
                    int k = 0;

                    for (int i = 0; i < dy; ++i)
                    {
                        memcpy(ptr + k * nBytesPerPixel, &ddColor2, nBytesPerPixel);
                        if (delta > 0)
                        {
                            ++k;
                            delta -= dy2;
                        }
                        delta += dx2;
                        ptr += pitch;
                    }
                }
            }
        };
    auto ClipAndDrawLine2 = [&rect, DrawLine2](int X1, int Y1, int X2, int Y2)
        {
            auto encode = [&rect](int x, int y)
                {
                    int c = 0;
                    if (x < rect.left) c = c | 0x1;
                    else if (x > rect.right) c = c | 0x2;
                    if (y > rect.bottom) c = c | 0x4;
                    else if (y < rect.top) c = c | 0x8;
                    return c;
                };
            auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
                {
                    int code1, code2, code;
                    int x = 0, y = 0;
                    code1 = encode(X1, Y1);
                    code2 = encode(X2, Y2);
                    while (code1 != 0 || code2 != 0)
                    {
                        if ((code1 & code2) != 0) return false;
                        code = code1;
                        if (code == 0) code = code2;
                        if ((0b1 & code) != 0)
                        {
                            x = rect.left;
                            y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                        }
                        else if ((0b10 & code) != 0)
                        {
                            x = rect.right;
                            y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                        }
                        else if ((0b100 & code) != 0)
                        {
                            y = rect.bottom;
                            x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                        }
                        else if ((0b1000 & code) != 0)
                        {
                            y = rect.top;
                            x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                        }
                        if (code == code1)
                        {
                            X1 = x;
                            Y1 = y;
                            code1 = encode(x, y);
                        }
                        else
                        {
                            X2 = x;
                            Y2 = y;
                            code2 = encode(x, y);
                        }
                    }
                    return true;
                };
            if (clip(X1, Y1, X2, Y2))
                DrawLine2(X1, Y1, X2, Y2);
        };
    auto ClipAndDrawLine = [&rect, DrawLine](int X1, int Y1, int X2, int Y2)
        {
            auto encode = [&rect](int x, int y)
                {
                    int c = 0;
                    if (x < rect.left) c = c | 0x1;
                    else if (x > rect.right) c = c | 0x2;
                    if (y > rect.bottom) c = c | 0x4;
                    else if (y < rect.top) c = c | 0x8;
                    return c;
                };
            auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
                {
                    int code1, code2, code;
                    int x = 0, y = 0;
                    code1 = encode(X1, Y1);
                    code2 = encode(X2, Y2);
                    while (code1 != 0 || code2 != 0)
                    {
                        if ((code1 & code2) != 0) return false;
                        code = code1;
                        if (code == 0) code = code2;
                        if ((0b1 & code) != 0)
                        {
                            x = rect.left;
                            y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                        }
                        else if ((0b10 & code) != 0)
                        {
                            x = rect.right;
                            y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                        }
                        else if ((0b100 & code) != 0)
                        {
                            y = rect.bottom;
                            x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                        }
                        else if ((0b1000 & code) != 0)
                        {
                            y = rect.top;
                            x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                        }
                        if (code == code1)
                        {
                            X1 = x;
                            Y1 = y;
                            code1 = encode(x, y);
                        }
                        else
                        {
                            X2 = x;
                            Y2 = y;
                            code2 = encode(x, y);
                        }
                    }
                    return true;
                };
            if (clip(X1, Y1, X2, Y2))
                DrawLine(X1, Y1, X2, Y2);
        };

    int halfCellWidth = 30 * W;
    int quaterCellWidth = 15 * W;
    int fullCellHeight = 30 * H;
    int halfCellHeight = 15 * H;

    int y1 = Y - 30;
    int x1 = X + 30;

    int x2 = halfCellWidth + X + 30 - 2;
    int y2 = quaterCellWidth + y1 - 1;

    int x3 = halfCellWidth - fullCellHeight + X + 29;
    int y3 = halfCellHeight + quaterCellWidth + y1 - 1;

    int x4 = X - fullCellHeight + 29;
    int y4 = halfCellHeight + y1 - 1;

    y1 -= 1;
    x1 -= 1;
    int x1L = x1 + 1;
    int x3L = x3 + 1;
    int y1L = y1 - 1;
    int y3L = y3 + 1;

    int x4B = x4;
    int y4B = y4;
    int x2T = x2 + 2;
    int y2T = y2 + 1;
    x4 -= 1;

    //   1
    //  # #
    // 4   2
    //  # #
    //   3

    auto drawCellOutline = [&](int inneroffset)
        {
            ClipAndDrawLine(x1, y1 + inneroffset, x1, y3 - inneroffset);
            ClipAndDrawLine(x1 - 1, y1 + inneroffset, x1 - 1, y3 - inneroffset);
            ClipAndDrawLine(x4 + 2 * inneroffset, y4, x2 - 2 * inneroffset, y4);

            ClipAndDrawLine(x1, y1 + inneroffset, x2T - 2 * inneroffset, y2T);
            ClipAndDrawLine(x2 - 2 * inneroffset, y2, x3, y3 - inneroffset);
            ClipAndDrawLine(x3L, y3L - inneroffset, x4B + 2 * inneroffset, y4B);
            ClipAndDrawLine(x4 + 2 * inneroffset, y4, x1L, y1L + inneroffset);
        };
    

    auto drawCellOutline2 = [&](int inneroffset)
        {
            ClipAndDrawLine2(x1 + 1, y1 + 1 + inneroffset, x1 + 1, y3 - inneroffset + 1);
            ClipAndDrawLine2(x1 + 1 - 1, y1 + 1 + inneroffset, x1 + 1 - 1, y3 - inneroffset + 1);
            ClipAndDrawLine2(x4 + 1 + 2 * inneroffset, y4 + 1, x2 + 1 - 2 * inneroffset, y4 + 1);

            ClipAndDrawLine2(x1 + 1, y1 + 1 + inneroffset, x2T + 1 - 2 * inneroffset, y2T + 1);
            ClipAndDrawLine2(x2 + 1 - 2 * inneroffset, y2 + 1, x3 + 1, y3 - inneroffset + 1);
            ClipAndDrawLine2(x3L + 1, y3L + 1 - inneroffset, x4B + 1 + 2 * inneroffset, y4B + 1);
            ClipAndDrawLine2(x4 + 1 + 2 * inneroffset, y4 + 1, x1L + 1, y1L + inneroffset + 1);
        };

    if (onlyX)
    {
        ClipAndDrawLine(x1, y1, x1, y3);
        ClipAndDrawLine(x4, y4, x2 , y4);
    }
    else
    {
        drawCellOutline2(0);
        drawCellOutline2(-1);

        drawCellOutline(0);
        drawCellOutline(-1);
    }
}

void CIsoViewExt::DrawLockedCellOutline(int X, int Y, int W, int H, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc, bool s1, bool s2, bool s3, bool s4)
{   
    X += 2;
    Y += 1;
    if (lpDesc->lpSurface == nullptr)
        return;

    RECT rect = CIsoViewExt::GetScaledWindowRect();

    auto lPitch = lpDesc->lPitch;
    auto nBytesPerPixel = *(int*)0x72A8C0;

    auto pRGB = (ColorStruct*)&color;
    BGRStruct ddColor;
    ddColor.R = pRGB->red;
    ddColor.G = pRGB->green;
    ddColor.B = pRGB->blue;

    auto DrawLine = [lPitch, nBytesPerPixel, ddColor, lpDesc, &rect](int X1, int Y1, int X2, int Y2)
    {
        int color = *(int*)&ddColor;

        if (X1 > X2) 
        {
            std::swap(X1, X2);
            std::swap(Y1, Y2);
        }

        int dx = X2 - X1;
        int dy = Y2 - Y1;

        auto ptr = (unsigned char*)lpDesc->lpSurface + lPitch * Y1 + X1 * nBytesPerPixel;

        if (dy == 0) 
        {
            for (int i = 0; i <= dx; ++i) 
            {
                memcpy(ptr, &ddColor, nBytesPerPixel);
                ptr += nBytesPerPixel;
            }
        }
        else if (dx == 0) 
        {
            int pitch = lPitch;
            if (dy < 0)
            {
                pitch = -pitch;
                dy = -dy;
            }

            for (int i = 0; i <= dy; ++i) 
            {
                memcpy(ptr, &ddColor, nBytesPerPixel);
                ptr += pitch;
            }
        }
        else 
        {
            int pitch = lPitch;
            if (dy < 0)
            {
                pitch = -pitch;
                dy = -dy;
            }

            int dx2 = 2 * dx;
            int dy2 = 2 * dy;

            if (dx > dy) 
            {
                int delta = dy2 - dx;
                for (int i = 0; i < dx; ++i) 
                {
                    memcpy(ptr + i * nBytesPerPixel, &ddColor, nBytesPerPixel);
                    if (delta > 0)
                    {
                        ptr += pitch;
                        delta -= dx2;
                    }
                    delta += dy2;
                }
            }
            else 
            {
                int delta = dx2 - dy;
                int k = 0;

                for (int i = 0; i < dy; ++i)
                {
                    memcpy(ptr + k * nBytesPerPixel, &ddColor, nBytesPerPixel);
                    if (delta > 0) 
                    {
                        ++k;
                        delta -= dy2;
                    }
                    delta += dx2;
                    ptr += pitch;
                }
            }
        }
    };
    auto ClipAndDrawLine = [&rect, DrawLine](int X1, int Y1, int X2, int Y2)
    {
        auto encode = [&rect](int x, int y)
        {
            int c = 0;
            if (x < rect.left) c = c | 0x1;
            else if (x > rect.right) c = c | 0x2;
            if (y > rect.bottom) c = c | 0x4;
            else if (y < rect.top) c = c | 0x8;
            return c;
        };
        auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
        {
            int code1, code2, code;
            int x = 0, y = 0;
            code1 = encode(X1, Y1);
            code2 = encode(X2, Y2);
            while (code1 != 0 || code2 != 0)
            {
                if ((code1 & code2) != 0) return false;
                code = code1;
                if (code == 0) code = code2;
                if ((0b1 & code) != 0)
                {
                    x = rect.left;
                    y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                }
                else if ((0b10 & code) != 0)
                {
                    x = rect.right;
                    y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                }
                else if ((0b100 & code) != 0)
                {
                    y = rect.bottom;
                    x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                }
                else if ((0b1000 & code) != 0)
                {
                    y = rect.top;
                    x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                }
                if (code == code1) 
                {
                    X1 = x;
                    Y1 = y;
                    code1 = encode(x, y);
                }
                else 
                {
                    X2 = x;
                    Y2 = y;
                    code2 = encode(x, y);
                }
            }
            return true;
        };
        if (clip(X1, Y1, X2, Y2))
            DrawLine(X1, Y1, X2, Y2);
    };

    int halfCellWidth = 30 * W;
    int quaterCellWidth = 15 * W;
    int fullCellHeight = 30 * H;
    int halfCellHeight = 15 * H;

    int y1 = Y - 30;
    int x1 = X + 30;

    int x2 = halfCellWidth + X + 30 - 2;
    int y2 = quaterCellWidth + y1 - 1;

    int x3 = halfCellWidth - fullCellHeight + X + 29;
    int y3 = halfCellHeight + quaterCellWidth + y1 - 1;

    int x4 = X - fullCellHeight + 29;
    int y4 = halfCellHeight + y1 - 1;

    y1 -= 1;
    x1 -= 1;
    int x1L = x1 + 1;
    int x3L = x3 + 1;
    int y1L = y1 - 1;
    int y3L = y3 + 1;

    int x4B = x4;
    int y4B = y4;
    int x2T = x2 + 2;
    int y2T = y2 + 1;
    x4 -= 1;

    //   1
    //  # #
    // 4   2
    //  # #
    //   3

    auto drawCellOutline = [&](int inneroffset)
        {   
            if (s1)
            ClipAndDrawLine(x1, y1 + inneroffset, x2T - 2 * inneroffset, y2T);
            if (s2)
            ClipAndDrawLine(x2 - 2 * inneroffset, y2, x3, y3 - inneroffset);
            if (s3)
            ClipAndDrawLine(x3L, y3L - inneroffset, x4B + 2 * inneroffset, y4B);
            if (s4)
            ClipAndDrawLine(x4 + 2 * inneroffset, y4, x1L, y1L + inneroffset);
        };


    drawCellOutline(0);

    
    // thicker
    if (!bUseDot)
    {
        drawCellOutline(2);
        drawCellOutline(1);
    }

}

void CIsoViewExt::DrawLockedCellOutlinePaintCursor(int X, int Y, int height, COLORREF color, HDC hdc, HWND hwnd, bool useHeightColor)
{   
    X += 6 / CIsoViewExt::ScaledFactor - 6 + 2;
    Y += 3 / CIsoViewExt::ScaledFactor - 3 + 1;
    if (!hdc)
        return;
    if (!hwnd)
        return;

    CRect rect;
    auto pThis = CIsoView::GetInstance();
    pThis->GetWindowRect(&rect);

    COLORREF heightColor = color;
    if (useHeightColor)
    {
        heightColor = CIsoViewExt::CellHilightColors[height];
    }

    auto DrawLine = [hwnd, hdc, &color](int X1, int Y1, int X2, int Y2, int dashLen, int gapLen)
        {
            float dx = static_cast<float>(X2 - X1);
            float dy = static_cast<float>(Y2 - Y1);
            float lineLength = std::sqrt(dx * dx + dy * dy);

            float ux = dx / lineLength;
            float uy = dy / lineLength;

            float totalDrawn = 0.0f;

            HPEN hPen = CreatePen(PS_SOLID, 1, color);
            HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);

            while (totalDrawn < lineLength)
            {
                float startX = X1 + ux * totalDrawn;
                float startY = Y1 + uy * totalDrawn;

                float drawLength = std::min((float)dashLen, lineLength - totalDrawn);
                float endX = startX + ux * drawLength;
                float endY = startY + uy * drawLength;

                MoveToEx(hdc, static_cast<int>(startX + 0.5f), static_cast<int>(startY + 0.5f), NULL);
                LineTo(hdc, static_cast<int>(endX + 0.5f), static_cast<int>(endY + 0.5f));

                totalDrawn += drawLength + gapLen;
            }

            SelectObject(hdc, hOldPen);
            DeleteObject(hPen);
        };
    auto DrawLineInner = [hwnd, heightColor, hdc, &rect](int X1, int Y1, int X2, int Y2)
    {
        PAINTSTRUCT ps;
        HPEN hPen;
        HPEN hPenOld;
        BeginPaint(hwnd, &ps);
        hPen = CreatePen(PS_SOLID, 0, heightColor);
        hPenOld = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, X1, Y1, NULL);
        LineTo(hdc, X2, Y2);
        SelectObject(hdc, hPenOld);
        DeleteObject(hPen);
        EndPaint(hwnd, &ps);
        
    };
    auto ClipAndDrawLine = [&rect, DrawLine, DrawLineInner](int X1, int Y1, int X2, int Y2, int type)
    {
        auto encode = [&rect](int x, int y)
        {
            int c = 0;
            if (x < rect.left) c = c | 0x1;
            else if (x > rect.right) c = c | 0x2;
            if (y > rect.bottom) c = c | 0x4;
            else if (y < rect.top) c = c | 0x8;
            return c;
        };
        auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
        {
            int code1, code2, code;
            int x = 0, y = 0;
            code1 = encode(X1, Y1);
            code2 = encode(X2, Y2);
            while (code1 != 0 || code2 != 0)
            {
                if ((code1 & code2) != 0) return false;
                code = code1;
                if (code == 0) code = code2;
                if ((0b1 & code) != 0)
                {
                    x = rect.left;
                    y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                }
                else if ((0b10 & code) != 0)
                {
                    x = rect.right;
                    y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                }
                else if ((0b100 & code) != 0)
                {
                    y = rect.bottom;
                    x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                }
                else if ((0b1000 & code) != 0)
                {
                    y = rect.top;
                    x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                }
                if (code == code1) 
                {
                    X1 = x;
                    Y1 = y;
                    code1 = encode(x, y);
                }
                else 
                {
                    X2 = x;
                    Y2 = y;
                    code2 = encode(x, y);
                }
            }
            return true;
        };
        if (clip(X1, Y1, X2, Y2))
        {
            if (type == 0)
                DrawLine(X1, Y1, X2, Y2, 1000, 0);
            else if (type == 1)
                DrawLineInner(X1, Y1, X2, Y2);
            else if (type == 2)
                DrawLine(X1, Y1, X2, Y2, std::max(4 / CIsoViewExt::ScaledFactor, 1.0), std::max(2 / CIsoViewExt::ScaledFactor, 1.0));
        }
           
    };

    double halfCellWidth = 30 / CIsoViewExt::ScaledFactor;
    double quaterCellWidth = 15 / CIsoViewExt::ScaledFactor;
    double fullCellHeight = 30 / CIsoViewExt::ScaledFactor;
    double halfCellHeight = 15 / CIsoViewExt::ScaledFactor;

    double y1 = Y - 30 / CIsoViewExt::ScaledFactor;
    double x1 = X + 30 / CIsoViewExt::ScaledFactor;

    double x2 = halfCellWidth + X + 30 / CIsoViewExt::ScaledFactor - 2;
    double y2 = quaterCellWidth + y1 - 1;

    double x3 = halfCellWidth - fullCellHeight + X + 30 / CIsoViewExt::ScaledFactor - 1;
    double y3 = halfCellHeight + quaterCellWidth + y1 - 1;

    double x4 = X - fullCellHeight + 30 / CIsoViewExt::ScaledFactor - 1;
    double y4 = halfCellHeight + y1 - 1;

    y1 -= 1;
    x1 -= 1;
    double x1L = x1 + 1;
    double x3L = x3 - 1;
    double y1L = y1 - 1;
    double y3L = y3;
    y3 += 1;
    x3 -= 2;
    double x4B = x4 - 2;
    double y4B = y4 - 1;
    double x2T = x2 + 2;
    double y2T = y2 + 1;

    //   1
    //  # #
    // 4   2
    //  # #
    //   3

    
    auto drawCellOutline = [&](int inneroffset, bool useheightcolor = false)
        {
            ClipAndDrawLine(x1, y1 + inneroffset, x2T - 2 * inneroffset, y2T, useheightcolor);
            ClipAndDrawLine(x2 - 2 * inneroffset, y2, x3, y3 - inneroffset, useheightcolor);
            ClipAndDrawLine(x3L, y3L - inneroffset, x4B + 2 * inneroffset, y4B, useheightcolor);
            ClipAndDrawLine(x4 + 2 * inneroffset, y4, x1L, y1L + inneroffset, useheightcolor);
        };
    drawCellOutline(0);
    drawCellOutline(1,true);
    if (CIsoViewExt::ScaledFactor < 0.76)
        drawCellOutline(2, true);
    if (CIsoViewExt::ScaledFactor < 0.31)
        drawCellOutline(3, true);

    if (useHeightColor)
    {
        drawCellOutline(-1);
        if (CIsoViewExt::ScaledFactor < 0.6)
            drawCellOutline(-2);
        if (CIsoViewExt::ScaledFactor < 0.31)
            drawCellOutline(-3);
    }

    color = ExtConfigs::CursorSelectionBound_HeightColor;
    auto drawHeightLine = [&](int offset)
        {
            ClipAndDrawLine(x2 + offset, y2, x2 + offset, y2 + height * 15 / CIsoViewExt::ScaledFactor, 2);
            ClipAndDrawLine(x4 - offset, y4, x4 - offset, y4 + height * 15 / CIsoViewExt::ScaledFactor, 2);
            ClipAndDrawLine(x3 + offset + 1, y3, x3 + offset + 1, y3 + height * 15 / CIsoViewExt::ScaledFactor, 2);
        };

    if (!CFinalSunApp::Instance->FlatToGround)
    {
        drawHeightLine(0);
        if (CIsoViewExt::ScaledFactor < 0.76)
            drawHeightLine(1);
        if (CIsoViewExt::ScaledFactor < 0.31)
            drawHeightLine(-1);
    }
}

void CIsoViewExt::DrawLockedCellOutlinePaint(int X, int Y, int W, int H, COLORREF color, bool bUseDot, HDC hdc, HWND hwnd, bool s1, bool s2, bool s3, bool s4)
{   
    if (!s1 && !s2 && !s3 && !s4)
        return;

    X += 6 / CIsoViewExt::ScaledFactor - 6 + 2;
    Y += 3 / CIsoViewExt::ScaledFactor - 3 + 1;
    if (!hdc)
        return;
    if (!hwnd)
        return;

    CRect rect;
    auto pThis = CIsoView::GetInstance();
    pThis->GetWindowRect(&rect);

    auto DrawLine = [hwnd, color, hdc, &rect](int X1, int Y1, int X2, int Y2)
    {
        PAINTSTRUCT ps;
        HPEN hPen;
        HPEN hPenOld;
        BeginPaint(hwnd, &ps);
        hPen = CreatePen(PS_SOLID, 0, color);
        hPenOld = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, X1, Y1, NULL);
        LineTo(hdc, X2, Y2);
        SelectObject(hdc, hPenOld);
        DeleteObject(hPen);
        EndPaint(hwnd, &ps);
        
    };
    auto ClipAndDrawLine = [&rect, DrawLine](int X1, int Y1, int X2, int Y2)
    {
        auto encode = [&rect](int x, int y)
        {
            int c = 0;
            if (x < rect.left) c = c | 0x1;
            else if (x > rect.right) c = c | 0x2;
            if (y > rect.bottom) c = c | 0x4;
            else if (y < rect.top) c = c | 0x8;
            return c;
        };
        auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
        {
            int code1, code2, code;
            int x = 0, y = 0;
            code1 = encode(X1, Y1);
            code2 = encode(X2, Y2);
            while (code1 != 0 || code2 != 0)
            {
                if ((code1 & code2) != 0) return false;
                code = code1;
                if (code == 0) code = code2;
                if ((0b1 & code) != 0)
                {
                    x = rect.left;
                    y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                }
                else if ((0b10 & code) != 0)
                {
                    x = rect.right;
                    y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                }
                else if ((0b100 & code) != 0)
                {
                    y = rect.bottom;
                    x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                }
                else if ((0b1000 & code) != 0)
                {
                    y = rect.top;
                    x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                }
                if (code == code1) 
                {
                    X1 = x;
                    Y1 = y;
                    code1 = encode(x, y);
                }
                else 
                {
                    X2 = x;
                    Y2 = y;
                    code2 = encode(x, y);
                }
            }
            return true;
        };
        if (clip(X1, Y1, X2, Y2))
            DrawLine(X1, Y1, X2, Y2);
    };

    double halfCellWidth = 30 / CIsoViewExt::ScaledFactor * W;
    double quaterCellWidth = 15 / CIsoViewExt::ScaledFactor * W;
    double fullCellHeight = 30 / CIsoViewExt::ScaledFactor * H;
    double halfCellHeight = 15 / CIsoViewExt::ScaledFactor * H;

    double y1 = Y - 30 / CIsoViewExt::ScaledFactor;
    double x1 = X + 30 / CIsoViewExt::ScaledFactor;

    double x2 = halfCellWidth + X + 30 / CIsoViewExt::ScaledFactor - 2;
    double y2 = quaterCellWidth + y1 - 1;

    double x3 = halfCellWidth - fullCellHeight + X + 30 / CIsoViewExt::ScaledFactor - 1;
    double y3 = halfCellHeight + quaterCellWidth + y1 - 1;

    double x4 = X - fullCellHeight + 30 / CIsoViewExt::ScaledFactor - 1;
    double y4 = halfCellHeight + y1 - 1;

    y1 -= 1;
    x1 -= 1;
    double x1L = x1 + 1;
    double x3L = x3 - 1;
    double y1L = y1 - 1;
    double y3L = y3;
    y3 += 1;
    x3 -= 2;
    double x4B = x4 - 2;
    double y4B = y4 - 1;
    double x2T = x2 + 2;
    double y2T = y2 + 1;

    //   1
    //  # #
    // 4   2
    //  # #
    //   3

    auto drawCellOutline = [&](int inneroffset)
        {
            if (s1)
            ClipAndDrawLine(x1, y1 + inneroffset, x2T - 2 * inneroffset, y2T);
            if (s2)
            ClipAndDrawLine(x2 - 2 * inneroffset, y2, x3, y3 - inneroffset);
            if (s3)
            ClipAndDrawLine(x3L, y3L - inneroffset, x4B + 2 * inneroffset, y4B);
            if (s4)
            ClipAndDrawLine(x4 + 2 * inneroffset, y4, x1L, y1L + inneroffset);
        };

    drawCellOutline(0);
    drawCellOutline(-1);
    if (CIsoViewExt::ScaledFactor < 0.6)
        drawCellOutline(-2);
    if (CIsoViewExt::ScaledFactor < 0.31)
        drawCellOutline(-3);

}

void CIsoViewExt::DrawLine(int x1, int y1, int x2, int y2, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc, bool bDashed)
{
    if (lpDesc->lpSurface == nullptr)
        return;

    RECT rect = CIsoViewExt::GetScaledWindowRect();

    auto lPitch = lpDesc->lPitch;
    auto nBytesPerPixel = *(int*)0x72A8C0;

    auto pRGB = (ColorStruct*)&color;
    BGRStruct ddColor;
    ddColor.R = pRGB->red;
    ddColor.G = pRGB->green;
    ddColor.B = pRGB->blue;

    auto DrawLine = [lPitch, nBytesPerPixel, ddColor, lpDesc, &rect, bDashed](int X1, int Y1, int X2, int Y2)
        {
            int color = *(int*)&ddColor;

            if (X1 > X2)
            {
                std::swap(X1, X2);
                std::swap(Y1, Y2);
            }

            int dx = X2 - X1;
            int dy = Y2 - Y1;

            auto ptr = (unsigned char*)lpDesc->lpSurface + lPitch * Y1 + X1 * nBytesPerPixel;

            const int dashOn = 3;
            const int dashOff = 3;
            const int dashPeriod = dashOn + dashOff;

            auto shouldDraw = [=](int step) {
                return !bDashed || (step % dashPeriod) < dashOn;
                };

            if (dy == 0)
            {
                for (int i = 0; i <= dx; ++i)
                {
                    if (shouldDraw(i))
                        memcpy(ptr, &ddColor, nBytesPerPixel);
                    ptr += nBytesPerPixel;
                }
            }
            else if (dx == 0)
            {
                int pitch = lPitch;
                if (dy < 0)
                {
                    pitch = -pitch;
                    dy = -dy;
                }

                for (int i = 0; i <= dy; ++i)
                {
                    if (shouldDraw(i))
                        memcpy(ptr, &ddColor, nBytesPerPixel);
                    ptr += pitch;
                }
            }
            else
            {
                int pitch = lPitch;
                if (dy < 0)
                {
                    pitch = -pitch;
                    dy = -dy;
                }

                int dx2 = 2 * dx;
                int dy2 = 2 * dy;

                if (dx > dy)
                {
                    int delta = dy2 - dx;
                    int yOffset = 0;
                    for (int i = 0; i <= dx; ++i)
                    {
                        if (shouldDraw(i))
                            memcpy(ptr + yOffset, &ddColor, nBytesPerPixel);
                        if (delta > 0)
                        {
                            yOffset += pitch;
                            delta -= dx2;
                        }
                        delta += dy2;
                        ptr += nBytesPerPixel;
                    }
                }
                else
                {
                    int delta = dx2 - dy;
                    int xOffset = 0;
                    for (int i = 0; i <= dy; ++i)
                    {
                        if (shouldDraw(i))
                            memcpy(ptr + xOffset * nBytesPerPixel, &ddColor, nBytesPerPixel);
                        if (delta > 0)
                        {
                            ++xOffset;
                            delta -= dy2;
                        }
                        delta += dx2;
                        ptr += pitch;
                    }
                }
            }
        };
    auto ClipAndDrawLine = [&rect, DrawLine](int X1, int Y1, int X2, int Y2)
        {
            auto encode = [&rect](int x, int y)
                {
                    int c = 0;
                    if (x < rect.left) c = c | 0x1;
                    else if (x > rect.right) c = c | 0x2;
                    if (y > rect.bottom) c = c | 0x4;
                    else if (y < rect.top) c = c | 0x8;
                    return c;
                };
            auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
                {
                    int code1, code2, code;
                    int x = 0, y = 0;
                    code1 = encode(X1, Y1);
                    code2 = encode(X2, Y2);
                    while (code1 != 0 || code2 != 0)
                    {
                        if ((code1 & code2) != 0) return false;
                        code = code1;
                        if (code == 0) code = code2;
                        if ((0b1 & code) != 0)
                        {
                            x = rect.left;
                            y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                        }
                        else if ((0b10 & code) != 0)
                        {
                            x = rect.right;
                            y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                        }
                        else if ((0b100 & code) != 0)
                        {
                            y = rect.bottom;
                            x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                        }
                        else if ((0b1000 & code) != 0)
                        {
                            y = rect.top;
                            x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                        }
                        if (code == code1)
                        {
                            X1 = x;
                            Y1 = y;
                            code1 = encode(x, y);
                        }
                        else
                        {
                            X2 = x;
                            Y2 = y;
                            code2 = encode(x, y);
                        }
                    }
                    return true;
                };
            if (clip(X1, Y1, X2, Y2))
            {



                DrawLine(X1, Y1, X2, Y2);
            }
                
        };

    ClipAndDrawLine(x1, y1, x2, y2);
}

void CIsoViewExt::DrawLockedLines(const std::vector<std::pair<MapCoord, MapCoord>>& lines, int X, int Y, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc)
{
    if (lpDesc->lpSurface == nullptr)
        return;

    RECT rect = CIsoViewExt::GetScaledWindowRect();

    auto lPitch = lpDesc->lPitch;
    auto nBytesPerPixel = *(int*)0x72A8C0;

    auto pRGB = (ColorStruct*)&color;
    BGRStruct ddColor;
    ddColor.R = pRGB->red;
    ddColor.G = pRGB->green;
    ddColor.B = pRGB->blue;

    auto DrawLine = [lPitch, nBytesPerPixel, ddColor, lpDesc, &rect](int X1, int Y1, int X2, int Y2)
    {
        int color = *(int*)&ddColor;

        if (X1 > X2)
        {
            std::swap(X1, X2);
            std::swap(Y1, Y2);
        }

        int dx = X2 - X1;
        int dy = Y2 - Y1;

        auto ptr = (unsigned char*)lpDesc->lpSurface + lPitch * Y1 + X1 * nBytesPerPixel;

        if (dy == 0)
        {
            for (int i = 0; i <= dx; ++i)
            {
                memcpy(ptr, &ddColor, nBytesPerPixel);
                ptr += nBytesPerPixel;
            }
        }
        else if (dx == 0)
        {
            int pitch = lPitch;
            if (dy < 0)
            {
                pitch = -pitch;
                dy = -dy;
            }

            for (int i = 0; i <= dy; ++i)
            {
                memcpy(ptr, &ddColor, nBytesPerPixel);
                ptr += pitch;
            }
        }
        else
        {
            int pitch = lPitch;
            if (dy < 0)
            {
                pitch = -pitch;
                dy = -dy;
            }

            int dx2 = 2 * dx;
            int dy2 = 2 * dy;

            if (dx > dy)
            {
                int delta = dy2 - dx;
                for (int i = 0; i < dx; ++i)
                {
                    memcpy(ptr + i * nBytesPerPixel, &ddColor, nBytesPerPixel);
                    if (delta > 0)
                    {
                        ptr += pitch;
                        delta -= dx2;
                    }
                    delta += dy2;
                }
            }
            else
            {
                int delta = dx2 - dy;
                int k = 0;

                for (int i = 0; i < dy; ++i)
                {
                    memcpy(ptr + k * nBytesPerPixel, &ddColor, nBytesPerPixel);
                    if (delta > 0)
                    {
                        ++k;
                        delta -= dy2;
                    }
                    delta += dx2;
                    ptr += pitch;
                }
            }
        }
    };
    auto ClipAndDrawLine = [&rect, DrawLine](int X1, int Y1, int X2, int Y2)
    {
        auto encode = [&rect](int x, int y)
        {
            int c = 0;
            if (x < rect.left) c = c | 0x1;
            else if (x > rect.right) c = c | 0x2;
            if (y > rect.bottom) c = c | 0x4;
            else if (y < rect.top) c = c | 0x8;
            return c;
        };
        auto clip = [&rect, encode](int& X1, int& Y1, int& X2, int& Y2) -> bool
        {
            int code1, code2, code;
            int x = 0, y = 0;
            code1 = encode(X1, Y1);
            code2 = encode(X2, Y2);
            while (code1 != 0 || code2 != 0)
            {
                if ((code1 & code2) != 0) return false;
                code = code1;
                if (code == 0) code = code2;
                if ((0b1 & code) != 0)
                {
                    x = rect.left;
                    y = Y1 + (Y2 - Y1) * (rect.left - X1) / (X2 - X1);
                }
                else if ((0b10 & code) != 0)
                {
                    x = rect.right;
                    y = Y1 + (Y2 - Y1) * (rect.right - X1) / (X2 - X1);
                }
                else if ((0b100 & code) != 0)
                {
                    y = rect.bottom;
                    x = X1 + (X2 - X1) * (rect.bottom - Y1) / (Y2 - Y1);
                }
                else if ((0b1000 & code) != 0)
                {
                    y = rect.top;
                    x = X1 + (X2 - X1) * (rect.top - Y1) / (Y2 - Y1);
                }
                if (code == code1)
                {
                    X1 = x;
                    Y1 = y;
                    code1 = encode(x, y);
                }
                else
                {
                    X2 = x;
                    Y2 = y;
                    code2 = encode(x, y);
                }
            }
            return true;
        };
        if (clip(X1, Y1, X2, Y2))
            DrawLine(X1, Y1, X2, Y2);
    };

    Y -= 30;
    X += 30;
    for (const auto& line : lines)
    {
        int x1 = X + line.first.X;
        int y1 = Y + line.first.Y;
        int x2 = X + line.second.X;
        int y2 = Y + line.second.Y;
        ClipAndDrawLine(x1, y1, x2, y2);
        ClipAndDrawLine(x1, y1, x2, y2);
        if (!bUseDot)
        {
            ClipAndDrawLine(x1 + 1, y1, x2 + 1, y2);
            ClipAndDrawLine(x1 - 1, y1, x2 - 1, y2);
            ClipAndDrawLine(x1 + 2, y1, x2 + 2, y2);
            ClipAndDrawLine(x1 - 2, y1, x2 - 2, y2);
        }
    }
}

int CIsoViewExt::GetSelectedSubcellInfantryIdx(int X, int Y, bool getSubcel)
{
    auto pIsoView = reinterpret_cast<CFinalSunDlg*>(CFinalSunApp::Instance->m_pMainWnd)->MyViewFrame.pIsoView;
    auto currentMapCoord = pIsoView->StartCell;
    int pos;  
    if (X != -1 && Y != -1)
    {
        currentMapCoord.X = X;
        currentMapCoord.Y = Y;
        pos = CMapData::Instance().GetCoordIndex(X, Y);
    }

    else
        pos = CMapData::Instance().GetCoordIndex(currentMapCoord.X, currentMapCoord.Y);
    if (CMapDataExt::GetInfantryAt(pos) != -1 || getSubcel)
    {
        auto& mouse = pIsoView->MouseCurrentPosition;

        RECT rect;
        pIsoView->GetWindowRect(&rect);
        int mouseX = mouse.x + rect.left + pIsoView->ViewPosition.x;
        int mouseY = mouse.y + rect.top + pIsoView->ViewPosition.y;

        int cX = currentMapCoord.X, cY = currentMapCoord.Y;
        CIsoViewExt::MapCoord2ScreenCoord(cX, cY);
        int CellCenterX = cX + 36 / CIsoViewExt::ScaledFactor;
        int CellCenterY = cY - 12 / CIsoViewExt::ScaledFactor;
        ppmfc::CString tmp;

        auto getSubcellInf = [&](int subpos)
            {
                for (int i = 0; i < 3; i++)
                {
                    int idx = CMapData::Instance->CellDatas[pos].Infantry[i];
                    if (idx != -1)
                    {
                        CInfantryData infData;
                        CMapData::Instance->GetInfantryData(idx, infData);
                        if (atoi(infData.SubCell) == subpos)
                            return idx;
                    }
                }
                return -1;
            };

        int count = 0;
        for (int i = 0; i < 3; i++)
        {
            int idx = CMapData::Instance->CellDatas[pos].Infantry[i];
            if (idx != -1)
            {
                count++;
            }
        }
        if (count == 1 && !ExtConfigs::InfantrySubCell_Edit_Single && !getSubcel)
        {
             return CMapDataExt::GetInfantryAt(pos);
        }
        else
        {
            int xDistance = (mouseX - CellCenterX) * CIsoViewExt::ScaledFactor;
            if (xDistance >= -10 && xDistance <= 6)
            {
                if (getSubcel)
                {
                    if (ExtConfigs::InfantrySubCell_Edit_FixCenter && ExtConfigs::InfantrySubCell_GameDefault)
                        return 4;
                    else if (ExtConfigs::InfantrySubCell_Edit_FixCenter && !ExtConfigs::InfantrySubCell_GameDefault)
                        return 1;
                    else
                    {
                        if (mouseY - CellCenterY > 2)
                            return 4;
                        return 1;
                    }
                }
                     
                int idx = getSubcellInf(4);
                if (idx == -1)
                    idx = getSubcellInf(0);
                if (idx == -1)
                    idx = getSubcellInf(1);
                if (idx != -1)
                    return idx;
            }
            else if (xDistance < -10)
            {
                if (getSubcel)
                    return 3;
                int idx = getSubcellInf(3);
                if (idx != -1)
                    return idx;
            }
            else if (xDistance > 6)
            {
                if (getSubcel)
                    return 2;
                int idx = getSubcellInf(2);
                if (idx != -1)
                    return idx;
            }
        }
    }
    return -1;
}

void CIsoViewExt::DrawBitmap(ppmfc::CString filename, int X, int Y, LPDDSURFACEDESC2 lpDesc)
{
    this->BlitTransparentDesc(CLoadingExt::GetSurfaceImageDataFromMap(filename + ".bmp")->lpSurface, this->lpDDBackBufferSurface, lpDesc, X, Y, -1, -1);
}

void CIsoViewExt::DrawCelltag(int X, int Y, LPDDSURFACEDESC2 lpDesc)
{
    auto image = CLoadingExt::GetSurfaceImageDataFromMap("CELLTAG");
    this->BlitTransparentDesc(image->lpSurface, this->lpDDBackBufferSurface, lpDesc, X + 25 - image->FullWidth / 2, Y + 12 - image->FullHeight / 2, -1, -1);
}

void CIsoViewExt::DrawWaypointFlag(int X, int Y, LPDDSURFACEDESC2 lpDesc)
{
    auto image = CLoadingExt::GetSurfaceImageDataFromMap("FLAG");
    this->BlitTransparentDesc(image->lpSurface, this->lpDDBackBufferSurface, lpDesc, X + 5 + 25 - image->FullWidth / 2, Y + 12 - image->FullHeight / 2, -1, -1);
}

void CIsoViewExt::FillArea(int X, int Y, int ID, int Subtile, int oriX, int oriY, std::set<MapCoord>* selectedCoords)
{
    bool isFirstRun = false;
    std::unique_ptr<std::set<MapCoord>> recordCoords;
    if (!selectedCoords)
    {
        recordCoords = std::make_unique<std::set<MapCoord>>();
        selectedCoords = recordCoords.get();
        isFirstRun = true;
    }

    auto& map = CMapData::Instance;
    
    if (!map->IsCoordInMap(X, Y))
        return;
    auto cell = map->GetCellAt(X, Y);
    cell->Flag.NotAValidCell = TRUE;

    if (cell->IsHidden())
        return;

    if (MultiSelection::SelectedCoords.size() > 0 && MultiSelection::IsSelected(oriX, oriY)) {
        bool skip = true;
        for (const auto& coord : MultiSelection::SelectedCoords) {
            if (coord.X == X && coord.Y == Y) {
                skip = false;
                break;
            }
        }
        if (skip)
            return;
    }

    int tileIndex_cell = CMapDataExt::GetSafeTileIndex(cell->TileIndex);

    int mapwidth, mapheight;
    mapwidth = map->Size.Width;
    mapheight = map->Size.Height;

    //const int BlockWaters[3] = { 6 , 7, 13 };
    int iWaterSet = CINI::CurrentTheater->GetInteger("General", "WaterSet", -1);

    int i, e;
    for (i = -1; i < 2; i++)
    {
        for (e = -1; e < 2; e++)
        {
            if (abs(i) == abs(e)) continue;
            int cur_x, cur_y;
            cur_x = X + i;
            cur_y = Y + e;

            if (!map->IsCoordInMap(cur_x, cur_y))
                continue;

            auto cell2 = map->TryGetCellAt(cur_x, cur_y);

            if (cell2->Flag.NotAValidCell) continue;

            bool match = false;
            int tileIndex_cell2 = CMapDataExt::GetSafeTileIndex(cell2->TileIndex);

            match = tileIndex_cell2 == tileIndex_cell && cell2->TileSubIndex == cell->TileSubIndex;
            if (ExtConfigs::FillArea_ConsiderLAT && !match)
            {
                for (auto& latPair : CMapDataExt::Tile_to_lat)
                {
                    int iSmoothSet = latPair[0];
                    int iLatSet = latPair[1];

                    if (iLatSet >= 0 && iSmoothSet >= 0 && iSmoothSet < CMapDataExt::TileSet_starts.size() && iLatSet < CMapDataExt::TileSet_starts.size() &&
                        (CMapDataExt::TileData[tileIndex_cell2].TileSet == iSmoothSet || CMapDataExt::TileData[tileIndex_cell2].TileSet == iLatSet) &&
                        (CMapDataExt::TileData[tileIndex_cell].TileSet == iSmoothSet || CMapDataExt::TileData[tileIndex_cell].TileSet == iLatSet))
                    {
                        if (cell2->TileSubIndex == cell->TileSubIndex) {
                            match = true;
                            break;
                        }
                    }
                }

            }
            if (ExtConfigs::FillArea_ConsiderWater && !match)
            {
                if (CMapDataExt::TileData[tileIndex_cell2].TileSet == iWaterSet && CMapDataExt::TileData[tileIndex_cell].TileSet == iWaterSet)
                {
                    //bool notWaterBlock = true;
                    //for (int bw : BlockWaters)
                    //{
                    //    if (bw == tileIndex_cell2 - CMapDataExt::TileSet_starts[iWaterSet] || bw == tileIndex_cell - CMapDataExt::TileSet_starts[iWaterSet])
                    //        notWaterBlock = false;
                    //}
                    //if (notWaterBlock)
                        match = true;
                }
            }


            if (tileIndex_cell2 != ID && match)
            {
                FillArea(cur_x, cur_y, ID, Subtile, oriX, oriY, selectedCoords);
            }

        }
    }
    selectedCoords->insert(MapCoord{ X,Y });
    if (isFirstRun)
    {
        if (ID >= 0 && ID < CMapDataExt::TileDataCount)
        {
            std::vector<TilePlacement> placements;
            auto& tile = CMapDataExt::TileData[ID];
            int tileOriginX = oriY - (tile.Width - 1);
            int tileOriginY = oriX - (tile.Height - 1);
            for (const auto& coord : *selectedCoords)
            {
                int localY = ((coord.Y - tileOriginX) % tile.Width + tile.Width) % tile.Width;
                int localX = ((coord.X - tileOriginY) % tile.Height + tile.Height) % tile.Height;

                int subtileIndex = localY + localX * tile.Width;

                placements.push_back(TilePlacement{
                    (short)coord.X,
                    (short)coord.Y,
                    (short)subtileIndex
                    });
            }

            for (auto& coord : placements)
            {
                if (tile.TileBlockDatas[coord.SubtileIndex].ImageData != NULL)
                {
                    bool isBridge = (tile.TileSet == CMapDataExt::BridgeSet || tile.TileSet == CMapDataExt::WoodBridgeSet);
                    auto cell = CMapData::Instance->GetCellAt(coord.X, coord.Y);
                    cell->TileIndex = ID;
                    cell->TileSubIndex = coord.SubtileIndex;
                    cell->Flag.AltIndex = isBridge ? 0 : STDHelpers::RandomSelectInt(0, tile.AltTypeCount + 1);
                    CMapDataExt::GetExtension()->SetHeightAt(coord.X, coord.Y, cell->Height + tile.TileBlockDatas[coord.SubtileIndex].Height);
                    CMapData::Instance->UpdateMapPreviewAt(coord.X, coord.Y);
                }
            }
        }
        selectedCoords->clear();
    } 
}

void CIsoViewExt::BlitText(const std::wstring& text, COLORREF textColor, COLORREF bgColor,
    CIsoView* pThis, void* dst, const RECT& window, const DDBoundary& boundary,
    int x, int y, int fontSize, BYTE alpha, bool bold)
{
    int bpp = 4;
    HDC hdcScreen = GetDC(NULL);
    HDC hdcMem = CreateCompatibleDC(hdcScreen);

    RECT textRect = { 0, 0, 1000, 1000 };
    HFONT hFont = CreateFontW(fontSize, 0, 0, 0, bold ? FW_BOLD : FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
        OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
        DEFAULT_PITCH | FF_DONTCARE, L"Cambria");
    HFONT oldFont = (HFONT)SelectObject(hdcMem, hFont);

    ::DrawTextW(hdcMem, text.c_str(), -1, &textRect, DT_CALCRECT | DT_LEFT | DT_TOP | DT_WORDBREAK);
    int swidth = textRect.right - textRect.left;
    int sheight = textRect.bottom - textRect.top;

    HBITMAP hBitmap = CreateCompatibleBitmap(hdcScreen, swidth, sheight);
    SelectObject(hdcMem, hBitmap);

    SetBkColor(hdcMem, bgColor);
    SetTextColor(hdcMem, textColor);
    SetBkMode(hdcMem, OPAQUE);

    swidth = std::min(swidth, 1000);
    sheight = std::min(sheight, 1000);

    RECT fillRect = { 0, 0, swidth, sheight };
    HBRUSH hBrush = CreateSolidBrush(bgColor);
    FillRect(hdcMem, &fillRect, hBrush);
    DeleteObject(hBrush);

    ::DrawTextW(hdcMem, text.c_str(), -1, &fillRect, DT_LEFT | DT_TOP | DT_WORDBREAK);

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = swidth;
    bmi.bmiHeader.biHeight = -sheight;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    auto tempPixels = new BYTE[swidth * sheight * bpp];
    GetDIBits(hdcMem, hBitmap, 0, sheight, tempPixels, &bmi, DIB_RGB_COLORS);

    int borderWidth = 3;
    int finalWidth = swidth + 2 * borderWidth;
    int finalHeight = sheight + 2 * borderWidth;

    auto src = new BGRStruct[finalWidth * finalHeight];

    BGRStruct bgbgr;
    bgbgr.B = (bgColor >> 16) & 0xFF;
    bgbgr.G = (bgColor >> 8) & 0xFF;
    bgbgr.R = bgColor & 0xFF;
    
    BGRStruct textbgr;
    textbgr.B = (textColor >> 16) & 0xFF;
    textbgr.G = (textColor >> 8) & 0xFF;
    textbgr.R = textColor & 0xFF;

    // bgColor border
    for (int y = 0; y < finalHeight; y++) {
        for (int x = 0; x < finalWidth; x++) {
            if (x <= 2 || x >= finalWidth - 3 || y <= 2 || y >= finalHeight - 3) {
                src[y * finalWidth + x] = bgbgr;
            }
        }
    }

    // textColor border
    for (int y = 0; y < finalHeight; y++) {
        for (int x = 0; x < finalWidth; x++) {
            if (x == 0 || x == finalWidth || y == 0 || y == finalHeight - 1) {
                src[y * finalWidth + x] = textbgr;
            }
        }
    }

    for (int y = 0; y < sheight; y++) {
        for (int x = 0; x < swidth; x++) {
            int srcIndex = (y + borderWidth) * finalWidth + (x + borderWidth);
            int tempIndex = (y * swidth + x) * 4;
            src[srcIndex] = BGRStruct(tempPixels[tempIndex + 0], // B
                tempPixels[tempIndex + 1], // G
                tempPixels[tempIndex + 2]);// R
        }
    }

    delete[] tempPixels;

    swidth = finalWidth;
    sheight = finalHeight;

    SelectObject(hdcMem, oldFont);
    DeleteObject(hFont);
    DeleteObject(hBitmap);
    DeleteDC(hdcMem);
    ReleaseDC(NULL, hdcScreen);

    if (src == NULL || dst == NULL) {
        delete[] src;
        return;
    }
    if (x + swidth < window.left || y + sheight < window.top) {
        delete[] src;
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        delete[] src;
        return;
    }

    RECT blrect;
    RECT srcRect;
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = swidth;
    srcRect.bottom = sheight;
    blrect.left = x;
    if (blrect.left < 0) {
        srcRect.left = 1 - blrect.left;
        //blrect.left=1;
    }
    blrect.top = y;
    if (blrect.top < 0) {
        srcRect.top = 1 - blrect.top;
        //blrect.top=1;
    }
    blrect.right = (x + swidth);
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        blrect.right = window.right;
    }
    blrect.bottom = (y + sheight);
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        blrect.bottom = window.bottom;
    }

    BGRStruct textBGR;
    auto pRGB = (ColorStruct*)&textColor;
    textBGR.R = pRGB->red;
    textBGR.G = pRGB->green;
    textBGR.B = pRGB->blue;

    int i, e;
    auto const surfaceEnd = (BYTE*)dst + boundary.dpitch * boundary.dwHeight;

    for (e = srcRect.top; e < srcRect.bottom; e++) {
        for (i = srcRect.left; i <= srcRect.right; i++) {
            if (blrect.left + i < 0) {
                continue;
            }
            
            const int spos = i + e * swidth;
            auto c = src[spos];
            auto dest = ((BYTE*)dst + (blrect.left + i) * bpp + (blrect.top + e) * boundary.dpitch);

            if (dest >= dst) {
                if (dest + bpp < surfaceEnd) {
                    if (alpha < 255 && c != textBGR)
                    {
                        BGRStruct oriColor = *(BGRStruct*)dest;
                        c.B = (c.B * alpha + oriColor.B * (255 - alpha)) / 255;
                        c.G = (c.G * alpha + oriColor.G * (255 - alpha)) / 255;
                        c.R = (c.R * alpha + oriColor.R * (255 - alpha)) / 255;
                    }
                    memcpy(dest, &c, bpp);
                }
            }
        }
    }
    delete[] src;
}

IDirectDrawSurface7* CIsoViewExt::BitmapToSurface(IDirectDraw7* pDD, const CBitmap& bitmap)
{
    BITMAP bm;
    GetObject(bitmap, sizeof(bm), &bm);

    DDSURFACEDESC2 desc = { 0 };
    ZeroMemory(&desc, sizeof(desc));
    desc.dwSize = sizeof(desc);
    desc.dwFlags = DDSD_CAPS | DDSD_HEIGHT | DDSD_WIDTH;
    desc.ddsCaps.dwCaps = DDSCAPS_OFFSCREENPLAIN;
    desc.dwWidth = bm.bmWidth;
    desc.dwHeight = bm.bmHeight;

    IDirectDrawSurface7* pSurface = nullptr;
    if (pDD->CreateSurface(&desc, &pSurface, nullptr) != DD_OK)
        return nullptr;

    pSurface->Restore();

    CDC bitmapDC;
    if (!bitmapDC.CreateCompatibleDC(nullptr)) {
        pSurface->Release();
        return nullptr;
    }
    bitmapDC.SelectObject(bitmap);

    HDC hSurfaceDC = nullptr;
    if (pSurface->GetDC(&hSurfaceDC) != DD_OK) {
        pSurface->Release();
        return nullptr;
    }

    CDC surfaceDC;
    surfaceDC.Attach(hSurfaceDC);

    auto success = surfaceDC.BitBlt(0, 0, bm.bmWidth, bm.bmHeight, &bitmapDC, 0, 0, SRCCOPY);
    surfaceDC.Detach();
    pSurface->ReleaseDC(hSurfaceDC);

    return pSurface;
}

void CIsoViewExt::BlitTransparent(LPDIRECTDRAWSURFACE7 pic, int x, int y, int width, int height, BYTE alpha, LPDIRECTDRAWSURFACE7 surface)
{
    auto pThis = CIsoView::GetInstance();
    if (pic == NULL) return;

    RECT r;
    if (surface == nullptr)
    {
        r = CIsoViewExt::GetScaledWindowRect();
        surface = pThis->lpDDBackBufferSurface;
    }
    else if (surface == pThis->lpDDBackBufferSurface)
    {
        r = CIsoViewExt::GetScaledWindowRect();
    }
    else
    {
        pThis->GetWindowRect(&r);
    }

    x += 1;
    y += 1;
    y -= 30;


    if (width == -1 || height == -1)
    {
        DDSURFACEDESC2 ddsd;
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(DDSURFACEDESC2);
        ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
        pic->GetSurfaceDesc(&ddsd);
        width = ddsd.dwWidth;
        height = ddsd.dwHeight;
    }

    if (x + width < 0 || y + height < 0) return;
    if (x > r.right || y > r.bottom) return;

    RECT blrect;
    RECT srcRect = { 0, 0, width, height };
    blrect.left = x;
    blrect.top = y;
    blrect.right = x + width;
    blrect.bottom = y + height;

    if (blrect.left < 0)
    {
        srcRect.left = -blrect.left;
        blrect.left = 0;
    }
    if (blrect.top < 0)
    {
        srcRect.top = -blrect.top;
        blrect.top = 0;
    }
    if (blrect.right > r.right)
    {
        srcRect.right = width - (blrect.right - r.right);
        blrect.right = r.right;
    }
    if (blrect.bottom > r.bottom)
    {
        srcRect.bottom = height - (blrect.bottom - r.bottom);
        blrect.bottom = r.bottom;
    }

    DDSURFACEDESC2 destDesc, srcDesc;
    memset(&destDesc, 0, sizeof(DDSURFACEDESC2));
    destDesc.dwSize = sizeof(DDSURFACEDESC2);
    memset(&srcDesc, 0, sizeof(DDSURFACEDESC2));
    srcDesc.dwSize = sizeof(DDSURFACEDESC2);

    if (surface->Lock(NULL, &destDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) != DD_OK)
        return;

    if (pic->Lock(NULL, &srcDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) != DD_OK)
    {
        surface->Unlock(NULL);
        return;
    }

    DDCOLORKEY colorKey;
    if (pic->GetColorKey(DDCKEY_SRCBLT, &colorKey) != DD_OK)
    {
        pic->Unlock(NULL);
        surface->Unlock(NULL);
        return;
    }

    DWORD colorKeyLow = colorKey.dwColorSpaceLowValue;
    DWORD colorKeyHigh = colorKey.dwColorSpaceHighValue;

    BYTE* destPixels = (BYTE*)destDesc.lpSurface;
    BYTE* srcPixels = (BYTE*)srcDesc.lpSurface;
    int destPitch = destDesc.lPitch;
    int srcPitch = srcDesc.lPitch;

    int maxDestY = destDesc.dwHeight;
    int maxDestX = destDesc.dwWidth;
    int maxSrcY = srcDesc.dwHeight;
    int maxSrcX = srcDesc.dwWidth;

    for (int j = 0; j < srcRect.bottom - srcRect.top; ++j)
    {
        if (y + j < 0 || y + j >= maxDestY) continue; 
        for (int i = 0; i < srcRect.right - srcRect.left; ++i)
        {
            if (x + i < 0 || x + i >= maxDestX) continue;

            int destIndex = (y + j) * destPitch + (x + i) * 4;
            int srcIndex = j * srcPitch + i * 4;

            if (srcIndex < 0 || srcIndex >= maxSrcY * srcPitch || destIndex < 0 || destIndex >= maxDestY * destPitch)
                continue;

            DWORD srcColor = *(DWORD*)(srcPixels + srcIndex);
            if (srcColor >= colorKeyLow && srcColor <= colorKeyHigh)
                continue; 

            BYTE srcR = srcPixels[srcIndex + 2];
            BYTE srcG = srcPixels[srcIndex + 1];
            BYTE srcB = srcPixels[srcIndex];

            BYTE destR = destPixels[destIndex + 2];
            BYTE destG = destPixels[destIndex + 1];
            BYTE destB = destPixels[destIndex];

            destPixels[destIndex + 2] = (srcR * alpha + destR * (255 - alpha)) / 255;
            destPixels[destIndex + 1] = (srcG * alpha + destG * (255 - alpha)) / 255;
            destPixels[destIndex] = (srcB * alpha + destB * (255 - alpha)) / 255;
        }
    }
    pic->Unlock(NULL);
    surface->Unlock(NULL);
}

void CIsoViewExt::BlitTransparentDesc(LPDIRECTDRAWSURFACE7 pic, LPDIRECTDRAWSURFACE7 surface, DDSURFACEDESC2* pDestDesc,
    int x, int y, int width, int height, BYTE alpha)
{
    auto pThis = CIsoView::GetInstance();
    if (pic == NULL || pDestDesc == NULL) return;

    RECT r;
    if (surface == pThis->lpDDBackBufferSurface)
    {
        r = CIsoViewExt::GetScaledWindowRect();
    }
    else
    {
        pThis->GetWindowRect(&r);
    }

    x += 1;
    y += 1;
    y -= 30;

    if (width == -1 || height == -1)
    {
        DDSURFACEDESC2 ddsd;
        memset(&ddsd, 0, sizeof(DDSURFACEDESC2));
        ddsd.dwSize = sizeof(DDSURFACEDESC2);
        ddsd.dwFlags = DDSD_WIDTH | DDSD_HEIGHT;
        pic->GetSurfaceDesc(&ddsd);
        width = ddsd.dwWidth;
        height = ddsd.dwHeight;
    }

    if (x + width < 0 || y + height < 0) return;
    if (x > r.right || y > r.bottom) return;

    RECT blrect;
    RECT srcRect = { 0, 0, width, height };
    blrect.left = x;
    blrect.top = y;
    blrect.right = x + width;
    blrect.bottom = y + height;

    if (blrect.left < 0)
    {
        srcRect.left = -blrect.left;
        blrect.left = 0;
    }
    if (blrect.top < 0)
    {
        srcRect.top = -blrect.top;
        blrect.top = 0;
    }
    if (blrect.right > r.right)
    {
        srcRect.right = width - (blrect.right - r.right);
        blrect.right = r.right;
    }
    if (blrect.bottom > r.bottom)
    {
        srcRect.bottom = height - (blrect.bottom - r.bottom);
        blrect.bottom = r.bottom;
    }

    DDSURFACEDESC2 srcDesc;
    memset(&srcDesc, 0, sizeof(DDSURFACEDESC2));
    srcDesc.dwSize = sizeof(DDSURFACEDESC2);

    if (pic->Lock(NULL, &srcDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) != DD_OK)
    {
        return;
    }

    DDCOLORKEY colorKey;
    if (pic->GetColorKey(DDCKEY_SRCBLT, &colorKey) != DD_OK)
    {
        pic->Unlock(NULL);
        return;
    }

    DWORD colorKeyLow = colorKey.dwColorSpaceLowValue;
    DWORD colorKeyHigh = colorKey.dwColorSpaceHighValue;

    BYTE* destPixels = (BYTE*)pDestDesc->lpSurface;
    BYTE* srcPixels = (BYTE*)srcDesc.lpSurface;
    int destPitch = pDestDesc->lPitch;
    int srcPitch = srcDesc.lPitch;

    int maxDestY = pDestDesc->dwHeight;
    int maxDestX = pDestDesc->dwWidth;
    int maxSrcY = srcDesc.dwHeight;
    int maxSrcX = srcDesc.dwWidth;

    for (int j = 0; j < srcRect.bottom - srcRect.top; ++j)
    {
        if (y + j < 0 || y + j >= maxDestY) continue;
        for (int i = 0; i < srcRect.right - srcRect.left; ++i)
        {
            if (x + i < 0 || x + i >= maxDestX) continue;

            int destIndex = (y + j) * destPitch + (x + i) * 4;
            int srcIndex = j * srcPitch + i * 4;

            if (srcIndex < 0 || srcIndex >= maxSrcY * srcPitch || destIndex < 0 || destIndex >= maxDestY * destPitch)
                continue;

            DWORD srcColor = *(DWORD*)(srcPixels + srcIndex);
            if (srcColor >= colorKeyLow && srcColor <= colorKeyHigh)
                continue;

            BYTE srcR = srcPixels[srcIndex + 2];
            BYTE srcG = srcPixels[srcIndex + 1];
            BYTE srcB = srcPixels[srcIndex];

            BYTE destR = destPixels[destIndex + 2];
            BYTE destG = destPixels[destIndex + 1];
            BYTE destB = destPixels[destIndex];

            destPixels[destIndex + 2] = (srcR * alpha + destR * (255 - alpha)) / 255;
            destPixels[destIndex + 1] = (srcG * alpha + destG * (255 - alpha)) / 255;
            destPixels[destIndex] = (srcB * alpha + destB * (255 - alpha)) / 255;
        }
    }

    pic->Unlock(NULL);
}

void CIsoViewExt::BlitSHPTransparent(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClass* pd, Palette* newPal, BYTE alpha, COLORREF houseColor, int extraLightType, bool remap)
{
    if (alpha == 0) return;
    ASSERT(pd->Flag != ImageDataFlag::SurfaceData);
    x += 31;
    y -= 29;
    int bpp = 4;

    if (newPal == NULL) {
        newPal = pd->pPalette;
    }

    BYTE* src = (BYTE*)pd->pImageBuffer;
    int swidth = pd->FullWidth;
    int sheight = pd->FullHeight;

    if (src == NULL || dst == NULL) {
        return;
    }

    if (x + swidth < window.left || y + sheight < window.top) {
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        return;
    }

    RECT blrect;
    RECT srcRect;
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = swidth;
    srcRect.bottom = sheight;
    blrect.left = x;
    if (blrect.left < 0) {
        srcRect.left = 1 - blrect.left;
        //blrect.left=1;
    }
    blrect.top = y;
    if (blrect.top < 0) {
        srcRect.top = 1 - blrect.top;
        //blrect.top=1;
    }
    blrect.right = (x + swidth);
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        blrect.right = window.right;
    }
    blrect.bottom = (y + sheight);
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        blrect.bottom = window.bottom;
    }

    int i, e;
    bool isMultiSelected = false;
    if (extraLightType == -10 || extraLightType >= 500)
    {
        isMultiSelected = MultiSelection::IsSelected(CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);
        if (extraLightType >= 500)
        {
            int overlay = extraLightType - 500;
            if (
            overlay == 0x18 || overlay == 0x19 || // BRIDGE1, BRIDGE2
                overlay == 0x3B || overlay == 0x3C || // RAILBRDG1, RAILBRDG2
                overlay == 0xED || overlay == 0xEE || // BRIDGEB1, BRIDGEB2
                (overlay >= 0x4A && overlay <= 0x65) || // LOBRDG 1-28
                (overlay >= 0xCD && overlay <= 0xEC) // LOBRDGB 1-4
                )
                isMultiSelected = MultiSelection::IsSelected(
                    CIsoViewExt::CurrentDrawCellLocation.X + 1,
                    CIsoViewExt::CurrentDrawCellLocation.Y + 1);
        }
    }

    BGRStruct color;
    auto pRGB = (ColorStruct*)&houseColor;
    color.R = pRGB->red;
    color.G = pRGB->green;
    color.B = pRGB->blue;
    if (LightingStruct::CurrentLighting != LightingStruct::NoLighting)
    {
        if (extraLightType >= 500)
            newPal = PalettesManager::GetOverlayPalette(newPal, CIsoViewExt::CurrentDrawCellLocation, extraLightType - 500);
        else
            newPal = PalettesManager::GetObjectPalette(newPal, color, remap, CIsoViewExt::CurrentDrawCellLocation, false, extraLightType);
    }
    else
        newPal = PalettesManager::GetPalette(newPal, color, remap);


    auto const surfaceEnd = (BYTE*)dst + boundary.dpitch * boundary.dwHeight;

    for (e = srcRect.top; e < srcRect.bottom; e++) {
        int left = pd->pPixelValidRanges[e].First;
        int right = pd->pPixelValidRanges[e].Last;

        if (left < srcRect.left) {
            left = srcRect.left;
        }
        if (right >= srcRect.right) {
            right = srcRect.right - 1;
        }

        for (i = left; i <= right; i++) {
            if (blrect.left + i < 0) {
                continue;
            }

            const int spos = i + e * swidth;
            BYTE val = src[spos];

            if (val) {
                auto dest = ((BYTE*)dst + (blrect.left + i) * bpp + (blrect.top + e) * boundary.dpitch);

                if (dest >= dst) {
                    BGRStruct c = newPal->Data[val];
                    if (dest + bpp < surfaceEnd) {
                        if (alpha < 255)
                        {
                            BGRStruct oriColor = *(BGRStruct*)dest;
                            c.B = (c.B * alpha + oriColor.B * (255 - alpha)) / 255;
                            c.G = (c.G * alpha + oriColor.G * (255 - alpha)) / 255;
                            c.R = (c.R * alpha + oriColor.R * (255 - alpha)) / 255;
                        }
                        if (isMultiSelected)
                        {
                            c.B = (c.B * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->B) / 3;
                            c.G = (c.G * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->G) / 3;
                            c.R = (c.R * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->R) / 3;
                        }
                        memcpy(dest, &c, bpp);
                    }
                }
            }
        }
    }
}

void CIsoViewExt::BlitSHPTransparent(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClassSafe* pd, Palette* newPal, BYTE alpha, COLORREF houseColor, int extraLightType, bool remap)
{
    if (alpha == 0) return;
    ASSERT(pd->Flag != ImageDataFlag::SurfaceData);
    x += 31;
    y -= 29;
    int bpp = 4;

    if (newPal == NULL) {
        newPal = pd->pPalette;
    }

    BYTE* src = (BYTE*)pd->pImageBuffer.get();
    int swidth = pd->FullWidth;
    int sheight = pd->FullHeight;

    if (src == NULL || dst == NULL) {
        return;
    }

    if (x + swidth < window.left || y + sheight < window.top) {
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        return;
    }

    RECT blrect;
    RECT srcRect;
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = swidth;
    srcRect.bottom = sheight;
    blrect.left = x;
    if (blrect.left < 0) {
        srcRect.left = 1 - blrect.left;
        //blrect.left=1;
    }
    blrect.top = y;
    if (blrect.top < 0) {
        srcRect.top = 1 - blrect.top;
        //blrect.top=1;
    }
    blrect.right = (x + swidth);
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        blrect.right = window.right;
    }
    blrect.bottom = (y + sheight);
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        blrect.bottom = window.bottom;
    }

    int i, e;
    bool isMultiSelected = false;
    if (extraLightType == -10 || extraLightType >= 500)
    {
        isMultiSelected = MultiSelection::IsSelected(CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);
    }

    BGRStruct color;
    auto pRGB = (ColorStruct*)&houseColor;
    color.R = pRGB->red;
    color.G = pRGB->green;
    color.B = pRGB->blue;
    if (LightingStruct::CurrentLighting != LightingStruct::NoLighting)
    {
        if (extraLightType >= 500)
            newPal = PalettesManager::GetOverlayPalette(newPal, CIsoViewExt::CurrentDrawCellLocation, extraLightType - 500);
        else
            newPal = PalettesManager::GetObjectPalette(newPal, color, remap, CIsoViewExt::CurrentDrawCellLocation, false, extraLightType);
    }
    else
        newPal = PalettesManager::GetPalette(newPal, color, remap);


    auto const surfaceEnd = (BYTE*)dst + boundary.dpitch * boundary.dwHeight;

    for (e = srcRect.top; e < srcRect.bottom; e++) {
        int left = pd->pPixelValidRanges[e].First;
        int right = pd->pPixelValidRanges[e].Last;

        if (left < srcRect.left) {
            left = srcRect.left;
        }
        if (right >= srcRect.right) {
            right = srcRect.right - 1;
        }

        for (i = left; i <= right; i++) {
            if (blrect.left + i < 0) {
                continue;
            }

            const int spos = i + e * swidth;
            BYTE val = src[spos];

            if (val) {
                auto dest = ((BYTE*)dst + (blrect.left + i) * bpp + (blrect.top + e) * boundary.dpitch);

                if (dest >= dst) {
                    BGRStruct c = newPal->Data[val];
                    if (dest + bpp < surfaceEnd) {
                        if (alpha < 255)
                        {
                            BGRStruct oriColor = *(BGRStruct*)dest;
                            c.B = (c.B * alpha + oriColor.B * (255 - alpha)) / 255;
                            c.G = (c.G * alpha + oriColor.G * (255 - alpha)) / 255;
                            c.R = (c.R * alpha + oriColor.R * (255 - alpha)) / 255;
                        }
                        if (isMultiSelected)
                        {
                            c.B = (c.B * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->B) / 3;
                            c.G = (c.G * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->G) / 3;
                            c.R = (c.R * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->R) / 3;
                        }
                        memcpy(dest, &c, bpp);
                    }
                }
            }
        }
    }
}

void CIsoViewExt::BlitSHPTransparent_Building(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClassSafe* pd, Palette* newPal, BYTE alpha,
    COLORREF houseColor, COLORREF addOnColor, bool isRubble, bool isTerrain)
{
    if (alpha == 0) return;
    ASSERT(pd->Flag != ImageDataFlag::SurfaceData);

    x += 31;
    y -= 29;
    int bpp = 4;

    if (newPal == NULL) {
        newPal = pd->pPalette;
    }

    BYTE* src = (BYTE*)pd->pImageBuffer.get();
    int swidth = pd->FullWidth;
    int sheight = pd->FullHeight;

    if (src == NULL || dst == NULL) {
        return;
    }

    if (x + swidth < window.left || y + sheight < window.top) {
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        return;
    }

    RECT blrect;
    RECT srcRect;
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = swidth;
    srcRect.bottom = sheight;
    blrect.left = x;
    if (blrect.left < 0) {
        srcRect.left = 1 - blrect.left;
        //blrect.left=1;
    }
    blrect.top = y;
    if (blrect.top < 0) {
        srcRect.top = 1 - blrect.top;
        //blrect.top=1;
    }
    blrect.right = (x + swidth);
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        blrect.right = window.right;
    }
    blrect.bottom = (y + sheight);
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        blrect.bottom = window.bottom;
    }

    int i, e;

    BGRStruct color;
    auto pRGB = (ColorStruct*)&houseColor;
    color.R = pRGB->red;
    color.G = pRGB->green;
    color.B = pRGB->blue;
    if (LightingStruct::CurrentLighting == LightingStruct::NoLighting)
    {
        newPal = PalettesManager::GetPalette(newPal, color, !isTerrain && !isRubble);
    }
    else
    {
        newPal = PalettesManager::GetObjectPalette(newPal, color, !isTerrain && !isRubble,
            CIsoViewExt::CurrentDrawCellLocation, false, isRubble || isTerrain ? 4 : 3);
    }

    auto const surfaceEnd = (BYTE*)dst + boundary.dpitch * boundary.dwHeight;

    for (e = srcRect.top; e < srcRect.bottom; e++) {
        int left = pd->pPixelValidRanges[e].First;
        int right = pd->pPixelValidRanges[e].Last;

        if (left < srcRect.left) {
            left = srcRect.left;
        }
        if (right >= srcRect.right) {
            right = srcRect.right - 1;
        }

        for (i = left; i <= right; i++) {
            if (blrect.left + i < 0) {
                continue;
            }

            const int spos = i + e * swidth;
            BYTE val = src[spos];

            if (val) {
                auto dest = ((BYTE*)dst + (blrect.left + i) * bpp + (blrect.top + e) * boundary.dpitch);

                if (dest >= dst) {
                    BGRStruct c = newPal->Data[val];
                    if (dest + bpp < surfaceEnd) {
                        if (alpha < 255)
                        {
                            BGRStruct oriColor = *(BGRStruct*)dest;
                            c.B = (c.B * alpha + oriColor.B * (255 - alpha)) / 255;
                            c.G = (c.G * alpha + oriColor.G * (255 - alpha)) / 255;
                            c.R = (c.R * alpha + oriColor.R * (255 - alpha)) / 255;
                        }
                        memcpy(dest, &c, bpp);
                    }
                }
            }
        }
    }
}

void CIsoViewExt::BlitSHPTransparent_AlphaImage(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClassSafe* pd)
{
    ASSERT(pd->Flag != ImageDataFlag::SurfaceData);

    x += 31;
    y -= 29;
    int bpp = 4;

    BYTE* src = (BYTE*)pd->pImageBuffer.get();
    int swidth = pd->FullWidth;
    int sheight = pd->FullHeight;

    if (src == NULL || dst == NULL) {
        return;
    }

    if (x + swidth < window.left || y + sheight < window.top) {
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        return;
    }

    RECT blrect;
    RECT srcRect;
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = swidth;
    srcRect.bottom = sheight;
    blrect.left = x;
    if (blrect.left < 0) {
        srcRect.left = 1 - blrect.left;
        //blrect.left=1;
    }
    blrect.top = y;
    if (blrect.top < 0) {
        srcRect.top = 1 - blrect.top;
        //blrect.top=1;
    }
    blrect.right = (x + swidth);
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        blrect.right = window.right;
    }
    blrect.bottom = (y + sheight);
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        blrect.bottom = window.bottom;
    }

    int i, e;


    auto const surfaceEnd = (BYTE*)dst + boundary.dpitch * boundary.dwHeight;

    for (e = srcRect.top; e < srcRect.bottom; e++) {
        int left = srcRect.left;
        int right = srcRect.right - 1;

        for (i = left; i <= right; i++) {
            if (blrect.left + i < 0) {
                continue;
            }

            const int spos = i + e * swidth;
            BYTE val = src[spos];

            if (val != 127) {
                auto dest = ((BYTE*)dst + (blrect.left + i) * bpp + (blrect.top + e) * boundary.dpitch);

                if (dest >= dst) {
                    BGRStruct c;
                    if (dest + bpp < surfaceEnd) {
                        BGRStruct oriColor = *(BGRStruct*)dest;
                        c.B = std::min(oriColor.B * val * 2 / 256, 255);
                        c.G = std::min(oriColor.G * val * 2 / 256, 255);
                        c.R = std::min(oriColor.R * val * 2 / 256, 255);
                        memcpy(dest, &c, bpp);
                    }
                }
            }
        }
    }
}

void CIsoViewExt::BlitSHPTransparent(LPDDSURFACEDESC2 lpDesc, int x, int y, ImageDataClass* pd, Palette* newPal, BYTE alpha, COLORREF houseColor)
{  
    auto pThis = CIsoView::GetInstance();
    RECT window = CIsoViewExt::GetScaledWindowRect();
    DDBoundary boundary{ lpDesc->dwWidth, lpDesc->dwHeight, lpDesc->lPitch };
    CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary, x, y, pd, newPal, alpha, houseColor);
}

void CIsoViewExt::BlitSHPTransparent(LPDDSURFACEDESC2 lpDesc, int x, int y, ImageDataClassSafe* pd, Palette* newPal, BYTE alpha, COLORREF houseColor)
{  
    auto pThis = CIsoView::GetInstance();
    RECT window = CIsoViewExt::GetScaledWindowRect();
    DDBoundary boundary{ lpDesc->dwWidth, lpDesc->dwHeight, lpDesc->lPitch };
    CIsoViewExt::BlitSHPTransparent(pThis, lpDesc->lpSurface, window, boundary, x, y, pd, newPal, alpha, houseColor);
}

void CIsoViewExt::BlitTerrain(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, CTileBlockClass* subTile, Palette* pal, BYTE alpha)
{
    if (alpha == 0) return;

    int bpp = 4;

    x += 61;
    y += 1;
    BYTE* src = (BYTE*)subTile->ImageData;
    int swidth = subTile->BlockWidth;
    int sheight = subTile->BlockHeight;

    if (src == NULL || dst == NULL) {
        return;
    }

    if (x + swidth < window.left || y + sheight < window.top) {
        return;
    }
    if (x >= window.right || y >= window.bottom) {
        return;
    }

    RECT blrect;
    RECT srcRect;
    srcRect.left = 0;
    srcRect.top = 0;
    srcRect.right = swidth;
    srcRect.bottom = sheight;
    blrect.left = x;
    if (blrect.left < 0) {
        srcRect.left = 1 - blrect.left;
        //blrect.left=1;
    }
    blrect.top = y;
    if (blrect.top < 0) {
        srcRect.top = 1 - blrect.top;
        //blrect.top=1;
    }
    blrect.right = (x + swidth);
    if (x + swidth > window.right) {
        srcRect.right = swidth - ((x + swidth) - window.right);
        blrect.right = window.right;
    }
    blrect.bottom = (y + sheight);
    if (y + sheight > window.bottom) {
        srcRect.bottom = sheight - ((y + sheight) - window.bottom);
        blrect.bottom = window.bottom;
    }

    Palette* newPal = pal;
    if (LightingStruct::CurrentLighting != LightingStruct::NoLighting)
    {
        BGRStruct color;
        newPal = PalettesManager::GetObjectPalette(newPal, color, false,
            CIsoViewExt::CurrentDrawCellLocation);
    }

    bool multiSelected = MultiSelection::IsSelected(CIsoViewExt::CurrentDrawCellLocation.X, CIsoViewExt::CurrentDrawCellLocation.Y);

    auto const surfaceEnd = (BYTE*)dst + boundary.dpitch * boundary.dwHeight;

    // we haved banned the other condition
    int i, e;
    if (subTile->HasValidImage)
    for (e = srcRect.top; e < srcRect.bottom; e++) {
        int left = subTile->pPixelValidRanges[e].First;
        int right = subTile->pPixelValidRanges[e].Last;

        if (left < srcRect.left) {
            left = srcRect.left;
        }
        if (right >= srcRect.right) {
            right = srcRect.right - 1;
        }

        for (i = left; i <= right; i++) {
            if (blrect.left + i < 0) {
                continue;
            }

            if (ExtConfigs::FlatToGroundHideExtra && CFinalSunApp::Instance->FlatToGround)
            {
                const int posInTile = i + subTile->XMinusExX + (e + subTile->YMinusExY) * 60;
                if (i + subTile->XMinusExX < 0 ||
                    i + subTile->XMinusExX >= 60 ||
                    e + subTile->YMinusExY < 0 ||
                    e + subTile->YMinusExY >= 30 ||
                    !TilePixels[posInTile])
                    continue;
            }

            const int spos = i + e * swidth;
            BYTE val = src[spos];

            if (val) {
                auto dest = ((BYTE*)dst + (blrect.left + i) * bpp + (blrect.top + e) * boundary.dpitch);

                if (dest >= dst) {
                    BGRStruct c = newPal->Data[val];
                    if (dest + bpp < surfaceEnd) {
                        if (alpha < 255)
                        {
                            BGRStruct oriColor = *(BGRStruct*)dest;
                            c.B = (c.B * alpha + oriColor.B * (255 - alpha)) / 255;
                            c.G = (c.G * alpha + oriColor.G * (255 - alpha)) / 255;
                            c.R = (c.R * alpha + oriColor.R * (255 - alpha)) / 255;
                        }
                        if (multiSelected)
                        {
                            c.B = (c.B * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->B) / 3;
                            c.G = (c.G * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->G) / 3;
                            c.R = (c.R * 2 + reinterpret_cast<RGBClass*>(&ExtConfigs::MultiSelectionColor)->R) / 3;
                        }
                        memcpy(dest, &c, bpp);
                    }
                }
            }
        }
    }
}

void CIsoViewExt::ScaleBitmap(CBitmap* pBitmap, int maxSize, COLORREF bgColor, bool removeHalo, bool trim)
{
    if (!pBitmap || maxSize <= 0) return;

    BITMAP bmpInfo = {};
    pBitmap->GetBitmap(&bmpInfo);
    int srcW = bmpInfo.bmWidth;
    int srcH = bmpInfo.bmHeight;

    if (bmpInfo.bmWidth == maxSize && bmpInfo.bmHeight == maxSize)
        return;

    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = srcW;
    bmi.bmiHeader.biHeight = -srcH;
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    std::vector<DWORD> srcPixels(srcW * srcH);
    {
        HDC hdc = GetDC(NULL);
        GetDIBits(hdc, (HBITMAP)(*pBitmap), 0, srcH, srcPixels.data(), &bmi, DIB_RGB_COLORS);
        ReleaseDC(NULL, hdc);
    }

    int left = srcW, right = 0, top = srcH, bottom = 0;
    DWORD bgRGB = RGB(GetBValue(bgColor), GetGValue(bgColor), GetRValue(bgColor));

    if (trim)
    {
        for (int y = 0; y < srcH; ++y)
        {
            for (int x = 0; x < srcW; ++x)
            {
                DWORD px = srcPixels[y * srcW + x];
                COLORREF pxColor = RGB(px & 0xFF, (px >> 8) & 0xFF, (px >> 16) & 0xFF);
                if (pxColor != bgRGB)
                {
                    if (x < left) left = x;
                    if (x > right) right = x;
                    if (y < top) top = y;
                    if (y > bottom) bottom = y;
                }
            }
        }
    }
    else
    {
        left = 0;
        top = 0;
        right = srcW;
        bottom = srcH;
    }

    bool empty = false;
    if (left > right || top > bottom)
    {
        empty = true;
        left = 0;
        top = 0;
        right = srcW;
        bottom = srcH;
    }

    int cropW = right - left + 1;
    int cropH = bottom - top + 1;
    float scale = std::min((float)maxSize / cropW, (float)maxSize / cropH);
    int newW = int(cropW * scale);
    int newH = int(cropH * scale);
    int offsetX = (maxSize - newW) / 2;
    int offsetY = (maxSize - newH) / 2;

    bmi.bmiHeader.biWidth = maxSize;
    bmi.bmiHeader.biHeight = -maxSize;

    void* pDstBits = nullptr;
    HBITMAP hNewBmp = CreateDIBSection(NULL, &bmi, DIB_RGB_COLORS, &pDstBits, NULL, 0);
    if (!hNewBmp || !pDstBits) return;

    DWORD* dst = (DWORD*)pDstBits;

    if (empty)
    {
        DWORD bgARGB = 0xFF000000 | (GetRValue(bgColor) << 16) | (GetGValue(bgColor) << 8) | GetBValue(bgColor);
        std::fill(dst, dst + maxSize * maxSize, bgARGB);
        pBitmap->DeleteObject();
        pBitmap->Attach(hNewBmp);
        return;
    }

    DWORD bgARGB = 0xFF000000 | (GetRValue(bgColor) << 16) | (GetGValue(bgColor) << 8) | GetBValue(bgColor);
    std::fill(dst, dst + maxSize * maxSize, bgARGB);

    for (int y = 0; y < newH; ++y)
    {
        for (int x = 0; x < newW; ++x)
        {
            float fx = left + x / scale;
            float fy = top + y / scale;

            int x0 = (int)fx;
            int y0 = (int)fy;
            int x1 = std::min(x0 + 1, srcW - 1);
            int y1 = std::min(y0 + 1, srcH - 1);

            x0 = std::clamp(x0, 0, srcW - 1);
            x1 = std::clamp(x1, 0, srcW - 1);
            y0 = std::clamp(y0, 0, srcH - 1);
            y1 = std::clamp(y1, 0, srcH - 1);

            float dx = fx - x0;
            float dy = fy - y0;

            DWORD c00 = srcPixels[y0 * srcW + x0];
            DWORD c10 = srcPixels[y0 * srcW + x1];
            DWORD c01 = srcPixels[y1 * srcW + x0];
            DWORD c11 = srcPixels[y1 * srcW + x1];

            auto extractRGB = [](DWORD c) {
                return std::tuple<int, int, int>(
                    c & 0xFF,
                    (c >> 8) & 0xFF,
                    (c >> 16) & 0xFF
                );
                };

            COLORREF cBG = bgRGB;

            auto isBG = [=](DWORD c) {
                COLORREF pxColor = RGB(c & 0xFF, (c >> 8) & 0xFF, (c >> 16) & 0xFF);
                return pxColor == cBG;
                };

            int r = 0, g = 0, b = 0;
            float totalWeight = 0.0f;

            auto blend = [&](DWORD color, float weight) {
                if (!isBG(color))
                {
                    int cr, cg, cb;
                    std::tie(cb, cg, cr) = extractRGB(color);
                    r += int(cr * weight);
                    g += int(cg * weight);
                    b += int(cb * weight);
                    totalWeight += weight;
                }
                };

            blend(c00, (1 - dx) * (1 - dy));
            blend(c10, dx * (1 - dy));
            blend(c01, (1 - dx) * dy);
            blend(c11, dx * dy);

            DWORD result = bgARGB;
            if (totalWeight > 0.0f)
            {
                r = int(r / totalWeight);
                g = int(g / totalWeight);
                b = int(b / totalWeight);
                result = 0xFF000000 | (r << 16) | (g << 8) | b;
            }

            int dxDst = offsetX + x;
            int dyDst = offsetY + y;
            dst[dyDst * maxSize + dxDst] = result;
        }
    }

    if (removeHalo)
    {
        for (int i = 0; i < maxSize * maxSize; ++i)
        {
            DWORD& px = dst[i];
            COLORREF c = RGB(px & 0xFF, (px >> 8) & 0xFF, (px >> 16) & 0xFF);
            if (c == bgRGB)
            {
                px = 0x00000000 | (GetRValue(bgColor) << 16) | (GetGValue(bgColor) << 8) | GetBValue(bgColor);
            }
        }
    }
    pBitmap->DeleteObject();
    pBitmap->Attach(hNewBmp);
    return;
}

std::vector<MapCoord> CIsoViewExt::GetTubePath(int x1, int y1, int x2, int y2, bool first)
{
    std::vector<MapCoord> path;
    int x = x1, y = y1;
    path.emplace_back(x, y);

    while (x != x2 || y != y2) {
        int dx = x2 - x;
        int dy = y2 - y;

        int step_x = (dx == 0 ? 0 : (dx > 0 ? 1 : -1));
        int step_y = (dy == 0 ? 0 : (dy > 0 ? 1 : -1));

        x += step_x;
        y += step_y;

        path.emplace_back(x, y);
    }
    int size = path.size();
    if (size > 2)
    {
        if (first && !(path[size - 1].X != x2 && path[size - 1].Y != y2 || path[size - 2].X != x2 && path[size - 2].Y != y2))
        {
            if (abs(x1 - x2) >= abs(y1 - y2))
            {
                if (x1 >= x2)
                {
                    path = GetTubePath(x1 - 2, y1, x2, y2, false);
                    path.insert(path.begin(), { x1 - 1,y1 });
                    path.insert(path.begin(), { x1,y1 });
                }
                else
                {
                    path = GetTubePath(x1 + 2, y1, x2, y2, false);
                    path.insert(path.begin(), { x1 + 1,y1 });
                    path.insert(path.begin(), { x1,y1 });
                }
            }
            else
            {
                if (y1 >= y2)
                {
                    path = GetTubePath(x1, y1 - 2, x2, y2, false);
                    path.insert(path.begin(), { x1,y1 - 1 });
                    path.insert(path.begin(), { x1,y1 });
                }
                else
                {
                    path = GetTubePath(x1, y1 + 2, x2, y2, false);
                    path.insert(path.begin(), { x1,y1 + 1 });
                    path.insert(path.begin(), { x1,y1 });
                }
            }
            return path;
        }
    }
    return path;
}

std::vector<int> CIsoViewExt::GetTubeDirections(const std::vector<MapCoord>& path)
{
    std::vector<int> directions;

    for (size_t i = 1; i < path.size(); ++i) {
        int dx = path[i].X - path[i - 1].X;
        int dy = path[i].Y - path[i - 1].Y;

        int dirCode = -1;
        if (dx == -1 && dy == 0) dirCode = 0;
        else if (dx == -1 && dy == 1) dirCode = 1;
        else if (dx == 0 && dy == 1) dirCode = 2;
        else if (dx == 1 && dy == 1) dirCode = 3;
        else if (dx == 1 && dy == 0) dirCode = 4;
        else if (dx == 1 && dy == -1) dirCode = 5;
        else if (dx == 0 && dy == -1) dirCode = 6;
        else if (dx == -1 && dy == -1) dirCode = 7;

        directions.push_back(dirCode);
    }

    return directions;
}

std::vector<MapCoord> CIsoViewExt::GetPathFromDirections(int x0, int y0, const std::vector<int>& directions)
{
    std::vector<MapCoord> path;
    path.emplace_back(x0, y0);

    const int dx[8] = { -1, -1,  0,  1, 1, 1,  0, -1 };
    const int dy[8] = { 0,  1,  1,  1, 0, -1, -1, -1 };

    int x = x0, y = y0;

    for (int dir : directions) {
        x += dx[dir];
        y += dy[dir];
        path.emplace_back(x, y);
    }

    return path;
}

RECT CIsoViewExt::GetScaledWindowRect()
{
    CRect rect;
    auto pThis = CIsoView::GetInstance();
    pThis->GetWindowRect(&rect);
    rect.right += rect.Width() * (CIsoViewExt::ScaledFactor - 1.0);
    rect.bottom += rect.Height() * (CIsoViewExt::ScaledFactor - 1.0);
    return rect;
}

void CIsoViewExt::MapCoord2ScreenCoord(int& X, int& Y, int flatMode)
{
    CRect rect;
    auto pThis = CIsoView::GetInstance();
    pThis->GetWindowRect(&rect);
    if (flatMode == 0)
        pThis->MapCoord2ScreenCoord(X, Y);
    else if (flatMode == 1)
        pThis->MapCoord2ScreenCoord_Flat(X, Y);
    else
        pThis->MapCoord2ScreenCoord_Height(X, Y);
    X = (X - pThis->ViewPosition.x - rect.left) / CIsoViewExt::ScaledFactor + pThis->ViewPosition.x + rect.left;
    Y = (Y - pThis->ViewPosition.y - rect.top) / CIsoViewExt::ScaledFactor + pThis->ViewPosition.y + rect.top;
}

bool CIsoViewExt::StretchCopySurfaceBilinear(LPDIRECTDRAWSURFACE7 srcSurface, CRect srcRect, LPDIRECTDRAWSURFACE7 dstSurface, CRect dstRect)
{
    if (!ExtConfigs::DDrawScalingBilinear)
    {
        dstSurface->Blt(&dstRect, srcSurface, &srcRect, DDBLT_WAIT, 0);
        return true;
    }

    DDSURFACEDESC2 srcDesc = { sizeof(DDSURFACEDESC2) };
    DDSURFACEDESC2 dstDesc = { sizeof(DDSURFACEDESC2) };

    if (srcSurface->Lock(NULL, &srcDesc, DDLOCK_READONLY | DDLOCK_WAIT, NULL) != DD_OK)
        return false;

    if (dstSurface->Lock(NULL, &dstDesc, DDLOCK_WAIT, NULL) != DD_OK) {
        srcSurface->Unlock(NULL);
        return false;
    }

    uint8_t* srcBits = (uint8_t*)srcDesc.lpSurface;
    uint8_t* dstBits = (uint8_t*)dstDesc.lpSurface;
    int srcPitch = srcDesc.lPitch;
    int dstPitch = dstDesc.lPitch;

    int srcW = srcRect.right - srcRect.left;
    int srcH = srcRect.bottom - srcRect.top;
    int dstW = dstRect.right - dstRect.left;
    int dstH = dstRect.bottom - dstRect.top;

    for (int y = 0; y < dstH; ++y) {
        float v = (y + 0.5f) * srcH / dstH - 0.5f;
        int srcY0 = (int)floor(v);
        int srcY1 = srcY0 + 1;
        float fy = v - srcY0;

        srcY0 = std::clamp(srcY0, 0, srcH - 1);
        srcY1 = std::clamp(srcY1, 0, srcH - 1);

        uint8_t* dstRow = dstBits + (dstRect.top + y) * dstPitch + dstRect.left * 4;

        for (int x = 0; x < dstW; ++x) {
            float u = (x + 0.5f) * srcW / dstW - 0.5f;
            int srcX0 = (int)floor(u);
            int srcX1 = srcX0 + 1;
            float fx = u - srcX0;

            srcX0 = std::clamp(srcX0, 0, srcW - 1);
            srcX1 = std::clamp(srcX1, 0, srcW - 1);

            uint32_t c00 = *(uint32_t*)(srcBits + (srcRect.top + srcY0) * srcPitch + (srcRect.left + srcX0) * 4);
            uint32_t c10 = *(uint32_t*)(srcBits + (srcRect.top + srcY0) * srcPitch + (srcRect.left + srcX1) * 4);
            uint32_t c01 = *(uint32_t*)(srcBits + (srcRect.top + srcY1) * srcPitch + (srcRect.left + srcX0) * 4);
            uint32_t c11 = *(uint32_t*)(srcBits + (srcRect.top + srcY1) * srcPitch + (srcRect.left + srcX1) * 4);

            auto unpack = [](uint32_t color, uint8_t& r, uint8_t& g, uint8_t& b) {
                r = (color >> 16) & 0xFF;
                g = (color >> 8) & 0xFF;
                b = color & 0xFF;
                };

            uint8_t r00, g00, b00;
            uint8_t r10, g10, b10;
            uint8_t r01, g01, b01;
            uint8_t r11, g11, b11;

            unpack(c00, r00, g00, b00);
            unpack(c10, r10, g10, b10);
            unpack(c01, r01, g01, b01);
            unpack(c11, r11, g11, b11);

            auto interp = [fx, fy](uint8_t c00, uint8_t c10, uint8_t c01, uint8_t c11) -> uint8_t {
                float top = c00 * (1 - fx) + c10 * fx;
                float bot = c01 * (1 - fx) + c11 * fx;
                float value = top * (1 - fy) + bot * fy;
                return static_cast<uint8_t>(value + 0.5f);
                };

            uint8_t r = interp(r00, r10, r01, r11);
            uint8_t g = interp(g00, g10, g01, g11);
            uint8_t b = interp(b00, b10, b01, b11);

            *(uint32_t*)(dstRow + x * 4) = (r << 16) | (g << 8) | b;
        }
    }

    srcSurface->Unlock(NULL);
    dstSurface->Unlock(NULL);
    return true;
}

void CIsoViewExt::DrawCreditOnMap(HDC hDC)
{
    auto pThis = CIsoView::GetInstance();
    CRect rect;
    pThis->GetWindowRect(&rect);
    int leftIndex = 0;
    int lineHeight = ExtConfigs::DisplayTextSize + 2;
    ::SetBkMode(hDC, OPAQUE);
    ::SetBkColor(hDC, RGB(0xFF, 0xFF, 0xFF));
    SetTextAlign(hDC, TA_LEFT);
    if (CIsoViewExt::DrawMoneyOnMap)
    {
        ppmfc::CString buffer;
        buffer.Format(Translations::TranslateOrDefault("MoneyOnMap", "Credits On Map: %d"), CMapData::Instance->MoneyCount);
        ::TextOut(hDC, rect.left + 10, rect.top + 10 + lineHeight * leftIndex++, buffer, buffer.GetLength());

        if (ExtConfigs::EnableMultiSelection)
        {
            if (MultiSelection::SelectedCoords.size())
            {
                int nCount = 0;
                auto pExt = CMapDataExt::GetExtension();
                pExt->InitOreValue();
                MultiSelection::ApplyForEach(
                    [&nCount, pExt](CellData& cell) {
                        nCount += pExt->GetOreValueAt(cell);
                    }
                );

                buffer.Format(Translations::TranslateOrDefault("MoneyOnMap.MultiSelection",
                    "MultiSelection Enabled. Selected Credits: %d"), nCount);
                ppmfc::CString buffer2;
                buffer2.Format(Translations::TranslateOrDefault("MoneyOnMap.MultiSelectionCoords",
                    ", Selected Tiles: %d"), MultiSelection::SelectedCoords.size());
                buffer += buffer2;
                ::TextOut(hDC, rect.left + 10, rect.top + 10 + lineHeight * leftIndex++, buffer, buffer.GetLength());
            }
        }
    }
    if (CFinalSunApp::Instance().FlatToGround)
    {
        ppmfc::CString buffer;
        buffer.Format(Translations::TranslateOrDefault("FlatToGroundModeEnabled", "2D Mode Enabled"));
        ::TextOut(hDC, rect.left + 10, rect.top + 10 + lineHeight * leftIndex++, buffer, buffer.GetLength());
    }
}

CRect CIsoViewExt::GetVisibleIsoViewRect()
{
    auto pThis = CIsoView::GetInstance();
    CRect rect;
    pThis->GetWindowRect(&rect);
    CRect screenRect(0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN));
    CRect destRect;
    destRect.IntersectRect(&rect, &screenRect);
    return destRect;
}

void CIsoViewExt::SpecialDraw(LPDIRECTDRAWSURFACE7 surface, int specialDraw)
{
    switch (specialDraw)
    {
    case 0:
    {
        auto pThis = CIsoView::GetInstance();
        CRect rect = CIsoViewExt::GetVisibleIsoViewRect();
        pThis->lpDDTempBufferSurface->Blt(&rect, surface, &rect, DDBLT_WAIT, 0);
        if (pThis->IsScrolling)
        {
            auto point = pThis->MoveCenterPosition;
            point.x += rect.left - 16 - 18;
            point.y += rect.top + 14 - 12;
            auto cursor = CLoadingExt::GetSurfaceImageDataFromMap("scrollcursor.bmp");
            CIsoViewExt::BlitTransparent(cursor->lpSurface, point.x, point.y, -1, -1, 255, surface);
        }

        HDC hDC;
        surface->GetDC(&hDC);
        int fontSize = ExtConfigs::DisplayTextSize;
        if (CIsoViewExt::ScaledFactor < 0.75)
            fontSize += 2;
        if (CIsoViewExt::ScaledFactor < 0.5)
            fontSize += 2;
        if (CIsoViewExt::ScaledFactor < 0.3)
            fontSize += 2;
        HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Cambria");
        HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

        DrawCreditOnMap(hDC);

        SelectObject(hDC, hOldFont);
        DeleteObject(hFont);
        surface->ReleaseDC(hDC);

        break;
    }
    case 1:
    {
        HDC hDC;
        surface->GetDC(&hDC);
        int fontSize = ExtConfigs::DisplayTextSize;
        if (CIsoViewExt::ScaledFactor < 0.75)
            fontSize += 2;
        if (CIsoViewExt::ScaledFactor < 0.5)
            fontSize += 2;
        if (CIsoViewExt::ScaledFactor < 0.3)
            fontSize += 2;
        HFONT hFont = CreateFont(fontSize, 0, 0, 0,  FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Cambria");
        HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

        DrawMouseMove(hDC);
        DrawCreditOnMap(hDC);

        SelectObject(hDC, hOldFont);
        DeleteObject(hFont);

        surface->ReleaseDC(hDC);
        break;
    }
    case 2:
    {
        HDC hDC;
        surface->GetDC(&hDC);
        int fontSize = ExtConfigs::DisplayTextSize;
        if (CIsoViewExt::ScaledFactor < 0.75)
            fontSize += 2;
        if (CIsoViewExt::ScaledFactor < 0.5)
            fontSize += 2;
        if (CIsoViewExt::ScaledFactor < 0.3)
            fontSize += 2;
        HFONT hFont = CreateFont(fontSize, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET,
            OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, NONANTIALIASED_QUALITY,
            DEFAULT_PITCH | FF_DONTCARE, "Cambria");
        HFONT hOldFont = (HFONT)SelectObject(hDC, hFont);

        DrawCopyBound(hDC);
        DrawCreditOnMap(hDC);

        SelectObject(hDC, hOldFont);
        DeleteObject(hFont);
        surface->ReleaseDC(hDC);
        break;
    }
    case 3:
    {
        HDC hDC;
        surface->GetDC(&hDC);
        DrawBridgeLine(hDC);
        surface->ReleaseDC(hDC);
        break;
    }
    default:
        break;
    }
}

void CIsoViewExt::MoveToMapCoord(int X, int Y)
{
    if (!CMapData::Instance->IsCoordInMap(X, Y))
        return;

    auto pThis = CIsoView::GetInstance();
    int nMapCoord = CMapData::Instance->GetCoordIndex(X, Y);
    RECT rect = GetScaledWindowRect();
    int x = 30 * (CMapData::Instance->MapWidthPlusHeight + X - Y) - (rect.right - rect.left) / 2 - rect.left;
    int y = 15 * (Y + X) - CMapData::Instance->CellDatas[nMapCoord].Height - (rect.bottom - rect.top) / 2 - rect.top;
    pThis->MoveTo(x, y);
    pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
    CFinalSunDlg::Instance->MyViewFrame.Minimap.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
}

void CIsoViewExt::Zoom(double offset)
{
    if (CMapData::Instance->MapWidthPlusHeight)
    {
        auto pThis = CIsoView::GetInstance();
        double scaledOld = CIsoViewExt::ScaledFactor;
        CRect oldRect = GetScaledWindowRect();
        if (offset == 0.0)
        {
            CIsoViewExt::ScaledFactor = 1.0;
        }
        else
        {
            CIsoViewExt::ScaledFactor += offset;
            CIsoViewExt::ScaledFactor = std::min(CIsoViewExt::ScaledMax, CIsoViewExt::ScaledFactor);
            CIsoViewExt::ScaledFactor = std::max(CIsoViewExt::ScaledMin, CIsoViewExt::ScaledFactor);
        }
        if (abs(CIsoViewExt::ScaledFactor - 1.0) <= 0.06)
            CIsoViewExt::ScaledFactor = 1.0;
        if (scaledOld != CIsoViewExt::ScaledFactor)
        {
            CRect newRect = GetScaledWindowRect();
            CRect oriRect;
            pThis->GetWindowRect(&oriRect);
            double mousePosX;
            double mousePosY;
            mousePosX = static_cast<double>(pThis->MouseCurrentPosition.x) / oriRect.Width();
            mousePosY = static_cast<double>(pThis->MouseCurrentPosition.y) / oriRect.Height();

            pThis->ViewPosition.x += (oldRect.Width() - newRect.Width()) * mousePosX;
            pThis->ViewPosition.y += (oldRect.Height() - newRect.Height()) * mousePosY;
            pThis->MoveTo(pThis->ViewPosition.x, pThis->ViewPosition.y);
            pThis->RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
            CFinalSunDlg::Instance->MyViewFrame.Minimap.RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_UPDATENOW);
        }
    }
}

void CIsoViewExt::DrawMultiMapCoordBorders(HDC hDC, const std::vector<MapCoord>& coords, COLORREF color)
{
    auto pThis = (CIsoViewExt*)CIsoView::GetInstance();
    for (const auto& mc : coords)
    {
        int x = mc.X;
        int y = mc.Y;
        CIsoViewExt::MapCoord2ScreenCoord(x, y);
        int drawX = x - CIsoViewExt::drawOffsetX;
        int drawY = y - CIsoViewExt::drawOffsetY;

        bool s1 = true;
        bool s2 = true;
        bool s3 = true;
        bool s4 = true;

        for (auto& coord : coords)
        {
            if (coord.X == mc.X - 1 && coord.Y == mc.Y)
            {
                s1 = false;
            }
            if (coord.X == mc.X + 1 && coord.Y == mc.Y)
            {
                s3 = false;
            }
            if (coord.X == mc.X && coord.Y == mc.Y - 1)
            {
                s4 = false;
            }

            if (coord.X == mc.X && coord.Y == mc.Y + 1)
            {
                s2 = false;
            }
        }
        pThis->DrawLockedCellOutlinePaint(drawX, drawY, 1, 1, color, false, hDC, pThis->m_hWnd, s1, s2, s3, s4);
    }
}

void CIsoViewExt::DrawLineHDC(HDC hDC, int x1, int y1, int x2, int y2, int color)
{
    auto pThis = (CIsoViewExt*)CIsoView::GetInstance();
    x1 += 36 / CIsoViewExt::ScaledFactor - 6;
    x2 += 36 / CIsoViewExt::ScaledFactor - 6;
    y1 -= 12.5 / CIsoViewExt::ScaledFactor + 2.5;
    y2 -= 12.5 / CIsoViewExt::ScaledFactor + 2.5;
    PAINTSTRUCT ps;
    HPEN hPen;
    HPEN hPenOld;
    BeginPaint(pThis->m_hWnd, &ps);
    hPen = CreatePen(PS_SOLID, CIsoViewExt::ScaledFactor < 0.61 ? 2 : 0, color);
    hPenOld = (HPEN)SelectObject(hDC, hPen);
    MoveToEx(hDC, x1 - CIsoViewExt::drawOffsetX, y1 - CIsoViewExt::drawOffsetY, NULL);
    LineTo(hDC, x2 - CIsoViewExt::drawOffsetX, y2 - CIsoViewExt::drawOffsetY);
    SelectObject(hDC, hPenOld);
    DeleteObject(hPen);
    EndPaint(pThis->m_hWnd, &ps);
}

BOOL CIsoViewExt::PreTranslateMessageExt(MSG* pMsg)
{
    return CIsoView::PreTranslateMessage(pMsg);
}

BOOL CIsoViewExt::OnMouseWheelExt(UINT Flags, short zDelta, CPoint point)
{
    return TRUE;
}
