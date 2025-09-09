#include "pch.h"

static void InitializeFOVHook() {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    InitConsole();
    Log("Initializing FOV Hook...");

    g_originalCallBytes = new BYTE[INSTRUCTION_PATCH_SIZE];
    if (IsValidAddress((void*)g_setFovFunc)) {
        memcpy(g_originalCallBytes, (void*)g_setFovFunc, INSTRUCTION_PATCH_SIZE);
        PatchSetFovCall(true);
    }
    else {
        delete[] g_originalCallBytes;
        g_originalCallBytes = nullptr;
        Log("Failed to patch FOV function.");
        return;
    }

    g_currentFOV.store(LoadFOV());
    g_targetFOV.store(g_currentFOV.load());
    ApplyFOVToGame(g_currentFOV.load());
    Log("Loaded FOV: " + std::to_string(g_currentFOV.load()));

    g_initialized.store(true);
    g_keyThread = std::thread(KeyPollLoop);
    g_smoothThread = std::thread(SmoothFOVLoop);

}

static void CleanupFOVHook() {
    g_initialized.store(false);

    if (g_keyThread.joinable()) g_keyThread.detach();
    if (g_smoothThread.joinable()) g_smoothThread.detach();

    PatchSetFovCall(false);

    delete[] g_originalCallBytes;
    g_originalCallBytes = nullptr;

    FreeConsoleIfEnabled();
    Log("FOV Hook cleaned up safely.");


}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        std::thread(InitializeFOVHook).detach();
    }
    else if (reason == DLL_PROCESS_DETACH) {
        CleanupFOVHook();
    }
    return TRUE;
}
