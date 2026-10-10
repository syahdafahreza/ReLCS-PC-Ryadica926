#include "CrashLogger.h"

#pragma comment(lib, "dbghelp.lib")

namespace ReLCS
{
    LPTOP_LEVEL_EXCEPTION_FILTER CrashLogger::s_pPrevFilter = nullptr;
    std::wstring CrashLogger::s_logFilePath = L"scripts\\crash.log";
    std::wstring CrashLogger::s_dumpFilePath = L"scripts\\crash.dmp";

    void CrashLogger::Init()
    {
        // Check if scripts/ directory exists, otherwise fallback to current directory
        DWORD attr = GetFileAttributesW(L"scripts");
        if (attr == INVALID_FILE_ATTRIBUTES || !(attr & FILE_ATTRIBUTE_DIRECTORY))
        {
            s_logFilePath = L"crash.log";
            s_dumpFilePath = L"crash.dmp";
        }
        else
        {
            s_logFilePath = L"scripts\\crash.log";
            s_dumpFilePath = L"scripts\\crash.dmp";
        }

        s_pPrevFilter = SetUnhandledExceptionFilter(CrashHandler);
        LogMessage("[CrashLogger] Initialized successfully. Monitoring for unhandled exceptions.");
    }

    void CrashLogger::Uninstall()
    {
        if (s_pPrevFilter)
        {
            SetUnhandledExceptionFilter(s_pPrevFilter);
            s_pPrevFilter = nullptr;
        }
    }

    void CrashLogger::LogMessage(const std::string& message)
    {
        std::ofstream logFile(s_logFilePath, std::ios::app);
        if (logFile.is_open())
        {
            SYSTEMTIME st;
            GetLocalTime(&st);
            char timeBuf[64];
            snprintf(timeBuf, sizeof(timeBuf), "[%04d-%02d-%02d %02d:%02d:%02d.%03d] ",
                st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
            logFile << timeBuf << message << std::endl;
        }
    }

    std::string CrashLogger::GetModuleName(void* pAddress)
    {
        HMODULE hMod = nullptr;
        if (GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCSTR>(pAddress), &hMod))
        {
            char path[MAX_PATH] = { 0 };
            GetModuleFileNameA(hMod, path, MAX_PATH);
            std::string s(path);
            size_t slash = s.find_last_of("\\/");
            if (slash != std::string::npos)
                return s.substr(slash + 1);
            return s;
        }
        return "UnknownModule";
    }

    std::string CrashLogger::GetExceptionDescription(DWORD code)
    {
        switch (code)
        {
        case EXCEPTION_ACCESS_VIOLATION:         return "EXCEPTION_ACCESS_VIOLATION (c0000005)";
        case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:    return "EXCEPTION_ARRAY_BOUNDS_EXCEEDED";
        case EXCEPTION_BREAKPOINT:                return "EXCEPTION_BREAKPOINT";
        case EXCEPTION_DATATYPE_MISALIGNMENT:     return "EXCEPTION_DATATYPE_MISALIGNMENT";
        case EXCEPTION_FLT_DIVIDE_BY_ZERO:       return "EXCEPTION_FLT_DIVIDE_BY_ZERO";
        case EXCEPTION_ILLEGAL_INSTRUCTION:       return "EXCEPTION_ILLEGAL_INSTRUCTION";
        case EXCEPTION_IN_PAGE_ERROR:             return "EXCEPTION_IN_PAGE_ERROR";
        case EXCEPTION_INT_DIVIDE_BY_ZERO:       return "EXCEPTION_INT_DIVIDE_BY_ZERO";
        case EXCEPTION_PRIV_INSTRUCTION:          return "EXCEPTION_PRIV_INSTRUCTION";
        case EXCEPTION_STACK_OVERFLOW:           return "EXCEPTION_STACK_OVERFLOW";
        default:
            char buf[64];
            snprintf(buf, sizeof(buf), "Unknown (0x%08X)", code);
            return std::string(buf);
        }
    }

    void CrashLogger::WriteMiniDump(EXCEPTION_POINTERS* pExceptionPointers, const std::wstring& dumpPath)
    {
        HANDLE hDumpFile = CreateFileW(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (hDumpFile != INVALID_HANDLE_VALUE)
        {
            MINIDUMP_EXCEPTION_INFORMATION mei;
            mei.ThreadId = GetCurrentThreadId();
            mei.ExceptionPointers = pExceptionPointers;
            mei.ClientPointers = FALSE;

            MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hDumpFile,
                MiniDumpWithIndirectlyReferencedMemory, &mei, nullptr, nullptr);
            CloseHandle(hDumpFile);
        }
    }

