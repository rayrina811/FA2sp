#pragma once

#include <CPropertyUnit.h>
#include "../FA2Expand.h"

class NOVTABLE CPropertyUnitExt : public CPropertyUnit
{
public:

	BOOL PreTranslateMessageExt(MSG* pMsg);
	BOOL OnCommandExt(WPARAM wParam, LPARAM lParam);

	static void ProgramStartupInit();

	static void StatusUpdate(LPARAM lParam);
	static void StatusProc(WORD nCode, LPARAM lParam);
	static void CurrentState(HWND nComboBox, int nNode);

private:

};