#include <Helpers/Macro.h>

// DEFINE_HOOK(54FC1E, CFileDialog_EnableExplorerStyle, 7)
// {
//     GET(CFileDialog*, pDialog, ESI);
// 
//     OPENFILENAME ofn = pDialog->m_ofn;
// 
//     ofn.Flags &= ~OFN_ENABLEHOOK;
// 
//     if (ofn.pvReserved)
//         R->EAX(GetOpenFileNameA(&ofn));
//     else
//         R->EAX(GetSaveFileNameA(&ofn));
// 
//     return 0x54FC37;
// }

DEFINE_HOOK(4248B3, CFinalSunDlg_OpenMap_ChangeDialogStyle, 7)
{
    R->Stack<int>(STACK_OFFS(0x60C, (0x398 - 0x8)), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST);
    return 0;
}

DEFINE_HOOK(42686A, CFinalSunDlg_SaveMap_ChangeDialogStyle, 5)
{
    R->Stack<int>(STACK_OFFS(0x3CC, (0x280 - 0x8)), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST);
    return 0;
}

DEFINE_HOOK(4D312E, CFinalSunDlg_ImportMap_ChangeDialogStyle, 5)
{
    R->Stack<int>(STACK_OFFS(0x310, (0x280 - 0x8)), OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT | OFN_FILEMUSTEXIST);
    return 0;
}

DEFINE_HOOK(40B7B3, CINIEditor_OnClickImportINI_ChangeDialogStyle, 5)
{
    R->Stack<int>(STACK_OFFS(0x3B0, (0x280 - 0x8)), OFN_FILEMUSTEXIST);
    return 0;
}
