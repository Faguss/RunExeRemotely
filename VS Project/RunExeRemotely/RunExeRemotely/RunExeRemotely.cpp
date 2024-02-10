// RunExeRemotely by Faguss (ofp-faguss.com)
// Program for restarting OFP server based on downloaded data

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <windows.h>
#include <tlhelp32.h>
#include <psapi.h>
#include <Urlmon.h>
#include <wininet.h>

#define TURNOFF_SIGNAL    L"?turnoff"
#define SHUTDOWN_SIGNAL   L"?shutdown"
#define DOWNLOAD_FILENAME L"RunExeRemotely.txt"
#define CONFIG_FILENAME   L"RunExeRemotely.cfg"

std::wstring utf16(std::string &input);
std::wstring FormatError(DWORD error);
DWORD Download(std::wstring url, std::wstring filename, bool overwrite = true);
std::wstring Read(std::wstring filename);
std::wstring DownloadAndRead(std::wstring url, std::wstring filename);
DWORD GetProcessID(std::wstring exename);
std::vector<HWND> GetWindowHandlesFromProcessID(DWORD wanted_pid);
std::vector<std::wstring> Tokenize(std::wstring text, std::wstring delimiter);
DWORD LaunchExe(std::wstring &exe_name, std::wstring &exe_arg);
std::wstring UInt2StrW(size_t num);
BOOL AbortShutdown();

int wmain(int argc, wchar_t** argv)
{
	DWORD sleep_time      = 5;
	std::wstring exe_name = L"coldwarassault_server.exe";
	std::wstring url      = L"";

	// Merge local config file and command line arguments into a single list
	std::vector<std::wstring> input_config = Tokenize(Read(L"RunExeRemotely.cfg"), L"\r\n;");

	for (int i=1; i<argc; i++) 
		input_config.push_back((std::wstring)argv[i]);

	// Parse configuration
	for (size_t i=0; i<input_config.size(); i++) {
		std::wstring namevalue = (std::wstring)input_config[i];
		size_t separator       = namevalue.find_first_of('=');

		if (separator != std::string::npos) {
			std::wstring name  = namevalue.substr(0, separator);
			std::wstring value = namevalue.substr(separator + 1);

			if (name.substr(0,1) == L"-")
				name = name.substr(1);

			std::wcout << name << L"=" << value << std::endl;

			if (name == L"sleep") {
				DWORD new_sleep_time = wcstoul(value.c_str(), NULL, 0);
				if (new_sleep_time > 0)
					sleep_time = new_sleep_time;
				continue;
			}

			if (name == L"exe") {
				exe_name = value;
				continue;
			}

			if (name == L"url") {
				url = value;
				continue;
			}
		}
	}

	if (url.empty()) {
		std::cout << "Missing -url= argument!" << std::endl;
		return 1;
	}

	std::wstring last_mode = L"";
	std::wstring new_mode  = L"";
	DWORD pid              = GetProcessID(exe_name);
	BOOL shutdown_enabled  = FALSE;

	// If the server is already running then assume it's with the arguments from the website
	// If the server is not running the launch it immediately so that the user doesn't have to wait sleep time
	{
		std::wstring current_mode = DownloadAndRead(url, DOWNLOAD_FILENAME);

		if (current_mode != SHUTDOWN_SIGNAL && current_mode != TURNOFF_SIGNAL && (pid || LaunchExe(exe_name, current_mode) == 0))
			last_mode = current_mode;
	}

	while (true) {
		Sleep(sleep_time * 1000);

		if (url.empty())
			continue;

		new_mode = DownloadAndRead(url, DOWNLOAD_FILENAME);

		if (new_mode.empty() || exe_name.empty())
			continue;

		pid = GetProcessID(exe_name);

		// Take action if selected option changed
		// or if server isn't running but should be
		// or if server is running but shouldn't be
		if (
			last_mode != new_mode ||
			(!pid && new_mode != TURNOFF_SIGNAL) ||
			( pid && new_mode == TURNOFF_SIGNAL)
		) {
			if (pid) {
				std::vector<HWND> windows = GetWindowHandlesFromProcessID(pid);

				if (windows.size() > 0) {
					std::cout << "Telling PID " << pid << " to close" << std::endl;

					for (size_t i=0; i<windows.size(); i++) 
						PostMessage(windows[i], WM_CLOSE, 0, 0);
				}
			} else {
				if (new_mode == SHUTDOWN_SIGNAL) {
					if (!shutdown_enabled) {
						HANDLE hToken        = NULL; 
						TOKEN_PRIVILEGES tkp = {0};

						if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
							if (LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid)) {
								tkp.PrivilegeCount = 1;
								tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

								if (AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0)) {
									::CloseHandle(hToken);

									if (ERROR_SUCCESS == GetLastError()) {
										wchar_t message[] = L"RunExeRemotely told the system to shut down in 30 seconds. You can abort it from the website";
										BOOL result = InitiateSystemShutdownEx(NULL, message, 30, FALSE, FALSE, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED);

										if (result || GetLastError() == ERROR_SHUTDOWN_IS_SCHEDULED) {
											shutdown_enabled = TRUE;
											last_mode        = new_mode;
											std::wcout << message << std::endl;
											return 0;
										} else {
											std::wcout << L"Failed to shutdown" << FormatError(GetLastError()) << std::endl;	
										}
									}
								}
							}
						}
					}
				} else {
					if (shutdown_enabled && AbortShutdown()) {
						shutdown_enabled = FALSE;
						std::wcout << L"Shutdown aborted from the website" << std::endl;
					}

					if (new_mode != TURNOFF_SIGNAL && LaunchExe(exe_name, new_mode) == 0)
						last_mode = new_mode;
				}
			}

		}
	}

	return 0;
}

