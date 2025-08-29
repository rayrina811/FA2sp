#include "Body.h"
#include <iostream>
#include <cmath>
#include <corecrt_math_defines.h>
#include "../../Source/CIsoView.h"
#include <random>
#include <chrono>
#include <numeric>
#include "../../Miscs/MultiSelection.h"
#include "../../FA2sp.h"

static unsigned seed;
static std::mt19937 rng;
static std::uniform_real_distribution<float> dist; 

static float gradient(int x, int y) {
    rng.seed(x * 374761393 + y * 668265263 + seed);
    return dist(rng); 
}

static float lerp(float a, float b, float t) {
    return a + t * (b - a);
}

static float fade(float t) {
    return t * t * t * (t * (t * 6 - 15) + 10);
}

static float perlinNoise(float x, float y) {
    int x0 = (int)floor(x);
    int y0 = (int)floor(y);
    int x1 = x0 + 1;
    int y1 = y0 + 1;

    float dx = x - x0;
    float dy = y - y0;

    float g00 = gradient(x0, y0);
    float g01 = gradient(x0, y1);
    float g10 = gradient(x1, y0);
    float g11 = gradient(x1, y1);

    float u = fade(dx);
    float v = fade(dy);

    float nx0 = lerp(g00, g10, u);
    float nx1 = lerp(g01, g11, u);
    return lerp(nx0, nx1, v);
}

static std::vector<int> selectTile(float noiseValue, std::vector<std::pair<std::vector<int>, float>>& tiles) {
    float totalProbability = 0.0f;
    for (const auto& tile : tiles) {
        totalProbability += tile.second;
    }

    if (totalProbability == 0.0f) {
        return std::vector<int>{0};
    }

    for (auto& tile : tiles) {
        tile.second /= totalProbability; 
    }

    std::vector<float> cumulativeProbabilities;
    float cumulative = 0.0f;
    for (const auto& tile : tiles) {
        cumulative += tile.second;
        cumulativeProbabilities.push_back(cumulative);
    }

    float normalizedValue = (noiseValue + 1.0f) * 0.5f;

    for (size_t i = 0; i < cumulativeProbabilities.size(); ++i) {
        if (normalizedValue <= cumulativeProbabilities[i]) {
            return tiles[i].first; 
        }
    }

    return tiles.back().first;
}

static int GetIndexByWeights(std::vector<float> weights) {
    float totalWeight = accumulate(weights.begin(), weights.end(), 0.0f);

    if (totalWeight > 1.0f) {
        for (float& weight : weights) {
            weight /= totalWeight; 
        }
        totalWeight = 1.0f;
    }

    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    float randomValue = dis(gen); 

    if (totalWeight < 1.0f) {
        if (randomValue > totalWeight) {
            return -1; 
        }
    }

    float cumulativeWeight = 0.0f;
    for (size_t i = 0; i < weights.size(); ++i) {
        cumulativeWeight += weights[i];
        if (randomValue <= cumulativeWeight) {
            return static_cast<int>(i);
        }
    }

    return -1;
}


static std::pair<float, float> rotateCoordinates(float x, float y, float angle) {
    float rad = angle * M_PI / 180.0f; 
    float cosA = cos(rad);
    float sinA = sin(rad);
    float newX = cosA * x - sinA * y;
    float newY = sinA * x + cosA * y;
    return { newX, newY };
}

static std::vector<int> getIgnoreTileSets()
{
    std::vector<int> roadSets;
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "PavedRoads", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "PavedRoadEnds", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "DirtRoadJunction", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "DirtRoadCurve", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "DirtRoadStraight", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "RoughGround", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "Medians", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "CliffRamps", -1));
    roadSets.push_back(CINI::CurrentTheater->GetInteger("General", "SlopeSetPieces", -1));
    return roadSets;
}
static bool isTerrainTypeIgnored(int ttype)
{
    switch (ttype)
    {
    case Rock7:
    case Rock8:
    case CliffRock:
    case Water:
    case Beach:
    case Railroad:
    case Tunnel:
        return true;
        break;
    default:
        break;
    }
    return false;
}

