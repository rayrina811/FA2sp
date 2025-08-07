#pragma once

#include <CLoading.h>
#include "../FA2Expand.h"
#include <CShpFile.h>
#include <vector>
#include <array>
#include <algorithm>
#include <map>
#include <unordered_map>
#include <unordered_set>
#include <CPalette.h>
#include <Drawing.h>
#include <fstream>
#include <set>

class ImageDataClass;
class Palette;

class NOVTABLE ImageDataClassSafe
{
public:

	std::unique_ptr<unsigned char[]> pImageBuffer; // draw from here, size = FullWidth*FullHeight

	struct ValidRangeData
	{
		short First;
		short Last;
	};
	std::unique_ptr<ValidRangeData[]> pPixelValidRanges;
	// size = FullHeight, stores the valid pixel from where to begin and end for each row
	// If it's an invalid row, then first = FullWidth - 1, second = 0 

	Palette* pPalette;
	short ValidX; // negative value for vxl, dunno why
	short ValidY; // negative value for vxl, dunno why
	short ValidWidth; // same as full for vxl, dunno why
	short ValidHeight; // same as full for vxl, dunno why
	short FullWidth;
	short FullHeight;
	ImageDataFlag Flag;
	BuildingImageFlag BuildingFlag; // see BuildingData
	BOOL IsOverlay; // Only OVRLXX_XX will set this true
};


class NOVTABLE ImageDataClassSurface
{
public:
	LPDIRECTDRAWSURFACE7 lpSurface; // Only available for flag = 0, if this is used, only ValidWidth and ValidHeight are set
	short FullWidth;
	short FullHeight;
	ImageDataFlag Flag;
};

struct ImageDataTransfer
{
	char imageID[0x100];
	ImageDataClassSafe pData;
};

struct AnimDisplayOrder
{
	int ZAdjust = 0;
	int YSort = 0;
	bool MainBody = false;
	ppmfc::CString AnimKey = "";
	ppmfc::CString IgnoreKey = "";
};

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
	static bool IsOverlayLoaded(ppmfc::CString pRegName);

	void LoadObjects(ppmfc::CString pRegName);
	void LoadOverlay(ppmfc::CString, int nIndex);
	
	// except buildings
	static ppmfc::CString GetImageName(ppmfc::CString ID, int nFacing, bool bShadow = false, bool bDeploy = false, bool bWater = false);
	static ppmfc::CString GetOverlayName(WORD ovr, BYTE ovrd, bool bShadow = false);
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
	static void LoadShp(ppmfc::CString ImageID, ppmfc::CString FileName, ppmfc::CString PalName, int nFrame, bool toServer = true);
	static void LoadShp(ppmfc::CString ImageID, ppmfc::CString FileName, Palette* pPal, int nFrame, bool toServer = true);
	static void LoadShpToSurface(ppmfc::CString ImageID, ppmfc::CString FileName, ppmfc::CString PalName, int nFrame);
	static void LoadShpToSurface(ppmfc::CString ImageID, unsigned char* pBuffer, int Width, int Height, Palette* pPal);
	static bool LoadShpToBitmap(ImageDataClassSafe* pData, CBitmap& outBitmap);
	static bool LoadShpToBitmap(ImageDataClass* pData, CBitmap& outBitmap);
	static void LoadSHPFrameSafe(int nFrame, int nFrameCount, unsigned char** ppBuffer, const ShapeHeader& header);
	static void LoadBitMap(ppmfc::CString ImageID, const CBitmap& cBitmap);
	void SetImageDataSafe(unsigned char* pBuffer, ppmfc::CString NameInDict, int FullWidth, int FullHeight, Palette* pPal, bool toServer = true);
	void SetImageData(unsigned char* pBuffer, ppmfc::CString NameInDict, int FullWidth, int FullHeight, Palette* pPal);
	// returns the mix index, -1 for in folder / pack, -2 for not found
	int HasFileMix(ppmfc::CString filename, int nMix = -114);

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

	void SetImageDataSafe(unsigned char* pBuffer, ImageDataClassSafe* pData, int FullWidth, int FullHeight, Palette* pPal);
	void SetImageData(unsigned char* pBuffer, ImageDataClass* pData, int FullWidth, int FullHeight, Palette* pPal);
	void ShrinkSHP(unsigned char* pIn, int InWidth, int InHeight, unsigned char*& pOut, int* OutWidth, int* OutHeight);
	void UnionSHP_Add(unsigned char* pBuffer, int Width, int Height, int DeltaX = 0, int DeltaY = 0, bool UseTemp = false, bool bShadow = false);
	void UnionSHP_GetAndClear(unsigned char*& pOutBuffer, int* OutWidth, int* OutHeight, bool UseTemp = false, bool bShadow = false);
	void VXL_Add(unsigned char* pCache, int X, int Y, int Width, int Height, bool shadow = false);
	void VXL_GetAndClear(unsigned char*& pBuffer, int* OutWidth, int* OutHeight, bool shadow = false);
	
	void SetValidBuffer(ImageDataClass* pData, int Width, int Height);
	void SetValidBufferSafe(ImageDataClassSafe* pData, int Width, int Height);

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
	static std::unordered_set<ppmfc::CString> LoadedOverlays;
	static Palette TempISOPalette;
	static bool IsLoadingObjectView;
	static std::unordered_set<ppmfc::CString> SwimableInfantries;
	ObjectType GetItemType(ppmfc::CString ID);
	static bool SaveCBitmapToFile(CBitmap* pBitmap, const ppmfc::CString& filePath, COLORREF bgColor);
	static bool LoadBMPToCBitmap(const ppmfc::CString& filePath, CBitmap& outBitmap);

	static std::unordered_set<ppmfc::CString> LoadedObjects;
	static std::unordered_set<int> Ra2dotMixes;
	static int TallestBuildingHeight;
