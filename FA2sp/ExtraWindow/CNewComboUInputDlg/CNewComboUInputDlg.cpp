#include "CNewComboUInputDlg.h"
#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"
#include "../../Ext/CFinalSunDlg/Body.h"
#include "../Common.h"

CNewComboUInputDlg::CNewComboUInputDlg(CWnd* pParent /*=NULL*/)
	: ppmfc::CDialog(313, pParent)
{
	m_Caption = "";
	m_Combo = "";
	m_ComboOri = "";
	m_Section = "";
	m_type = 0;
}

void CNewComboUInputDlg::DoDataExchange(ppmfc::CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	ppmfc::CString combo = m_Combo.c_str();
	DDX_CBString(pDX, 1460, combo);
	m_Combo = combo;

	FString buffer;

	if (Translations::GetTranslationItem("OK", buffer))
		GetDlgItem(1)->SetWindowTextA(buffer);
	
	if (Translations::GetTranslationItem("NewComboUInputCaption", buffer))
		SetWindowTextA(buffer);

	if (m_Caption != "")
		GetDlgItem(1459)->SetWindowTextA(m_Caption);

}


BOOL CNewComboUInputDlg::OnInitDialog()
{
	ppmfc::CDialog::OnInitDialog();

	CComboBox* box = (CComboBox*)GetDlgItem(1460);

	switch (m_type)
	{
	case COMBOUINPUT_MANUAL:
		if (!ReadValue && UseStrictOrder) {
			auto&& entries = mmh.ParseIndicies(m_Section, true);
			for (size_t i = 0, sz = entries.size(); i < sz; i++)
			{
				FString output;
				output.Format("%d - %s", i, entries[i]);

				FString uiname = CViewObjectsExt::QueryUIName(entries[i], true);
				if (uiname != entries[i] && uiname != "" && uiname != "MISSING")
				{
					FString tmp = output;
					output.Format("%s - %s", tmp, uiname);
				}

				m_ManualStrings.push_back(output);
				box->AddString(m_ManualStrings.back());
			}
		}
		else {
			for (auto& kvp : mmh.GetUnorderedUnionSection(m_Section))
			{
				FString text = ReadValue ? kvp.second : kvp.first;
				FString output;
				FString uiname = CViewObjectsExt::QueryUIName(text, true);
				if (!ReadValue && (m_Section == "Triggers" || m_Section == "Events" || m_Section == "Actions")) {
					uiname = ExtraWindow::GetTriggerName(text);
				}
				else if (!ReadValue && m_Section == "Tags") {
					uiname = ExtraWindow::GetTagName(text);
				}
				else if (!ReadValue && m_Section == "AITriggerTypes") {
					uiname = ExtraWindow::GetAITriggerName(text);
				}

				if (uiname != "MISSING" && uiname != "")
					output.Format("%s - %s", text, uiname);

				if (LoadValueAsName && !ReadValue) {
					FString uinameValue = CViewObjectsExt::QueryUIName(kvp.second, true);
					if (uinameValue != "MISSING" && uinameValue != "" && uinameValue != kvp.second) {
						output.Format("%s - %s - %s", text, kvp.second, uinameValue);
					}
					else {
						output.Format("%s - %s", text, kvp.second);
					}
				}
				m_ManualStrings.push_back(output);
				box->AddString(m_ManualStrings.back());
			}
		}
		break;
	case COMBOUINPUT_ALL_CUSTOM:
		for (const auto& srt : m_CustomStrings)
		{
			box->AddString(srt.c_str());
		}
		break;
	}

	box->SetCurSel(0);
	return TRUE;
}

void CNewComboUInputDlg::OnOK()
{
	UpdateData(TRUE);

	m_ComboOri = m_Combo;
	FString::TrimIndex(m_Combo);

	EndDialog(0);

	//CDialog::OnOK();
}

void CNewComboUInputDlg::OnCancel()
{
	m_ComboOri = "";
	m_Combo = "";
	EndDialog(0);
	//CDialog::OnCancel();
}