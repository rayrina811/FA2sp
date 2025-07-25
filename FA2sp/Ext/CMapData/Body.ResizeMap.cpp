#include "Body.h"

bool CMapDataExt::ResizeMapExt(MapRect* const pRect)
{
	for (int i = 0; i < UndoRedoDataCount; i++)
	{
		int size = (UndoRedoData->Right - UndoRedoData->Left) * (UndoRedoData->Bottom - UndoRedoData->Top);
		GameDeleteArray(UndoRedoData[i].IgnoreAltImages, size);
		GameDeleteArray(UndoRedoData[i].Overlay, size);
		GameDeleteArray(UndoRedoData[i].OverlayData, size);
		GameDeleteArray(UndoRedoData[i].TileIndex, size);
		GameDeleteArray(UndoRedoData[i].TileIndexHiPart, size);
		GameDeleteArray(UndoRedoData[i].TilesubIndex, size);
		GameDeleteArray(UndoRedoData[i].Height, size);
		GameDeleteArray(UndoRedoData[i].IceGrowth, size);
		UndoRedoData[i].IgnoreAltImages = NULL;
		UndoRedoData[i].Overlay = NULL;
		UndoRedoData[i].OverlayData = NULL;
		UndoRedoData[i].TileIndex = NULL;
		UndoRedoData[i].TileIndexHiPart = NULL;
		UndoRedoData[i].TilesubIndex = NULL;
		UndoRedoData[i].Height = NULL;
		UndoRedoData[i].IceGrowth = NULL;
	}
	if (UndoRedoData != NULL) GameDeleteArray(UndoRedoData, UndoRedoDataCount);
	GameDeleteArray(CellDatas, CellDataCount);
	CellDatas = NULL;
	CellDataCount = 0;
	UndoRedoData = NULL;
	UndoRedoDataCount = 0;
	UndoRedoCurrentDataIndex = -1;
	ppmfc::CString mapSize;
	mapSize.Format("0,0,%d,%d", pRect->Width, pRect->Height);
	INI.WriteString("Map", "Size", mapSize);

	MapRect newLocalSize = LocalSize;
	newLocalSize.Width += pRect->Width - Size.Width;
	newLocalSize.Height += pRect->Height - Size.Height;

	mapSize.Format("%d,%d,%d,%d", newLocalSize.Left, newLocalSize.Top, newLocalSize.Width, newLocalSize.Height);
	INI.WriteString("Map", "LocalSize", mapSize);

	Size.Width = pRect->Width - pRect->Left;
	Size.Height = pRect->Height - pRect->Top;
	LocalSize = newLocalSize;
	MapWidthPlusHeight = Size.Width + Size.Height;

	CellDatas = new(CellData[(MapWidthPlusHeight + 1) * (MapWidthPlusHeight + 1)]);
	CellDataCount = (MapWidthPlusHeight + 1) * (MapWidthPlusHeight + 1);

	if (IsoPackData != NULL) GameDeleteArray(IsoPackData, IsoPackDataCount);
	IsoPackData = NULL;
	IsoPackDataCount = 0;

	CellDataExts.resize(CellDataCount);

	return true;
}