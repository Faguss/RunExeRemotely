#pragma once
#include <windows.h>
#include <Urlmon.h>
#include <wininet.h>
#include "common.h"
#include "resource.h"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <tlhelp32.h>
#include <psapi.h>

DWORD WINAPI RunExeRemotelyMain(__in LPVOID lpParameter);
std::string utf8(const wchar_t* input, int input_size);
std::string utf8(std::wstring input);
std::wstring utf16(std::string &input);
std::wstring FormatError(DWORD error);
DWORD Download(std::wstring url, std::wstring filename, bool overwrite, HWND log_handle, std::wstring &log_buffer);
std::wstring Read(std::wstring filename, HWND log_handle, std::wstring &log_buffer);
std::wstring DownloadAndRead(std::wstring url, std::wstring filename, HWND log_handle, std::wstring &log_buffer);
DWORD GetProcessID(std::wstring exename);
std::vector<HWND> GetWindowHandlesFromProcessID(DWORD wanted_pid);
std::vector<std::wstring> Tokenize(std::wstring text, std::wstring delimiter);
DWORD LaunchExe(std::wstring &exe_name, std::wstring &exe_arg, HWND log_handle, std::wstring &log_buffer);
std::wstring Int2StrW(int num, bool leading_zero=false);
std::wstring UInt2StrW(size_t num);
void WindowTextToString(HWND control, std::wstring &str);
void UpdateLog(HWND control, std::wstring &buffer, std::wstring new_text);
BOOL AbortShutdown(HWND control, std::wstring &buffer);
