#include "Body.h"
#include "../../Helpers/TheaterHelpers.h"
#include "../CMapData/Body.h"

CCreateMap3B* CCreateMap3BExt::Instance = nullptr;
ppmfc::CMenu* CCreateMap3BExt::TheaterMenu = nullptr;

void CCreateMap3BExt::ProgramStartupInit()
{
	RunTime::ResetMemoryContentAt(0x595650, &CCreateMap3BExt::OnCommandExt);
	RunTime::ResetMemoryContentAt(0x595694, &CCreateMap3BExt::OnInitDialogExt);
}

BOOL CCreateMap3BExt::OnCommandExt(WPARAM wParam, LPARAM lParam)
{
	WORD wmID = LOWORD(wParam);
	WORD wmMsg = HIWORD(wParam);

	if (wmID == 2001)
	{
		TheaterSelectProc(wmMsg, lParam);
	}

	return this->ppmfc::CDialog::OnCommand(wParam, lParam);
}

void CCreateMap3BExt::TheaterSelectProc(WORD nCode, LPARAM lParam)
{
	HWND hComboBox = reinterpret_cast<HWND>(lParam);
	ppmfc::CString t;
	switch (nCode)
	{
	case CBN_DROPDOWN:
		break;
	case CBN_SELCHANGE:
	case CBN_DBLCLK:
		CMapDataExt::BitmapImporterTheater = TheaterHelpers::GetEnabledTheaterNames()[::SendMessage(hComboBox, CB_GETCURSEL, NULL, NULL)];
		break;
	default:
		break;
	}
	return;
}

BOOL CCreateMap3BExt::OnInitDialogExt()
{
	BOOL ret = ppmfc::CDialog::OnInitDialog();
	if (!ret)
		return FALSE;

	auto pCBTheater = (ppmfc::CComboBox*)this->GetDlgItem(2001);
	for (auto& str : TheaterHelpers::GetEnabledTheaterNames())
	{
		pCBTheater->AddString(TheaterHelpers::GetTranslatedName(str));
	}
	pCBTheater->SetCurSel(0);
	CMapDataExt::BitmapImporterTheater = TheaterHelpers::GetEnabledTheaterNames()[0];

	return TRUE;
}