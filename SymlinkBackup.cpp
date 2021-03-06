// SymlinkBackup.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "SymlinkBackupRestoreCommon.h"

int main(int argc, char* argv[])
{
	bool interactiveMode = true;
	for (int i = 0; i < argc; ++i)
	{
		if (strstr(argv[i], "task"))
			interactiveMode = false;
	}

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
		system("pause");
		return -1;
	}
	
	std::vector<Symlink_s> fileSymlinks;
	std::vector<Symlink_s> dirSymlinks;
	std::vector<std::wstring> dirsToVisit;
	std::wstring currDir = startDir;
	for (;;)
	{
		if (findData.dwFileAttributes & (FILE_ATTRIBUTE_SYSTEM | FILE_ATTRIBUTE_HIDDEN))
			goto next_file;

		if (!wcscmp(findData.cFileName, L".") || !wcscmp(findData.cFileName, L".."))
			goto next_file;

		if (findData.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
		{
			printf("Found symbolic link: %ls\n", findData.cFileName);
			const std::wstring symlinkSourceFilename = currDir + findData.cFileName;
			const HANDLE hSymlinkTarget = OpenExistingFile(symlinkSourceFilename.c_str());

			wchar_t symlinkTargetFilename[MAX_PATH] = {};
			DWORD symlinkTargetFilenameLen = GetFinalPathNameByHandleW(hSymlinkTarget, symlinkTargetFilename, (DWORD)ARRAY_SIZE(symlinkTargetFilename) - 1, FILE_NAME_NORMALIZED);
			CloseHandle(hSymlinkTarget);
			printf("\tSymbolic link target: %ls\n\n", symlinkTargetFilename);

			if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
				dirSymlinks.push_back({ std::move(symlinkSourceFilename), symlinkTargetFilename });
			else
				fileSymlinks.push_back({ std::move(symlinkSourceFilename), symlinkTargetFilename });
		}
		else if (findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			//printf("Found directory: %ls%ls\n", currDir.c_str(), findData.cFileName);
			dirsToVisit.push_back(currDir + std::wstring(findData.cFileName) + L"\\");
			
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

	std::wstring output;
	output.reserve(4096);
	if (!fileSymlinks.empty() || !dirSymlinks.empty())
	{
		const HANDLE hSymlinkBackupFile = CreateFileW(SYMLINK_BACKUP_FILENAME, GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr );

		DWORD bytesWritten = -1;
		
		if (fileSymlinks.empty())
			output += L"\n";
		for (const Symlink_s& symlink : fileSymlinks)
			output += symlink.source + L"," + symlink.target + L"\n";

		output += L"\n";

		for (const Symlink_s& symlink : dirSymlinks)
			output += symlink.source + L"," + symlink.target + L"\n";

		WriteFile(hSymlinkBackupFile, output.c_str(), (DWORD)((output.length()) * sizeof(wchar_t)), &bytesWritten, nullptr);
		CloseHandle(hSymlinkBackupFile);
	}

	if (interactiveMode)
		system("pause");
	return 0;
}
