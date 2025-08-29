#include "SaveMap.h"

#include <Helpers/Macro.h>

#include <CINI.h>
#include <CFinalSunApp.h>
#include <CFinalSunDlg.h>
#include <CLoading.h>
#include <CMapData.h>

#include "../FA2sp.h"
#include "../FA2sp.Constants.h"

#include "../Algorithms/sha1.h"
#include "../Algorithms/base64.h"

#include <algorithm>
#include <map>
#include <sstream>
#include <fstream>
#include <format>
#include "FileWatcher.h"
#include "../Helpers/STDHelpers.h"
#include "../Algorithms/lcw.h"
#include "../Algorithms/lzo.h"
#include "../Helpers/Translations.h"
#include "../Ext/CMapData/Body.h"
#include "Palettes.h"
#include "Hooks.INI.h"

std::optional<std::filesystem::file_time_type> SaveMapExt::SaveTime;

DEFINE_HOOK(4D5505, CSaveOption_CTOR_DefaultValue, 0)
{
    int nValue = CMapData::Instance->IsMultiOnly() ? 
        ExtConfigs::SaveMap_DefaultPreviewOptionMP : 
        ExtConfigs::SaveMap_DefaultPreviewOptionSP
        ;

    R->EBX(std::clamp(nValue, 0, 2));
    
    return 0x4D550E;
}

