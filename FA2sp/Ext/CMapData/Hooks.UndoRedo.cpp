#include "Body.h"

DEFINE_HOOK(4BBEC0, CMapData_DoUndo, 5)
{
	GET(CMapDataExt*, pThis, ECX);

	if (pThis->UndoRedoDatas.size() == 0) return 0x4BC170;
	if (pThis->UndoRedoDataIndex < 0) return 0x4BC170;

	pThis->UndoRedoDataIndex -= 1;
	pThis->UndoRedoDataIndex = std::min(pThis->UndoRedoDataIndex, (int)pThis->UndoRedoDatas.size() - 2);

	int left, top, width, height;
	auto& data = pThis->UndoRedoDatas[pThis->UndoRedoDataIndex + 1];
	left = data.left;
	top = data.top;
	width = data.right - left;
	height = data.bottom - top;

	int i, e;
	for (i = 0; i < width; i++)
	{
		for (e = 0; e < height; e++)
		{
			int pos_w, pos_r;
			pos_r = i + e * width;
			pos_w = left + i + (top + e) * pThis->MapWidthPlusHeight;
			auto cell = pThis->GetCellAt(pos_w);
			auto& cellExt = pThis->CellDataExts[pos_w];

			cell->Height = data.bHeight[pos_r];
			cell->TileIndexHiPart = data.bMapData[pos_r];
			cell->TileSubIndex = data.bSubTile[pos_r];
			cell->IceGrowth = data.bMapData2[pos_r];
			cell->TileIndex = data.wGround[pos_r];

			pThis->DeleteTiberium(std::min(cellExt.NewOverlay, (word)0xFF), cell->OverlayData);
			cellExt.NewOverlay = data.overlay[pos_r];
			cell->Overlay = std::min(data.overlay[pos_r], (word)0xFF);
			cell->OverlayData = data.overlaydata[pos_r];
			pThis->AddTiberium(std::min(cellExt.NewOverlay, (word)0xFF), cell->OverlayData);

			cell->Flag.RedrawTerrain = data.bRedrawTerrain[pos_r];
			cell->Flag.AltIndex = data.bRNDData[pos_r];

			pThis->UpdateMapPreviewAt(left + i, top + e);
		}
	}

	return 0x4BC170;
}

DEFINE_HOOK(4BC1C0, CMapData_DoRedo, 5)
{
	GET(CMapDataExt*, pThis, ECX);

	if (pThis->UndoRedoDatas.size() <= pThis->UndoRedoDataIndex + 1 || !pThis->UndoRedoDatas.size()) 
		return 0x4BC486;

	pThis->UndoRedoDataIndex += 1;

	if (pThis->UndoRedoDataIndex + 1 >= pThis->UndoRedoDatas.size()) 
		pThis->UndoRedoDataIndex = pThis->UndoRedoDatas.size() - 2;

	int left, top, width, height;
	auto& data = pThis->UndoRedoDatas[pThis->UndoRedoDataIndex + 1];
	left = data.left;
	top = data.top;
	width = data.right - left;
	height = data.bottom - top;

	int i, e;
	for (i = 0; i < width; i++)
	{
		for (e = 0; e < height; e++)
		{
			int pos_w, pos_r;
			pos_r = i + e * width;
			pos_w = left + i + (top + e) * pThis->MapWidthPlusHeight;

			auto cell = pThis->GetCellAt(pos_w);
			auto& cellExt = pThis->CellDataExts[pos_w];

			cell->Height = data.bHeight[pos_r];
			cell->TileIndexHiPart = data.bMapData[pos_r];
			cell->TileSubIndex = data.bSubTile[pos_r];
			cell->IceGrowth = data.bMapData2[pos_r];
			cell->TileIndex = data.wGround[pos_r];

			pThis->DeleteTiberium(std::min(cellExt.NewOverlay, (word)0xFF), cell->OverlayData);
			cellExt.NewOverlay = data.overlay[pos_r];
			cell->Overlay = std::min(data.overlay[pos_r], (word)0xFF);
			cell->OverlayData = data.overlaydata[pos_r];
			pThis->AddTiberium(std::min(cellExt.NewOverlay, (word)0xFF), cell->OverlayData);

			cell->Flag.RedrawTerrain = data.bRedrawTerrain[pos_r];
			cell->Flag.AltIndex = data.bRNDData[pos_r];

			pThis->UpdateMapPreviewAt(left + i, top + e);
		}
	}

	return 0x4BC486;
}

