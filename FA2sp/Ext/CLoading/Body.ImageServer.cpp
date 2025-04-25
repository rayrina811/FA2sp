#include <string>
#include <iostream>
#include <CFinalSunApp.h>
#include <CFinalSunDlg.h>
#include <TlHelp32.h>
#include "../../FA2sp.h"
#include "Body.h"
#include <../FA2sp/Ext/CMapData/Body.h>
#include <thread>
#include "../../Helpers/Translations.h"

HANDLE CLoadingExt::hPipeData = NULL;
HANDLE CLoadingExt::hPipePing = NULL;
std::string PipeNameData;
std::string PipeNamePing;
std::string PipeName;

//static bool Is64BitProcess()
//{
//#if defined(_WIN64)
//    return true;
//#else
//    return false;
//#endif
//}
//
//static bool Is64BitOS()
//{
//    BOOL isWow64 = FALSE;
//
//    typedef BOOL(WINAPI* LPFN_ISWOW64PROCESS)(HANDLE, PBOOL);
//    LPFN_ISWOW64PROCESS fnIsWow64Process = (LPFN_ISWOW64PROCESS)
//        GetProcAddress(GetModuleHandleW(L"kernel32"), "IsWow64Process");
//
//    if (fnIsWow64Process) {
//        if (!fnIsWow64Process(GetCurrentProcess(), &isWow64)) {
//            return false; 
//        }
//    }
//
//    return Is64BitProcess() || isWow64;
//}

bool CLoadingExt::CheckProcessExists(const wchar_t* processName) 
{
    if (!ExtConfigs::LoadImageDataFromServer)
        return false;

    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return false;

    PROCESSENTRY32W entry;
    entry.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(snapshot, &entry)) {
        CloseHandle(snapshot);
        return false;
    }

    bool exists = false;
    do {
        wchar_t fileName[_MAX_FNAME] = { 0 };
        _wsplitpath_s(entry.szExeFile, nullptr, 0, nullptr, 0, fileName, _MAX_FNAME, nullptr, 0);

        if (wcscmp(fileName, processName) == 0) {
            exists = true;
            break;
        }
    } while (Process32NextW(snapshot, &entry));

    CloseHandle(snapshot);
    return exists;
}

bool CLoadingExt::StartImageServerProcess(bool firstRun)
{
    if (!ExtConfigs::LoadImageDataFromServer)
        return false;

    PipeName = "\\\\.\\pipe\\ImagePipe_" + std::to_string(GetCurrentProcessId());
    PipeNameData = PipeName + "_data";
    PipeNamePing = PipeName + "_ping";

    bool needReconnect = false;
    if (!firstRun)
    {
        HANDLE hPipeCheck = CreateFileA(
            PipeNameData.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            0,
            NULL
        );

        if (hPipeCheck != INVALID_HANDLE_VALUE)
        {
            CloseHandle(hPipeCheck);
            return true;
        }
        else
        {
            needReconnect = true;
        }
    } 

    std::string exePath = CFinalSunApp::ExePath();
    exePath += "\\ImageServer.exe"; //Is64BitOS() ? "\\ImageServer64.exe" :

    std::string command = exePath + " \"" + PipeName + "\"";

    Logger::Raw("[ImageServer] About to create process %s\n", exePath.c_str());

    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION pi;

    BOOL result = CreateProcessA(
        exePath.c_str(),
        const_cast<char*>(command.c_str()),
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!result) {

        ::MessageBoxA(NULL,
            Translations::TranslateOrDefault("LaunchImageServerFailed", "Failed to launch image server!\n"
                "Please check whether ImageServer.exe exists,\n"
                "or turn off LoadImageDataFromServer."),
            Translations::TranslateOrDefault("Error", "Error"), MB_ICONERROR);
        exit(0);

        return false;
    }

    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    if (needReconnect)
        ConnectToImageServer();

    return true;
}

