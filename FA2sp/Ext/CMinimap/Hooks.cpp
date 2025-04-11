#include <Helpers/Macro.h>

#include "Body.h"

#include <CMapData.h>
#include <CFinalSunDlg.h>
#include <CIsoView.h>
#include "../CIsoView/Body.h"

DEFINE_HOOK(4D1E34, CMinimap_Update_NOTOPMOST, 7)
{
	CFinalSunDlg::Instance->MyViewFrame.Minimap.SetWindowPos(ppmfc::CWnd::FromHandle(HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOACTIVATE | SWP_NOMOVE | SWP_NOSIZE);
	return 0;
}

DEFINE_HOOK(4D1B50, CMinimap_OnDraw, 7)
{
    GET(CMinimap*, pThis, ECX);
    GET_STACK(CDC*, pDC, 0x4);

	BITMAPINFO bmi;
	int nStride;
	BYTE* pData = nullptr;
	CMapData::Instance->GetMapPreview(pData, &bmi, nStride);
	if (pData)
	{
		// fix bottom left and right top pixels in the bottom-up bitmap
		memcpy(&pData[0], &pData[sizeof(RGBTRIPLE)], sizeof(RGBTRIPLE));
		auto firstLine = (bmi.bmiHeader.biHeight - 1) * nStride;
		memcpy(&pData[firstLine + (bmi.bmiHeader.biWidth - 1) * sizeof(RGBTRIPLE)],
			&pData[firstLine + (bmi.bmiHeader.biWidth - 2) * sizeof(RGBTRIPLE)], sizeof(RGBTRIPLE));
	}

	if (!pData) return 0x4D1CE0;

	RECT r;
	pThis->GetClientRect(&r);
	StretchDIBits(pDC->m_hDC, 0, 0, r.right, r.bottom, 0, 0, 
		CMapData::Instance->Size.Width * 2, CMapData::Instance->Size.Height,
		pData, &bmi, DIB_RGB_COLORS, SRCCOPY);

	auto GetMiniMapPos = [bmi](int i, int e)
		{
			auto pIsoView = CIsoView::GetInstance();
			i += pIsoView->ViewPosition.x;
			e += pIsoView->ViewPosition.y;
			pIsoView->ScreenCoord2MapCoord_Flat(i, e);
			int x, y;
			const int pheight = bmi.bmiHeader.biHeight;

			const DWORD dwIsoSize = CMapData::Instance->MapWidthPlusHeight;
			y = e / 2 + i / 2;
			x = dwIsoSize - i + e;

			int tx, ty;
			tx = CMapData::Instance->Size.Width;
			ty = CMapData::Instance->Size.Height;

			ty = ty / 2 + tx / 2;
			tx = dwIsoSize - CMapData::Instance->Size.Width + CMapData::Instance->Size.Height;

			x -= tx;
			y -= ty;

			x += pheight;
			y += pheight / 2;
			return CPoint(x, y);
		};

	RECT cr;
	CRect selRect;
	CIsoView::GetInstance()->GetWindowRect(&cr);

	auto topLeft = GetMiniMapPos(cr.left, cr.top);
	auto topRight = GetMiniMapPos(cr.right, cr.top);
	auto bottomLeft = GetMiniMapPos(cr.left, cr.bottom);
	auto bottomRight = GetMiniMapPos(cr.right, cr.bottom);
	auto left = std::min(topLeft.x, topRight.x);
	auto top = std::min(topLeft.y, topRight.y);
	auto right = std::max(bottomLeft.x, bottomRight.x);
	auto bottom = std::max(bottomLeft.y, bottomRight.y);

	selRect.left = left;
	selRect.top = top;
	selRect.right = right;
	selRect.bottom = bottom;

	pDC->Draw3dRect(&selRect, RGB(200, 0, 0), RGB(120, 0, 0));

    return 0x4D1CE0;
}

DEFINE_HOOK(4D1E70, CMinimap_OnMouseMove, 7)
{
	GET(CMinimap*, pThis, ECX);
	GET_STACK(UINT, nFlags, 0x4);
	GET_STACK(ppmfc::CPoint, point, 0x8);

	if (nFlags == MK_LBUTTON)
	{
		RECT cr;
		pThis->GetClientRect(&cr);
		RECT r = CIsoViewExt::GetScaledWindowRect();

		float defaultXSize = (CMapData::Instance->Size.Width * 2);
		float defaultYSize = (CMapData::Instance->Size.Height);
		float resizedXScale = cr.right / defaultXSize;
		float resizedYScale = cr.bottom / defaultYSize;

		auto pIsoView = CIsoView::GetInstance();

		int x = (point.x / resizedXScale) / 2 + CMapData::Instance->Size.Height / 2;
		int y = (point.y / resizedYScale) + CMapData::Instance->Size.Width / 2;

		pIsoView->MoveTo((x - r.right / 60 / 2) * 60, (y - r.bottom / 30 / 2) * 30);
		pIsoView->RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
		pThis->RedrawWindow(NULL, NULL, RDW_INVALIDATE | RDW_UPDATENOW);
	}

	return 0x4D1F57;
}

//
//DEFINE_HOOK(4D1E0F, CMinimap_UpdateDialog_Size, 8)
//{
//    R->ECX(R->Stack32(STACK_OFFS(0x2C, 0x10)));
//    R->EAX(R->EAX() + R->EDI());
//    return 0x4D1E17;
//}