DEFINE_HOOK(4BB990, CMapData_SaveUndoRedoData, 7)
{
	GET(CMapDataExt*, pThis, ECX);
	GET_STACK(BOOL, bEraseFollowing, 0x4);
	GET_STACK(int, left, 0x8);
	GET_STACK(int, top, 0xC);
	GET_STACK(int, right, 0x10);
	GET_STACK(int, bottom, 0x14);

	if (left < 0) left = 0;
	if (top < 0) top = 0;
	if (right > pThis->MapWidthPlusHeight) right = pThis->MapWidthPlusHeight;
	if (bottom > pThis->MapWidthPlusHeight) bottom = pThis->MapWidthPlusHeight;

	if (right == 0) right = pThis->MapWidthPlusHeight;
	if (bottom == 0) bottom = pThis->MapWidthPlusHeight;

	int e;
	if (bEraseFollowing)
	{
		pThis->UndoRedoDatas.resize(pThis->UndoRedoDataIndex + 1);
	}

	//pThis->UndoRedoDataIndex++;

	if (pThis->UndoRedoDatas.size() + 1 > ExtConfigs::UndoRedoLimit)
	{
		//pThis->UndoRedoDataIndex = ExtConfigs::UndoRedoLimit - 1;
		pThis->UndoRedoDatas.erase(pThis->UndoRedoDatas.begin());
	}

	auto& data = pThis->UndoRedoDatas.emplace_back();
	pThis->UndoRedoDataIndex = pThis->UndoRedoDatas.size() - 1;

	int width, height;
	width = right - left;
	height = bottom - top;

	int size = width * height;
	data.left = left;
	data.top = top;
	data.right = right;
	data.bottom = bottom;
	data.bHeight = std::make_unique<BYTE[]>(size);
	data.bMapData = std::make_unique<WORD[]>(size);
	data.bSubTile = std::make_unique<BYTE[]>(size);
	data.bMapData2 = std::make_unique<BYTE[]>(size);
	data.wGround = std::make_unique<WORD[]>(size);
	data.overlay = std::make_unique<WORD[]>(size);
	data.overlaydata = std::make_unique<BYTE[]>(size);
	data.bRedrawTerrain = std::make_unique<BOOL[]>(size);
	data.bRNDData = std::make_unique<BYTE[]>(size);

	int i;
	for (i = 0; i < width; i++)
	{
		for (e = 0; e < height; e++)
		{
			int pos_w, pos_r;
			pos_w = i + e * width;
			pos_r = left + i + (top + e) * pThis->MapWidthPlusHeight;
			auto cell = pThis->GetCellAt(pos_r);
			auto& cellExt = pThis->CellDataExts[pos_r];
			data.bHeight[pos_w] = cell->Height;
			data.bMapData[pos_w] = cell->TileIndexHiPart;
			data.bSubTile[pos_w] = cell->TileSubIndex;
			data.bMapData2[pos_w] = cell->IceGrowth;
			data.wGround[pos_w] = cell->TileIndex;
			data.overlay[pos_w] = cellExt.NewOverlay;
			data.overlaydata[pos_w] = cell->OverlayData;
			data.bRedrawTerrain[pos_w] = cell->Flag.RedrawTerrain;
			data.bRNDData[pos_w] = cell->Flag.AltIndex;
		}
	}

	return 0x4BBEBD;
}
