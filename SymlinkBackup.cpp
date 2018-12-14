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

int main()
{
	int returnCode = 0;
	wchar_t startDir[MAX_PATH];
	const DWORD startDirLen = GetCurrentDirectory((DWORD)ARRAY_SIZE(startDir), startDir);
	startDir[startDirLen] = '*';
	startDir[startDirLen + 1] = 0;

	WIN32_FIND_DATA findData = {};
	HANDLE hFind = FindFirstFile(startDir, &findData);
	startDir[startDirLen] = 0;
	if (hFind == INVALID_HANDLE_VALUE)
	{
		printf("Unable to find files in folder %ls\n", startDir);
		returnCode = 1;
		system("pause");
		return returnCode;
	}
	
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
			const HANDLE hSymlinkTarget = CreateFile((currDir + findData.cFileName).c_str(), 0, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);

			wchar_t symlinkTargetFilename[MAX_PATH] = {};
			DWORD symlinkTargetFilenameLen = GetFinalPathNameByHandle(hSymlinkTarget, symlinkTargetFilename, ARRAY_SIZE(symlinkTargetFilename) - 1, FILE_NAME_NORMALIZED);
			CloseHandle(hSymlinkTarget);
			printf("\tSymbolic link target: %ls\n\n", symlinkTargetFilename );
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
				hFind = FindFirstFile((currDir + L"*").c_str(), &findData);
			} while (hFind == INVALID_HANDLE_VALUE && !dirsToVisit.empty());
			
			if (hFind == INVALID_HANDLE_VALUE)
				break;
		}	
	}
	
	FindClose(hFind);

	system("pause");
	return returnCode;
}