void CMapDataExt::CreateRandomGround(int TopX, int TopY, int BottomX, int BottomY, int scale, std::vector<std::pair<std::vector<int>, float>> tiles, bool override, bool multiSelection, bool onlyClear)
{
    seed = std::chrono::system_clock::now().time_since_epoch().count();
    rng = std::mt19937(seed); 
    dist = std::uniform_real_distribution<float>(-1.0f, 1.0f); 

    std::vector<int> roadSets = getIgnoreTileSets();

    int randomAngle = STDHelpers::RandomSelectInt(0, 180);

    // used tile sets
    std::vector<int> tileindexes;

    // add clear tile if totalProbability < 1.0
    float totalProbability = 0.0f;
    for (const auto& tile : tiles) {
        totalProbability += tile.second;
        for (const auto& idx : tile.first) {
            tileindexes.push_back(idx);
        }
    }

    if (totalProbability < 1.0f) {
        tiles.push_back(std::make_pair(std::vector<int>{0xFFFF}, 1.0f - totalProbability));
    }

    auto tileNameHasShore = [&](int setIdx)
        {
            ppmfc::CString secName;
            secName.Format("TileSet%04d", setIdx);
            auto setName = CINI::CurrentTheater->GetString(secName, "SetName");
            setName.MakeLower();
            if (setName.Find("shore") != -1)
                return true;
            return false;
        };

    int numTiles = tiles.size();

    for (int i = TopX; i <= BottomX; ++i) {
        for (int j = TopY; j <= BottomY; ++j) {
            if (!CMapData::Instance->IsCoordInMap(i, j)) continue;
            CMapDataExt::CellDataExts[CMapData::Instance->GetCoordIndex(i, j)].AddRandomTile = false;
        }
    }

    for (int i = TopX; i <= BottomX; ++i) {
        for (int j = TopY; j <= BottomY; ++j) {
            if (!CMapData::Instance->IsCoordInMap(i, j)) continue;
            auto cell = CMapData::Instance->GetCellAt(i, j);
            if (cell->IsHidden()) continue;
            if (multiSelection) {
                bool skip = true;
                for (const auto& coord : MultiSelection::SelectedCoords) {
                    if (coord.X == i && coord.Y == j)
                        skip = false;
                }
                if (skip)
                    continue;
            }

            if (std::find(tileindexes.begin(), tileindexes.end(), CMapDataExt::GetSafeTileIndex(cell->TileIndex)) == tileindexes.end()) {
                auto ttype = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[cell->TileSubIndex].TerrainType;
                if (isTerrainTypeIgnored(ttype)) {
                    continue;
                }
                if (std::find(roadSets.begin(), roadSets.end(), CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet) != roadSets.end()) {
                    continue;
                }
                auto tile = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)];
                bool skip = false;
                for (int m = 0; m < tile.Width; m++)
                {
                    for (int n = 0; n < tile.Height; n++)
                    {
                        int subIdx = n * tile.Width + m;

                        if (CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[subIdx].ImageData == NULL) {
                            continue;
                        }

                        auto ttype2 = CMapDataExt::GetExtension()->GetLandType(CMapDataExt::GetSafeTileIndex(cell->TileIndex), subIdx);
                        if (isTerrainTypeIgnored(ttype2)) {
                            skip = true;
                            break;
                        }
                    }
                }
                if (skip)
                    continue;

                if (CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[cell->TileSubIndex].RampType != 0) continue;

                if (tileNameHasShore(CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet)) continue;
            }

            if (onlyClear) {
                CMapDataExt::GetExtension()->PlaceTileAt(i, j, 0);
                continue;
            }

            float x = (float)i * 10 / scale;
            float y = (float)j * 10 / scale;

            auto [rotatedX, rotatedY] = rotateCoordinates(x, y, randomAngle);
            float noiseValue = perlinNoise(rotatedX, rotatedY);

            std::vector<int> tileIndexes = selectTile(noiseValue, tiles);

            if (!override) {
                if (tileIndexes.size() == 1 && tileIndexes[0] == 0xFFFF) {
                    continue;
                }
            }

            CMapDataExt::GetExtension()->PlaceTileAt(i, j, STDHelpers::RandomSelectInt(tileIndexes), 1);
        }
    }
    if (!CFinalSunApp::Instance->DisableAutoShore) {
        CMapData::Instance->CreateShore(TopX - 1, TopY - 1, BottomX + 1, BottomY + 1);
    }
    if (!CFinalSunApp::Instance->DisableAutoLat) {
        for (int i = TopX - 2; i <= BottomX + 2; ++i) {
            for (int j = TopY - 2; j <= BottomY + 2; ++j) {
                if (!CMapData::Instance->IsCoordInMap(i, j))
                    continue;
                CMapDataExt::SmoothTileAt(i, j, true);
            }
        }
    }

}

