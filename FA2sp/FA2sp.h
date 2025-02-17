#pragma once

#include "Logger.h"
#include "Ext/FA2Expand.h"
#include "Helpers/MultimapHelper.h"

#include <Helpers/Macro.h>
#include <MFC/ppmfc_cstring.h>

#include <map>

typedef unsigned char byte;

class FA2sp
{
public:
    static HANDLE hInstance;
    static std::string STDBuffer;
    static ppmfc::CString Buffer;
    static std::map<ppmfc::CString, ppmfc::CString> TutorialTextsMap;
    static void* pExceptionHandler;

    static void ExtConfigsInitialize();
};

class ExtConfigs
{
public:
    static bool IsQuitingProgram;
    static bool BrowserRedraw;
    static int  ObjectBrowser_GuessMode;
    static int  ObjectBrowser_GuessMax;
    static bool ObjectBrowser_CleanUp;
    static bool ObjectBrowser_SafeHouses;
    static bool ObjectBrowser_Foundation;
    static bool LoadLunarWater;
    static bool LoadCivilianStringtable;
    static bool AllowIncludes;
    static bool AllowPlusEqual;
    static bool TutorialTexts_Hide;
    static bool TutorialTexts_Fix;
    static bool TutorialTexts_Viewer;
    static bool SkipTipsOfTheDay;
    static bool SortByTriggerName;
    static bool SortByLabelName;
    static bool DisplayTriggerID;
    static bool NewTriggerPlusID;
    static bool AdjustDropdownWidth;
    static int AdjustDropdownWidth_Factor;
    static int AdjustDropdownWidth_Max;
    static int CopySelectionBound_Color;
    static int CursorSelectionBound_Color;
    static int WeaponRangeBound_Color;
    static int WeaponRangeMinimumBound_Color;
    static int SecondaryWeaponRangeBound_Color;
    static int SecondaryWeaponRangeMinimumBound_Color;
    static int GapRangeBound_Color;
    static int SensorsRangeBound_Color;
    static int CloakRangeBound_Color;
    static int PsychicRangeBound_Color;
    static int GuardRangeBound_Color;
    static int SightRangeBound_Color;
    static bool WeaponRangeBound_SubjectToElevation;
    static int CursorSelectionBound_HeightColor;
    static int Waypoint_Color;
    static bool Waypoint_Background;
    static int Waypoint_Background_Color;
    static int BaseNodeIndex_Color;
    static bool BaseNodeIndex_Background;
    static bool BaseNodeIndex;
    static int BaseNodeIndex_Background_Color;
    static CPoint Waypoint_Text_ExtraOffset;
    static bool ExtWaypoints;
    static int UndoRedoLimit;
    static bool UndoRedo_ShiftPlaceTile;
    static bool UndoRedo_HoldPlaceOverlay;
    static bool UseRGBHouseColor;
    static bool SaveMap_AutoSave;
    static int SaveMap_AutoSave_Interval;
    static int SaveMap_AutoSave_MaxCount;
    static bool SaveMap_OnlySaveMAP;
    //static bool SaveMap_MultiPlayOnlySaveYRM;
    //static bool SaveMap_SinglePlayOnlySaveMAP;
    static int SaveMap_DefaultPreviewOptionMP;
    static int SaveMap_DefaultPreviewOptionSP;
    static bool SaveMap_FileEncodingComment;
    static bool VerticalLayout;
    static int RecentFileLimit;
    static int MultiSelectionColor;
    static int TerrainGeneratorColor;
    static bool MultiSelectionShiftDeselect;
    static bool RandomTerrainObjects;
    static unsigned int MaxVoxelFacing;
    static bool DDrawInVideoMem;
    static bool DDrawEmulation;
    static bool NoHouseNameTranslation;
    static bool BetterHouseNameTranslation;
    static bool EnableMultiSelection;
    static bool ExtendedValidationNoError;
    static bool HideNoRubbleBuilding;
    static bool ModernObjectBrowser;
    static bool PlayerAtXForTechnos;
    static bool FileWatcher;
    static bool LoadRA2MixFilesOnly;
    static bool ExtVariables;
    static bool TestNotLoaded;
    static bool CloneWithOrderedID;
    static bool InfantrySubCell_GameDefault;
    static bool InfantrySubCell_Edit;
    static bool InfantrySubCell_Edit_Single;
    static bool InfantrySubCell_Edit_Drag;
    static bool InfantrySubCell_Edit_Place;
    static bool InfantrySubCell_Edit_FixCenter;
    static bool InfantrySubCell_OccupationBits;
    static bool PlaceStructureOverlappingCheck;
    static bool PlaceStructureResort;
    static bool PlaceStructureUpgrades;
    static bool PlaceStructureUpgradeStrength;
    static bool InitializeMap;
    static bool ReloadGameFromMapFolder;
    static bool ArtImageSwap;
    static bool ExtraRaiseGroundTerrainSupport;
    static bool ExtendedValidationAres;
    static bool AIRepairDefaultYes;
    static bool AISellableDefaultYes;
    static bool TriggerList_AttachedTriggers;
    static bool TagList_AttachedObjects;
    static bool SaveMaps_BetterMapPreview;
    static bool CursorSelectionBound_AutoColor;
    static bool FillArea_ConsiderLAT;
    static bool FillArea_ConsiderWater;
    static bool DPIAware;
    static bool SkipBrushSizeChangeOnTools;
    static bool INIEditor_IgnoreTeams;
    //static bool StringBufferFixedAllocation;
    static int RangeBound_MaxRange;
    static int SearchCombobox_MaxCount;
    static int NewTheaterType;
    static bool UseStrictNewTheater;
    static ppmfc::CString CloneWithOrderedID_Digits;
    static ppmfc::CString NewTriggerPlusID_Digits;
    static ppmfc::CString Waypoint_SkipCheckList;
};

class Variables
{
public:
    static MultimapHelper Rules;
    static MultimapHelper FAData;
    static MultimapHelper Rules_FAData;
    static std::map<ppmfc::CString, std::vector<std::pair<ppmfc::CString, ppmfc::CString>>> OrderedRulesIndicies;
    static std::map<ppmfc::CString, std::vector<std::pair<ppmfc::CString, ppmfc::CString>>> OrderedRulesIndiciesWithoutMap;
};