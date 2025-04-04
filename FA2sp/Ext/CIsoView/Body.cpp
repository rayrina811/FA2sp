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
bool CIsoViewExt::PasteShowOutline = true;

bool CIsoViewExt::DrawStructuresFilter = false;
bool CIsoViewExt::DrawInfantriesFilter = false;
bool CIsoViewExt::DrawUnitsFilter = false;
bool CIsoViewExt::DrawAircraftsFilter = false;
bool CIsoViewExt::DrawBasenodesFilter = false;
bool CIsoViewExt::DrawCellTagsFilter = false;

bool CIsoViewExt::AutoPropertyBrush[4] = { false };
bool CIsoViewExt::IsPressingALT = false;
ppmfc::CString CIsoViewExt::CurrentCellObjectHouse = "";

Cell3DLocation CIsoViewExt::CurrentDrawCellLocation;

float CIsoViewExt::drawOffsetX;
float CIsoViewExt::drawOffsetY;

COLORREF CIsoViewExt::_cell_hilight_colors[16] = {
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

void CIsoViewExt::ProgramStartupInit()
{
    // RunTime::ResetMemoryContentAt(0x594518, CIsoViewExt::PreTranslateMessageExt);
}

void CIsoViewExt::AddTube(int EnterX, int EnterY, int ExitX, int ExitY)
{
    Logger::Raw("Generating tube from (%d, %d) to (%d, %d)\n", EnterX, EnterY, ExitX, ExitY);

    CTubeData tube;
    memset(tube.Directions, 0xFF, sizeof(tube.Directions));
    tube.EnterX = EnterX;
    tube.EnterY = EnterY;
    tube.ExitX = ExitX;
    tube.ExitY = ExitY;
    
    int nTubeSteps = 0;
    MapCoord coordToMove = { tube.EnterX - tube.ExitX,tube.EnterY - tube.ExitY };
    while (true)
    {
        if (coordToMove.Y < 0 && coordToMove.X < 0)
        {
            coordToMove += MapCoord::Facings[FACING_SOUTH];
            tube.Directions[nTubeSteps++] = FACING_SOUTH;
        }
        else if (coordToMove.Y < 0 && coordToMove.X > 0)
        {
            coordToMove += MapCoord::Facings[FACING_EAST];
            tube.Directions[nTubeSteps++] = FACING_EAST;
        }
        else if (coordToMove.Y < 0 && coordToMove.X == 0)
        {
            coordToMove += MapCoord::Facings[FACING_SOUTHEAST];
            tube.Directions[nTubeSteps++] = FACING_SOUTHEAST;
        }
        else if (coordToMove.Y > 0 && coordToMove.X < 0)
        {
            coordToMove += MapCoord::Facings[FACING_WEST];
            tube.Directions[nTubeSteps++] = FACING_WEST;
        }
        else if (coordToMove.Y > 0 && coordToMove.X > 0)
        {
            coordToMove += MapCoord::Facings[FACING_NORTH];
            tube.Directions[nTubeSteps++] = FACING_NORTH;
        }
        else if (coordToMove.Y > 0 && coordToMove.X == 0)
        {
            coordToMove += MapCoord::Facings[FACING_NORTHWEST];
            tube.Directions[nTubeSteps++] = FACING_NORTHWEST;
        }
        else if (coordToMove.Y == 0 && coordToMove.X < 0)
        {
            coordToMove += MapCoord::Facings[FACING_SOUTHWEST];
            tube.Directions[nTubeSteps++] = FACING_WEST;
        }
        else if (coordToMove.Y == 0 && coordToMove.X > 0)
        {
            coordToMove += MapCoord::Facings[FACING_NORTHEAST];
            tube.Directions[nTubeSteps++] = FACING_NORTHEAST;
        }
        else
        {
            // coordToMove.Y == 0 && coordToMove.X == 0
            break;
        }

        if (nTubeSteps > 100)
        {
            ::MessageBox(NULL, "Cannot generate a too long tube!", "Error", MB_OK);
            return;
        }
    }
    if (nTubeSteps == 0)
        return;
    tube.EnterFacing = tube.Directions[0];
    CMapData::Instance->AddTube(&tube);
    
    // Add a reversed tube
    std::swap(tube.EnterX, tube.ExitX);
    std::swap(tube.EnterY, tube.ExitY);
    
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

    tube.EnterFacing = GetOppositeFacing(tube.EnterFacing);

    for (char* p = tube.Directions, *q = tube.Directions + nTubeSteps - 1; p <= q; ++p, --q)
    {
        char tmp = *p;
        *p = GetOppositeFacing(*q);
        *q = GetOppositeFacing(tmp);
    }

    CMapData::Instance->AddTube(&tube);
}

void CIsoViewExt::DrawLockedCellOutlineX(int X, int Y, int W, int H, COLORREF color, COLORREF colorX, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc)
{
    X += 3;
    Y += 1;
    if (lpDesc->lpSurface == nullptr)
        return;

    RECT rect;
    this->GetWindowRect(&rect);

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

    drawCellOutline2(0);
    drawCellOutline2(-1);

    drawCellOutline(0);
    drawCellOutline(-1);


    //   1
    //  # #
    // 4   2
    //  # #
    //   3'

    //ÒõÓ°
    //ClipAndDrawLine2(x1, y1+1, x2, y2+1);
    //ClipAndDrawLine2(x2, y2+1, x3, y3+1);
    //ClipAndDrawLine2(x3, y3+1, x4, y4+1);
    //ClipAndDrawLine2(x4, y4+1, x1, y1+1);
    //ClipAndDrawLine2(x1, y1+1, x3, y3+1);
    //ClipAndDrawLine2(x2, y2+1, x4, y4+1);
    //
    //ClipAndDrawLine(x1, y1, x2, y2);
    //ClipAndDrawLine(x2, y2, x3, y3);
    //ClipAndDrawLine(x3, y3, x4, y4);
    //ClipAndDrawLine(x4, y4, x1, y1);
    //ClipAndDrawLine(x1, y1, x3, y3);
    //ClipAndDrawLine(x2, y2, x4, y4);
    //
    ////ClipAndDrawLine(x1, y1-1, x2, y2-1);
    ////ClipAndDrawLine(x2, y2-1, x3, y3-1);
    ////ClipAndDrawLine(x3, y3-1, x4, y4-1);
    ////ClipAndDrawLine(x4, y4-1, x1, y1-1);
    ////ClipAndDrawLine(x1, y1-1, x3, y3-1);
    ////ClipAndDrawLine(x2, y2-1, x4, y4-1);
    //
    //
    //// thicker
    //if (!bUseDot)
    //{
    //    ClipAndDrawLine(x1 + 1, y1, x2 + 1, y2);
    //    ClipAndDrawLine(x1 - 1, y1, x2 - 1, y2);
    //    ClipAndDrawLine(x1 + 2, y1, x2 + 2, y2);
    //    ClipAndDrawLine(x1 - 2, y1, x2 - 2, y2);
    //
    //    ClipAndDrawLine(x2 + 1, y2, x3 + 1, y3);
    //    ClipAndDrawLine(x2 - 1, y2, x3 - 1, y3);
    //    ClipAndDrawLine(x2 + 2, y2, x3 + 2, y3);
    //    ClipAndDrawLine(x2 - 2, y2, x3 - 2, y3);
    //
    //    ClipAndDrawLine(x3 + 1, y3, x4 + 1, y4);
    //    ClipAndDrawLine(x3 - 1, y3, x4 - 1, y4);
    //    ClipAndDrawLine(x3 + 2, y3, x4 + 2, y4);
    //    ClipAndDrawLine(x3 - 2, y3, x4 - 2, y4);
    //
    //    ClipAndDrawLine(x4 + 1, y4, x1 + 1, y1);
    //    ClipAndDrawLine(x4 - 1, y4, x1 - 1, y1);
    //    ClipAndDrawLine(x4 + 2, y4, x1 + 2, y1);
    //    ClipAndDrawLine(x4 - 2, y4, x1 - 2, y1);
    //}

}

void CIsoViewExt::DrawLockedCellOutline(int X, int Y, int W, int H, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc, bool s1, bool s2, bool s3, bool s4)
{   
    X += 2;
    Y += 1;
    if (lpDesc->lpSurface == nullptr)
        return;

    RECT rect;
    this->GetWindowRect(&rect);

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
    X += 2;
    if (!hdc)
        return;
    if (!hwnd)
        return;

    RECT rect;
    this->GetWindowRect(&rect);

    COLORREF heightColor = color;
    if (useHeightColor)
    {
        heightColor = CIsoViewExt::_cell_hilight_colors[height];
    }

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
    auto ClipAndDrawLine = [&rect, DrawLine, DrawLineInner](int X1, int Y1, int X2, int Y2, bool inner = false)
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
            if (inner)
                DrawLineInner(X1, Y1, X2, Y2);
            else
                DrawLine(X1, Y1, X2, Y2);
        }
           
    };

    int halfCellWidth = 30 * 1;
    int quaterCellWidth = 15 * 1;
    int fullCellHeight = 30 * 1;
    int halfCellHeight = 15 * 1;

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
    int x3L = x3 - 1;
    int y1L = y1 - 1;
    int y3L = y3;
    y3 += 1;
    x3 -= 2;
    int x4B = x4 - 2;
    int y4B = y4 - 1;
    int x2T = x2 + 2;
    int y2T = y2 + 1;

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
    if (useHeightColor)
        drawCellOutline(-1);


}

