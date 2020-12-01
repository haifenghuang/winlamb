/*
 * Part of WinLamb - Win32 API Lambda Library
 * https://github.com/rodrigocfd/winlamb
 * This library is released under the MIT License.
 */

#pragma once
#include <array>
#include <optional>
#include <string_view>
#include <system_error>
#include <vector>
#include <Windows.h>
#include "com.h"
#include "str.h"
#pragma comment(lib, "Version.lib")

/// Executable/DLL related utilities.
///
/// #include <exe.h>
namespace wl::exe {

/// Retrieves the program's command line, with the tokens parsed.
/// @see https://docs.microsoft.com/en-us/windows/win32/api/processenv/nf-processenv-getcommandlinew
[[nodiscard]] inline std::vector<std::wstring> command_line()
{
	return str::split_quoted(GetCommandLineW());
}

/// Retrieves path to current running EXE itself.
/// @see https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulefilenamew
[[nodiscard]] inline std::wstring own_path()
{
	wchar_t buf[MAX_PATH + 1]{};
	if (GetModuleFileNameW(nullptr, buf, ARRAYSIZE(buf)) == 0) { // full path name
		throw std::system_error(GetLastError(), std::system_category(),
			"GetModuleFileName() failed in " __FUNCTION__ "().");
	}
	std::wstring ret = buf;
	ret.resize(ret.find_last_of(L'\\')); // truncate removing EXE filename and trailing backslash

#if defined(_MSC_VER) && defined(_DEBUG)
	ret.resize(ret.find_last_of(L'\\')); // bypass "Debug" folder, remove trailing backslash too
#ifdef _WIN64
	ret.resize(ret.find_last_of(L'\\')); // bypass "x64" folder, remove trailing backslash again
#endif
#endif

	return ret;
}

/// Reads the version information from an EXE or DLL.
[[nodiscard]] inline std::optional<std::array<UINT, 4>>
	read_version(std::wstring_view exeOrDllPath)
{
	DWORD szVer = GetFileVersionInfoSizeW(exeOrDllPath.data(), nullptr);
	if (szVer == 0) {
		throw std::system_error(GetLastError(), std::system_category(),
			"GetFileVersionInfoSize() failed in " __FUNCTION__ "().");
	}

	std::vector<wchar_t> infoBlock(szVer, L'\0');
	if (GetFileVersionInfoW(exeOrDllPath.data(), 0, szVer, &infoBlock[0]) == 0) {
		throw std::system_error(GetLastError(), std::system_category(),
			"GetFileVersionInfo() failed in " __FUNCTION__ "().");
	}

	BYTE* lpBuf = nullptr;
	UINT blockSize = 0;
	if (VerQueryValueW(&infoBlock[0], L"\\", reinterpret_cast<void**>(&lpBuf), &blockSize) == FALSE
		|| blockSize == 0)
	{
		return std::nullopt; // no information available, not an error
	}

	VS_FIXEDFILEINFO* verInfo = reinterpret_cast<VS_FIXEDFILEINFO*>(lpBuf);
	return {{
		(verInfo->dwFileVersionMS >> 16) & 0xffff,
		(verInfo->dwFileVersionMS >>  0) & 0xffff,
		(verInfo->dwFileVersionLS >> 16) & 0xffff,
		(verInfo->dwFileVersionLS >>  0) & 0xffff
	}};
}

/// Reads version information of current running EXE itself.
[[nodiscard]] inline std::optional<std::array<UINT, 4>> read_own_version()
{
	wchar_t buf[MAX_PATH + 1]{};
	if (GetModuleFileNameW(nullptr, buf, ARRAYSIZE(buf)) == 0) { // full path name
		throw std::system_error(GetLastError(), std::system_category(),
			"GetModuleFileName() failed in " __FUNCTION__ "().");
	}
	return read_version(buf);
}

/// Synchronous execution of a command line, in a separated process.
/// @see https://docs.microsoft.com/en-us/windows/win32/api/processthreadsapi/nf-processthreadsapi-createprocessw
inline DWORD run_process(std::wstring_view cmdLine)
{
	SECURITY_ATTRIBUTES sa{};
	sa.nLength = sizeof(sa);
	sa.bInheritHandle = TRUE;

	STARTUPINFO si{};
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESHOWWINDOW;
	si.wShowWindow = SW_SHOW;

	PROCESS_INFORMATION pi{};
	DWORD dwExitCode = 1; // returned by executed program

	std::wstring cmdLine2 = cmdLine.data(); // http://blogs.msdn.com/b/oldnewthing/archive/2009/06/01/9673254.aspx

	if (CreateProcessW(nullptr, &cmdLine2[0], &sa, nullptr, FALSE,
		0, nullptr, nullptr, &si, &pi) == 0)
	{
		throw std::system_error(GetLastError(), std::system_category(),
			"CreateProcess() failed in " __FUNCTION__ "().");
	}

	WaitForSingleObject(pi.hProcess, INFINITE); // the program flow is stopped here to wait
	GetExitCodeProcess(pi.hProcess, &dwExitCode);
	CloseHandle(pi.hThread);
	CloseHandle(pi.hProcess);
	return dwExitCode;
}

/// Runs the shell-associated program to the given file
/// Ex.: for a TXT file, will run Notepad.
/// @see https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutew
inline void run_associated_shell(
	std::wstring_view fileToBeOpened, INT showCmd = SW_SHOWNORMAL)
{
	com::lib comLib{com::lib::init::NOW};

	int h = static_cast<int>(
		reinterpret_cast<INT_PTR>(
			ShellExecuteW(nullptr, L"open", fileToBeOpened.data(), nullptr, nullptr, showCmd) ));

	if (h <= 8) {
		throw std::system_error(h, std::system_category(),
			"ShellExecute failed.");
	} else if (h <= 32) {
		throw std::runtime_error(
			str::unicode_to_ansi(
				str::format(L"ShellExecute failed: error %d in " __FUNCTION__ "().", h) ));
	}
}

}//namespace wl::exe