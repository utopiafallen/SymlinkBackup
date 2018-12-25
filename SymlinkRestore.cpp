// SymlinkRestore.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include "SymlinkBackupRestoreCommon.h"

int main()
{
	const HANDLE symlinkBackupFile = OpenExistingFile(SYMLINK_BACKUP_FILENAME);
	if (symlinkBackupFile == INVALID_HANDLE_VALUE)
	{
		printf("%ls was not found.\n", SYMLINK_BACKUP_FILENAME);
		return -1;
	}

	FILE_STANDARD_INFO fileStandardInfo = {};
	GetFileInformationByHandleEx(symlinkBackupFile, FileStandardInfo, &fileStandardInfo, sizeof(fileStandardInfo));

	if (fileStandardInfo.EndOfFile.HighPart > 0)
	{
		printf("%ls is too large to restore.\n", SYMLINK_BACKUP_FILENAME);
		CloseHandle(symlinkBackupFile);
		return -2;
	}

	DWORD bytesRead = 0;
	void* fileBuf = malloc(fileStandardInfo.EndOfFile.LowPart);
	ReadFile(symlinkBackupFile, fileBuf, fileStandardInfo.EndOfFile.LowPart, &bytesRead, nullptr);
	if (bytesRead != fileStandardInfo.EndOfFile.LowPart)
	{
		wchar_t* errorMsg = nullptr;
		FormatMessage(
			FORMAT_MESSAGE_ALLOCATE_BUFFER |
			FORMAT_MESSAGE_FROM_SYSTEM |
			FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL,
			GetLastError(),
			MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			(LPTSTR)&errorMsg,
			0, NULL);
		printf("%ls failed to read correctly. %ls\n", SYMLINK_BACKUP_FILENAME, errorMsg);
		LocalFree(errorMsg);
		free(fileBuf);
		CloseHandle(symlinkBackupFile);
		return -3;
	}

	CloseHandle(symlinkBackupFile);

	std::vector<Symlink_s> fileSymlinks;
	std::vector<Symlink_s> dirSymlinks;
	wchar_t* readWriteCursor = static_cast<wchar_t*>(fileBuf);
	const wchar_t* const endOfFile = reinterpret_cast<wchar_t*>(static_cast<u8*>(fileBuf) + fileStandardInfo.AllocationSize.LowPart);
	const wchar_t* symlinkSource = readWriteCursor;
	const wchar_t* symlinkTarget = nullptr;
	bool isFileSymlinks = true;
	while (readWriteCursor != endOfFile)
	{
		if (*readWriteCursor == L',')
		{
			*readWriteCursor = 0;
			symlinkTarget = readWriteCursor + 1;
		}
		else if (*readWriteCursor == L'\n')
		{
			*readWriteCursor = 0;

			if (isFileSymlinks && symlinkTarget)
				fileSymlinks.push_back({symlinkSource, symlinkTarget});
			else if (!isFileSymlinks && symlinkTarget)
				dirSymlinks.push_back({symlinkSource, symlinkTarget});

			if (*(readWriteCursor + 1) == L'\n')
			{
				isFileSymlinks = false;
				symlinkSource = readWriteCursor + 2;
				++readWriteCursor;
			}
			else
			{
				symlinkSource = readWriteCursor + 1;
			}
			symlinkTarget = nullptr;
		}
		
		++readWriteCursor;
	}

	free(fileBuf);

	for (const Symlink_s& symlink : fileSymlinks)
	{
		printf("Restoring %ls ==> %ls\n", symlink.source.c_str(), symlink.target.c_str());
		CreateSymbolicLinkW(symlink.source.c_str(), symlink.target.c_str(), 0);
	}
		

	for (const Symlink_s& symlink : dirSymlinks)
	{
		printf("Restoring %ls ==> %ls\n", symlink.source.c_str(), symlink.target.c_str());
		CreateSymbolicLinkW(symlink.source.c_str(), symlink.target.c_str(), SYMBOLIC_LINK_FLAG_DIRECTORY);
	}
		
	system("pause");
	return 0;
}
