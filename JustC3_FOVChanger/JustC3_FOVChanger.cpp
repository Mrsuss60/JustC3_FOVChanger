#include "pch.h"

void DetectVersionAndSetOffsets() {
    g_patchAddr1 = FindPattern(PATTERN_1, MASK_1);
    g_patchAddr2 = FindPattern(PATTERN_2, MASK_2);
    g_patchAddr3 = FindPattern(PATTERN_3, MASK_3);

    if (g_patchAddr1 == 0 || g_patchAddr2 == 0) {
        Log("Patch patterns not found! Mod may not work.");
        return;
    }
    uintptr_t offsetFound = g_patchAddr1 - g_gameBaseAddr;
    if (offsetFound < 0x200000) {
        Log("Detected Version: NEW (Denuvo Removed)");
        g_Camera_FOV_Addr = g_gameBaseAddr + OFFSET_CAM_NEW;
    }

    else if (offsetFound > 0x3000000) {
        Log("Detected Version: OLD (1.05 / Denuvo)");
        g_Camera_FOV_Addr = g_gameBaseAddr + OFFSET_CAM_OLD;
    }
    else {
        Log("WARNING: Unknown Version Offset (" + std::to_string(offsetFound) + "). Defaulting to New.");
        g_Camera_FOV_Addr = g_gameBaseAddr + OFFSET_CAM_NEW;
    }

    std::cout << " > Camera FOV Address: 0x" << std::hex << g_Camera_FOV_Addr << std::endl;
    std::cout << " > Patch 1: 0x" << std::hex << g_patchAddr1 << std::endl;
}

static void InitializeFOVHook() {
    std::this_thread::sleep_for(std::chrono::seconds(5));
    InitConsole();
    Log("Initializing Version FOV Hook...");

    g_gameBaseAddr = (uintptr_t)GetModuleHandle(NULL);
    if (!g_gameBaseAddr) return;

    DetectVersionAndSetOffsets();

    if (IsValidAddress((void*)g_patchAddr1)) {
        g_backupBytes1.resize(PATTERN_1.size());
        memcpy(g_backupBytes1.data(), (void*)g_patchAddr1, PATTERN_1.size());
        NopMemory(g_patchAddr1, PATTERN_1.size());
    }
    if (IsValidAddress((void*)g_patchAddr2)) {
        g_backupBytes2.resize(PATTERN_2.size());
        memcpy(g_backupBytes2.data(), (void*)g_patchAddr2, PATTERN_2.size());
        NopMemory(g_patchAddr2, PATTERN_2.size());
    }
    if (IsValidAddress((void*)g_patchAddr3)) {
        g_backupBytes3.resize(PATTERN_3.size());
        memcpy(g_backupBytes3.data(), (void*)g_patchAddr3, PATTERN_3.size());
        NopMemory(g_patchAddr3, PATTERN_3.size());
    }

    Log("Patches Applied.");

    LoadConfig();

    g_initialized.store(true);
    g_keyThread = std::thread(KeyPollLoop);
}

static void CleanupFOVHook() {
    g_initialized.store(false);

    if (g_keyThread.joinable()) {
        g_keyThread.join();
    }

    if (!g_backupBytes1.empty()) PatchMemory((void*)g_patchAddr1, g_backupBytes1.data(), g_backupBytes1.size());
    if (!g_backupBytes2.empty()) PatchMemory((void*)g_patchAddr2, g_backupBytes2.data(), g_backupBytes2.size());
    if (!g_backupBytes3.empty()) PatchMemory((void*)g_patchAddr3, g_backupBytes3.data(), g_backupBytes3.size());

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