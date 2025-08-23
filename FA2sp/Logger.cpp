#include "Logger.h"
#include <windows.h>
#include <share.h>
#include <codecvt>
#include <locale>
#include <iostream>
#include <fcntl.h>
#include <io.h>

char Logger::pTime[24];
char Logger::pBuffer[0x800];
FILE* Logger::pFile;
bool Logger::bInitialized;

void Logger::Initialize() {
	pFile = _fsopen("FA2sp.log", "w", _SH_DENYWR);
	bInitialized = pFile;
	if (bInitialized)
		fprintf(pFile, "\xEF\xBB\xBF");
	Time(pTime);
	Raw("FA2sp Logger Initializing at %s.\n", pTime);
}

void Logger::Close() {
	if (bInitialized)
	{
		Time(pTime);
		Raw("FA2sp Logger Closing at %s.\n", pTime);
		fclose(pFile);
	}
}

void Logger::Put(const char* pBuffer) {
	if (bInitialized) {
		fputs(pBuffer, pFile);
		fflush(pFile);
	}
}

void Logger::Time(char* ret) {
	SYSTEMTIME Sys;
	GetLocalTime(&Sys);
	sprintf_s(ret, 24, "%04d/%02d/%02d %02d:%02d:%02d:%03d",
		Sys.wYear, Sys.wMonth, Sys.wDay, Sys.wHour, Sys.wMinute,
		Sys.wSecond, Sys.wMilliseconds);
}

void Logger::Wrap(unsigned int cnt) {
	if (bInitialized)
	{
		while (cnt--)
			fprintf_s(pFile, "\n");
		fflush(pFile);
	}
}