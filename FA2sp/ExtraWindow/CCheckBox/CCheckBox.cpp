#include <FA2PP.h>
#include <CFinalSunDlg.h>
#include "CCheckBox.h"
ppmfc::CString InputBox(const char* Sentence, const char* Caption)
{
	CCheckBox inp;
	inp.SetCaption(Caption);
	inp.SetSentence(Sentence);
	char* res = (char*)inp.DoModal();
	ppmfc::CString cstr = res;

	return cstr;
}

CCheckBox::CCheckBox(CWnd* pParent /*=NULL*/)
	: CDialog(CCheckBox::IDD, pParent)
{
	//{{AFX_DATA_INIT(CCheckBox)

	//}}AFX_DATA_INIT
}


void CCheckBox::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CCheckBox)

	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CCheckBox, CDialog)
	//{{AFX_MSG_MAP(CCheckBox)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()



void CCheckBox::OnOK()
{
	ppmfc::CString text;

	if (text.GetLength() == 0) { EndDialog(NULL); };

	char* str;
	str = new(char[text.GetLength()]);
	strcpy(str, (LPCTSTR)text);
	EndDialog((int)str);
}

void CCheckBox::OnCancel()
{
	EndDialog(NULL);
}

void CCheckBox::SetCaption(ppmfc::CString Caption)
{
	m_Caption = Caption;
}

void CCheckBox::SetSentence(ppmfc::CString Sentence)
{
	m_Text = Sentence;
}

BOOL CCheckBox::OnInitDialog()
{
	CDialog::OnInitDialog();

	SetWindowText(m_Caption);

	SetDlgItemText(IDOK, "确认");
	SetDlgItemText(IDCANCEL, "取消");

	return FALSE;
}