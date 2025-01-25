#pragma once
#include <FA2PP.h>
#include <CFinalSunDlg.h>

ppmfc::CString InputBox(const char* Sentence, const char* Caption);

class CCheckBox : public ppmfc::CDialog
{
	// Konstruktion
public:
	void SetSentence(ppmfc::CString Sentence);
	void SetCaption(ppmfc::CString Caption);
	CCheckBox(CWnd* pParent = NULL);   // Standardkonstruktor

	// Dialogfelddaten
		//{{AFX_DATA(CCheckBox)
	enum { IDD = 1919810 };
	//}}AFX_DATA


// ¨¹berschreibungen
	// Vom Klassen-Assistenten generierte virtuelle Funktions¨¹berschreibungen
	//{{AFX_VIRTUAL(CCheckBox)
protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV-Unterst¨¹tzung
	//}}AFX_VIRTUAL

// Implementierung
protected:

	// Generierte Nachrichtenzuordnungsfunktionen
	//{{AFX_MSG(CCheckBox)
	virtual void OnOK();
	virtual void OnCancel();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	//DECLARE_MESSAGE_MAP()
private:
	ppmfc::CString m_Text;
	ppmfc::CString m_Caption;
};

