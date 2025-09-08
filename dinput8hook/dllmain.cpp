#include "pch.h"

typedef HRESULT(WINAPI* DirectInput8Create_t)(HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

namespace {

    DirectInput8Create_t g_originalDirectInput8Create = nullptr;
    HMODULE g_originalDinput8 = nullptr;
    HMODULE g_fovDll = nullptr;

    constexpr wchar_t ORIGINAL_DINPUT_DLL[] = L"dinput8.dll";
    constexpr char FOV_DLL_NAME[] = "JustC3-FOVChanger.dll";

    bool LoadOriginalDinput8() {
        if (g_originalDinput8) return true;

        wchar_t systemPath[MAX_PATH];
        if (!GetSystemDirectoryW(systemPath, MAX_PATH)) return false;

        wsprintfW(systemPath, L"%s\\%s", systemPath, ORIGINAL_DINPUT_DLL);
        g_originalDinput8 = LoadLibraryW(systemPath);
        if (!g_originalDinput8) return false;

        g_originalDirectInput8Create = reinterpret_cast<DirectInput8Create_t>(
            GetProcAddress(g_originalDinput8, "DirectInput8Create")
            );
        if (!g_originalDirectInput8Create) {
            FreeLibrary(g_originalDinput8);
            g_originalDinput8 = nullptr;
            return false;
        }

        return true;
    }

    void LoadFOVDll() {
        if (!g_fovDll) g_fovDll = LoadLibraryA(FOV_DLL_NAME);
    }

    void Cleanup() {
        if (g_fovDll) {
            FreeLibrary(g_fovDll);
            g_fovDll = nullptr;
        }

        if (g_originalDinput8) {
            FreeLibrary(g_originalDinput8);
            g_originalDinput8 = nullptr;
            g_originalDirectInput8Create = nullptr;
        }
    }


    unsigned int __stdcall InitThread(void*) {
        LoadOriginalDinput8();
        LoadFOVDll();
        return 0;
    }

}
extern "C" __declspec(dllexport) HRESULT WINAPI DirectInput8Create(
    HINSTANCE hinst,
    DWORD dwVersion,
    REFIID riidltf,
    LPVOID* ppvOut,
    LPUNKNOWN punkOuter
) {
    if (!LoadOriginalDinput8()) return E_FAIL;
    return g_originalDirectInput8Create(hinst, dwVersion, riidltf, ppvOut, punkOuter);
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID lpReserved) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hModule);
        _beginthreadex(nullptr, 0, InitThread, nullptr, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
        Cleanup();
        break;
    }
    return TRUE;
}