void CIsoViewExt::DrawLockedCellOutlinePaint(int X, int Y, int W, int H, COLORREF color, bool bUseDot, HDC hdc, HWND hwnd, bool s1, bool s2, bool s3, bool s4)
{   
    X += 1;
    if (!hdc)
        return;
    if (!hwnd)
        return;

    RECT rect;
    this->GetWindowRect(&rect);


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
    int x3L = x3 - 1;
    int y1L = y1 - 1;
    int y3L = y3;
    y3 += 1;
    x3 -= 2;
    int x4B = x4 - 2;
    int y4B = y4 - 1;
    int x2T = x2 + 2;
    int y2T = y2 + 1;

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

}

void CIsoViewExt::DrawTopRealBorder(int x1, int y1, int x2, int y2, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc)
{
    if (lpDesc->lpSurface == nullptr)
        return;

    RECT rect;
    this->GetWindowRect(&rect);

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
            {



                DrawLine(X1, Y1, X2, Y2);
            }
                
        };

    ClipAndDrawLine(x1, y1, x2, y2);

    //ClipAndDrawLine(x1, y1 - 1, x2, y2 - 1);
    //ClipAndDrawLine(x1, y1 + 1, x2, y2 + 1);
    //ClipAndDrawLine(x1, y1 - 2, x2, y2 - 2);
    //ClipAndDrawLine(x1, y1 + 2, x2, y2 + 2);


}

