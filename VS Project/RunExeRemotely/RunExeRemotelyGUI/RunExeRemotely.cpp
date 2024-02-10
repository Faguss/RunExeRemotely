// RunExeRemotely by Faguss (ofp-faguss.com)
// Program for restarting OFP server based on downloaded data

#include "RunExeRemotely.h"

#define TURNOFF_SIGNAL    L"?turnoff"
#define SHUTDOWN_SIGNAL   L"?shutdown"
#define DOWNLOAD_FILENAME L"RunExeRemotely.txt"
#define CONFIG_FILENAME   L"RunExeRemotely.cfg"

DWORD WINAPI RunExeRemotelyMain(__in LPVOID lpParameter)
{
	GlobalVariables *global = (GlobalVariables*)lpParameter;
	DWORD sleep_time        = 5;
	std::wstring exe_name   = L"coldwarassault_server.exe";
	std::wstring url        = L"";
	std::wstring log_buffer = L"";

	// Parse configuration
	{
		std::vector<std::wstring> input_config = Tokenize(Read(CONFIG_FILENAME, global->controls[LOG_GENERAL], log_buffer), L"\r\n;");

		for (size_t i=0; i<input_config.size(); i++) {
			std::wstring namevalue = (std::wstring)input_config[i];
			size_t separator       = namevalue.find_first_of('=');

			if (separator != std::string::npos) {
				std::wstring name  = namevalue.substr(0, separator);
				std::wstring value = namevalue.substr(separator + 1);

				if (name.substr(0,1) == L"-")
					name = name.substr(1);

				UpdateLog(global->controls[LOG_GENERAL], log_buffer, (name+L" = "+value));

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
	}

	SetWindowText(global->controls[INPUT_EXE_NAME], exe_name.c_str());
	SetWindowText(global->controls[INPUT_URL], url.c_str());
	SetWindowText(global->controls[INPUT_SLEEP], UInt2StrW(sleep_time).c_str());

	std::wstring last_mode     = L"";
	std::wstring new_mode      = L"";
	DWORD pid                  = GetProcessID(exe_name);
	BOOL user_stopped_shutdown = FALSE;
	BOOL shutdown_enabled      = FALSE;

	// If the server is already running then assume it's with the arguments from the website
	// If the server is not running the launch it immediately so that the user doesn't have to wait sleep time
	{
		std::wstring current_mode = DownloadAndRead(url, DOWNLOAD_FILENAME, global->controls[LOG_GENERAL], log_buffer);

		if (current_mode != SHUTDOWN_SIGNAL && current_mode != TURNOFF_SIGNAL && (pid || LaunchExe(exe_name, current_mode, global->controls[LOG_GENERAL], log_buffer) == 0))
			last_mode = current_mode;
	}

	while (true) {
		Sleep(sleep_time * 1000);

		if (global->user_input & USER_INPUT_UPDATE_CONFIG) {
			std::wstring new_sleep = L"";
			std::string new_config = "";
			WindowTextToString(global->controls[INPUT_EXE_NAME], exe_name);
			WindowTextToString(global->controls[INPUT_URL], url);
			WindowTextToString(global->controls[INPUT_SLEEP], new_sleep);

			new_config += "exe=" + utf8(exe_name) + "\nurl=" + utf8(url) + "\n";
			UpdateLog(global->controls[LOG_GENERAL], log_buffer, (L"exe = "+exe_name));
			UpdateLog(global->controls[LOG_GENERAL], log_buffer, (L"url = "+url));

			DWORD new_sleep_time = wcstoul(new_sleep.c_str(), NULL, 0);
			if (new_sleep_time > 0)
				sleep_time = new_sleep_time;
			new_config += "sleep=" + utf8(UInt2StrW(sleep_time)) + "\n";
			UpdateLog(global->controls[LOG_GENERAL], log_buffer, (L"sleep = "+UInt2StrW(sleep_time)));

			FILE *f;
			errno_t f_error = _wfopen_s(&f, CONFIG_FILENAME, L"w");
			if (f_error == 0) {
				fprintf(f, "%s", new_config.c_str());
				fclose(f);
			}

			EnableWindow(global->controls[BUTTON_UPDATE], 1);
			global->user_input &= ~USER_INPUT_UPDATE_CONFIG;
		}

		if (global->user_input & USER_INPUT_ABORT_SHUTDOWN && AbortShutdown(global->controls[LOG_GENERAL],log_buffer)) {
			shutdown_enabled      = FALSE;
			user_stopped_shutdown = TRUE;
			global->user_input   &= ~USER_INPUT_ABORT_SHUTDOWN;
			UpdateLog(global->controls[LOG_GENERAL],log_buffer,L"Shutdown aborted manually");
		}

		if (url.empty())
			continue;

		new_mode = DownloadAndRead(url, DOWNLOAD_FILENAME, global->controls[LOG_GENERAL], log_buffer);

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
					UpdateLog(global->controls[LOG_GENERAL],log_buffer,(L"Telling PID " + UInt2StrW(pid) + L" to close"));

					for (size_t i=0; i<windows.size(); i++) 
						PostMessage(windows[i], WM_CLOSE, 0, 0);
				}
			} else {
				if (new_mode == SHUTDOWN_SIGNAL) {
					if (!shutdown_enabled && !user_stopped_shutdown) {
						HANDLE hToken        = NULL; 
						TOKEN_PRIVILEGES tkp = {0};

						if (OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &hToken)) {
							if (LookupPrivilegeValue(NULL, SE_SHUTDOWN_NAME, &tkp.Privileges[0].Luid)) {
								tkp.PrivilegeCount = 1;
								tkp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED; 

								if (AdjustTokenPrivileges(hToken, FALSE, &tkp, 0, NULL, 0)) {
									::CloseHandle(hToken);

									if (ERROR_SUCCESS == GetLastError()) {
										wchar_t message[] = L"RunExeRemotely told the system to shut down in 30 seconds. You can abort it from the program or from the website";
										BOOL result = InitiateSystemShutdownEx(NULL, message, 31, FALSE, FALSE, SHTDN_REASON_MAJOR_OTHER | SHTDN_REASON_MINOR_OTHER | SHTDN_REASON_FLAG_PLANNED);

										if (result || GetLastError() == ERROR_SHUTDOWN_IS_SCHEDULED) {
											shutdown_enabled = TRUE;
											last_mode        = new_mode;
											UpdateLog(global->controls[LOG_GENERAL],log_buffer,(std::wstring)message);
											EnableWindow(global->controls[BUTTON_ABORT], 1);
										} else
											UpdateLog(global->controls[LOG_GENERAL],log_buffer,L"Failed to shutdown" + FormatError(GetLastError()));
									}
								}
							}
						}
					}
				} else {
					user_stopped_shutdown = FALSE;

					if (shutdown_enabled && AbortShutdown(global->controls[LOG_GENERAL],log_buffer)) {
						shutdown_enabled = FALSE;
						UpdateLog(global->controls[LOG_GENERAL],log_buffer,L"Shutdown aborted from the website");
					}

					if (new_mode != TURNOFF_SIGNAL && LaunchExe(exe_name, new_mode, global->controls[LOG_GENERAL], log_buffer) == 0)
						last_mode = new_mode;
				}
			}
		}
	}

	return 0;
}

