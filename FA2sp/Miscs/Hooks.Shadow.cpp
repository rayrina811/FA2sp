#include <Helpers/Macro.h>
#include <Drawing.h>
#include <CPalette.h>
#include <CLoading.h>

#include "Palettes.h"

#include "../Ext/CFinalSunDlg/Body.h"
#include "../Ext/CIsoView/Body.h"
#include "../Ext/CLoading/Body.h"
#include "../Ext/CMapData/Body.h"

DEFINE_HOOK(470BC5, CIsoView_Draw_Building, 7)
{
	if (!CIsoViewExt::DrawShadows || ExtConfigs::ShadowDisplaySetting == 0)
		return 0;

	GET_STACK(CBuildingRenderData, data, STACK_OFFS(0xD18, 0xC0C));
	auto& coord = CIsoViewExt::CurrentDrawCellLocation;
	auto cell = CMapData::Instance->GetCellAt(coord.X, coord.Y);
	if (cell->Structure != -1)
	{
		LEA_STACK(LPDDSURFACEDESC2, lpDesc, STACK_OFFS(0xD18, 0x92C));
		int X = coord.X;
		int Y = coord.Y;
		CIsoView::MapCoord2ScreenCoord(X, Y);
		X -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB0));
		Y -= R->Stack<float>(STACK_OFFS(0xD18, 0xCB8));

		int nFacing = 0;
		if (Variables::Rules.GetBool(data.ID, "Turret") && !Variables::Rules.GetBool(data.ID, "TurretAnimIsVoxel"))
			nFacing = 7 - (data.Facing / 32) % 8;

		const int HP = CMapDataExt::CurrentRenderBuildingStrength;
		int status = CLoadingExt::GBIN_NORMAL;
		if (HP == 0)
			status = CLoadingExt::GBIN_RUBBLE;
		else if (static_cast<int>((CMapDataExt::ConditionYellow + 0.001f) * 256) > HP)
			status = CLoadingExt::GBIN_DAMAGED;

		const auto& imageName = CLoadingExt::GetBuildingImageName(data.ID, nFacing, status, true);
		auto pData = ImageDataMapHelper::GetImageDataFromMap(imageName);
		CIsoViewExt::BlitSHPTransparent(lpDesc, X - pData->FullWidth / 2, Y - pData->FullHeight / 2, pData, NULL, 128);
	}
	return 0;
}