// FA2 SaveMap is almost O(N^4), who wrote that?
DEFINE_HOOK(428D97, CFinalSunDlg_SaveMap, 7)
{
    GET(CINI*, pINI, EAX);
    GET_STACK(CFinalSunDlg*, pThis, STACK_OFFS(0x3F4, 0x36C));
    REF_STACK(ppmfc::CString, filepath, STACK_OFFS(0x3F4, -0x4));

    GET_STACK(int, previewOption, STACK_OFFS(0x3F4, 0x1AC));

    if (SaveMapExt::IsAutoSaving)
        previewOption = 2; //no preview to save time

    pThis->MyViewFrame.StatusBar.SetWindowText(Translations::TranslateOrDefault("SavingMap", "Saving map..."));
    pThis->MyViewFrame.StatusBar.UpdateWindow();

    FileWatcher::IsSavingMap = true;

    ppmfc::CString buffer;
    buffer.Format("%d", pINI->GetInteger("FA2spVersionControl", "Version") + 1);
    pINI->WriteString("FA2spVersionControl", "Version", buffer);

    Logger::Raw("SaveMap : Now removing empty sections and keys.\n");
    std::vector<ppmfc::CString> sectionsToRemove;
    for (auto& section_pair : pINI->Dict)
    {
        buffer = section_pair.first;
        buffer.Trim();
        if (buffer.GetLength() == 0 || section_pair.second.GetEntities().size() == 0)
            sectionsToRemove.push_back(section_pair.first);

        std::vector<ppmfc::CString> keysToRemove;
        for (auto& key_pair : section_pair.second.GetEntities())
        {
            buffer = key_pair.first;
            buffer.Trim();
            if (buffer.GetLength() == 0)
                keysToRemove.push_back(key_pair.first);
        }

        for (auto& key : keysToRemove)
            pINI->DeleteKey(section_pair.first, key);

        if (section_pair.second.GetEntities().size() == 0)
            sectionsToRemove.push_back(section_pair.first);
    }
    for (auto& section : sectionsToRemove)
        pINI->DeleteSection(section);

    if (previewOption == 2)
    {
        // No preview / hidden preview.
        Logger::Raw("SaveMap : Generating a hidden map preview.\n");
        pINI->DeleteSection("Preview");
        pINI->DeleteSection("PreviewPack");
        pINI->WriteString("Preview", "Size", "0,0,1,1");
        pINI->WriteString("PreviewPack", "1", "BwADABQAAAARAAA=");
    }
    else if (previewOption == 0)
    {
        // Generate new preview.
        Logger::Raw("SaveMap : Generating a new map preview.\n");

        if (ExtConfigs::SaveMaps_BetterMapPreview && CMapData::Instance->IsMultiOnly())
        {
            auto image = std::unique_ptr<unsigned char[]>(new unsigned char[256 * 512 * 3] {0});
            auto imageLocal = std::unique_ptr<unsigned char[]>(new unsigned char[256 * 512 * 3] {0});

            auto safeColorBtye = [](int x)
                {
                    if (x > 255)
                        x = 255;
                    if (x < 0)
                        x = 0;
                    return (byte)x;
                };
            auto heightExtraLight = [safeColorBtye](int rgb, int h, LightingStruct ret)
                {
                    return safeColorBtye(rgb * (ret.Ambient - ret.Ground + ret.Level * h));
                };
            auto isSafePos = [](int x, int y)
                {
                    int dPows = x * CMapData::Instance().MapWidthPlusHeight + y;
                    if (dPows < CMapData::Instance().CellDataCount)
                        return true;
                    return false;
                };
            auto getPos = [](int x, int y)
                {
                    int dPows = x * CMapData::Instance().MapWidthPlusHeight + y;
                    if (dPows < CMapData::Instance().CellDataCount)
                        return dPows;
                    return 0;
                };

            std::vector<int[2]>playerLocation;

            pINI->DeleteSection("Preview");
            pINI->DeleteSection("PreviewPack");

            auto& map = CINI::CurrentDocument();
            auto thisTheater = map.GetString("Map", "Theater");

            CTileTypeClass* tiledata = nullptr;
            if (thisTheater == "TEMPERATE")
                tiledata = CTileTypeInfo::Temperate().Datas;
            if (thisTheater == "SNOW")
                tiledata = CTileTypeInfo::Snow().Datas;
            if (thisTheater == "URBAN")
                tiledata = CTileTypeInfo::Urban().Datas;
            if (thisTheater == "NEWURBAN")
                tiledata = CTileTypeInfo::NewUrban().Datas;
            if (thisTheater == "LUNAR")
                tiledata = CTileTypeInfo::Lunar().Datas;
            if (thisTheater == "DESERT")
                tiledata = CTileTypeInfo::Desert().Datas;

            auto size = STDHelpers::SplitString(map.GetString("Map", "Size", "0,0,0,0"));
            auto lSize = STDHelpers::SplitString(map.GetString("Map", "LocalSize", "0,0,0,0"));

            int mapwidth = atoi(size[2]);
            int mapheight = atoi(size[3]);

            int mpL = atoi(lSize[0]);
            int mpT = atoi(lSize[1]);
            int mpW = atoi(lSize[2]);
            int mpH = atoi(lSize[3]);

            int lb = mpL * 2 - 1;
            int rb = (mpL + mpW) * 2 - 1;
            int tb = mpT - 2 - 2;
            int bb = mpT + mpH + 2 - 1;
            int lwidth = rb - lb + 1;
            int lheight = bb - tb + 1;


            auto& mapData = CMapData::Instance();

            ppmfc::CString pSize;
            pSize.Format("0,0,%d,%d", lwidth, lheight);
            pINI->WriteString("Preview", "Size", pSize);

            std::vector<MapCoord> PlayerLocations;

            for (auto& cell : CMapDataExt::CellDataExts)
            {
                cell.AroundPlayerLocation = false;
                cell.AroundHighBridge = false;
            }
            for (int i = 0; i < mapData.CellDataCount; i++)
            {
                CellDataExt& cellExt = CMapDataExt::CellDataExts[i];
                CellData& cell = mapData.CellDatas[i];
                int X = i / mapData.MapWidthPlusHeight;
                int Y = i % mapData.MapWidthPlusHeight;

                if (mapData.IsMultiOnly() && cell.Waypoint != -1)
                {
                    auto pSection = CINI::CurrentDocument->GetSection("Waypoints");
                    auto& pWP = *pSection->GetKeyAt(cell.Waypoint);
                    if (atoi(pWP) < 8)
                    {
                        bool found = false;
                        MapCoord pl;
                        pl.X = 0;
                        pl.Y = 0;

                        for (int y = 0; y < mapheight; y++)
                        {
                            for (int x = 0; x < mapwidth * 2; x++)
                            {
                                int dx = x;
                                int dy = y * 2 + x % 2;
                                int rx = (dx + dy) / 2 + 1;
                                int ry = dy - rx + mapwidth + 1;

                                if (rx == X && ry == Y)
                                {
                                    pl.X = x;
                                    pl.Y = y;
                                    found = true;
                                    break;
                                }
                            }
                            if (found)
                                break;
                        }

                        PlayerLocations.push_back(pl);
                    }
                }

                auto overlay = cell.Overlay;
                auto overlayD = cell.OverlayData;
                if (overlay == 24 || overlay == 25 || overlay == 237 || overlay == 238) //high bridge
                {
                    if (overlayD >= 0 && overlayD <= 8) //NW-SE
                    {
                        CMapDataExt::CellDataExts[getPos(X, Y)].AroundHighBridge = true;
                        CMapDataExt::CellDataExts[getPos(X, Y - 1)].AroundHighBridge = true;
                        CMapDataExt::CellDataExts[getPos(X, Y + 1)].AroundHighBridge = true;

                    }
                    else if (overlayD >= 9 && overlayD <= 17) //NE-SW
                    {
                        CMapDataExt::CellDataExts[getPos(X, Y)].AroundHighBridge = true;
                        CMapDataExt::CellDataExts[getPos(X - 1, Y)].AroundHighBridge = true;
                        CMapDataExt::CellDataExts[getPos(X + 1, Y)].AroundHighBridge = true;
                    }
                }


            }

            int index = 0;
            int index2 = 0;
            for (int y = 0; y < mapheight; y++)
            {
                for (int x = 0; x < mapwidth * 2; x++)
                {
                    int dx = x;
                    int dy = y * 2 + x % 2;
                    int rx = (dx + dy) / 2 + 1;
                    int ry = dy - rx + mapwidth + 1;


                    int dPows = rx * mapData.MapWidthPlusHeight + ry;
                    if (dPows < mapData.CellDataCount)
                    {
                        if (mapData.IsCoordInMap(ry, rx))
                        {
                            CellDataExt& cellExt = CMapDataExt::CellDataExts[dPows];
                            CellData& cell = mapData.CellDatas[dPows];
                            int tileIndex = cell.TileIndex;
                            if (tileIndex == 65535)
                                tileIndex = 0;

                            auto colorL = tiledata[tileIndex].TileBlockDatas[cell.TileSubIndex].RadarColorLeft;
                            RGBClass2 color;
                            //RadarColorLeft is BGR
                            color.R = colorL.B;
                            color.G = colorL.G;
                            color.B = colorL.R;

                            auto overlay = cell.Overlay;
                            auto overlayD = cell.OverlayData;
                            if (overlay != 255)
                            {
                                auto radarColor = CMapDataExt::GetOverlayTypeData(overlay).RadarColor;
                                if (overlay >= 27 && overlay <= 38) //gems
                                    color = RGB(radarColor.R, radarColor.G, radarColor.B);
                                else if (overlay >= 102 && overlay <= 166) //ores
                                    color = RGB(radarColor.R, radarColor.G, radarColor.B);
                                else if (overlay == 100 || overlay == 101 || overlay == 231 || overlay == 232) //broken bridge
                                { }
                                else if (overlay == 24 || overlay == 25 || overlay == 237 || overlay == 238) //high bridge
                                { }
                                else
                                    color = RGB(91, 91, 93);
                            }
                            if (cellExt.AroundHighBridge)
                                color = RGB(107, 109, 107);

                            int type = cell.TerrainType;
                            std::string name;
                            if (auto pTerrain = CINI::Rules().GetSection("TerrainTypes"))
                            {
                                int index = 0;
                                for (auto& pT : pTerrain->GetEntities())
                                {
                                    if (index == type)
                                    {
                                        name = pT.second;
                                        break;
                                    }

                                    index++;
                                }
                            }
                            if (!name.empty())
                            {
                                if (name.find("TREE") != std::string::npos)
                                    color = RGB(0, 194, 0);
                                else if (name.find("TIBTRE") != std::string::npos)
                                    color = RGB(10, 10, 10);
                                else
                                    color = RGB(69, 68, 69);
                            }

                            //no need to get house color
                            if (cell.Structure != -1 || cell.Infantry[0] != -1 || cell.Infantry[1] != -1 || cell.Infantry[2] != -1 || cell.Unit != -1 || cell.Aircraft != -1)
                                color = RGB(123, 125, 123);
                            if (cell.Structure != -1)
                            {
                                auto pSection = CINI::CurrentDocument->GetSection("Structures");
                                auto& pStr = *pSection->GetValueAt(cell.Structure);
                                auto atoms = STDHelpers::SplitString(pStr, 1);
                                auto& name = atoms[1];

                                if (auto pSection2 = CINI::FAData->GetSection("NeuralTechStructure"))
                                {
                                    for (auto& nameL : pSection2->GetEntities())
                                        if (nameL.second == name)
                                        {
                                            color = RGB(215, 215, 215);
                                            break;
                                        }
                                            
                                }
                            }

                            LightingStruct ret;
                            switch (CFinalSunDlgExt::CurrentLighting)
                            {
                            case 31001:
                            case 31002:
                            case 31003:
                                ret = LightingStruct::GetCurrentLighting();
                                break;
                            default:
                                ret.Red = 1.0f;
                                ret.Green = 1.0f;
                                ret.Blue = 1.0f;
                                ret.Ambient = 1.0f;
                                ret.Ground = 0.0f;
                                ret.Level = 0.0078125f;
                                break;
                            }

                            color.R = safeColorBtye(heightExtraLight(color.R, cell.Height, ret) * ret.Red);
                            color.G = safeColorBtye(heightExtraLight(color.G, cell.Height, ret) * ret.Green);
                            color.B = safeColorBtye(heightExtraLight(color.B, cell.Height, ret) * ret.Blue);

                            for (auto& pl : PlayerLocations)
                            {
                                if (pl.X - x <= 2 && pl.X - x >= -1 && pl.Y - y <= 2 && pl.Y - y >= -1)
                                    color = RGB(240, 0, 0);
                            }    

                            byte r = (byte)color.R;
                            byte g = (byte)color.G;
                            byte b = (byte)color.B;
                            
                            image[index++] = r;
                            image[index++] = g;
                            image[index++] = b;

                        }
                        else
                        {
                            image[index++] = 0;
                            image[index++] = 0;
                            image[index++] = 0;
                        }
                        //get localsize preview
                        if (x >= lb && x <= rb
                            && y >= tb && y <= bb)
                        {
                            imageLocal[index2++] = image[index - 3];
                            imageLocal[index2++] = image[index - 2];
                            imageLocal[index2++] = image[index - 1];
                        }
                    }
                }
            }


            auto data = lzo::compress(imageLocal.get(), sizeof(byte) * 3 * lwidth * lheight);
            data = base64::encode(data);
            pINI->WriteBase64String("PreviewPack", data.data(), data.length());
        }
        else
            CMapData::Instance->UpdateINIFile(SaveMapFlag::UpdatePreview);
    }
    else
    {
        // Do not update preview.
        Logger::Raw("SaveMap : Retaining current map preview.\n");
    }

    Logger::Raw("SaveMap : Trying to save map to %s.\n", filepath);
    
    std::ofstream fout;
    fout.open(filepath, std::ios::out | std::ios::trunc);
    if (fout.is_open())
    {
        pINI->DeleteSection("Digest");

        std::ostringstream oss;
        FString comments;

        if (ExtConfigs::SaveMap_FileEncodingComment)
        {
            comments += "; ";
            comments += Translations::TranslateOrDefault("SaveMap_FileEncodingComment1", "���ļ�����Ϊ ANSI/GBK����ʹ�ô˸�ʽ��");
            comments += "\n";
            comments += "; ";
            comments += Translations::TranslateOrDefault("SaveMap_FileEncodingComment2", "Warning: If the first line appears as gibberish");
            comments += "\n";
            comments += "; ";
            comments += Translations::TranslateOrDefault("SaveMap_FileEncodingComment3", "and Chinese characters are used, DO NOT modify this file");
            comments += "\n";
            comments += "\n";
        }
        
        comments += "; Map created with FinalAlert 2(tm) Mission Editor\n";
        comments += "; Get it at http://www.westwood.com\n";
        comments += "; note that all comments were truncated\n";
        comments += "\n";
        comments += "; This FA2 uses FA2sp created by secsome, modified by Handama & E1Elite\n";
        comments += "; Get the lastest dll at https://github.com/handama/FA2sp\n";
        comments += "; Current version : "  PRODUCT_STR  ", "  __str(HDM_PRODUCT_VERSION)  "\n\n";

        oss << comments;

        auto saveSection = [&oss](INISection* pSection, FString sectionName)
            {
                auto& exclude = INIIncludes::MapIncludedKeys;
                if (!exclude.empty() && exclude.find(sectionName) != exclude.end())
                {
                    std::vector<int> skipLines;
                    std::vector<int> useOriginLines;

                    auto& keys = exclude[sectionName];
                    int index = 0;
                    for (auto& pair : pSection->GetEntities())
                    {
                        if (keys.find(pair.first) != keys.end())
                        {
                            if (keys[pair.first] == "")
                            {
                                skipLines.push_back(index);
                            }
                            else
                            {
                                useOriginLines.push_back(index);
                            }
                        }
                        index++;
                    }
                    if (skipLines.size() < pSection->GetEntities().size())
                    {
                        oss << "[" << sectionName << "]\n";
                        index = 0;
                        for (auto& pair : pSection->GetEntities())
                        {
                            if (std::find(skipLines.begin(), skipLines.end(), index) != skipLines.end())
                            {

                            }
                            else if (std::find(useOriginLines.begin(), useOriginLines.end(), index) != useOriginLines.end())
                            {
                                oss << pair.first << "=" << keys[pair.first] << "\n";
                            }
                            else
                            {
                                oss << pair.first << "=" << pair.second << "\n";
                            }
                            index++;
                        }
                        oss << "\n";
                    }
                }
                else
                {
                    oss << "[" << sectionName << "]\n";
                    for (const auto& pair : pSection->GetEntities())
                        oss << pair.first << "=" << pair.second << "\n";
                    oss << "\n";
                }
            };

        if (!SaveMapExt::IsAutoSaving && ExtConfigs::SaveMap_PreserveINISorting)
        {
            if (!pINI->SectionExists("Header"))
            {
                pINI->WriteString("Header", "NumberStartingPoints", "0");
            }
            for (const auto& sectionName : CMapDataExt::MapIniSectionSorting)
            {
                if (sectionName == "Digest")
                    continue;
                if (const auto pSection = pINI->GetSection(sectionName))
                {
                    saveSection(pSection, sectionName);
                }
            }
            for (auto& section : pINI->Dict)
            {
                if (!strcmp(section.first, "Digest"))
                    continue;

                auto it = std::find(CMapDataExt::MapIniSectionSorting.begin(), CMapDataExt::MapIniSectionSorting.end(), section.first);
                if (it == CMapDataExt::MapIniSectionSorting.end())
                {
                    saveSection(&section.second, section.first);
                }
            }
        }
        else
        {
            // Add "Header" for single-player map to prevent loading error
            if (const auto pSection = pINI->GetSection("Header"))
            {
                oss << "[Header]\n";
                for (const auto& pair : pSection->GetEntities())
                    oss << pair.first << "=" << pair.second << "\n";
                oss << "\n";
            }
            else if (!CMapData::Instance->IsMultiOnly())
            {
                oss << "[Header]\n";
                oss << "NumberStartingPoints" << "=" << "0" << "\n";
                oss << "\n";
            }

            // Dirty fix: vanilla YR needs "Preview" and "PreviewPack" before "Map"
            // So we just put them at first.
            if (const auto pSection = pINI->GetSection("Preview"))
            {
                oss << "[Preview]\n";
                for (const auto& pair : pSection->GetEntities())
                    oss << pair.first << "=" << pair.second << "\n";
                oss << "\n";
            }
            if (const auto pSection = pINI->GetSection("PreviewPack"))
            {
                oss << "[PreviewPack]\n";
                for (const auto& pair : pSection->GetEntities())
                    oss << pair.first << "=" << pair.second << "\n";
                oss << "\n";
            }

            for (auto& section : pINI->Dict)
            {
                if (!strcmp(section.first, "Preview") 
                    || !strcmp(section.first, "PreviewPack") 
                    || !strcmp(section.first, "Header") 
                    || !strcmp(section.first, "Digest"))
                    continue;

                saveSection(&section.second, section.first);
            }
        }

        // Generate the Digest
        unsigned char hash[20];
        const auto& hash_source = oss.str();
        SHA1::hash(hash, hash_source.data(), hash_source.length());
        
        char hash_value[64] = { 0 };
        sprintf_s(
            hash_value,
            "%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x",
            hash[0], hash[1], hash[2], hash[3], hash[4],
            hash[5], hash[6], hash[7], hash[8], hash[9],
            hash[10], hash[11], hash[12], hash[13], hash[14],
            hash[15], hash[16], hash[17], hash[18], hash[19]
            );
        Logger::Raw("SaveMap : Map SHA1 hash: %s\n", hash_value);

        // As sha1 hash length is only 20, the length of base64 result won't
        // go over the limitation of uublock's 70 per line. So only one row!
        oss << "[Digest]\n1=" << base64::encode(hash, 20) << "\n";

        // Now just write the file
        fout << oss.str();
        fout.flush();
        fout.close();

        Logger::Raw("SaveMap : Successfully saved %u sections.\n", pINI->Dict.size());
    }
    else
    {
        ppmfc::CString buffer;
        buffer.Format("Failed to create file %s.\n", filepath);
        Logger::Put(buffer);
        ppmfc::CString buffer2;
        buffer2.Format(Translations::TranslateOrDefault("CannotCreateFile", "Cannot create file: %s.\n"), filepath);
        ::MessageBox(NULL, buffer2, Translations::TranslateOrDefault("Error", "Error"), MB_OK | MB_ICONERROR);
    }

    return 0x42A859;
}

