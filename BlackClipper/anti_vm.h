#pragma once
#include <Windows.h>
#include <iostream>
#include <vector>
#include <string>
#include <TlHelp32.h>
#include <iphlpapi.h>
#include <intrin.h>
#pragma comment(lib, "iphlpapi.lib")

namespace anti_vm {

    // Простая функция XOR-обфускации строки
    inline std::string xor_str(const std::string& input, char key) {
        std::string output = input;
        for (auto& c : output) {
            c ^= key;
        }
        return output;
    }

    // Расширенный список процессов, характерных для песочниц и VM
    bool check_processes() {
        // Список процессов (обфусцированный XOR, ключ 0x55)
        std::vector<std::string> vm_processes_xored = {
            xor_str("vboxservice.exe", 0x55),
            xor_str("vboxtray.exe", 0x55),
            xor_str("vmtoolsd.exe", 0x55),
            xor_str("vmwaretray.exe", 0x55),
            xor_str("vmusrvc.exe", 0x55),
            xor_str("vmsrvc.exe", 0x55),
            xor_str("sandboxierpcss.exe", 0x55),
            xor_str("procmon.exe", 0x55),
            xor_str("wireshark.exe", 0x55),
            xor_str("fiddler.exe", 0x55),
            xor_str("filemon.exe", 0x55),
            xor_str("regmon.exe", 0x55),
            xor_str("cuckoomon.exe", 0x55),
            xor_str("xenservice.exe", 0x55),
            xor_str("qemu-ga.exe", 0x55)
        };

        std::vector<std::string> vm_processes;
        for (auto& s : vm_processes_xored) {
            vm_processes.push_back(xor_str(s, 0x55));
        }

        HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
        if (snapshot == INVALID_HANDLE_VALUE) return false;

        PROCESSENTRY32W processEntry = { sizeof(PROCESSENTRY32W) };
        if (Process32FirstW(snapshot, &processEntry)) {
            do {
                char processName[MAX_PATH];
                wcstombs(processName, processEntry.szExeFile, MAX_PATH);
                _strlwr_s(processName, MAX_PATH);

                for (const auto& vm_process : vm_processes) {
                    if (strstr(processName, vm_process.c_str())) {
                        CloseHandle(snapshot);
                        return true; // Найден процесс VM/песочницы
                    }
                }
            } while (Process32NextW(snapshot, &processEntry));
        }
        CloseHandle(snapshot);
        return false;
    }

    // Проверка характерных устройств
    bool check_devices() {
        std::vector<std::string> vm_devices = {
            "\\\\.\\VBoxMiniRdrDN",
            "\\\\.\\VBoxGuest",
            "\\\\.\\vmci",
            "\\\\.\\HGFS",
            "\\\\.\\pipe\\VBoxTrayIPC",
            "\\\\.\\pipe\\cuckoo", // для Cuckoo Sandbox
            "\\\\.\\pipe\\vmwaredebug"
        };

        for (const auto& device : vm_devices) {
            HANDLE h = CreateFileA(device.c_str(),
                GENERIC_READ,
                FILE_SHARE_READ | FILE_SHARE_WRITE,
                nullptr,
                OPEN_EXISTING,
                FILE_ATTRIBUTE_NORMAL,
                nullptr);
            if (h != INVALID_HANDLE_VALUE) {
                CloseHandle(h);
                return true; 
            }
        }
        return false;
    }

    // Проверка размера RAM (VM обычно имеют меньше памяти)
    bool check_memory_size() {
        MEMORYSTATUSEX memoryStatus = { sizeof(MEMORYSTATUSEX) };
        GlobalMemoryStatusEx(&memoryStatus);
        DWORDLONG totalPhysMem = memoryStatus.ullTotalPhys;
        return (totalPhysMem < 2ULL * 1024ULL * 1024ULL * 1024ULL);
    }

    // Проверка количества процессоров
    bool check_cpu_cores() {
        SYSTEM_INFO sysInfo;
        GetSystemInfo(&sysInfo);
        return (sysInfo.dwNumberOfProcessors < 2);
    }

    // Проверка VM DLL
    bool check_vm_dlls() {
        std::vector<std::string> vm_dlls = {
            "vboxmrxnp.dll",    // Только самые специфичные DLL
            "vmhgfs.dll",       // от VirtualBox и VMware
            "vboxhook.dll"      // которых нет на обычных ПК
        };
        
        char systemDir[MAX_PATH];
        GetSystemDirectoryA(systemDir, MAX_PATH);
    
        for (const auto& dll : vm_dlls) {
            std::string dllPath = std::string(systemDir) + "\\" + dll;
            if (GetFileAttributesA(dllPath.c_str()) != INVALID_FILE_ATTRIBUTES) {
                return true;
            }
        }
        return false;
    }

    // Проверка через RDTSC задержку
    bool check_emulation() {
        ULONGLONG t1 = __rdtsc();
        Sleep(500);
        ULONGLONG t2 = __rdtsc();
        // Если выполняется слишком быстро, возможно эмуляция
        return (t2 - t1) < 1000000;
    }

