#pragma once

#include "pch.h"

namespace ReLCS
{
    class CrashLogger
    {
    public:
        static void Init();
        static void Uninstall();
        static void LogMessage(const std::string& message);

    private:
        static LONG WINAPI CrashHandler(EXCEPTION_POINTERS* pExceptionPointers);
        static std::string GetModuleName(void* pAddress);
        static std::string GetExceptionDescription(DWORD code);
        static void WriteMiniDump(EXCEPTION_POINTERS* pExceptionPointers, const std::wstring& dumpPath);

        static LPTOP_LEVEL_EXCEPTION_FILTER s_pPrevFilter;
        static std::wstring s_logFilePath;
        static std::wstring s_dumpFilePath;
    };
}
