#pragma once

#include <vector>
#include "FA2PP.h"
#include "../../FA2sp/Helpers/FString.h"
#include "../../Helpers/MultimapHelper.h"

#define COMBOUINPUT_HOUSES 0
#define COMBOUINPUT_COUNTRIES 1
#define COMBOUINPUT_TRIGGERS 2
#define COMBOUINPUT_TAGS 3
#define COMBOUINPUT_HOUSES_N 4
#define COMBOUINPUT_COUNTRIES_N 5
#define COMBOUINPUT_MANUAL 6
#define COMBOUINPUT_ALL_CUSTOM 7

class CNewComboUInputDlg : public ppmfc::CDialog
{
public:
	int m_type;
	std::vector<FString> m_ManualStrings;
	CNewComboUInputDlg(CWnd* pParent = NULL);
	FString m_Caption;
	FString m_Combo;
	FString m_ComboOri;
	FString m_Section;
	std::vector<std::string> m_CustomStrings;
	bool ReadValue = true;
	bool LoadValueAsName = false;
	bool UseStrictOrder = false;
	int LoadFrom = 3;
	MultimapHelper mmh;

protected:
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

};