    // Проверка гипервизора по CPUID
    bool check_cpuid_hv() {
        int cpuInfo[4] = { 0 };
        __cpuid(cpuInfo, 1);
        if ((cpuInfo[2] & (1 << 31)) != 0) {
            __cpuid(cpuInfo, 0x40000000);
            char vendor[13] = { 0 };
            memcpy(vendor + 0, &cpuInfo[1], 4);
            memcpy(vendor + 4, &cpuInfo[2], 4);
            memcpy(vendor + 8, &cpuInfo[3], 4);
            
            if (strstr(vendor, "VMwareVMware") ||
                strstr(vendor, "KVMKVMKVM") ||
                strstr(vendor, "VBoxVBoxVBox")) {
                return true;
            }
            return false;
        }
        return false;
    }

    // Проверка реестра на ключи VMware/VirtualBox
    bool check_registry_keys() {
        struct RegCheck {
            HKEY root;
            const char* subkey;
        } checks[] = {
            { HKEY_LOCAL_MACHINE, "SOFTWARE\\Oracle\\VirtualBox Guest Additions" },
            { HKEY_LOCAL_MACHINE, "SOFTWARE\\VMware, Inc.\\VMware Tools" },
            { HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\VBoxGuest" },
            { HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Services\\vmhgfs" }
        };

        for (auto& c : checks) {
            HKEY hKey;
            if (RegOpenKeyExA(c.root, c.subkey, 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
                RegCloseKey(hKey);
                return true;
            }
        }
        return false;
    }

    // Проверка BIOS/DMI строк (через реестр)
    bool check_bios_dmi() {
        // Проверим значение реестра SystemBiosVersion
        HKEY hKey;
        if (RegOpenKeyExA(HKEY_LOCAL_MACHINE, "HARDWARE\\DESCRIPTION\\System", 0, KEY_READ, &hKey) == ERROR_SUCCESS) {
            char buffer[256];
            DWORD size = sizeof(buffer);
            if (RegQueryValueExA(hKey, "SystemBiosVersion", NULL, NULL, (LPBYTE)buffer, &size) == ERROR_SUCCESS) {
                std::string biosVersion = buffer;
                _strlwr_s((char*)biosVersion.data(), biosVersion.size() + 1);
                // Проверяем на ключевые слова
                if (biosVersion.find("vbox") != std::string::npos ||
                    biosVersion.find("vmware") != std::string::npos ||
                    biosVersion.find("virtual") != std::string::npos ||
                    biosVersion.find("qemu") != std::string::npos ||
                    biosVersion.find("xen") != std::string::npos) {
                    RegCloseKey(hKey);
                    return true;
                }
            }
            RegCloseKey(hKey);
        }
        return false;
    }

    // Проверка MAC-адресов
    // Характерные префиксы: VMware: 00:05:69 или 00:0C:29, VirtualBox: 08:00:27
    bool check_mac_address() {
        PIP_ADAPTER_INFO AdapterInfo;
        DWORD dwBufLen = sizeof(IP_ADAPTER_INFO);
        AdapterInfo = (IP_ADAPTER_INFO *)malloc(sizeof(IP_ADAPTER_INFO));

        if (AdapterInfo == NULL) return false;

        if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == ERROR_BUFFER_OVERFLOW) {
            free(AdapterInfo);
            AdapterInfo = (IP_ADAPTER_INFO *)malloc(dwBufLen);
        }

        if (AdapterInfo == NULL) return false;

        if (GetAdaptersInfo(AdapterInfo, &dwBufLen) == NO_ERROR) {
            PIP_ADAPTER_INFO pAdapter = AdapterInfo;
            while (pAdapter) {
                if (pAdapter->AddressLength == 6) {
                    unsigned char mac[6];
                    for (int i = 0; i < 6; i++) mac[i] = pAdapter->Address[i];

                    char macStr[18];
                    sprintf_s(macStr, "%02X:%02X:%02X:%02X:%02X:%02X",
                        mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

                    std::string macString = macStr;
                    // Приведём к нижнему регистру для удобства поиска
                    for (auto& c : macString) c = (char)tolower((unsigned char)c);

                    if (macString.find("00:05:69") != std::string::npos ||
                        macString.find("00:0c:29") != std::string::npos ||
                        macString.find("08:00:27") != std::string::npos) {
                        free(AdapterInfo);
                        return true;
                    }
                }
                pAdapter = pAdapter->Next;
            }
        }

        free(AdapterInfo);
        return false;
    }

    // Проверка "песочницы" - примитивная (уже есть в некоторых функциях)
    // Можно добавить проверку специфичных процессов или замедление исполнения
    bool check_sandbox() {
        // Дополнительная задержка - песочницы могут "таймить" программу.
        Sleep(100 + (rand() % 200)); 
        // Если мало CPU, мало памяти, есть типичные процессы - уже проверено выше.
        // Здесь можно вернуть false, т.к. мы и так делаем много проверок.
        return false;
    }

    bool detect_vm() {
        bool detected = false;
        
        if (check_processes()) {
            detected = true;
        }
        if (check_devices()) {
            detected = true;
        }
        if (check_vm_dlls()) {
            detected = true;
        }
        if (check_cpuid_hv()) {
            detected = true;
        }
        
        if (check_memory_size()) detected = true;
        if (check_cpu_cores()) detected = true;
        if (check_emulation()) detected = true;
        
        return detected;
    }

}

// Для использования в другом месте
inline bool __check_vm() {
    return anti_vm::detect_vm();
}