    static bool SafeReadDword(const uint32_t* pAddr, uint32_t* pOutVal)
    {
        __try
        {
            *pOutVal = *pAddr;
            return true;
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            return false;
        }
    }

    LONG WINAPI CrashLogger::CrashHandler(EXCEPTION_POINTERS* pExceptionPointers)
    {
        if (!pExceptionPointers || !pExceptionPointers->ExceptionRecord || !pExceptionPointers->ContextRecord)
            return EXCEPTION_CONTINUE_SEARCH;

        EXCEPTION_RECORD* rec = pExceptionPointers->ExceptionRecord;
        CONTEXT* ctx = pExceptionPointers->ContextRecord;

        std::string modName = GetModuleName(rec->ExceptionAddress);
        std::string desc = GetExceptionDescription(rec->ExceptionCode);

        std::stringstream ss;
        ss << "\n=======================================================\n";
        ss << "              GTA Re:LCS CRASH REPORT                  \n";
        ss << "=======================================================\n";
        ss << "Exception: " << desc << "\n";
        ss << "Address:   0x" << std::hex << std::uppercase << reinterpret_cast<uintptr_t>(rec->ExceptionAddress)
           << " in module: " << modName << "\n";

        if (rec->ExceptionCode == EXCEPTION_ACCESS_VIOLATION && rec->NumberParameters >= 2)
        {
            ULONG_PTR accessType = rec->ExceptionInformation[0];
            ULONG_PTR targetAddr = rec->ExceptionInformation[1];
            ss << "Violation: " << (accessType == 0 ? "Read from" : (accessType == 1 ? "Write to" : "Execute at"))
               << " 0x" << std::hex << targetAddr << "\n";

            if (targetAddr == 0 || targetAddr < 0x1000)
            {
                ss << "Reason:    Null pointer dereference.\n";
            }
            else
            {
                ss << "Reason:    Memory collision / unmapped address.\n"
                   << "           (Often caused by standard VC plugins trying to access VC addresses in modified Re:LCS exe)\n";
            }
        }

        ss << "\nRegisters (x86):\n";
        ss << "  EAX: 0x" << std::hex << ctx->Eax << "   EBX: 0x" << ctx->Ebx << "\n";
        ss << "  ECX: 0x" << std::hex << ctx->Ecx << "   EDX: 0x" << ctx->Edx << "\n";
        ss << "  ESI: 0x" << std::hex << ctx->Esi << "   EDI: 0x" << ctx->Edi << "\n";
        ss << "  EBP: 0x" << std::hex << ctx->Ebp << "   ESP: 0x" << ctx->Esp << "\n";
        ss << "  EIP: 0x" << std::hex << ctx->Eip << "   EFLAGS: 0x" << ctx->EFlags << "\n";

        ss << "\nStack dump (top 16 DWORDs):\n";
        uint32_t* pStack = reinterpret_cast<uint32_t*>(ctx->Esp);
        for (int i = 0; i < 16; i++)
        {
            uint32_t stackVal = 0;
            if (SafeReadDword(&pStack[i], &stackVal))
            {
                ss << "  [ESP+" << std::dec << (i * 4) << "] = 0x" << std::hex << stackVal
                   << " (" << GetModuleName(reinterpret_cast<void*>(stackVal)) << ")\n";
            }
            else
            {
                ss << "  [ESP+" << std::dec << (i * 4) << "] = <unreadable>\n";
                break;
            }
        }

        ss << "=======================================================\n";

        std::string report = ss.str();
        LogMessage(report);
        WriteMiniDump(pExceptionPointers, s_dumpFilePath);

        std::string msg = "GTA Re:LCS has encountered a fatal error.\n\n"
                          "Exception: " + desc + "\n"
                          "Address: 0x" + std::to_string(reinterpret_cast<uintptr_t>(rec->ExceptionAddress)) +
                          " (" + modName + ")\n\n"
                          "A detailed log was written to:\n" +
                          std::filesystem::path(s_logFilePath).string() + "\n"
                          "Crash dump was written to:\n" +
                          std::filesystem::path(s_dumpFilePath).string();

        MessageBoxA(nullptr, msg.c_str(), "ReLCS-Cleo Crash Intercepted", MB_ICONERROR | MB_OK);

        if (s_pPrevFilter)
            return s_pPrevFilter(pExceptionPointers);

        return EXCEPTION_CONTINUE_SEARCH;
    }
}