DEFINE_HOOK(42B30F, CFinalSunDlg_SaveMap_SkipMapDTOR, 7)
{
    return 0x42B323;
}

DEFINE_HOOK(42B2AF, CFinalSunDlg_SaveMap_SkipDeleteFile, 7)
{
    return 0x42B2C2;
}

DEFINE_HOOK(42686A, CFinalSunDlg_SaveMap_SetDefaultExtension, 5)
{
    int defaultExtention = 1;
    
    if (ExtConfigs::SaveMap_OnlySaveMAP)
    {
        defaultExtention = 4;
    }
    else if (CMapData::Instance->IsMultiOnly() && CLoading::HasMdFile())
    {
        defaultExtention = 2;
    }
    else if (CMapData::Instance->IsMultiOnly() && !CLoading::HasMdFile())
    {
        defaultExtention = 3;
    }
    else if (!CMapData::Instance->IsMultiOnly())
    {
        defaultExtention = 4;
    }

    R->Stack<int>(STACK_OFFS(0x3CC, (0x280 + 0x14)), defaultExtention);

    return 0;
}

//ppmfc::CString filePath;
//DEFINE_HOOK(4268DC, CFinalSunDlg_SaveMap_RenameMapPath, 7)
//{
//    GET(CFinalSunDlg*, pThis, ECX);
//
//    filePath = CFinalSunApp::Instance().MapPath();
//
//    if (ExtConfigs::SaveMap_OnlySaveMAP)
//    {
//        int nExtIndex = filePath.ReverseFind('.');
//        if (nExtIndex == -1)
//            filePath += ".map";
//        else
//            filePath = filePath.Mid(0, nExtIndex) + ".map";
//    }
//    else if (ExtConfigs::SaveMap_MultiPlayOnlySaveYRM && CMapData::Instance->IsMultiOnly())
//    {
//        int nExtIndex = filePath.ReverseFind('.');
//        if (nExtIndex == -1)
//            filePath += ".yrm";
//        else
//            filePath = filePath.Mid(0, nExtIndex) + ".yrm";
//    }
//    else if (!CMapData::Instance->IsMultiOnly() && ExtConfigs::SaveMap_SinglePlayOnlySaveMAP)
//    {
//        int nExtIndex = filePath.ReverseFind('.');
//        if (nExtIndex == -1)
//            filePath += ".map";
//        else
//            filePath = filePath.Mid(0, nExtIndex) + ".map";
//    }
//
//    strcpy(CFinalSunApp::Instance().MapPath, filePath);
//
//    return 0;
//}
//
//DEFINE_HOOK(426921, CFinalSunDlg_SaveMap_RenameMapPath2, 6)
//{
//    R->Stack<LPCSTR>(STACK_OFFS(0x3CC, 0x3BC), filePath);
//    return 0;
//}

