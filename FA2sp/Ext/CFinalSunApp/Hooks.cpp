#include "Body.h"
#include <CFinalSunDlg.h>
#include <CIsoView.h>

#include <Helpers/Macro.h>
#include "../CFinalSunDlg/Body.h"
#include "../../FA2sp.Constants.h"
#include "../../Helpers/Translations.h"
#include "../../FA2sp.h"
#include "../CIsoView/Body.h"

#include <shlobj.h>
#include <afxwin.h>
#include "../../Helpers/STDHelpers.h"

DEFINE_HOOK(41FAD0, CFinalSunApp_InitInstance, 8)
{
    R->EAX(CFinalSunAppExt::GetInstance()->InitInstanceExt());

    return 0x422052;
}

DEFINE_HOOK(4229E0, CFinalSunApp_ProcessMessageFilter, 7)
{
    REF_STACK(LPMSG, lpMsg, 0x8);
    if (!CMapData::Instance->MapWidthPlusHeight) return 0;
    if (CViewObjectsExt::CurrentConnectedTileType < 0 || CViewObjectsExt::CurrentConnectedTileType > CViewObjectsExt::ConnectedTileSets.size()) return 0;
    int currentCTtype = CViewObjectsExt::ConnectedTileSets[CViewObjectsExt::CurrentConnectedTileType].Type;
    if (currentCTtype == CViewObjectsExt::DirtRoad || currentCTtype == CViewObjectsExt::CityDirtRoad || currentCTtype == CViewObjectsExt::Highway
        && !CViewObjectsExt::IsInPlaceCliff_OnMouseMove)
    {
        if (lpMsg->message == WM_KEYDOWN)
        {
            auto point = CIsoView::GetInstance()->GetCurrentMapCoord(CIsoView::GetInstance()->MouseCurrentPosition);
            if (lpMsg->wParam == VK_PRIOR || lpMsg->wParam == VK_UP)
            {
                if (CViewObjectsExt::CliffConnectionHeight < 14 && CViewObjectsExt::LastPlacedCT.GetNextHeightOffset() < 1 || CViewObjectsExt::NextCTHeightOffset < 0 || CViewObjectsExt::CliffConnectionHeight < 13)
                {
                    CViewObjectsExt::RaiseNextCT();
                    CViewObjectsExt::PlaceConnectedTile_OnMouseMove(point.X, point.Y, false);
                }

            }
            else if (lpMsg->wParam == VK_NEXT || lpMsg->wParam == VK_DOWN)
            {
                if (CViewObjectsExt::CliffConnectionHeight > 0 || CViewObjectsExt::NextCTHeightOffset > 0 || CViewObjectsExt::CliffConnectionHeight == 0 && CViewObjectsExt::LastPlacedCT.GetNextHeightOffset() > 0)
                {
                    CViewObjectsExt::LowerNextCT();
                    CViewObjectsExt::PlaceConnectedTile_OnMouseMove(point.X, point.Y, false);
                }
            }
        }

    }
    return 0;
}

DEFINE_HOOK(50E220, CFinalSunDlg_SelectMainExecutive, 7)
{
    GET(ppmfc::CDialog*, pTSOptions, ECX);

    auto IsFileOpenDialogAvailable = []() {
        IFileOpenDialog* pTest = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pTest));
        if (SUCCEEDED(hr)) {
            pTest->Release();
            return true;
        }
        return false;
        };
    if (IsFileOpenDialogAvailable()) {
        IFileOpenDialog* pFileOpen = nullptr;
        HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, nullptr, CLSCTX_ALL, IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));
#ifdef CHINESE
        COMDLG_FILTERSPEC filterSpecs[] = {
          {L"Mix 文件 (ra2md.mix)", L"*.mix"},
          {L"可执行文件 (gamemd.exe)", L"*.exe"},
          {L"所有文件 (*.*)", L"*.*"}
        };
#else
        COMDLG_FILTERSPEC filterSpecs[] = {
          {L"Mix Files (ra2md.mix)", L"*.mix"},
          {L"Executable Files (gamemd.exe)", L"*.exe"},
          {L"All Files (*.*)", L"*.*"}
        };
#endif
        pFileOpen->SetFileTypes(3, filterSpecs);

        if (SUCCEEDED(hr)) {
            hr = pFileOpen->Show(nullptr);

            if (SUCCEEDED(hr)) {
                IShellItem* pItem = nullptr;
                hr = pFileOpen->GetResult(&pItem);

                if (SUCCEEDED(hr)) {
                    PWSTR pszFilePath = nullptr;
                    pItem->GetDisplayName(SIGDN_FILESYSPATH, &pszFilePath);

                    if (pszFilePath != L"\0")
                        pTSOptions->GetDlgItem(1034)->SetWindowText(STDHelpers::WStringToString(pszFilePath).c_str());

                    CoTaskMemFree(pszFilePath);
                    pItem->Release();
                }
            }
            pFileOpen->Release();
        }
    }
    else {
        OPENFILENAMEW ofn = { sizeof(ofn) };
        WCHAR szFile[MAX_PATH] = L"";

#ifdef CHINESE
        ofn.lpstrFilter = L"Mix 文件 (ra2md.mix)\0*.mix\0可执行文件 (gamemd.exe)\0*.exe\0所有文件 (*.*)\0*.*\0";
        ofn.lpstrTitle = L"选择文件";
#else
        ofn.lpstrFilter = L"Mix Files (ra2md.mix)\0*.mix\0Executable Files (gamemd.exe)\0*.exe\0All Files (*.*)\0*.*\0";
        ofn.lpstrTitle = L"Select File";
#endif

        ofn.lpstrFile = szFile;
        ofn.nMaxFile = MAX_PATH;
        ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

        if (GetOpenFileNameW(&ofn)) {
            pTSOptions->GetDlgItem(1034)->SetWindowText(STDHelpers::WStringToString(szFile).c_str());
        }
    }
    
    return 0x50E304;
}



//DEFINE_HOOK(41F720, CFinalSunApp_Initialize, 7)
//{
//    SetProcessDPIAware();
//    return 0;
//}

