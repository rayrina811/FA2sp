#include <vector>
#include "../../FA2sp.h"

#define COMBOUINPUT_HOUSES 0
#define COMBOUINPUT_COUNTRIES 1
#define COMBOUINPUT_TRIGGERS 2
#define COMBOUINPUT_TAGS 3
#define COMBOUINPUT_HOUSES_N 4
#define COMBOUINPUT_COUNTRIES_N 5
#define COMBOUINPUT_MANUAL 6

class CNewComboUInputDlg : public ppmfc::CDialog
{
public:
	int m_type;
	std::vector<ppmfc::CString> m_ManualStrings;
	CNewComboUInputDlg(CWnd* pParent = NULL);
	ppmfc::CString m_Caption;
	ppmfc::CString m_Combo;
	ppmfc::CString m_Section;
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
