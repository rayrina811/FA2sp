#pragma once
#include "FA2PP.H"

class CListUInputDlg : public ppmfc::CDialog
{
public:
	CListUInputDlg(CWnd* pParent = NULL);
	ppmfc::CString m_Caption;
	std::vector<ppmfc::CString> m_selectedTexts;
	std::vector<ppmfc::CString> m_CustomStrings;

protected:
	virtual void DoDataExchange(ppmfc::CDataExchange* pDX);
	virtual BOOL OnInitDialog();
	virtual void OnOK();
	virtual void OnCancel();

};