DEFINE_HOOK(42A8F5, CFinalSunDlg_SaveMap_ReplaceCopyFile, 7)
{
    REF_STACK(ppmfc::CString, filepath, STACK_OFFS(0x3F4, -0x4));

    std::ifstream fin;
    fin.open(filepath, std::ios::in | std::ios::binary);
    if (fin.is_open())
    {
        fin.close();

        if (!SaveMapExt::IsAutoSaving) {
            SaveMapExt::SaveTime = std::filesystem::last_write_time(filepath.m_pchData);
            FileWatcher::IsMapJustSaved = true;
            FileWatcher::IsSavingMap = false;
        }
        return 0x42A92D;
    }
    return 0x42A911;
}
 
DEFINE_HOOK(42B2EA, CFinalSunDlg_SaveMap_SkipStringDTOR, C)
{
    return 0x42B30F;
}

void SaveMapExt::ResetTimer()
{
    StopTimer();
    if (ExtConfigs::SaveMap_AutoSave_Interval >= 30)
    {
        if (Timer = SetTimer(NULL, NULL, 1000 * ExtConfigs::SaveMap_AutoSave_Interval, SaveMapCallback))
            Logger::Debug("Successfully created timer with ID = %p.\n", Timer);
        else
            Logger::Debug("Failed to create timer! Auto-save is currently unable to use!\n");
    }
}