std::wstring utf16(std::string &input)
{
	if (input.empty())
		return std::wstring();

	std::string *ptr_input = &input;
	std::string crop       = "";

	if (input.size() > INT_MAX) {
		crop      = input.substr(0, INT_MAX);
		ptr_input = &crop;
	}

	int input_size = static_cast<int>((*ptr_input).size());
	int output_size = MultiByteToWideChar(CP_UTF8, 0, (*ptr_input).c_str(), input_size, NULL, 0);

	std::wstring output(output_size, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, (*ptr_input).c_str(), input_size, &output[0], output_size);

	return output;
}

std::wstring FormatError(DWORD error)
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

	std::wstring output = L" - #" + UInt2StrW(error) + L" " + (std::wstring)lpMsgBuf + L"\n";

	if (lpMsgBuf != NULL)
		LocalFree(lpMsgBuf);

	return output;
};

DWORD Download(std::wstring url, std::wstring filename, bool overwrite)
{
	if (overwrite) {
		BOOL result = DeleteFile(filename.c_str());
		DWORD error = GetLastError();

		if (!result && error != ERROR_FILE_NOT_FOUND)
			std::wcout << L"Failed to delete " << filename << FormatError(error) << std::endl;
	}

	DeleteUrlCacheEntry(url.c_str());

	HRESULT result = URLDownloadToFileW(NULL, url.c_str(), filename.c_str(), 0, NULL);

	if (result != S_OK)
		std::wcout << L"Failed to download " << FormatError(result) << std::endl;

	return result;
}

std::wstring Read(std::wstring filename)
{
	std::ifstream file(filename.c_str());
	std::string buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	if (file.is_open()) {
		file.close();
	} else {
		wchar_t error[256] = L"";
		_wcserror_s(error, 256, errno);
		std::wcout << L"Failed to open " << filename << L" " << error << std::endl;
	}

	return utf16(buffer);
}

std::wstring DownloadAndRead(std::wstring url, std::wstring filename)
{
	if (Download(url, filename) == 0)
		return Read(filename);

	return L"";
}

