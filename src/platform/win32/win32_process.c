#include <sourcery/process/process.h>

#if defined(PLATFORM_WINDOWS)

#pragma warning(suppress : 5105)
#	include <windows.h>
#pragma warning(disable : 5105)

int
platformRunCLIProcess(char* invoc)
{

	// I totally kidnapped this from Microsoft Docs, lol
	STARTUPINFOA si = {0};
	PROCESS_INFORMATION pi = {0};
	si.cb = sizeof(si);

	if (!CreateProcessA(NULL, invoc, NULL, NULL,
		FALSE, 0, NULL, NULL, &si, &pi))
	{
		return -1;
	}

	WaitForSingleObject(pi.hProcess, INFINITE);

	CloseHandle(pi.hProcess);
	CloseHandle(pi.hThread);

	return 0;

}

#endif