bool CLoadingExt::ConnectToImageServer() 
{
    if (!ExtConfigs::LoadImageDataFromServer)
        return false;

    const DWORD totalTimeoutMs = 10000; 
    DWORD retryInterval = 30; 
    DWORD startTime = GetTickCount();

    while (true) {
        hPipeData = CreateFileA(
            PipeNameData.c_str(),
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_FLAG_OVERLAPPED,
            NULL
        );

        DWORD error = GetLastError();
        DWORD elapsed = GetTickCount() - startTime;

        if (hPipeData != INVALID_HANDLE_VALUE) {
            DWORD mode = PIPE_READMODE_MESSAGE;

            Logger::Raw("[ImageServer] Successfully connected to ImagePipe, handle: %x\n", hPipeData);

            if (SetNamedPipeHandleState(hPipeData, &mode, NULL, NULL)) {
                return true;
            }
            return true;
        }

        if (elapsed >= totalTimeoutMs) break;
        Sleep(retryInterval);
    }
    return false;
}

void CLoadingExt::StartPingThread(std::atomic<bool>& running) 
{
    std::thread([=, &running]() {

        if (!ExtConfigs::LoadImageDataFromServer)
            return;

        while (running.load()) {
            hPipePing = CreateFileA(
                PipeNamePing.c_str(),
                GENERIC_READ | GENERIC_WRITE,
                0,
                NULL,
                OPEN_EXISTING,
                FILE_FLAG_OVERLAPPED,
                NULL
            );
         
            Sleep(3000);
            CloseHandle(hPipePing);
        }
        return;
        }).detach();
}

bool CLoadingExt::WriteImageData(HANDLE hPipe, const ppmfc::CString& imageID, const ImageDataClassSafe* data)
{
    char charArray[256] = {};
    std::strncpy(charArray, (imageID +"\233" + CLoading::Instance->GetTheaterSuffix()).m_pchData, sizeof(charArray) - 1);

    size_t imageSize = data->FullWidth * data->FullHeight;
    size_t rangeSize = data->FullHeight * sizeof(ImageDataClassSafe::ValidRangeData);
    
    size_t totalSize = sizeof(char[256]) +
        sizeof(data->ValidX) + sizeof(data->ValidY) + sizeof(data->ValidWidth) + sizeof(data->ValidHeight) +
        sizeof(data->FullWidth) + sizeof(data->FullHeight) + sizeof(data->Flag) +
        sizeof(data->BuildingFlag) + sizeof(data->IsOverlay) + sizeof(data->pPalette) +
        imageSize + rangeSize;

    if (imageSize + rangeSize == 0)
        return false;

    std::vector<char> buffer(totalSize);
    char* ptr = buffer.data();
    
    memcpy(ptr, charArray, 256);
    ptr += 256;
    
    memcpy(ptr, &data->ValidX, sizeof(data->ValidX)); ptr += sizeof(data->ValidX);
    memcpy(ptr, &data->ValidY, sizeof(data->ValidY)); ptr += sizeof(data->ValidY);
    memcpy(ptr, &data->ValidWidth, sizeof(data->ValidWidth)); ptr += sizeof(data->ValidWidth);
    memcpy(ptr, &data->ValidHeight, sizeof(data->ValidHeight)); ptr += sizeof(data->ValidHeight);
    memcpy(ptr, &data->FullWidth, sizeof(data->FullWidth)); ptr += sizeof(data->FullWidth);
    memcpy(ptr, &data->FullHeight, sizeof(data->FullHeight)); ptr += sizeof(data->FullHeight);
    memcpy(ptr, &data->Flag, sizeof(data->Flag)); ptr += sizeof(data->Flag);
    memcpy(ptr, &data->BuildingFlag, sizeof(data->BuildingFlag)); ptr += sizeof(data->BuildingFlag);
    memcpy(ptr, &data->IsOverlay, sizeof(data->IsOverlay)); ptr += sizeof(data->IsOverlay);
    memcpy(ptr, &data->pPalette, sizeof(data->pPalette)); ptr += sizeof(data->pPalette);

    memcpy(ptr, data->pImageBuffer.get(), imageSize); ptr += imageSize;
    memcpy(ptr, data->pPixelValidRanges.get(), rangeSize); ptr += rangeSize;

    DWORD written = 0;
    return WriteFile(hPipe, buffer.data(), static_cast<DWORD>(buffer.size()), &written, NULL) && written == buffer.size();
}