std::string utf8(const wchar_t* input, int input_size)
{
	// https://mariusbancila.ro/blog/2008/10/20/writing-utf-8-files-in-c/
	
	if (input_size == 0)
		return std::string();
	
	int output_size = WideCharToMultiByte(CP_UTF8, 0, input, input_size, NULL, 0, NULL, NULL);

	std::string output(output_size, '\0');
	WideCharToMultiByte(CP_UTF8, 0, input, input_size, const_cast<char*>(output.c_str()), output_size, NULL, NULL);

	return output;
}

std::string utf8(std::wstring input)
{
	if (input.empty())
		return std::string();

	std::wstring *ptr_input = &input;
	std::wstring crop       = L"";

	if (input.size() > INT_MAX) {
		crop      = input.substr(0, INT_MAX);
		ptr_input = &crop;
	}

	return utf8((*ptr_input).c_str(), static_cast<int>((*ptr_input).size()));
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

DWORD Download(std::wstring url, std::wstring filename, bool overwrite, HWND log_handle, std::wstring &log_buffer)
{
	if (overwrite) {
		BOOL result = DeleteFile(filename.c_str());
		DWORD error = GetLastError();

		if (!result && error != ERROR_FILE_NOT_FOUND)
			UpdateLog(log_handle,log_buffer,L"Failed to delete " + filename + FormatError(error));
	}

	DeleteUrlCacheEntry(url.c_str());

	HRESULT result = URLDownloadToFileW(NULL, url.c_str(), filename.c_str(), 0, NULL);

	if (result != S_OK)
		UpdateLog(log_handle,log_buffer,L"Failed to download " + FormatError(result));

	return result;
}

std::wstring Read(std::wstring filename, HWND log_handle, std::wstring &log_buffer)
{
	std::ifstream file(filename.c_str());
	std::string buffer((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());

	if (file.is_open()) {
		file.close();
	} else {
		wchar_t error[256] = L"";
		_wcserror_s(error, 256, errno);
		UpdateLog(log_handle,log_buffer,(L"Failed to open " + filename + L" " + error));
	}

	return utf16(buffer);
}

std::wstring DownloadAndRead(std::wstring url, std::wstring filename, HWND log_handle, std::wstring &log_buffer)
{
	if (Download(url, filename, 1, log_handle, log_buffer) == 0)
		return Read(filename, log_handle, log_buffer);

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

DWORD LaunchExe(std::wstring &exe_name, std::wstring &exe_arg, HWND log_handle, std::wstring &log_buffer) 
{
	UpdateLog(log_handle, log_buffer, (L"Starting " + exe_name + L" " + exe_arg));

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
		UpdateLog(log_handle, log_buffer, L"PID: " + UInt2StrW(GetProcessId(EI.hProcess)));
		CloseHandle(EI.hProcess);
		return 0;
	} else {
		DWORD error = GetLastError();
		std::wcout << L"Failed to launch" + exe_name + FormatError(error) << std::endl;
		return error;
	}
}

std::wstring Int2StrW(int num, bool leading_zero)
{
	const int buffer_size       = 256;
	wchar_t buffer[buffer_size] = L"";
	wchar_t *buffer_ptr         = buffer;
	
	swprintf_s(buffer_ptr, buffer_size, L"%s%d", (leading_zero && num < 10 ? L"0" : L""), num);
	return (std::wstring)buffer;
}

std::wstring UInt2StrW(size_t num)
{
	const int buffer_size       = 16;
	wchar_t buffer[buffer_size] = L"";
	swprintf(buffer, buffer_size, L"%u", num);
	return (std::wstring)buffer;
}

void WindowTextToString(HWND control, std::wstring &str)
{
	int length = GetWindowTextLength(control);
	str.reserve(length+1);
	str.resize(length);
	GetWindowText(control, &str[0], length+1);
}

void UpdateLog(HWND control, std::wstring &buffer, std::wstring new_text)
{
	if (control) {
		SYSTEMTIME st;
		GetLocalTime(&st);

		new_text = 
			L"[" + Int2StrW(st.wHour, LEADING_ZERO) + 
			L":" + Int2StrW(st.wMinute, LEADING_ZERO) + 
			L":" + Int2StrW(st.wSecond, LEADING_ZERO) +
			L"]  " + new_text + L"\r\n";

		if (buffer.length() + new_text.length() > 32767)
			buffer = L"";

		buffer += new_text;

		SetWindowText(control, buffer.c_str());
		SendMessage(control, EM_SETSEL, 0, -1);
		SendMessage(control, EM_SETSEL, ULONG_MAX, -1);
		SendMessage(control, EM_SCROLLCARET, 0, 0);
	}
}

BOOL AbortShutdown(HWND control, std::wstring &buffer)
{
	BOOL abort  = AbortSystemShutdown(NULL);
	DWORD error = GetLastError();

	if (abort || error == ERROR_NO_SHUTDOWN_IN_PROGRESS)
		return TRUE;
	else {
		UpdateLog(control,buffer,L"Failed to abort shutdown" + FormatError(error));
		return FALSE;
	}
}