void SaveMapExt::StopTimer()
{
    if (Timer != NULL)
    {
        KillTimer(NULL, Timer);
        Timer = NULL;
    }
}

void SaveMapExt::RemoveEarlySaves()
{
    if (ExtConfigs::SaveMap_AutoSave_MaxCount != -1)
    {
        struct FileTimeComparator
        {
            bool operator()(const FILETIME& a, const FILETIME& b) const { return CompareFileTime(&a, &b) == -1; }
        };

        std::map<FILETIME, ppmfc::CString, FileTimeComparator> m;
        auto mapName = CINI::CurrentDocument->GetString("Basic", "Name", "No Name");

        /*
        * Fix : Windows file name cannot begin with space and cannot have following characters:
        * \ / : * ? " < > |
        */
        for (int i = 0; i < mapName.GetLength(); ++i)
            if (mapName[i] == '\\' || mapName[i] == '/' || mapName[i] == ':' ||
                mapName[i] == '*' || mapName[i] == '?' || mapName[i] == '"' ||
                mapName[i] == '<' || mapName[i] == '>' || mapName[i] == '|'
                )
                mapName.SetAt(i, '-');

        const auto ext =
            !ExtConfigs::SaveMap_OnlySaveMAP && CMapData::Instance->IsMultiOnly() ?
            CLoading::HasMdFile() ?
            "yrm" :
            "mpr" :
            "map";

        ppmfc::CString buffer;
        buffer.Format("%s\\AutoSaves\\%s\\%s-*.%s",
            CFinalSunApp::ExePath(),
            mapName,
            mapName,
            ext
        );

        WIN32_FIND_DATA Data;
        auto hFindData = FindFirstFile(buffer, &Data);
        while (hFindData != INVALID_HANDLE_VALUE)
        {
            m[Data.ftLastWriteTime] = Data.cFileName;
            if (!FindNextFile(hFindData, &Data))
                break;
        }

        int count = m.size() - ExtConfigs::SaveMap_AutoSave_MaxCount;
        if (count <= 0)
            return;

        auto itr = m.begin();
        while (count != 0)
        {
            buffer.Format("%s\\AutoSaves\\%s\\%s", CFinalSunApp::ExePath(), mapName, itr->second);
            DeleteFile(buffer);
            ++itr;
            --count;
        }
    }
}