void CMapDataExt::CreateRandomOverlay(int TopX, int TopY, int BottomX, int BottomY, std::vector<std::pair<std::vector<TerrainGeneratorOverlay>, float>> overlays, bool override, bool multiSelection, bool onlyClear)
{
    std::vector<float> weights;
    for (auto& ovr : overlays) {
        weights.push_back(ovr.second);
    }
    std::vector<int> roadSets = getIgnoreTileSets();
    for (int i = TopX; i <= BottomX; ++i) {
        for (int j = TopY; j <= BottomY; ++j) {
            if (!CMapData::Instance->IsCoordInMap(i, j)) continue;
            int pos = CMapData::Instance->GetCoordIndex(i, j);
            auto cell = CMapData::Instance->GetCellAt(pos);
            if (cell->IsHidden()) continue;
            if (multiSelection) {
                bool skip = true;
                for (const auto& coord : MultiSelection::SelectedCoords) {
                    if (coord.X == i && coord.Y == j)
                        skip = false;
                }
                if (skip)
                    continue;
            }
            auto ttype = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[cell->TileSubIndex].TerrainType;

            if (isTerrainTypeIgnored(ttype)) {
                continue;
            }
            if (std::find(roadSets.begin(), roadSets.end(), CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet) != roadSets.end()) {
                continue;
            }
            if (override) {
                CMapDataExt::GetExtension()->SetNewOverlayAt(pos, 0xFFFF);
                CMapData::Instance->SetOverlayDataAt(pos, 0);
            }
            if (onlyClear) {
                CMapDataExt::GetExtension()->SetNewOverlayAt(pos, 0xFFFF);
                CMapData::Instance->SetOverlayDataAt(pos, 0);
                continue;
            }
            int randomIdx = GetIndexByWeights(weights);
            if (-1 < randomIdx && randomIdx < overlays.size()) {
                auto& overlayGroup = overlays[randomIdx].first;
                auto& overlay = overlayGroup[STDHelpers::RandomSelectInt(0, overlayGroup.size())];

                CMapDataExt::GetExtension()->SetNewOverlayAt(pos, overlay.Overlay);
                CMapData::Instance->SetOverlayDataAt(pos, STDHelpers::RandomSelectInt(overlay.AvailableOverlayData));
            }
        }
    }
}

void CMapDataExt::CreateRandomTerrain(int TopX, int TopY, int BottomX, int BottomY, std::vector<std::pair<std::vector<FString>, float>> terrains, bool override, bool multiSelection, bool onlyClear)
{
    std::vector<float> weights;
    for (auto& ter : terrains) {
        weights.push_back(ter.second);
    }
    std::vector<int> roadSets = getIgnoreTileSets();
    for (int i = TopX; i <= BottomX; ++i) {
        for (int j = TopY; j <= BottomY; ++j) {
            if (!CMapData::Instance->IsCoordInMap(i, j)) continue;
            int pos = CMapData::Instance->GetCoordIndex(i, j);
            auto cell = CMapData::Instance->GetCellAt(pos);
            if (cell->IsHidden()) continue;
            if (multiSelection) {
                bool skip = true;
                for (const auto& coord : MultiSelection::SelectedCoords) {
                    if (coord.X == i && coord.Y == j)
                        skip = false;
                }
                if (skip)
                    continue;
            }
            auto ttype = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[cell->TileSubIndex].TerrainType;

            if (isTerrainTypeIgnored(ttype)) {
                continue;
            }
            if (std::find(roadSets.begin(), roadSets.end(), CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet) != roadSets.end()) {
                continue;
            }
            if (override) {
                if (cell->Terrain > -1) {
                    CMapData::Instance->DeleteTerrainData(cell->Terrain);
                }
            }
            if (onlyClear) {
                if (cell->Terrain > -1) {
                    CMapData::Instance->DeleteTerrainData(cell->Terrain);
                }
                continue;
            }
            int randomIdx = GetIndexByWeights(weights);
            if (-1 < randomIdx && randomIdx < terrains.size()) {
                if (cell->Terrain > -1) {
                    CMapData::Instance->DeleteTerrainData(cell->Terrain);
                }
                auto& terrain = terrains[randomIdx].first;
                CMapData::Instance->SetTerrainData(STDHelpers::RandomSelect(terrain), pos);
            }
        }
    }
}