bool CLoadingExt::SendImageToServer(const ppmfc::CString& imageID, const ImageDataClassSafe* imageData)
{
    if (!imageData->pImageBuffer)
        return false;
    DWORD bytesWritten;
    WriteImageData(hPipeData, imageID, imageData);
    return true;

}

bool CLoadingExt::ReadImageData(HANDLE hPipe, ImageDataClassSafe& data)
{
    char buffer[256] = {};
    DWORD readBytes = 0;
    bool success = true;

    const size_t charSize = sizeof(char[256]);
    const size_t headerSize =
        sizeof(data.ValidX) + sizeof(data.ValidY) + sizeof(data.ValidWidth) + sizeof(data.ValidHeight) +
        sizeof(data.FullWidth) + sizeof(data.FullHeight) + sizeof(data.Flag) +
        sizeof(data.BuildingFlag) + sizeof(data.IsOverlay) + sizeof(data.pPalette);

    if (!ReadFile(hPipe, buffer, charSize, &readBytes, NULL))
        success = false;
    buffer[255] = '\0';

    if (!success || strncmp(buffer, "NOT_FOUND", 9) == 0)
    {
        return false;
    }
    std::vector<char> headerBuf(headerSize);
    if (!ReadFile(hPipe, headerBuf.data(), headerSize, &readBytes, NULL))
        success = false;

    const char* ptr = headerBuf.data();
    memcpy(&data.ValidX, ptr, sizeof(data.ValidX)); ptr += sizeof(data.ValidX);
    memcpy(&data.ValidY, ptr, sizeof(data.ValidY)); ptr += sizeof(data.ValidY);
    memcpy(&data.ValidWidth, ptr, sizeof(data.ValidWidth)); ptr += sizeof(data.ValidWidth);
    memcpy(&data.ValidHeight, ptr, sizeof(data.ValidHeight)); ptr += sizeof(data.ValidHeight);
    memcpy(&data.FullWidth, ptr, sizeof(data.FullWidth)); ptr += sizeof(data.FullWidth);
    memcpy(&data.FullHeight, ptr, sizeof(data.FullHeight)); ptr += sizeof(data.FullHeight);
    memcpy(&data.Flag, ptr, sizeof(data.Flag)); ptr += sizeof(data.Flag);
    memcpy(&data.BuildingFlag, ptr, sizeof(data.BuildingFlag)); ptr += sizeof(data.BuildingFlag);
    memcpy(&data.IsOverlay, ptr, sizeof(data.IsOverlay)); ptr += sizeof(data.IsOverlay);
    memcpy(&data.pPalette, ptr, sizeof(data.pPalette)); ptr += sizeof(data.pPalette);

    size_t imageSize = data.FullWidth * data.FullHeight;
    size_t rangeSize = data.FullHeight * sizeof(ImageDataClassSafe::ValidRangeData);

    data.pImageBuffer = std::make_unique<unsigned char[]>(imageSize);
    data.pPixelValidRanges = std::make_unique<ImageDataClassSafe::ValidRangeData[]>(data.FullHeight);

    if (!ReadFile(hPipe, data.pImageBuffer.get(), static_cast<DWORD>(imageSize), &readBytes, NULL))
        success = false;
    if (!ReadFile(hPipe, data.pPixelValidRanges.get(), static_cast<DWORD>(rangeSize), &readBytes, NULL))
        success = false;

    return success;
}

bool CLoadingExt::RequestImageFromServer(const ppmfc::CString& imageID, ImageDataClassSafe& outImageData)
{
    ppmfc::CString requestRealID = imageID + "\233" + CLoading::Instance->GetTheaterSuffix();
    char requestID[256] = {};
    requestID[0] = '#';
    strncpy(requestID + 1, requestRealID, 0xFF);
    requestID[sizeof(requestID) - 1] = '\0';

    DWORD bytesWritten = 0;
    WriteFile(hPipeData, requestID, sizeof(requestID), &bytesWritten, NULL);
    return ReadImageData(hPipeData, outImageData);
}

void CLoadingExt::SendRequestText(const char* text)
{
    DWORD bytesWritten = 0;
    WriteFile(hPipeData, text, strlen(text) + 1, &bytesWritten, NULL);
}