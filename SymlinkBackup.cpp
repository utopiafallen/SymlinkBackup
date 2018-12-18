// SymlinkBackup.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <iostream>
#include <vector>

template<typename T, size_t arrSize>
constexpr size_t ARRAY_SIZE(T const (&)[arrSize]) { return arrSize; }

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

struct Symlink_s
{
	std::wstring source; // Where the symlink was found.
	std::wstring target; // Where the symlink points to.
};

int main()
{
	int returnCode = 0;

	wchar_t startDir[MAX_PATH];
	const DWORD startDirLen = GetCurrentDirectoryW((DWORD)ARRAY_SIZE(startDir), startDir);
	startDir[startDirLen] = '*';
	startDir[startDirLen + 1] = 0;

	WIN32_FIND_DATA findData = {};
	HANDLE hFind = FindFirstFileW(startDir, &findData);
	startDir[startDirLen] = 0;
	if (hFind == INVALID_HANDLE_VALUE)
	{
		printf("Unable to find files in folder %ls\n", startDir);
		returnCode = 1;
		system("pause");
		return returnCode;
	}
	
	std::vector<Symlink_s> symlinks;
	std::vector<std::wstring> dirsToVisit;
	std::wstring currDir = startDir;
	for (;;)
	{
		if (findData.dwFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN))
			goto next_file;

		if (!wcscmp(findData.cFileName, L".") || !wcscmp(findData.cFileName, L".."))
			goto next_file;

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			//printf("Found directory: %ls%ls\n", currDir.c_str(), findData.cFileName);
			dirsToVisit.push_back(currDir + std::wstring(findData.cFileName) + L"\\");
		}
		else if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
		{
			printf("Found symbolic link: %ls\n", findData.cFileName);
			const std::wstring symlinkSourceFilename = currDir + findData.cFileName;
			const HANDLE hSymlinkTarget = CreateFileW(symlinkSourceFilename.c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

			wchar_t symlinkTargetFilename[MAX_PATH] = {};
			DWORD symlinkTargetFilenameLen = GetFinalPathNameByHandleW(hSymlinkTarget, symlinkTargetFilename, (DWORD)ARRAY_SIZE(symlinkTargetFilename) - 1, FILE_NAME_NORMALIZED);
			CloseHandle(hSymlinkTarget);
			printf("\tSymbolic link target: %ls\n\n", symlinkTargetFilename );

			symlinks.push_back({std::move(symlinkSourceFilename), symlinkTargetFilename});
		}

next_file:
		if (!FindNextFile(hFind, &findData))
		{
			if (dirsToVisit.empty())
				break;

			FindClose(hFind);
			do
			{
				currDir = std::move(dirsToVisit.back());
				dirsToVisit.pop_back();
				hFind = FindFirstFileW((currDir + L"*").c_str(), &findData);
			} while (hFind == INVALID_HANDLE_VALUE && !dirsToVisit.empty());
			
			if (hFind == INVALID_HANDLE_VALUE)
				break;
		}	
	}
	
	FindClose(hFind);

	if (!symlinks.empty())
	{
		const HANDLE hSymlinkBackupFile = CreateFileW(L"symlinks.backup", GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );

		DWORD bytesWritten = -1;
		std::wstring output;
		output.reserve(MAX_PATH * 2 + 3);
		for (const Symlink_s& symlink : symlinks)
		{
			output = symlink.source + L"," + symlink.target + L"\n";
			WriteFile(hSymlinkBackupFile, output.c_str(), (DWORD)(output.length() * sizeof(wchar_t)), &bytesWritten, nullptr);
		}
		CloseHandle(hSymlinkBackupFile);
	}

	system("pause");
	return returnCode;
}
