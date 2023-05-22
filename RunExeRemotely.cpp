// RunExeRemotely by Faguss (ofp-faguss.com)
// Program for restarting OFP server based on downloaded data

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
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

vector<HWND> GetWindowsFromProcessID(DWORD wanted_pid)
{
	vector<HWND> container;
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

vector<wstring> Tokenize(wstring text, wstring delimiter)
{
	vector<wstring> container;
	bool first_item   = false;
	bool inQuote      = false;
	char custom_delim = ' ';
	bool use_unQuote  = true;
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

int wmain(int argc, wchar_t** argv)
{
	DWORD sleep_time = 5000;
	wstring exe_name = L"coldwarassault_server.exe";
	wstring url      = L"";

	// Merge local config file and command line arguments into a single list
	vector<wstring> input_config = Tokenize(Read(L"RunExeRemotely.cfg"), L"\n;");

	for (int i=1; i<argc; i++) 
		input_config.push_back((wstring)argv[i]);

	// Parse configuration
	for (size_t i=0; i<input_config.size(); i++) {
		wstring namevalue = (wstring)input_config[i];
		size_t separator  = namevalue.find_first_of('=');

		if (separator != string::npos) {
			wstring name  = namevalue.substr(0, separator);
			wstring value = namevalue.substr(separator + 1);

			if (name.substr(0,1) == L"-")
				name = name.substr(1);

			wcout << name << L"=" << value << endl;

			if (name == L"sleep") {
				sleep_time = wcstoul(value.c_str(), NULL, 0) * 1000;
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
		cout << "Missing -url= argument!" << endl;
		return 1;
	}

	wstring turnoff_signal    = L"?turnoff";
	wstring download_filename = L"RunExeRemotely.txt";
	wstring last_mode         = L"";
	DWORD pid                 = GetProcessID(exe_name);
	bool show_message         = true;

	if (pid)
		last_mode = DownloadAndRead(url, download_filename);	// if the server is already running then assume it's the correct one and don't restart
	else
		last_mode = L"";

	while (true) {
		Sleep(sleep_time);
		wstring new_mode = DownloadAndRead(url, download_filename);
		pid              = GetProcessID(exe_name);

		//data was downloaded AND (selected option has changed OR process is not running and option other than turnoff was selected OR process is running and turnoff option was selected)

		if (new_mode != L"" && (last_mode != new_mode || (!pid && new_mode != turnoff_signal) || (new_mode == turnoff_signal && pid))) {
			if (last_mode != new_mode && show_message) {
				wcout << L"Selected new option" << endl;
				show_message = false;
			}

			if (pid) {
				vector<HWND> windows = GetWindowsFromProcessID(pid);

				if (windows.size() > 0) {
					cout << "Tell pid " << pid << " to close" << endl;

					for (size_t i=0; i<windows.size(); i++) 
						PostMessage(windows[i], WM_CLOSE, 0, 0);
				}
			} else {
				if (new_mode != turnoff_signal) {
					wcout << "Start " << exe_name << " " << new_mode << endl;

					wstring command_line = L"start \"\" " + exe_name + L" " + new_mode;
					_wsystem(command_line.c_str());
				}

				show_message = true;
				last_mode    = new_mode;
			}

		}
	}

	return 0;
}
