#include <windows.h>
#include <tlhelp32.h>
#include <wchar.h>
#include "ScanTheRunning.h"

//检查后台正在运行的进程

int ScanTheRunning(const wchar_t* programName) {
    HANDLE hSnapshot;
    PROCESSENTRY32W pe32;
    int found = 1;

    hSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hSnapshot == INVALID_HANDLE_VALUE)
        return 1;

    pe32.dwSize = sizeof(PROCESSENTRY32W);

    if (Process32FirstW(hSnapshot, &pe32)) {
        do {
            if (_wcsicmp(pe32.szExeFile, programName) == 0) {
                found = 0;
                break;
            }
        } while (Process32NextW(hSnapshot, &pe32));
    }

    CloseHandle(hSnapshot);
    return found;
}