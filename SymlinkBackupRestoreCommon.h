#pragma once
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

static const wchar_t* SYMLINK_BACKUP_FILENAME = L"symlinks.backup";

struct Symlink_s
{
	std::wstring source; // Where the symlink was found.
	std::wstring target; // Where the symlink points to.
};

inline HANDLE OpenExistingFile(const wchar_t* filename)
{
	// Backup semantics necessary to get a handle to a directory.
	return CreateFileW(filename, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, nullptr);
}