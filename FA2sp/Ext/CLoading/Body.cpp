#include "Body.h"

#include <CFinalSunApp.h>
#include <CMixFile.h>
#include <CINI.h>
#include "../../Helpers/Translations.h"
#include "../../FA2sp.h"
bool CLoadingExt::HasFile_ReadyToReadFromFolder = false;
Palette CLoadingExt::TempISOPalette = { 0 };
bool CLoadingExt::IsLoadingObjectView = false;
std::unordered_set<ppmfc::CString> CLoadingExt::SwimableInfantries;

bool CLoadingExt::InitMixFilesFix()
{
	HasMdFile = true;

	// Load encrypted packages
	ResourcePackManager::instance().clear();
	if (auto pSection = CINI::FAData->GetSection("ExtraPackages"))
	{
		std::map<int, ppmfc::CString> collector;

		for (const auto& [key, index] : pSection->GetIndices())
			collector[index] = key;

		ppmfc::CString path;

		for (const auto& [_, key] : collector)
		{
			if (CINI::FAData->GetBool("ExtraPackages", key))
				path = CFinalSunApp::Instance->ExePath;
			else
				path = CFinalSunApp::Instance->FilePath;
			path += "\\" + key;

			if (ResourcePackManager::instance().loadPack(path.m_pchData))
			{
				Logger::Raw("[MixLoader][Package] %s loaded.\n", path);
			}
			else
			{
				Logger::Raw("[MixLoader][Package] %s failed!\n", path);
			}
		}
	}
	// Load Extra Mixes
	if (auto pSection = CINI::FAData->GetSection("ExtraMixes"))
	{
		std::map<int, ppmfc::CString> collector;

		for (const auto& [key, index] : pSection->GetIndices())
			collector[index] = key;

		ppmfc::CString path;

		for (const auto& [_, key] : collector)
		{
			if (CINI::FAData->GetBool("ExtraMixes", key))
				path = CFinalSunApp::Instance->ExePath;
			else
				path = CFinalSunApp::Instance->FilePath;
			path += "\\" + key;
			if (auto id = CMixFile::Open(path, 0))
			{
				Logger::Raw("[MixLoader][EXTRA] %04d - %s loaded.\n", id, path);
			}
			else
			{
				Logger::Raw("[MixLoader][EXTRA] %s failed!\n", path);
			}
		}
	}

	ppmfc::CString Dir = CFinalSunApp::Instance->FilePath();
	Dir += "\\";
	auto LoadMixFile = [this, Dir](const char* Mix, int Parent = 0)
	{
		if (Parent)
		{
			int result = CMixFile::Open(Mix, Parent);
			if (result)
				Logger::Raw("[MixLoader] %04d - %s loaded.\n", result, Mix);
			else
				Logger::Raw("[MixLoader] %s failed!\n", Mix);
			return ExtConfigs::DisableDirectoryCheck || result;
		}
		else
		{
			ppmfc::CString FullPath = Dir + Mix;
			int result = CMixFile::Open(FullPath, 0);
			if (result)
			{
				Logger::Raw("[MixLoader] %04d - %s loaded.\n", result, FullPath);
				return ExtConfigs::DisableDirectoryCheck || result;
			}
			if (int nMix = SearchFile(Mix))
			{
				result = CMixFile::Open(Mix, nMix);
				if (result)
					Logger::Raw("[MixLoader] %04d - %s loaded.\n", result, Mix);
				else
					Logger::Raw("[MixLoader] %s failed!\n", Mix);
				return ExtConfigs::DisableDirectoryCheck || result;
			}
			Logger::Raw("[MixLoader] %s failed!\n", Mix);
			return ExtConfigs::DisableDirectoryCheck || result;
		}
	};
	auto SetMixFile = [LoadMixFile](const char* Mix, int& value)
	{
		value = LoadMixFile(Mix);
		return value;
	};

	ppmfc::CString fa2extra = CFinalSunApp::Instance->ExePath();
	fa2extra += "\\";
	fa2extra += "fa2extra.mix";
	if (auto id = CMixFile::Open(fa2extra, 0))
	{
		Logger::Raw("[MixLoader] %04d - %s loaded.\n", id, fa2extra);
	}
	else
	{
		Logger::Raw("[MixLoader] %s failed!\n", fa2extra);
	}

	ppmfc::CString format = "EXPAND" + CINI::FAData->GetString("Filenames", "MixExtension", "MD") + "%02d.MIX";
	for (int i = 99; i >= 0; --i)
	{
		ppmfc::CString filename; 
		filename.Format(format, i);
		LoadMixFile(filename);
	}

	if (!LoadMixFile("RA2MD.MIX"))		return false;
	if (!LoadMixFile("RA2.MIX"))		return false;
	if (!LoadMixFile("CACHEMD.MIX"))	return false;
	if (!LoadMixFile("CACHE.MIX"))		return false;
	if (!LoadMixFile("LOCALMD.MIX"))	return false;
	if (!LoadMixFile("LOCAL.MIX"))		return false;

	// Init_Expansion_Mixfiles
	// ECACHE and ELOCAL
	WIN32_FIND_DATA fd;
	HANDLE hf = FindFirstFile(Dir + "ECACHE*.MIX", &fd);
	if (hf != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (fd.dwFileAttributes & (FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN))
				continue;
			LoadMixFile(fd.cFileName);
		} while (FindNextFile(hf, &fd));
		FindClose(hf);
	}
	hf = FindFirstFile(Dir + "ELOCAL*.MIX", &fd);
	if (hf != INVALID_HANDLE_VALUE)
	{
		do
		{
			if (fd.dwFileAttributes & (FILE_ATTRIBUTE_TEMPORARY | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN))
				continue;
			LoadMixFile(fd.cFileName);
		} while (FindNextFile(hf, &fd));
		FindClose(hf);
	}

	// Init_Secondary_Mixfiles
	if (!LoadMixFile("CONQMD.MIX"))		return false;
	if (!LoadMixFile("GENERMD.MIX"))	return false;
	if (!LoadMixFile("GENERIC.MIX"))	return false;
	if (!LoadMixFile("ISOGENMD.MIX"))	return false;
	if (!LoadMixFile("ISOGEN.MIX"))		return false;
	if (!LoadMixFile("CONQUER.MIX"))	return false;

	//MARBLE should be ahead of normal theater mixes
	ppmfc::CString FullPath = CFinalSunApp::ExePath();
	FullPath += "\\MARBLE.MIX";
	int result = CMixFile::Open(FullPath, 0);
	if (result)
	{
		Logger::Raw("[MixLoader] %04d - %s loaded.\n", result, FullPath);
		CFinalSunApp::Instance->MarbleLoaded = TRUE;
	}
	else
	{
		if (LoadMixFile("MARBLE.MIX"))
			CFinalSunApp::Instance->MarbleLoaded = TRUE;
		else
		{
			CFinalSunApp::Instance->MarbleLoaded = FALSE;
			ppmfc::CString pMessage = Translations::TranslateOrDefault("MarbleMadnessNotLoaded",
				"Failed to load marble.mix! Framework mode won't be able to use!");
			::MessageBox(NULL, pMessage, Translations::TranslateOrDefault("Error", "Error"), MB_OK | MB_ICONEXCLAMATION);
		}
	}


	// Init_Theaters
	LoadMixFile("TEMPERATMD.MIX");
	LoadMixFile("ISOTEMMD.MIX");
	LoadMixFile("TEMPERAT.MIX");
	LoadMixFile("ISOTEMP.MIX");
	LoadMixFile("TEM.MIX");
	
	LoadMixFile("SNOWMD.MIX");
	LoadMixFile("ISOSNOMD.MIX");
	LoadMixFile("SNOW.MIX");
	LoadMixFile("ISOSNOW.MIX");
	LoadMixFile("ISOSNO.MIX");
	LoadMixFile("SNO.MIX");

	LoadMixFile("URBANMD.MIX");
	LoadMixFile("ISOURBMD.MIX");
	LoadMixFile("URBAN.MIX");
	LoadMixFile("ISOURB.MIX");
	LoadMixFile("URB.MIX");

	LoadMixFile("DESERT.MIX");
	LoadMixFile("ISODES.MIX");
	LoadMixFile("ISODESMD.MIX");
	LoadMixFile("DES.MIX");
	LoadMixFile("DESERTMD.MIX");

	LoadMixFile("URBANNMD.MIX");
	LoadMixFile("ISOUBNMD.MIX");
	LoadMixFile("URBANN.MIX");
	LoadMixFile("ISOUBN.MIX");
	LoadMixFile("UBN.MIX");

	LoadMixFile("LUNARMD.MIX");
	LoadMixFile("ISOLUNMD.MIX");
	LoadMixFile("LUNAR.MIX");
	LoadMixFile("ISOLUN.MIX");
	LoadMixFile("LUN.MIX");

	LoadMixFile("LANGMD.MIX");
	LoadMixFile("LANGUAGE.MIX");
	
	return true;
}