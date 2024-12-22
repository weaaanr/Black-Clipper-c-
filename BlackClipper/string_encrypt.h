#pragma once
#include <Windows.h>

namespace runtime {
    DWORD hash_api_name(const char* str) {
        DWORD hash = 0;
        while (*str) {
            hash = (hash << 5) + hash + (*str++);
        }
        return hash;
    }

    template<typename T>
    T get_api_by_hash(DWORD hash) {
        HMODULE hMod = GetModuleHandleA("kernel32.dll");
        PIMAGE_DOS_HEADER dos = (PIMAGE_DOS_HEADER)hMod;
        PIMAGE_NT_HEADERS nt = (PIMAGE_NT_HEADERS)((BYTE*)hMod + dos->e_lfanew);
        PIMAGE_EXPORT_DIRECTORY exports = (PIMAGE_EXPORT_DIRECTORY)((BYTE*)hMod + 
            nt->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);
        
        DWORD* names = (DWORD*)((BYTE*)hMod + exports->AddressOfNames);
        for (DWORD i = 0; i < exports->NumberOfNames; i++) {
            const char* name = (const char*)((BYTE*)hMod + names[i]);
            if (hash_api_name(name) == hash)
                return (T)GetProcAddress(hMod, name);
        }
        return nullptr;
    }

    template<typename T>
    void runtime_encrypt(T* const data, size_t size) {
        // Создаем временный буфер для шифрования
        T* temp = new T[size];
        memcpy(temp, data, size);

        BYTE key = static_cast<BYTE>(GetTickCount64());
        for(size_t i = 0; i < size; i++) {
            temp[i] ^= key;
        }

        // Копируем зашифрованные данные обратно
        memcpy(const_cast<T*>(data), temp, size);
        delete[] temp;
    }

    // Перегрузка для строковых литералов
    template<typename T>
    void runtime_encrypt(const T* data, size_t size) {
        T* mutable_data = const_cast<T*>(data);
        runtime_encrypt(mutable_data, size);
    }
} 