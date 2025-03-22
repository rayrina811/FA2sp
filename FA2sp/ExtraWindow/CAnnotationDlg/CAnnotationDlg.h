#include <vector>
#include "../../FA2sp.h"
#include "../../Helpers/Translations.h"

class CAnnotationDlg : public ppmfc::CDialog
{
public:
	CAnnotationDlg(CWnd* pParent = NULL);
	ppmfc::CString m_Text;
	ppmfc::CString m_FontSize;
	ppmfc::CString m_TextColor;
	ppmfc::CString m_BgColor;
    BOOL m_Bold;

    virtual int DoModal() override
    {
        HMODULE hModule = LoadLibrary(TEXT("Riched32.dll"));
        if (!hModule)
            ::MessageBox(NULL, Translations::TranslateOrDefault("FailedLoadRiched32DLL", "Could not Load Riched32.dll£¡"), Translations::TranslateOrDefault("Error", "Error"), MB_ICONERROR);

        return ppmfc::CDialog::DoModal();
    }

protected:
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();
    BOOL PreTranslateMessage(MSG* pMsg) override
    {
        if (pMsg->message == WM_KEYDOWN && pMsg->wParam == VK_RETURN)
        {
            GetDlgItem(1000)->SendMessage(EM_REPLACESEL, FALSE, (LPARAM)"\n");
            return TRUE;
        }

        return ppmfc::CDialog::PreTranslateMessage(pMsg);
    }
};
