#include "CListUInputDlg.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Helpers/FString.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../Common.h"

CListUInputDlg::CListUInputDlg(CWnd* pParent /*=NULL*/)
	: ppmfc::CDialog(321, pParent)
{
	m_Caption = "";
}

void CListUInputDlg::DoDataExchange(ppmfc::CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	ppmfc::CWnd* box = (ppmfc::CWnd*)GetDlgItem(1000);
	ppmfc::DDX_Control(pDX, 1000, *box);

	FString buffer;
	if (Translations::GetTranslationItem("OK", buffer))
		GetDlgItem(1)->SetWindowTextA(buffer);
	
	if (Translations::GetTranslationItem("NewComboUInputCaption", buffer))
		SetWindowTextA(buffer);

	if (m_Caption != "")
		GetDlgItem(1002)->SetWindowTextA(m_Caption);
}


BOOL CListUInputDlg::OnInitDialog()
{
	ppmfc::CDialog::OnInitDialog();
	CListBox* box = (CListBox*)GetDlgItem(1000);
	for (const auto& srt : m_CustomStrings)
	{
		box->AddString(srt);
	}
	box->SetCurSel(0);

	return TRUE;  
}

void CListUInputDlg::OnOK()
{
	UpdateData(TRUE);
	CListBox* box = (CListBox*)GetDlgItem(1000);
	int count = box->GetSelCount();

	if (count > 0)
	{
		std::vector<int> selectedIndices(count);
		box->GetSelItems(count, selectedIndices.data());

		for (int index : selectedIndices)
		{
			char buffer[512] = { 0 };
			::SendMessage(box->GetSafeHwnd(), LB_GETTEXT, index, (LPARAM)buffer);
			m_selectedTexts.push_back(buffer);
		}
	}
	
	EndDialog(0);

	//CDialog::OnOK();
}

void CListUInputDlg::OnCancel()
{
	m_selectedTexts.clear();
	EndDialog(0);
	//CDialog::OnCancel();
}