DWORD GetProcessID(std::wstring exename)
{
	wchar_t current_path[MAX_PATH];
	GetCurrentDirectory(MAX_PATH,current_path);

	PROCESSENTRY32 processInfo = {0};
	DWORD pid                  = 0;
	processInfo.dwSize         = sizeof(processInfo);
	HANDLE processesSnapshot   = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);

	if (processesSnapshot != INVALID_HANDLE_VALUE) {
		Process32First(processesSnapshot, &processInfo);

		do {
			if (_wcsicmp(processInfo.szExeFile, exename.c_str()) == 0) {
				HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, processInfo.th32ProcessID);
				if (hProcess) {
					wchar_t exe_path[MAX_PATH];
					if (GetModuleFileNameExW(hProcess, NULL, exe_path, MAX_PATH) && _wcsnicmp(current_path, exe_path, wcslen(exe_path)-exename.size()-1) == 0)
						pid = processInfo.th32ProcessID;

					CloseHandle(hProcess);
				}
			}
		} while (Process32Next(processesSnapshot, &processInfo));

		CloseHandle(processesSnapshot);
	}

	return pid;
};

std::vector<HWND> GetWindowHandlesFromProcessID(DWORD wanted_pid)
{
	std::vector<HWND> container;
	HWND current_hwnd = NULL;

	do {
		current_hwnd      = FindWindowEx(NULL, current_hwnd, NULL, NULL);
		DWORD current_pid = 0;

		GetWindowThreadProcessId(current_hwnd, &current_pid);

		if (current_pid == wanted_pid)
			container.push_back(current_hwnd);
	} while (current_hwnd != NULL);

	return container;
}

std::vector<std::wstring> Tokenize(std::wstring text, std::wstring delimiter)
{
	std::vector<std::wstring> container;
	bool inQuote      = false;
	size_t word_start = 0;
	bool word_started = false;
	
	// Split line into parts
	for (size_t pos=0;  pos<=text.length();  pos++) {
		bool isToken = pos == text.length();
		
		for (size_t i=0;  !isToken && i<delimiter.length();  i++)
			if (text.substr(pos,1) == delimiter.substr(i,1))
				isToken = true;
				
		if (text.substr(pos,1) == L"\"")
			inQuote = !inQuote;
			
		// Mark beginning of the word
		if (!isToken  &&  !word_started) {
			word_start   = pos;
			word_started = true;
		}

		// Mark end of the word
		if (isToken  &&  word_started  &&  !inQuote) {
			container.push_back(text.substr(word_start, pos-word_start));
			word_started = false;
		}
	}

	return container;
}

DWORD LaunchExe(std::wstring &exe_name, std::wstring &exe_arg) 
{
	std::wcout << "Starting " << exe_name << " " << exe_arg << std::endl;

	wchar_t pwd[MAX_PATH];
	GetCurrentDirectory(MAX_PATH,pwd);
	std::wstring full_path = (std::wstring)pwd + L"\\" + exe_name;

	SHELLEXECUTEINFO EI;
	memset( &EI, 0, sizeof(SHELLEXECUTEINFO));
	EI.cbSize       = sizeof(SHELLEXECUTEINFO);
	EI.fMask        = SEE_MASK_NOCLOSEPROCESS;
	EI.lpVerb       = L"open";
	EI.lpFile       = full_path.c_str();
	EI.lpParameters = exe_arg.c_str();
	EI.lpDirectory  = pwd;
	EI.nShow        = SW_SHOW;

	if (ShellExecuteEx(&EI)) {
		std::wcout << L"PID: " << GetProcessId(EI.hProcess) << std::endl;
		CloseHandle(EI.hProcess);
		return 0;
	} else {
		DWORD error = GetLastError();
		std::wcout << L"Failed to launch" << exe_name << FormatError(error) << std::endl;
		return error;
	}
}

std::wstring UInt2StrW(size_t num)
{
	const int buffer_size       = 16;
	wchar_t buffer[buffer_size] = L"";
	swprintf(buffer, buffer_size, L"%u", num);
	return (std::wstring)buffer;
}

BOOL AbortShutdown()
{
	BOOL abort  = AbortSystemShutdown(NULL);
	DWORD error = GetLastError();

	if (abort || error == ERROR_NO_SHUTDOWN_IN_PROGRESS)
		return TRUE;
	else {
		std::wcout << L"Failed to abort shutdown" << FormatError(error) << std::endl;
		return FALSE;
	}
}
