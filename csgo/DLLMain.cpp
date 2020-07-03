#include <thread>
#include "Hooks.h"
#include "Utils\Utils.h"
#include "Utils\GlobalVars.h"

HINSTANCE HThisModule;

int OnDllAttach()
{
    Hooks::Init();

	while (true)
		Sleep(10000000);

	Hooks::Restore();

	FreeLibraryAndExitThread(HThisModule, 0);
}

BOOL WINAPI DllMain(HMODULE hModule, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH && GetModuleHandleA("csgo.exe"))
    {
        CreateThread(NULL, NULL, (LPTHREAD_START_ROUTINE)OnDllAttach, NULL, NULL, NULL);
    }

    return TRUE;
}

