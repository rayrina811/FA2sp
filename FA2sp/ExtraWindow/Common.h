#pragma once
#include "../FA2sp.h"

class ExtraWindow
{
public:
    static void CenterWindowPos(HWND parent, HWND target);
    static ppmfc::CString GetTeamDisplayName(const char* id);
    static ppmfc::CString GetAITriggerDisplayName(const char* id);
    static ppmfc::CString FormatTriggerDisplayName(const char* id, const char* name);
    static ppmfc::CString GetEventDisplayName(const char* id, int index = -1);
    static ppmfc::CString GetActionDisplayName(const char* id, int index = -1);
    static void SetEditControlFontSize(HWND hWnd, float nFontSizeMultiplier, bool richEdit = false, const char* newFont = "");
    static int FindCBStringExactStart(HWND hComboBox, const char* searchText);
    static void SyncComboBoxContent(HWND hSource, HWND hTarget, bool addNone = false);
    static void AdjustDropdownWidth(HWND hWnd);
    static ppmfc::CString GetTriggerDisplayName(const char* id);
    static ppmfc::CString GetTriggerName(const char* id);
    static ppmfc::CString GetAITriggerName(const char* id);
    static ppmfc::CString GetTagName(const char* id);

    static void LoadParams(HWND& hWnd, ppmfc::CString idx);
    static void LoadParam_Waypoints(HWND& hWnd);
    static void LoadParam_ActionList(HWND& hWnd);
    static void LoadParam_CountryList(HWND& hWnd);
    static void LoadParam_HouseAddon_Multi(HWND& hWnd);
    static void LoadParam_HouseAddon_MultiAres(HWND& hWnd);
    static void LoadParam_TechnoTypes(HWND& hWnd, int specificType = -1, int style = 0, bool sort = true);
    static void LoadParam_Triggers(HWND& hWnd);
    static void LoadParam_Tags(HWND& hWnd);
    static void LoadParam_Stringtables(HWND& hWnd);

    static RECT rectComboLBox;
    static HWND hComboLBox;
    static bool bEnterSearch;
    static void GetCurrentDropdown();
    static bool IsClickInsideDropdown();    
    // true means click inside combobox
    static bool OnCloseupCComboBox(HWND& hWnd, std::map<int, ppmfc::CString>& labels, bool isComboboxSelectOnly = false);
    static void OnEditCComboBox(HWND& hWnd, std::map<int, ppmfc::CString>& labels);

    static bool SortLabels(ppmfc::CString a, ppmfc::CString b);
    static void SortTeams(HWND& hWnd, ppmfc::CString section, int& selectedIndex, ppmfc::CString id = "");
    static void SortAITriggers(HWND& hWnd, int& selectedIndex, ppmfc::CString id = "");
    static bool IsLabelMatch(const char* target, const char* source, bool exactMatch = false);
    static ppmfc::CString GetCloneName(ppmfc::CString oriName);
    static void LoadFrom(MultimapHelper& mmh, ppmfc::CString loadfrom);

private:
    static CINI& map;
    static CINI& fadata;
    static MultimapHelper& rules;
};



