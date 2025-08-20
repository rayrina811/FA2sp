#include "Body.h"

DEFINE_HOOK(4BBEC0, CMapData_DoUndo, 5)
{
	GET(CMapDataExt*, pThis, ECX);

	if (pThis->UndoRedoDatas.size() == 0) return 0x4BC170;
	if (pThis->UndoRedoDataIndex < 0) return 0x4BC170;

	pThis->UndoRedoDataIndex -= 1;
	pThis->UndoRedoDataIndex = std::min(pThis->UndoRedoDataIndex, (int)pThis->UndoRedoDatas.size() - 2);

	int left, top, width, height;
	auto* data = pThis->UndoRedoDatas.get(pThis->UndoRedoDataIndex + 1);
	if (auto* tr = dynamic_cast<TerrainRecord*>(data)) {
		// make current record for redo
		pThis->UndoRedoDatas.insert(pThis->UndoRedoDataIndex + 2, 
			std::move(pThis->MakeTerrainRecord(tr->left, tr->top, tr->right, tr->bottom)));

		left = tr->left;
		top = tr->top;
		width = tr->right - left;
		height = tr->bottom - top;

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

				cell->Height = tr->bHeight[pos_r];
				cell->TileIndexHiPart = tr->bMapData[pos_r];
				cell->TileSubIndex = tr->bSubTile[pos_r];
				cell->IceGrowth = tr->bMapData2[pos_r];
				cell->TileIndex = tr->wGround[pos_r];

				pThis->DeleteTiberium(std::min(cellExt.NewOverlay, (word)0xFF), cell->OverlayData);
				cellExt.NewOverlay = tr->overlay[pos_r];
				cell->Overlay = std::min(tr->overlay[pos_r], (word)0xFF);
				cell->OverlayData = tr->overlaydata[pos_r];
				pThis->AddTiberium(std::min(cellExt.NewOverlay, (word)0xFF), cell->OverlayData);

				cell->Flag.RedrawTerrain = tr->bRedrawTerrain[pos_r];
				cell->Flag.AltIndex = tr->bRNDData[pos_r];

				pThis->UpdateMapPreviewAt(left + i, top + e);
			}
		}
	}
	else if (auto* ur = dynamic_cast<ObjectRecord*>(data)) {

	}
	else if (auto* mr = dynamic_cast<MixedRecord*>(data)) {

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
	auto* data = pThis->UndoRedoDatas.get(pThis->UndoRedoDataIndex + 1);
	if (auto* tr = dynamic_cast<TerrainRecord*>(data)) {
		left = tr->left;
		top = tr->top;
		width = tr->right - left;
		height = tr->bottom - top;

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

				cell->Height = tr->bHeight[pos_r];
				cell->TileIndexHiPart = tr->bMapData[pos_r];
				cell->TileSubIndex = tr->bSubTile[pos_r];
				cell->IceGrowth = tr->bMapData2[pos_r];
				cell->TileIndex = tr->wGround[pos_r];

				pThis->DeleteTiberium(std::min(cellExt.NewOverlay, (word)0xFF), cell->OverlayData);
				cellExt.NewOverlay = tr->overlay[pos_r];
				cell->Overlay = std::min(tr->overlay[pos_r], (word)0xFF);
				cell->OverlayData = tr->overlaydata[pos_r];
				pThis->AddTiberium(std::min(cellExt.NewOverlay, (word)0xFF), cell->OverlayData);

				cell->Flag.RedrawTerrain = tr->bRedrawTerrain[pos_r];
				cell->Flag.AltIndex = tr->bRNDData[pos_r];

				pThis->UpdateMapPreviewAt(left + i, top + e);
			}
		}
	}
	else if (auto* ur = dynamic_cast<ObjectRecord*>(data)) {

	}
	else if (auto* mr = dynamic_cast<MixedRecord*>(data)) {

	}
	pThis->UndoRedoDatas.erase(pThis->UndoRedoDataIndex + 1);

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

	if (bEraseFollowing)
	{
		pThis->UndoRedoDatas.resize(pThis->UndoRedoDataIndex + 1);
	}

	if (pThis->UndoRedoDatas.size() + 1 > ExtConfigs::UndoRedoLimit)
	{
		pThis->UndoRedoDatas.erase(0);
	}
	
	pThis->UndoRedoDataIndex = pThis->UndoRedoDatas.size();
	pThis->UndoRedoDatas.add(std::move(pThis->MakeTerrainRecord(left, top, right, bottom)));

	return 0x4BBEBD;
}

DEFINE_HOOK(4616BA, SkipUndo_CIsoView_OnLButtonDown_1, 6)
{
	return 0x4616D8;
}

DEFINE_HOOK(464AC1, SkipUndo_CIsoView_OnLButtonDown_2, 6)
{
	return 0x46686A;
}

DEFINE_HOOK(466D45, SkipUndo_CIsoView_OnLButtonUp_1, 6)
{
	return 0x466D5F;
}