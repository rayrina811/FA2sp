#pragma once

#include <CLoading.h>
#include "../FA2Expand.h"
#include <CShpFile.h>
#include <vector>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <CPalette.h>

class ImageDataClass;
class Palette;

class NOVTABLE CLoadingExt : public CLoading
{
public:

	//hook function to replace in virtual function map
	//BOOL PreTranslateMessageExt(MSG* pMsg);

	//static void ProgramStartupInit();

	static CLoadingExt* GetExtension()
	{
		return (CLoadingExt*)CLoading::Instance();
	}

	static bool HasFile_ReadyToReadFromFolder;

	bool InitMixFilesFix();
	static bool IsObjectLoaded(ppmfc::CString pRegName);

	void LoadObjects(ppmfc::CString pRegName);
	
	// except buildings
	static ppmfc::CString GetImageName(ppmfc::CString ID, int nFacing, bool bShadow = false, bool bDeploy = false, bool bWater = false);
	// only buildings
	enum
	{
		GBIN_NORMAL,
		GBIN_RUBBLE,		
		GBIN_DAMAGED,
	};
	static ppmfc::CString GetBuildingImageName(ppmfc::CString ID, int nFacing, int state, bool bShadow = false);
	
	static void ClearItemTypes();
	void GetFullPaletteName(ppmfc::CString& PaletteName);
	static void LoadShp(ppmfc::CString ImageID, ppmfc::CString FileName, ppmfc::CString PalName, int nFrame);
	static void LoadShpToSurface(ppmfc::CString ImageID, ppmfc::CString FileName, ppmfc::CString PalName, int nFrame);
	static void LoadShpToSurface(ppmfc::CString ImageID, unsigned char* pBuffer, int Width, int Height, Palette* pPal);
	static bool LoadShpToBitmap(ImageDataClass* pData, CBitmap& outBitmap);
	static void LoadSHPFrameSafe(int nFrame, int nFrameCount, unsigned char** ppBuffer, const ShapeHeader& header);
	static void LoadBitMap(ppmfc::CString ImageID, const CBitmap& cBitmap);
	void SetImageData(unsigned char* pBuffer, ppmfc::CString NameInDict, int FullWidth, int FullHeight, Palette* pPal);

	static int GetITheaterIndex()
	{
		switch (CLoading::Instance->TheaterIdentifier)
		{
		case 'A':
			return 1;
		case 'U':
			return 2;
		case 'D':
			return 5;
		case 'L':
			return 4;
		case 'N':
			return 3;
		case 'T':
		default:
			return 0;
		}
	}

private:
	static ppmfc::CString* __cdecl GetDictName(ppmfc::CString* ret, const char* ID, int nFacing) { JMP_STD(0x475450); }
	static ppmfc::CString GetDictName(ppmfc::CString ID, int nFacing)
	{
		ppmfc::CString buffer;
		GetDictName(&buffer, ID, nFacing);
		return buffer;
	}

	void LoadBuilding(ppmfc::CString ID);
	void LoadBuilding_Normal(ppmfc::CString ID);
	void LoadBuilding_Rubble(ppmfc::CString ID);
	void LoadBuilding_Damaged(ppmfc::CString ID, bool loadAsRubble = false);

	void LoadInfantry(ppmfc::CString ID);
	void LoadTerrainOrSmudge(ppmfc::CString ID, bool terrain);
	void LoadVehicleOrAircraft(ppmfc::CString ID);

	void SetImageData(unsigned char* pBuffer, ImageDataClass* pData, int FullWidth, int FullHeight, Palette* pPal);
	void ShrinkSHP(unsigned char* pIn, int InWidth, int InHeight, unsigned char*& pOut, int* OutWidth, int* OutHeight);
	void UnionSHP_Add(unsigned char* pBuffer, int Width, int Height, int DeltaX = 0, int DeltaY = 0, bool UseTemp = false, bool bShadow = false);
	void UnionSHP_GetAndClear(unsigned char*& pOutBuffer, int* OutWidth, int* OutHeight, bool UseTemp = false, bool bShadow = false);
	void VXL_Add(unsigned char* pCache, int X, int Y, int Width, int Height, bool shadow = false);
	void VXL_GetAndClear(unsigned char*& pBuffer, int* OutWidth, int* OutHeight, bool shadow = false);
	
	void SetValidBuffer(ImageDataClass* pData, int Width, int Height);

	int ColorDistance(const ColorStruct& color1, const ColorStruct& color2); 
	std::vector<int> GeneratePalLookupTable(Palette* first, Palette* second);

public:
	enum class ObjectType{
		Unknown = -1,
		Infantry = 0,
		Vehicle = 1,
		Aircraft = 2,
		Building = 3,
		Terrain = 4,
		Smudge = 5
	};

	static ppmfc::CString GetArtID(ppmfc::CString ID);
	ppmfc::CString GetVehicleOrAircraftFileID(ppmfc::CString ID);
	ppmfc::CString GetTerrainOrSmudgeFileID(ppmfc::CString ID);
	ppmfc::CString GetBuildingFileID(ppmfc::CString ID);
	ppmfc::CString GetInfantryFileID(ppmfc::CString ID);
	static std::vector<ppmfc::CString> LoadedOverlays;
	static Palette TempISOPalette;
	static bool IsLoadingObjectView;
	static std::vector<ppmfc::CString> SwimableInfantries;
	ObjectType GetItemType(ppmfc::CString ID);
	static bool SaveCBitmapToFile(CBitmap* pBitmap, const ppmfc::CString& filePath, COLORREF bgColor);
	static bool LoadBMPToCBitmap(const ppmfc::CString& filePath, CBitmap& outBitmap);

	static std::unordered_set<ppmfc::CString> LoadedObjects;
	static int TallestBuildingHeight;
private:

	void DumpFrameToFile(unsigned char* pBuffer, Palette* pPal, int Width, int Height, ppmfc::CString name);
	
	struct SHPUnionData
	{
		unsigned char* pBuffer;
		int Width;
		int Height;
		int DeltaX;
		int DeltaY;
	};

	static std::vector<SHPUnionData> UnionSHP_Data[2];
	static std::vector<SHPUnionData> UnionSHPShadow_Data[2];
	static std::unordered_map<ppmfc::CString, ObjectType> ObjectTypes;
	static unsigned char VXL_Data[0x10000];
	static unsigned char VXL_Shadow_Data[0x10000];
};