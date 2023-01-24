#include "memory.h"
#include "fileio.h"

#define _AMD64_
#include <fileapi.h>
#include <handleapi.h>

u8 *ReadEntireFile(char *file_path, u32 *file_size) {
	HANDLE file_handle = CreateFile(file_path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (file_handle == INVALID_HANDLE_VALUE) return 0;
	
	*file_size = GetFileSize(file_handle, 0);
	if (*file_size == 0) return 0;
	
	u8 *buffer = BumpAlloc(*file_size + 1);
	
	DWORD bytes_read = 0;
	ReadFile(file_handle, buffer, *file_size, &bytes_read, NULL);
	CloseHandle(file_handle);
	if (bytes_read != *file_size) {
		*file_size = 0;
		BumpFree(buffer);
		return 0;
	};
	
	return buffer;
}
