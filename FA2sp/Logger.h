#pragma once

#include "FA2PP.h"
#include "../FA2sp/Helpers/FString.h"

class Logger {
public:
    enum class kLoggerType { Raw = -1, Debug, Info, Warn, Error };
    static void Initialize();
    static void Close();

    template<typename T>
    static void convertArg(std::vector<std::variant<int, double, const char*, const wchar_t*>>& vec, T&& arg) {
        using U = std::decay_t<T>;
        if constexpr (std::is_same_v<U, FString>) {
            vec.push_back(arg.c_str());
        }
        else if constexpr (std::is_same_v<U, ppmfc::CString>) {
            vec.push_back(arg.m_pchData);
        }
        else if constexpr (std::is_same_v<U, std::string>) {
            vec.push_back(arg.c_str());
        }
        else if constexpr (std::is_same_v<U, const char*>) {
            vec.push_back(arg);
        }
        else if constexpr (std::is_same_v<U, char*>) {
            vec.push_back(static_cast<const char*>(arg));
        }
        else if constexpr (std::is_same_v<U, const wchar_t*>) {
            vec.push_back(arg);
        }
        else if constexpr (std::is_same_v<U, int>) {
            vec.push_back(arg);
        }
        else if constexpr (std::is_same_v<U, size_t>) {
            vec.push_back(static_cast<int>(arg));
        }
        else if constexpr (std::is_same_v<U, unsigned long>) {
            vec.push_back(static_cast<int>(arg));
        }
        else if constexpr (std::is_same_v<U, short>) {
            vec.push_back(static_cast<int>(arg));
        }
        else if constexpr (std::is_same_v<U, unsigned short>) {
            vec.push_back(static_cast<int>(arg));
        }
        else if constexpr (std::is_same_v<U, char>) {
            vec.push_back(static_cast<int>(arg));
        }
        else if constexpr (std::is_same_v<U, unsigned char>) {
            vec.push_back(static_cast<int>(arg));
        }
        else if constexpr (std::is_same_v<U, bool>) {
            vec.push_back(static_cast<int>(arg));
        }
        else if constexpr (std::is_same_v<U, double>) {
            vec.push_back(arg);
        }
        else if constexpr (std::is_same_v<U, float>) {
            vec.push_back(static_cast<double>(arg));
        }
        else if constexpr (std::is_same_v<U, void*>) {
            vec.push_back(reinterpret_cast<intptr_t>(arg));
        }
        else if constexpr (std::is_same_v<U, int*>) {
            vec.push_back(reinterpret_cast<intptr_t>(arg));
        }
        else {
            static_assert(false, "Unsupported argument type for Logger");
        }
    }

    static std::string FormatVImpl(const char* format, const std::vector<std::variant<int, double, const char*, const wchar_t*>>& args);
public:
    template<typename... Args>
    static void Write(kLoggerType type, const char* format, Args&&... args) {
        if (!bInitialized) {
            return;
        }

        std::vector<std::variant<int, double, const char*, const wchar_t*>> convertedArgs;
        (convertArg(convertedArgs, std::forward<Args>(args)), ...);

        std::string message = FormatVImpl(format, convertedArgs);

        std::string type_str;
        switch (type) {
        case kLoggerType::Raw:
            type_str = "";
            break;
        case kLoggerType::Debug:
            type_str = "Debug";
            break;
        case kLoggerType::Info:
            type_str = "Info";
            break;
        case kLoggerType::Warn:
            type_str = "Warn";
            break;
        case kLoggerType::Error:
            type_str = "Error";
            break;
        default:
            type_str = "";
            break;
        }

        if (type == kLoggerType::Raw) {
            fprintf_s(pFile, "%s", message.c_str());
        }
        else {
            fprintf_s(pFile, "[%s] %s", type_str.c_str(), message.c_str());
        }
        fflush(pFile);
    }

    template<typename... Args>
    static void Debug(const char* format, Args&&... args) {
        Write(kLoggerType::Debug, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Warn(const char* format, Args&&... args) {
        Write(kLoggerType::Warn, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Error(const char* format, Args&&... args) {
        Write(kLoggerType::Error, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Info(const char* format, Args&&... args) {
        Write(kLoggerType::Info, format, std::forward<Args>(args)...);
    }

    template<typename... Args>
    static void Raw(const char* format, Args&&... args) {
        Write(kLoggerType::Raw, format, std::forward<Args>(args)...);
    }

    static void Put(const char*);
    static void Time(char*);
    static void Wrap(unsigned int cnt = 1);

private:
    static char pTime[24];
    static char pBuffer[0x800];
    static FILE* pFile;
    static bool bInitialized;
};