void CALLBACK SaveMapExt::SaveMapCallback(HWND hwnd, UINT message, UINT iTimerID, DWORD dwTime)
{
    Logger::Debug("SaveMapCallback called, trying to auto save map. hwnd = %08X, message = %d, iTimerID = %d, dwTime = %d.\n",
        (int)hwnd, message, iTimerID, dwTime);

    if (!CMapData::Instance->MapWidthPlusHeight || !CMapData::Instance->FieldDataAllocated)
    {
        StopTimer();
        return;
    }
    if (CIsoView::GetInstance()->lpDDPrimarySurface->IsLost() != DD_OK)
    {
        StopTimer();
        return;
    }

    SYSTEMTIME time;
    GetLocalTime(&time);

    auto mapName = CINI::CurrentDocument->GetString("Basic", "Name", "No Name");

    /*
    * Fix : Windows file name cannot begin with space and cannot have following characters:
    * \ / : * ? " < > |
    */
    for (int i = 0; i < mapName.GetLength(); ++i)
        if (mapName[i] == '\\' || mapName[i] == '/' || mapName[i] == ':' ||
            mapName[i] == '*' || mapName[i] == '?' || mapName[i] == '"' ||
            mapName[i] == '<' || mapName[i] == '>' || mapName[i] == '|'
            )
            mapName.SetAt(i, '-');

    const auto ext =
        !ExtConfigs::SaveMap_OnlySaveMAP && CMapData::Instance->IsMultiOnly() ?
        CLoading::HasMdFile() ?
        "yrm" :
        "mpr" :
        "map";

    ppmfc::CString buffer = CFinalSunApp::ExePath();
    buffer += "\\AutoSaves\\";
    CreateDirectory(buffer, nullptr);
    buffer += mapName;
    CreateDirectory(buffer, nullptr);

    buffer.Format("%s\\AutoSaves\\%s\\%s-%04d%02d%02d-%02d%02d%02d-%03d.%s",
        CFinalSunApp::ExePath(),
        mapName,
        mapName,
        time.wYear, time.wMonth, time.wDay,
        time.wHour, time.wMinute, time.wSecond,
        time.wMilliseconds,
        ext
    );

    IsAutoSaving = true;
    CFinalSunDlg::Instance->SaveMap(buffer);
    IsAutoSaving = false;

    RemoveEarlySaves();
}