void CIsoViewExt::DrawLockedLines(const std::vector<std::pair<MapCoord, MapCoord>>& lines, int X, int Y, COLORREF color, bool bUseDot, bool bUsePrimary, LPDDSURFACEDESC2 lpDesc)
{
    if (lpDesc->lpSurface == nullptr)
        return;

    RECT rect;
    this->GetWindowRect(&rect);

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
        CIsoView::MapCoord2ScreenCoord(cX, cY);
        int CellCenterX = cX + 36;
        int CellCenterY = cY - 12;
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
            int xDistance = mouseX - CellCenterX;
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

void CIsoViewExt::DrawBitmap(ppmfc::CString filename, int X, int Y)
{
    this->BltToBackBuffer(ImageDataMapHelper::GetImageDataFromMap(filename + ".bmp")->lpSurface, X, Y, -1, -1);
}

void CIsoViewExt::DrawCelltag(int X, int Y)
{
    this->BltToBackBuffer(ImageDataMapHelper::GetImageDataFromMap("CELLTAG")->lpSurface, X, Y, -1, -1);
}

void CIsoViewExt::DrawWaypointFlag(int X, int Y)
{
    this->BltToBackBuffer(ImageDataMapHelper::GetImageDataFromMap("FLAG")->lpSurface, X + 5, Y, -1, -1);
}

void CIsoViewExt::DrawTube(CellData* pData, int X, int Y)
{
    if (auto pTubeData = CMapData::Instance->GetTubeData(pData->Tube))
    {
        auto suffix = pData->TubeDataIndex;
        if (pData->TubeDataIndex >= 2)
            suffix = pTubeData->Directions[pData->TubeDataIndex - 2] + 2;
        FA2sp::Buffer.Format("TUBE%d", suffix);
        if (auto lpSurface = ImageDataMapHelper::GetImageDataFromMap(FA2sp::Buffer)->lpSurface)
            this->BltToBackBuffer(lpSurface, X + 7, Y + 1, -1, -1);
    }
}

void CIsoViewExt::FillArea(int X, int Y, int ID, int Subtile, int oriX, int oriY)
{
    auto& map = CMapData::Instance;
    if (ID > CMapDataExt::TileDataCount || ID < 0) return;
    
    if (CMapDataExt::TileData[ID].Width != 1 || CMapDataExt::TileData[ID].Height != 1)
    {
        ::MessageBox(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd,
            Translations::TranslateOrDefault("FillAreaNot1x1", "You can only use 1x1 tiles to fill areas."),
            Translations::TranslateOrDefault("Error", "Error"), NULL);
        return;
    }

    auto cell = map->TryGetCellAt(X, Y);
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
            if (cur_x < 1 || cur_y < 1 || cur_x + cur_y<mapwidth + 1 || cur_x + cur_y>mapwidth + mapheight * 2 || (cur_y + 1 > mapwidth && cur_x - 1 < cur_y - mapwidth) || (cur_x + 1 > mapwidth && cur_y + mapwidth - 1 < cur_x)) continue;

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
                FillArea(cur_x, cur_y, ID, Subtile, oriX, oriY);
            }

        }
    }
    CMapDataExt::GetExtension()->PlaceTileAt(X, Y, ID, Subtile);
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
                        BGRStruct oriColor;
                        memcpy(&oriColor, dest, bpp);
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