private:

	void DumpFrameToFile(unsigned char* pBuffer, Palette* pPal, int Width, int Height, ppmfc::CString name);
	void SortDisplayOrderByZAdjust(std::vector<AnimDisplayOrder>& displayOrder) {
		std::stable_sort(displayOrder.begin(), displayOrder.end(),
			[](const AnimDisplayOrder& a, const AnimDisplayOrder& b) {
				return a.ZAdjust > b.ZAdjust;
			});
	}

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


public:
	static std::unordered_map<ppmfc::CString, std::unique_ptr<ImageDataClassSafe>> CurrentFrameImageDataMap;
	static std::unordered_map<ppmfc::CString, std::unique_ptr<ImageDataClassSafe>> ImageDataMap;
	static std::unordered_map<ppmfc::CString, std::unique_ptr<ImageDataClassSurface>> SurfaceImageDataMap;

	static bool IsImageLoaded(const ppmfc::CString& name);
	static ImageDataClassSafe* GetImageDataFromMap(const ppmfc::CString& name);
	static ImageDataClassSafe* GetImageDataFromServer(const ppmfc::CString& name);
	static bool IsSurfaceImageLoaded(const ppmfc::CString& name);
	static ImageDataClassSurface* GetSurfaceImageDataFromMap(const ppmfc::CString& name);

	static HANDLE hPipeData;
	static std::atomic<bool> PingServerRunning;
	static std::string PipeNameData;
	static std::string PipeNamePing;
	static std::string PipeName;

	static bool StartImageServerProcess();
	static bool ConnectToImageServer();
	static void StartPingThread();

	static bool WriteImageData(HANDLE hPipe, const ppmfc::CString& imageID, const ImageDataClassSafe* data);
	static bool ReadImageData(HANDLE hPipe, ImageDataClassSafe& data);
	static bool SendImageToServer(const ppmfc::CString& imageID, const ImageDataClassSafe* imageData);
	static bool RequestImageFromServer(const ppmfc::CString& imageID, ImageDataClassSafe& outImageData);
	static void SendRequestText(const char* text);
};

#pragma pack(push, 1)
struct FileEntry
{
	uint32_t offset;
	uint32_t enc_size;
	uint32_t original_size;
};
#pragma pack(pop)
static_assert(sizeof(FileEntry) == 12, "FileEntry must be 12 bytes");

struct CaseInsensitiveHash 
{
	size_t operator()(const std::string& key) const {
		std::string lower;
		lower.reserve(key.size());
		for (char ch : key) lower.push_back(std::tolower(static_cast<unsigned char>(ch)));
		return std::hash<std::string>()(lower);
	}
};

struct CaseInsensitiveEqual 
{
	bool operator()(const std::string& lhs, const std::string& rhs) const {
		if (lhs.size() != rhs.size()) return false;
		for (size_t i = 0; i < lhs.size(); ++i) {
			if (std::tolower(static_cast<unsigned char>(lhs[i])) != std::tolower(static_cast<unsigned char>(rhs[i])))
				return false;
		}
		return true;
	}
};

class ResourcePack 
{
public:
	bool load(const std::string& filename);
	std::unique_ptr<uint8_t[]> getFileData(const std::string& filename, size_t* out_size = nullptr);

private:
	std::unordered_map<std::string, FileEntry, CaseInsensitiveHash, CaseInsensitiveEqual> index_map;
	uint32_t index_size = 0; 
	std::string file_path;
	std::ifstream file_stream; 

	bool aesDecryptBlockwise(const uint8_t* input, size_t len, std::vector<uint8_t>& output);
	std::array<uint8_t, 32> get_aes_key();
};

class ResourcePackManager 
{
public:

	static ResourcePackManager& instance();
	bool loadPack(const std::string& packPath);
	std::unique_ptr<uint8_t[]> getFileData(const std::string& filename, size_t* out_size = nullptr);
	void clear();

private:
	std::vector<std::unique_ptr<ResourcePack>> packs;
};