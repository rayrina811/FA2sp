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
	m_Section = "";
	m_type = 0;
}

void CNewComboUInputDlg::DoDataExchange(ppmfc::CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_CBString(pDX, 1460, m_Combo);

	ppmfc::CString buffer;

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
			if (LoadFrom == 1 || LoadFrom == 2) {
				const auto& indicies = LoadFrom == 1 ? Variables::GetRulesSection(m_Section) : Variables::GetRulesMapSection(m_Section);
				int idx = 0;
				for (auto& pair : indicies)
				{
					ppmfc::CString output;
					output.Format("%d - %s", idx, pair.second);
					ppmfc::CString uiname = CViewObjectsExt::QueryUIName(pair.second, true);
					if (uiname != pair.second && uiname != "" && uiname != "MISSING")
						output.Format("%s - %s", output, uiname);

					m_ManualStrings.push_back(output);
					box->AddString(m_ManualStrings.back());
					idx++;
				}
			}
			else {
				auto&& entries = mmh.ParseIndicies(m_Section, true);
				for (size_t i = 0, sz = entries.size(); i < sz; i++)
				{
					ppmfc::CString output;
					output.Format("%d - %s", i, entries[i]);

					ppmfc::CString uiname = CViewObjectsExt::QueryUIName(entries[i], true);
					if (uiname != entries[i] && uiname != "" && uiname != "MISSING")
						output.Format("%s - %s", output, uiname);

					m_ManualStrings.push_back(output);
					box->AddString(m_ManualStrings.back());
				}
			}
		}
		else {
			for (auto& kvp : mmh.GetUnorderedUnionSection(m_Section))
			{
				ppmfc::CString text = ReadValue ? kvp.second : kvp.first;
				ppmfc::CString output;
				ppmfc::CString uiname = CViewObjectsExt::QueryUIName(text, true);
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
					ppmfc::CString uinameValue = CViewObjectsExt::QueryUIName(kvp.second, true);
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
		
	}

	box->SetCurSel(0);


	return TRUE;  
}

void CNewComboUInputDlg::OnOK()
{
	UpdateData(TRUE);

	if (m_type == COMBOUINPUT_MANUAL)
	{
		STDHelpers::TrimIndex(m_Combo);
	}

	EndDialog(0);

	//CDialog::OnOK();
}

void CNewComboUInputDlg::OnCancel()
{
	//CDialog::OnCancel();
}