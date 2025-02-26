#include <vector>
#include "../../FA2sp.h"

class CSelectAutoShore : public ppmfc::CDialog
{
public:
	CSelectAutoShore(CWnd* pParent = NULL);
	std::vector<std::pair<int, int>> Groups;
	ppmfc::CString m_Combo;

protected:
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

};
