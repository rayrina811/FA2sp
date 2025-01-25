#pragma once

#include <CPropertyInfantry.h>
#include "../FA2Expand.h"

#include <CObjectDatas.h>

class NOVTABLE CPropertyInfantryExt : public CPropertyInfantry
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