void CMapDataExt::CreateRandomSmudge(int TopX, int TopY, int BottomX, int BottomY, std::vector<std::pair<std::vector<FString>, float>> smudges, bool override, bool multiSelection, bool onlyClear)
{
    std::vector<float> weights;
    for (auto& smu : smudges) {
        weights.push_back(smu.second);
    }
    std::vector<int> roadSets = getIgnoreTileSets();
    auto& rules = Variables::RulesMap;
    for (int i = TopX; i <= BottomX; ++i) {
        for (int j = TopY; j <= BottomY; ++j) {
            if (!CMapData::Instance->IsCoordInMap(i, j)) continue;
            int pos = CMapData::Instance->GetCoordIndex(i, j);
            auto cell = CMapData::Instance->GetCellAt(pos);
            if (cell->IsHidden()) continue;
            if (multiSelection) {
                bool skip = true;
                for (const auto& coord : MultiSelection::SelectedCoords) {
                    if (coord.X == i && coord.Y == j)
                        skip = false;
                }
                if (skip)
                    continue;
            }
            auto ttype = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileBlockDatas[cell->TileSubIndex].TerrainType;

            if (isTerrainTypeIgnored(ttype)) {
                continue;
            }
            if (std::find(roadSets.begin(), roadSets.end(), CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cell->TileIndex)].TileSet) != roadSets.end()) {
                continue;
            }
            if (override) {
                if (cell->Smudge > -1) {
                    CMapData::Instance->DeleteSmudgeData(cell->Smudge);
                }
            }
            if (onlyClear) {
                if (cell->Smudge > -1) {
                    CMapData::Instance->DeleteSmudgeData(cell->Smudge);
                }
                continue;
            }
            int randomIdx = GetIndexByWeights(weights);
            if (-1 < randomIdx && randomIdx < smudges.size()) {
                auto& smudgeGroup = smudges[randomIdx].first;

                CSmudgeData smudge;
                smudge.X = j;
                smudge.Y = i;//opposite
                smudge.Flag = 0;
                smudge.TypeID = STDHelpers::RandomSelect(smudgeGroup).c_str();

                //check unplaceable tiles
                int width = rules.GetInteger(smudge.TypeID, "Width", 1);
                int height = rules.GetInteger(smudge.TypeID, "Height", 1);
                bool skip = false;
                for (int k = 0; k < width; k++)
                    for (int l = 0; l < height; l++)
                    {
                        if (!CMapData::Instance->IsCoordInMap(i + l, j + k)) continue;
                        auto cellCheck = CMapData::Instance->GetCellAt(i + l, j + k);

                        auto ttype = CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cellCheck->TileIndex)].TileBlockDatas[cellCheck->TileSubIndex].TerrainType;

                        if (isTerrainTypeIgnored(ttype)) {
                            skip = true;
                        }
                        if (CMapDataExt::TileData[CMapDataExt::GetSafeTileIndex(cellCheck->TileIndex)].TileBlockDatas[cellCheck->TileSubIndex].RampType != 0) skip = true;
                    }

               
                //check overlapping
                if (!skip) {
                    for (auto& thisSmudge : CMapData::Instance->SmudgeDatas) {
                        if (thisSmudge.X <= 0 || thisSmudge.Y <= 0 || thisSmudge.Flag)
                            continue;
                        int thisWidth = rules.GetInteger(thisSmudge.TypeID, "Width", 1);
                        int thisHeight = rules.GetInteger(thisSmudge.TypeID, "Height", 1);
                        int thisX = thisSmudge.Y;
                        int thisY = thisSmudge.X;//opposite
                        for (int m = 0; m < thisWidth; m++)
                            for (int n = 0; n < thisHeight; n++)
                                for (int k = 0; k < width; k++)
                                    for (int l = 0; l < height; l++)
                                        if (thisY + m == j + k && thisX + n == i + l)
                                            skip = true;

                    }
                }
                if (!skip) {
                    if (cell->Smudge > -1) {
                        CMapData::Instance->DeleteSmudgeData(cell->Smudge);
                    }
                    CMapData::Instance->SetSmudgeData(&smudge);
                    CMapData::Instance->UpdateFieldSmudgeData(false);
                }
            }
        }
    }
}