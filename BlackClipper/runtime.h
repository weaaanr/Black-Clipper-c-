#pragma once
#include <Windows.h>

namespace runtime {
    class RuntimeObfuscation {
    private:
        typedef HANDLE (WINAPI *pCreateFileW)(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
        typedef BOOL (WINAPI *pWriteFile)(HANDLE, LPCVOID, DWORD, LPDWORD, LPOVERLAPPED);
        
        pCreateFileW _CreateFileW;
        pWriteFile _WriteFile;

    public:
        RuntimeObfuscation() {
            HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");
            _CreateFileW = (pCreateFileW)GetProcAddress(hKernel32, "CreateFileW");
            _WriteFile = (pWriteFile)GetProcAddress(hKernel32, "WriteFile");
        }

        HANDLE CreateFileWrapper(LPCWSTR path) {
            return _CreateFileW(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
        }
    };

    template<typename Func>
    void obfuscate_flow(Func f) {
        volatile int x = __rdtsc() & 0xFF;
        switch(x % 4) {
            case 0: x += 1; f(); break;
            case 1: f(); x -= 1; break;
            case 2: if(x) f(); break;
            case 3: while(x-- > 0) if(x == 1) f();
        }
    }
} 