void CIsoViewExt::BlitTransparent(LPDIRECTDRAWSURFACE7 pic, int x, int y, int width, int height, BYTE alpha)
{
    auto pThis = CIsoView::GetInstance();
    if (pic == NULL) return;

    x += 1;
    y += 1;
    y -= 30;

    RECT r;
    pThis->GetWindowRect(&r);

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

    if (pThis->lpDDBackBufferSurface->Lock(NULL, &destDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) != DD_OK)
        return;

    if (pic->Lock(NULL, &srcDesc, DDLOCK_WAIT | DDLOCK_SURFACEMEMORYPTR, NULL) != DD_OK)
    {
        pThis->lpDDBackBufferSurface->Unlock(NULL);
        return;
    }

    DDCOLORKEY colorKey;
    if (pic->GetColorKey(DDCKEY_SRCBLT, &colorKey) != DD_OK)
    {
        pic->Unlock(NULL);
        pThis->lpDDBackBufferSurface->Unlock(NULL);
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
    pThis->lpDDBackBufferSurface->Unlock(NULL);
}

void CIsoViewExt::BlitSHPTransparent(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClass* pd, Palette* newPal, BYTE alpha, int houseColor)
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

    if (houseColor > -1)
    {
        BGRStruct color;
        auto pRGB = (ColorStruct*)&houseColor;
        color.R = pRGB->red;
        color.G = pRGB->green;
        color.B = pRGB->blue;
        newPal = PalettesManager::GetPalette(newPal, color, true);
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
                            BGRStruct oriColor;
                            memcpy(&oriColor, dest, bpp);
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

void CIsoViewExt::BlitSHPTransparent_Building(CIsoView* pThis, void* dst, const RECT& window,
    const DDBoundary& boundary, int x, int y, ImageDataClass* pd, Palette* newPal, BYTE alpha, int houseColor, int addOnColor)
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

    if (houseColor > -1)
    {
        BGRStruct color;
        auto pRGB = (ColorStruct*)&houseColor;
        color.R = pRGB->red;
        color.G = pRGB->green;
        color.B = pRGB->blue;
        if (LightingStruct::CurrentLighting == LightingStruct::NoLighting)
        {
            newPal = PalettesManager::GetPalette(newPal, color, true);
        }
        else
        {
            newPal = PalettesManager::GetObjectPalette(newPal, color, true,
                CIsoViewExt::CurrentDrawCellLocation, false, 3);
        }
    }

    BGRStruct addOn = { 0,0,0 };
    if (addOnColor > -1)
    {
        auto pRGB = (ColorStruct*)&addOnColor;
        addOn.R = pRGB->red;
        addOn.G = pRGB->green;
        addOn.B = pRGB->blue;
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
                        if (addOnColor > -1)
                        {
                            c.B = (c.B + addOn.B) / 2;
                            c.G = (c.G + addOn.G) / 2;
                            c.R = (c.R + addOn.R) / 2;
                        }
                        if (alpha < 255)
                        {
                            BGRStruct oriColor;
                            memcpy(&oriColor, dest, bpp);
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
    const DDBoundary& boundary, int x, int y, ImageDataClass* pd)
{
    ASSERT(pd->Flag != ImageDataFlag::SurfaceData);

    x += 31;
    y -= 29;
    int bpp = 4;

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
                        BGRStruct oriColor;
                        memcpy(&oriColor, dest, bpp);
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

void CIsoViewExt::BlitSHPTransparent(LPDDSURFACEDESC2 lpDesc, int x, int y, ImageDataClass* pd, Palette* newPal, BYTE alpha, int houseColor)
{  
    auto pThis = CIsoView::GetInstance();
    RECT window;
    pThis->GetWindowRect(&window);
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

            const int spos = i + e * swidth;
            BYTE val = src[spos];

            if (val) {
                auto dest = ((BYTE*)dst + (blrect.left + i) * bpp + (blrect.top + e) * boundary.dpitch);

                if (dest >= dst) {
                    BGRStruct c = newPal->Data[val];
                    if (dest + bpp < surfaceEnd) {
                        if (alpha < 255)
                        {
                            BGRStruct oriColor;
                            memcpy(&oriColor, dest, bpp);
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


BOOL CIsoViewExt::PreTranslateMessageExt(MSG* pMsg)
{
    return CIsoView::PreTranslateMessage(pMsg);
}

BOOL CIsoViewExt::OnMouseWheelExt(UINT Flags, short zDelta, CPoint point)
{
    return TRUE;
}
