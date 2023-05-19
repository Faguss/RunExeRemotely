// RunExeRemotely by Faguss (ofp-faguss.com)
// Program for restarting OFP server based on downloaded data

#include <iostream>
#include <fstream>
#include <string>
#include <windows.h>
#include <tlhelp32.h>

using namespace std;

wstring string2wide(const string& input)
{
	if (input.empty())
		return wstring();

	size_t output_length = MultiByteToWideChar(CP_UTF8, 0, input.c_str(), (int)input.length(), 0, 0);
	wstring output(output_length, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, input.c_str(), (int)input.length(), &output[0], (int)input.length());

	return output;
}

string wide2string(const wchar_t* input, int input_length)
{
	int output_length = WideCharToMultiByte(CP_UTF8, 0, input, input_length, NULL, 0, NULL, NULL);

	if (output_length == 0)
		return "";

	string output(output_length, ' ');
	WideCharToMultiByte(CP_UTF8, 0, input, input_length, const_cast<char*>(output.c_str()), output_length, NULL, NULL);

	return output;
}

string wide2string(const wstring& input)
{
	return wide2string(input.c_str(), (int)input.size());
}

wstring FormatError(int error)
{
	if (error == 0)
		return L"\n";

	LPTSTR lpMsgBuf = 0;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
		NULL,
		error,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);

	return L"   - " + (wstring)lpMsgBuf + L"\n";
};

int Download(wstring url, wstring filename, bool overwrite = true)
{
	if (overwrite)
		DeleteFile(filename.c_str());

	wstring arguments = L" --tries=3 --no-check-certificate --no-clobber --remote-encoding=utf-8 --output-document=" + filename + L" " + url;

	// Execute program
	PROCESS_INFORMATION pi;
	STARTUPINFO si;
	ZeroMemory(&si, sizeof(si));
	ZeroMemory(&pi, sizeof(pi));
	si.cb          = sizeof(si);
	si.dwFlags     = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
	si.wShowWindow = SW_SHOW;
	si.hStdOutput  = NULL;
	si.hStdInput   = NULL;
	si.hStdOutput  = NULL;
	si.hStdError   = GetStdHandle(STD_ERROR_HANDLE);

	if (CreateProcess(L"wget.exe", &arguments[0], NULL, NULL, false, 0, NULL, NULL, &si, &pi)) {
		DWORD exit_code;
		Sleep(10);

		do {
			GetExitCodeProcess(pi.hProcess, &exit_code);
			Sleep(100);
		} while (exit_code == STILL_ACTIVE);

		CloseHandle(pi.hProcess);
		CloseHandle(pi.hThread);
	}
	else {
		int errorCode = GetLastError();
		wcout << FormatError(errorCode);
		return errorCode;
	}

	Sleep(100);
	return 0;
}

wstring Read(wstring filename)
{
	string filenameA = wide2string(filename);
	string buffer    = "";
	fstream file;
	file.open(filenameA.c_str(), ios::in);

	if (file.is_open()) {
		char c;
		while (file.get(c))
			buffer += c;

		file.close();
	}
	else {
		TCHAR buffer[1024] = L"";
		_wcserror_s(buffer, 1024, errno);
		wcout << L"Failed to open " << filename << " " << buffer << endl;
	}

	return string2wide(buffer);
}

wstring DownloadAndRead(wstring url, wstring filename)
{
	if (Download(url, filename) == 0)
		return Read(filename);

	return L"";
}

DWORD GetProcessID(wstring exename)
{
	PROCESSENTRY32 processInfo;
	DWORD pid                = 0;
	processInfo.dwSize       = sizeof(processInfo);
	HANDLE processesSnapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (processesSnapshot != INVALID_HANDLE_VALUE) {
		Process32First(processesSnapshot, &processInfo);

		do {
			if (_wcsicmp(processInfo.szExeFile, exename.c_str()) == 0)
				pid = processInfo.th32ProcessID;
		} while (Process32Next(processesSnapshot, &processInfo));

		CloseHandle(processesSnapshot);
	}

	return pid;
};

HWND GetFirstWindowFromProcessID(DWORD wanted_pid)
{
	HWND current_hwnd = NULL;

	do {
		current_hwnd      = FindWindowEx(NULL, current_hwnd, NULL, NULL);
		DWORD current_pid = 0;

		GetWindowThreadProcessId(current_hwnd, &current_pid);

		if (current_pid == wanted_pid)
			return current_hwnd;
	} while (current_hwnd != NULL);

	return NULL;
}

int wmain(int argc, wchar_t** argv)
{
	DWORD sleep_time = 5000;
	wstring exe_name = L"coldwarassault_server.exe";
	wstring url      = L"https://ofp-faguss.com/ofpserverremote.txt";

	for (int i=1; i<argc; i++) {
		wstring namevalue = (wstring)argv[i];
		size_t separator  = namevalue.find_first_of('=');

		if (separator != string::npos) {
			wstring name  = namevalue.substr(0, separator);
			wstring value = namevalue.substr(separator + 1);

			if (name == L"-sleep") {
				sleep_time = wcstoul(value.c_str(), NULL, 0) * 1000;
				continue;
			}

			if (name == L"-exe") {
				exe_name = value;
				continue;
			}

			if (name == L"-url") {
				url = value;
				continue;
			}
		}
	}

	wstring turnoff_signal    = L"?turnoff";
	wstring download_filename = L"RunExeRemotely.txt";
	wstring last_mode         = L"";
	DWORD pid                 = GetProcessID(exe_name);
	bool show_message         = true;

	if (pid)
		last_mode = DownloadAndRead(url, download_filename);	// if server is already running then assume it's the correct one and don't restart
	else
		last_mode = L"";

	while (true) {
		Sleep(sleep_time);
		wstring new_mode = DownloadAndRead(url, download_filename);
		pid              = GetProcessID(exe_name);

		//data was downloaded AND (selected option has changed OR process is not running and option other than turnoff was selected OR process is running and turnoff option was selected)

		if (new_mode!=L"" && (last_mode != new_mode || (!pid && new_mode != turnoff_signal) || (new_mode == turnoff_signal && pid))) {
			if (last_mode != new_mode && show_message) {
				wcout << L"Selected new option: " << new_mode << endl;
				show_message = false;
			}

			if (pid) {
				HWND window = GetFirstWindowFromProcessID(pid);

				if (window) {
					PostMessage(window, WM_CLOSE, 0, 0);
					cout << "Tell pid " << pid << " to close" << endl;
				}
			} else {
				if (new_mode != turnoff_signal) {
					wcout << "Start " << exe_name << " " << new_mode << endl;

					PROCESS_INFORMATION pi;
					STARTUPINFO si;
					ZeroMemory(&si, sizeof(si));
					ZeroMemory(&pi, sizeof(pi));
					si.cb = sizeof(si);
					si.dwFlags = STARTF_USESHOWWINDOW | STARTF_USESTDHANDLES;
					si.wShowWindow = SW_SHOW;

					if (CreateProcess(exe_name.c_str(), &new_mode[0], NULL, NULL, true, 0, NULL, NULL, &si, &pi)) {
						CloseHandle(pi.hProcess);
						CloseHandle(pi.hThread);

						show_message = true;
						last_mode    = new_mode;
					}
					else {
						DWORD errorCode = GetLastError();
						wcout << FormatError(errorCode);
					}
				}
				else {
					show_message = true;
					last_mode    = new_mode;
				}
			}

		}
	}

	return 0;
}