bool SaveMapExt::IsAutoSaving = false;
UINT_PTR SaveMapExt::Timer = NULL;


DEFINE_HOOK(426E50, CFinalSunDlg_SaveMap_AutoSave_StopTimer, 7)
{
    SaveMapExt::StopTimer();
    return 0;
}

DEFINE_HOOK(42B3AC, CFinalSunDlg_SaveMap_AutoSave_ResetTimer, 7)
{
    SaveMapExt::ResetTimer();
    return 0;
}

DEFINE_HOOK(427949, CFinalSunDlg_SaveMap_AutoSave_SkipDialog, A)
{
    return SaveMapExt::IsAutoSaving ? 0x428CF6 : 0;
}

DEFINE_HOOK(42B294, CFinalSunDlg_SaveMap_AutoSave_SkipEditFilesMenu, 8)
{
    return SaveMapExt::IsAutoSaving ? 0x42B2AF : 0;
}

DEFINE_HOOK(437D84, CFinalSunDlg_LoadMap_StopTimer, 5)
{
    if (ExtConfigs::SaveMap_AutoSave)
        SaveMapExt::StopTimer();
    return 0;
}

DEFINE_HOOK(438D90, CFinalSunDlg_LoadMap_ResetTimer, 7)
{
    if (ExtConfigs::SaveMap_AutoSave && CMapData::Instance->MapWidthPlusHeight)
        SaveMapExt::ResetTimer();
    return 0;
}

DEFINE_HOOK(42CBE0, CFinalSunDlg_CreateMap_StopTimer, 5)
{
    if (ExtConfigs::SaveMap_AutoSave)
        SaveMapExt::StopTimer();
    return 0;
}

DEFINE_HOOK(42E18E, CFinalSunDlg_CreateMap_ResetTimer, 7)
{
    if (ExtConfigs::SaveMap_AutoSave && CMapData::Instance->MapWidthPlusHeight)
        SaveMapExt::ResetTimer();
    return 0;
}