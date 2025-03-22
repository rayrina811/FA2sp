#include "CAnnotationDlg.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../Common.h"

CAnnotationDlg::CAnnotationDlg(CWnd* pParent /*=NULL*/)
	: ppmfc::CDialog(322, pParent)
{
	m_Text = "";
	m_FontSize = "20";
	m_TextColor = "0x000000";
	m_BgColor = "0xFFFFFF";
	m_Bold = FALSE;
}

void CAnnotationDlg::DoDataExchange(ppmfc::CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Text(pDX, 1000, m_Text);
	DDX_Text(pDX, 1002, m_FontSize);
	DDX_Text(pDX, 1004, m_TextColor);
	DDX_Text(pDX, 1006, m_BgColor);
	DDX_Check(pDX, 1007, m_Bold);

	ppmfc::CString buffer;

	if (Translations::GetTranslationItem("OK", buffer))
		GetDlgItem(1)->SetWindowTextA(buffer);

	if (Translations::GetTranslationItem("AnnotationFontSize", buffer))
		GetDlgItem(1001)->SetWindowTextA(buffer);

	if (Translations::GetTranslationItem("AnnotationTextColor", buffer))
		GetDlgItem(1003)->SetWindowTextA(buffer);

	if (Translations::GetTranslationItem("AnnotationBackgroundColor", buffer))
		GetDlgItem(1005)->SetWindowTextA(buffer);

	if (Translations::GetTranslationItem("AnnotationBold", buffer))
		GetDlgItem(1007)->SetWindowTextA(buffer);

	if (Translations::GetTranslationItem("AnnotationCaption", buffer))
		SetWindowTextA(buffer);
}


BOOL CAnnotationDlg::OnInitDialog()
{
	ppmfc::CDialog::OnInitDialog();

	GetDlgItem(1002)->SetWindowTextA(m_FontSize);
	GetDlgItem(1004)->SetWindowTextA(m_TextColor);
	GetDlgItem(1006)->SetWindowTextA(m_BgColor);
	m_Text.Replace("\\n", "\n");
	GetDlgItem(1000)->SetWindowTextA(m_Text);
	if (m_Bold)
		::SendMessage(GetDlgItem(1007)->GetSafeHwnd(), BM_SETCHECK, BST_CHECKED, 0);

	return TRUE;  
}

void CAnnotationDlg::OnOK()
{
	UpdateData(TRUE);
	EndDialog(0);

	//CDialog::OnOK();
}

void CAnnotationDlg::OnCancel()
{
	//CDialog::OnCancel();
}
