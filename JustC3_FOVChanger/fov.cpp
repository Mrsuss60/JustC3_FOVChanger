#include "pch.h"

float DegToRad(float degrees) {
    return degrees * (3.14159265359f / 180.0f);
}

bool ApplyFOVToGame(float newFOV) {
    float radians = DegToRad(newFOV);

    if (!IsValidAddress((void*)g_Camera_FOV_Addr)) return false;

    uintptr_t* baseAddrPtr = (uintptr_t*)g_Camera_FOV_Addr;
    if (!IsValidAddress(baseAddrPtr)) return false;
    uintptr_t baseAddr = *baseAddrPtr;

    if (!IsValidAddress((void*)baseAddr)) return false;
    uintptr_t* currentCameraPtr = (uintptr_t*)(baseAddr + g_currentCameraOff);
    if (!IsValidAddress(currentCameraPtr)) return false;
    uintptr_t currentCamera = *currentCameraPtr;
    if (!IsValidAddress((void*)currentCamera)) return false;

    BYTE* flagsPtr = (BYTE*)(currentCamera + g_cameraFlagsOff);
    if (IsValidAddress(flagsPtr)) {
        BYTE flagsValue = *flagsPtr | 0x10;
        PatchMemory(flagsPtr, &flagsValue, sizeof(BYTE));
    }

    float* fov1Ptr = (float*)(currentCamera + g_fovOff1);
    float* fov2Ptr = (float*)(currentCamera + g_fovOff2);

    if (IsValidAddress(fov1Ptr)) PatchMemory(fov1Ptr, &radians, sizeof(float));
    if (IsValidAddress(fov2Ptr)) PatchMemory(fov2Ptr, &radians, sizeof(float));

    return true;
}

void SetPatchesEnabled(bool enable) {
    if (enable) {
        if (IsValidAddress((void*)g_patchAddr1)) NopMemory(g_patchAddr1, PATTERN_1.size());
        if (IsValidAddress((void*)g_patchAddr2)) NopMemory(g_patchAddr2, PATTERN_2.size());
        if (IsValidAddress((void*)g_patchAddr3)) NopMemory(g_patchAddr3, PATTERN_3.size());
        Log("Mods Enabled: Game reset functions disabled.");
    }
    else {
        if (IsValidAddress((void*)g_patchAddr1) && !g_backupBytes1.empty())
            PatchMemory((void*)g_patchAddr1, g_backupBytes1.data(), g_backupBytes1.size());

        if (IsValidAddress((void*)g_patchAddr2) && !g_backupBytes2.empty())
            PatchMemory((void*)g_patchAddr2, g_backupBytes2.data(), g_backupBytes2.size());

        if (IsValidAddress((void*)g_patchAddr3) && !g_backupBytes3.empty())
            PatchMemory((void*)g_patchAddr3, g_backupBytes3.data(), g_backupBytes3.size());

        Log("Mods Disabled: Original game code restored.");
    }
}

bool ReadGameFOV(float& outFov) {
    if (!IsValidAddress((void*)g_Camera_FOV_Addr)) return false;

    uintptr_t* baseAddrPtr = (uintptr_t*)g_Camera_FOV_Addr;
    if (!IsValidAddress(baseAddrPtr)) return false;
    uintptr_t baseAddr = *baseAddrPtr;
    if (!IsValidAddress((void*)baseAddr)) return false;
    uintptr_t* currentCameraPtr = (uintptr_t*)(baseAddr + g_currentCameraOff);
    if (!IsValidAddress(currentCameraPtr)) return false;
    uintptr_t currentCamera = *currentCameraPtr;
    if (!IsValidAddress((void*)currentCamera)) return false;
    float* fovPtr = (float*)(currentCamera + g_fovOff1);
    if (!IsValidAddress(fovPtr)) return false;
    outFov = *fovPtr * (180.0f / 3.14159265359f);
    return true;
}

void KeyPollLoop() {
    static bool lastToggle = false;

    while (g_initialized.load()) {

        if (HasConfigChanged()) {
            LoadConfig();
            Log("Config Reloaded!");
        }

        bool currToggle = (GetAsyncKeyState(g_ToggleKey) & 0x8000) != 0;
        if (currToggle && !lastToggle) {
            g_fovOverrideEnabled = !g_fovOverrideEnabled;

            SetPatchesEnabled(g_fovOverrideEnabled);

            if (g_fovOverrideEnabled) {
                ApplyFOVToGame(g_currentFOV.load());
            }
        }
        lastToggle = currToggle;

        if (g_fovOverrideEnabled) {
            ApplyFOVToGame(g_currentFOV.load());

            bool changed = false;
            float fov = g_currentFOV.load();

            if (GetAsyncKeyState(g_IncreaseKey) & 0x8000) {
                fov += g_Step;
                changed = true;
                Log("FOV Increased: " + std::to_string(fov));
            }
            if (GetAsyncKeyState(g_DecreaseKey) & 0x8000) {
                fov -= g_Step;
                changed = true;
                Log("FOV Decreased: " + std::to_string(fov));
            }

            if (changed) {
                fov = std::clamp(fov, MIN_FOV, MAX_FOV);
                g_currentFOV.store(fov);
                SaveConfig();
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}