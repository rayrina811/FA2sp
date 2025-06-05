#include "Body.h"

#include "../CFinalSunDlg/Body.h"

#include "../../Helpers/Translations.h"

bool iTheFirst = true;
int iNode = NULL;

void CPropertyInfantryExt::ProgramStartupInit()
{
	RunTime::ResetMemoryContentAt(0x593FF8, &CPropertyInfantryExt::PreTranslateMessageExt);
	RunTime::ResetMemoryContentAt(0x593FE0, &CPropertyInfantryExt::OnCommandExt);
}

BOOL CPropertyInfantryExt::PreTranslateMessageExt(MSG* pMsg)
{
#define DECLARE_BST(id) \
if(pMsg->hwnd == this->GetDlgItem(id)->GetSafeHwnd()) \
{ \
	bool tmp = ::SendMessage(::GetDlgItem(*this, id), BM_GETCHECK, 0, 0) == BST_CHECKED; \
	::SendMessage(::GetDlgItem(*this, id), BM_SETCHECK, tmp ? BST_UNCHECKED : BST_CHECKED, 0); \
	CViewObjectsExt::InfantryBrushBools[id-1300] = tmp == false; \
}

	if (pMsg->message == WM_LBUTTONUP)
	{
		DECLARE_BST(1300)
	else DECLARE_BST(1301)
else DECLARE_BST(1302)
	else DECLARE_BST(1303)
	else DECLARE_BST(1304)
	else DECLARE_BST(1305)
	else DECLARE_BST(1306)
	else DECLARE_BST(1307)
	else DECLARE_BST(1308)
	else DECLARE_BST(1309)
	}

#undef DECLARE_BST

	return this->ppmfc::CDialog::PreTranslateMessage(pMsg);
}

BOOL CPropertyInfantryExt::OnCommandExt(WPARAM wParam, LPARAM lParam)
{
	WORD wmID = LOWORD(wParam);
	WORD wmMsg = HIWORD(wParam);

	if (wmID == 1082)
	{
		StatusProc(wmMsg, lParam);
	}
	if (wmID == 1 || wmID == 2)
	{
		iTheFirst = true;
	}

	return this->ppmfc::CDialog::OnCommand(wParam, lParam);
}


void CPropertyInfantryExt::StatusUpdate(LPARAM lParam)
{
	HWND hStatusComboBox = reinterpret_cast<HWND>(lParam);

	if (iTheFirst)
	{
		iNode = ::SendMessage(hStatusComboBox, CB_GETCURSEL, 0, 0);
		iTheFirst = false;
	}


	while (::SendMessage(hStatusComboBox, CB_DELETESTRING, 0, NULL) != CB_ERR);

	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 0, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Ambush", "Ambush")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 1, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.AreaGuard", "Area Guard")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 2, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Attack", "Attack")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 3, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Capture", "Capture")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 4, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Construction", "Construction")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 5, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Enter", "Enter")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 6, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Guard", "Guarde")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 7, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Harmless", "Harmless")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 8, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Harvest", "Harvest")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 9, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Hunt", "Hunt")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 10, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Missile", "Missile")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 11, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Move", "Move")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 12, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Open", "Open")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 13, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Patrol", "Patrol")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 14, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.QMove", "QMove")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 15, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Repair", "Repair")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 16, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Rescue", "Rescue")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 17, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Retreat", "Retreat")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 18, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Return", "Return")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 19, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Sabotage", "Sabotage")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 20, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Selling", "Selling")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 21, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Sleep", "Sleep")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 22, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Sticky", "Sticky")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 23, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Stop", "Stop")));
	::SendMessage(hStatusComboBox, CB_INSERTSTRING, 24, reinterpret_cast<LPARAM>(Translations::TranslateOrDefault("FootClassStatus.Unload", "Unload")));

	::SendMessage(hStatusComboBox, CB_SETCURSEL, iNode, 0);


	return;
}

void CPropertyInfantryExt::StatusProc(WORD nCode, LPARAM lParam)
{
	HWND hStatusComboBox = reinterpret_cast<HWND>(lParam);
	if (::SendMessage(hStatusComboBox, CB_GETCOUNT, NULL, NULL) <= 0)
		return;
	switch (nCode)
	{
	case CBN_DROPDOWN:
		StatusUpdate(lParam);
		break;
	//case CBN_SELCHANGE:
	case CBN_DBLCLK:
	case CBN_CLOSEUP:
		CurrentState(hStatusComboBox, ::SendMessage(hStatusComboBox, CB_GETCURSEL, NULL, NULL));
		break;
	default:
		break;
	}
	return;
}

void CPropertyInfantryExt::CurrentState(HWND nComboBox, int Node)
{
	switch (Node)
	{
	case 0:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Ambush")), 0);
		iNode = Node;
		break;
	case 1:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Area Guard")), 0);
		iNode = Node;
		break;
	case 2:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Attack")), 0);
		iNode = Node;
		break;
	case 3:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Capture")), 0);
		iNode = Node;
		break;
	case 4:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Construction")), 0);
		iNode = Node;
		break;
	case 5:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Enter")), 0);
		iNode = Node;
		break;
	case 6:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Guard")), 0);
		iNode = Node;
		break;
	case 7:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Harmless")), 0);
		iNode = Node;
		break;
	case 8:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Harvest")), 0);
		iNode = Node;
		break;
	case 9:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Hunt")), 0);
		iNode = Node;
		break;
	case 10:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Missile")), 0);
		iNode = Node;
		break;
	case 11:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Move")), 0);
		iNode = Node;
		break;
	case 12:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Open")), 0);
		iNode = Node;
		break;
	case 13:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Patrol")), 0);
		iNode = Node;
		break;
	case 14:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("QMove")), 0);
		iNode = Node;
		break;
	case 15:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Repair")), 0);
		iNode = Node;
		break;
	case 16:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Rescue")), 0);
		iNode = Node;
		break;
	case 17:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Retreat")), 0);
		iNode = Node;
		break;
	case 18:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Return")), 0);
		iNode = Node;
		break;
	case 19:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Sabotage")), 0);
		iNode = Node;
		break;
	case 20:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Selling")), 0);
		iNode = Node;
		break;
	case 21:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Sleep")), 0);
		iNode = Node;
		break;
	case 22:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Sticky")), 0);
		iNode = Node;
		break;
	case 23:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Stop")), 0);
		iNode = Node;
		break;
	case 24:
		::SendMessage(nComboBox, CB_SETCURSEL, ::SendMessage(nComboBox, CB_INSERTSTRING, 25, reinterpret_cast<LPARAM>("Unload")), 0);
		iNode = Node;
		break;
	default:
		break;
	}
	return;
}
