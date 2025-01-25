#pragma once

#include <CPropertyAircraft.h>
#include "../FA2Expand.h"

class NOVTABLE CPropertyAircraftExt : public CPropertyAircraft
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