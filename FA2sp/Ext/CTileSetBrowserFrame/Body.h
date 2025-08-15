#pragma once

#include <CTileSetBrowserFrame.h>
#include "../FA2Expand.h"
#include "../../ExtraWindow/CTerrainGenerator/CTerrainGenerator.h"
#include "../../FA2sp/Helpers/FString.h"

class NOVTABLE CTileSetBrowserFrameExt : public CTileSetBrowserFrame
{
public:

	static void ProgramStartupInit();

	enum class TabPage : int
	{
		TilesetBrowser = 0,
		TriggerSort = 1,

		TagSort = 2,
		TeamSort = 3,
		TaskforceSort = 4,
		ScriptSort = 5,
		WaypointSort = 6,
	};

	//
	// Ext Functions
	//

	BOOL OnInitDialogExt();
	BOOL PreTranslateMessageExt(MSG* pMsg);
	BOOL OnNotifyExt(WPARAM wParam, LPARAM lParam, LRESULT* pResult);
	BOOL OnCommandExt(WPARAM wParam, LPARAM lParam);

	

	CTileSetBrowserFrameExt() {};
	~CTileSetBrowserFrameExt() {};

	// Functional Functions
	void OnBNTileManagerClicked();
	void OnBNSearchClicked();
	void OnBNTerrainGeneratorClicked();

	void InitTabControl();

private:

public:
	static CTerrainGenerator m_terrainGenerator;
	static HWND hTabCtrl;
};