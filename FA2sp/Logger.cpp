#include "Logger.h"
#include <windows.h>
#include <share.h>
#include <codecvt>
#include <locale>
#include <iostream>

char Logger::pTime[24];
char Logger::pBuffer[0x800];
FILE* Logger::pFile;
bool Logger::bInitialized;

std::string Logger::FormatVImpl(const char* format, const std::vector<std::variant<int, double, const char*, const wchar_t*>>& args) {
    constexpr int FORCE_ANSI = 0x10000;
    constexpr int FORCE_UNICODE = 0x20000;

    std::string format_str = format;

    size_t argIndex = 0;
    std::string result;
    result.reserve(format_str.length() + 32);

    for (const char* p = format_str.c_str(); *p; ++p) {
        if (*p != '%') {
            result += *p;
            continue;
        }

        ++p;
        if (*p == '%') {
            result += '%';
            continue;
        }

        std::string formatSpec = "%";
        int nWidth = 0;
        int nPrecision = 0;
        int nModifier = 0;

        while (*p == '#' || *p == '*' || *p == '-' || *p == '+' || *p == '0' || *p == ' ') {
            formatSpec += *p;
            if (*p == '*') {
                if (argIndex >= args.size()) throw std::runtime_error("Too few arguments for width");
                nWidth = std::get<int>(args[argIndex++]);
            }
            ++p;
        }

        if (isdigit(*p)) {
            formatSpec += std::string(p, strspn(p, "0123456789"));
            nWidth = atoi(p);
            while (isdigit(*p)) ++p;
        }

        if (*p == '.') {
            formatSpec += '.';
            ++p;
            if (*p == '*') {
                formatSpec += '*';
                if (argIndex >= args.size()) throw std::runtime_error("Too few arguments for precision");
                nPrecision = std::get<int>(args[argIndex++]);
                ++p;
            }
            else if (isdigit(*p)) {
                formatSpec += std::string(p, strspn(p, "0123456789"));
                nPrecision = atoi(p);
                while (isdigit(*p)) ++p;
            }
        }

        switch (*p) {
        case 'h':
            nModifier = FORCE_ANSI;
            formatSpec += 'h';
            ++p;
            break;
        case 'l':
            nModifier = FORCE_UNICODE;
            formatSpec += 'l';
            ++p;
            break;
        case 'F':
        case 'N':
        case 'L':
            formatSpec += *p;
            ++p;
            break;
        }

        if (argIndex >= args.size()) throw std::runtime_error("Too few arguments for format specifier");
        formatSpec += *p;

        switch (*p | nModifier) {
        case 'c':
        case 'C':
        case 'c' | FORCE_ANSI:
        case 'C' | FORCE_ANSI:
        {
            int value = std::get<int>(args[argIndex++]);
            char buf[16];
            snprintf(buf, sizeof(buf), formatSpec.c_str(), value);
            result += buf;
            break;
        }
        case 'c' | FORCE_UNICODE:
        case 'C' | FORCE_UNICODE:
        {
            wchar_t value = static_cast<wchar_t>(std::get<int>(args[argIndex++]));
            char buf[16];
            snprintf(buf, sizeof(buf), "%lc", value);
            result += buf;
            break;
        }
        case 's':
        case 's' | FORCE_ANSI:
        case 'S':
        case 'S' | FORCE_ANSI:
        {
            const char* str = std::get<const char*>(args[argIndex++]);
            char buf[1024];
            snprintf(buf, sizeof(buf), formatSpec.c_str(), str ? str : "(null)");
            result += buf;
            break;
        }
        case 's' | FORCE_UNICODE:
        case 'S' | FORCE_UNICODE:
        {
            const wchar_t* str = std::get<const wchar_t*>(args[argIndex++]);
            char buf[1024];
            if (str) {
                std::wstring ws(str);
                std::string s(ws.begin(), ws.end());
                snprintf(buf, sizeof(buf), formatSpec.c_str(), s.c_str());
            }
            else {
                snprintf(buf, sizeof(buf), formatSpec.c_str(), "(null)");
            }
            result += buf;
            break;
        }
        case 'd':
        case 'i':
        case 'u':
        case 'x':
        case 'X':
        case 'o':
        case 'p':
        case 'n':
        {
            int value = std::get<int>(args[argIndex++]);
            char buf[64];
            snprintf(buf, sizeof(buf), formatSpec.c_str(), value);
            result += buf;
            break;
        }
        case 'e':
        case 'f':
        case 'g':
        case 'G':
        {
            double value = std::get<double>(args[argIndex++]);
            char buf[128];
            snprintf(buf, sizeof(buf), formatSpec.c_str(), value);
            result += buf;
            break;
        }
        default:
            throw std::invalid_argument("Unsupported format specifier: " + std::string(formatSpec.c_str()));
        }
    }

    if (argIndex < args.size()) {
        throw std::runtime_error("Too many arguments provided");
    }

    return result;
}

void Logger::Initialize() {
	pFile = _fsopen("FA2sp.log", "w", _SH_DENYWR);
	bInitialized = pFile;
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