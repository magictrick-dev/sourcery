#include <sourcery/generics.h>

#if defined(PLATFORM_WINDOWS)

#pragma warning(suppress : 5105)
#	include <windows.h>
#pragma warning(disable : 5105)

#include <sourcery/memory/alloc.h>


bool virtual_allocate(void** region, size_t* region_size, uint64 base)
{

	bool allocation_success = false;

	// Attempt to allocate.
	LPVOID allocation_ptr = VirtualAlloc((LPVOID)base, *region_size, MEM_COMMIT|MEM_RESERVE, PAGE_READWRITE);
	if (allocation_ptr != NULL)
	{

		// The allocation was a success.
		allocation_success = true;
		*region = allocation_ptr;

		// Determine how much the OS actually gave back to us.
		MEMORY_BASIC_INFORMATION memory_info = {0};
		DWORD bytes_queried = VirtualQuery(allocation_ptr, &memory_info, sizeof(memory_info));

		*region_size = (size_t)memory_info.RegionSize;

	}

	return allocation_success;
}

bool virtual_free(void** region)
{

	bool free_success = false;

	if (VirtualFree(*region, (SIZE_T)NULL, MEM_RELEASE))
	{
		free_success = true;
		region = NULL;
	}

	return free_success;

}

#endif
