#include "pch.h"

float DegToRad(float degrees) {
    return degrees * (3.14159265359f / 180.0f);
}

void SaveFOV(float fov) {
    std::ofstream out("JustC3-FOVChanger.cfg", std::ios::trunc);
    if (out) out << "FOV value = " << fov;
}

float LoadFOV() {
    std::ifstream in("JustC3-FOVChanger.cfg");
    float fov = DEFAULT_FOV;
    if (in) {
        std::string key1, key2;
        char eq;
        float value;
        if (in >> key1 >> key2 >> eq >> value && key1 == "FOV" && key2 == "value" && eq == '=') {
            fov = value;
        }
    }
    return fov;
}

bool ApplyFOVToGame(float newFOV) {
    float radians = DegToRad(newFOV);
    uintptr_t* baseAddrPtr = (uintptr_t*)g_cameraManagerAddr;
    if (!IsValidAddress(baseAddrPtr)) return false;
    uintptr_t baseAddr = *baseAddrPtr;
    if (!IsValidAddress((void*)baseAddr)) return false;
    uintptr_t* currentCameraPtr = (uintptr_t*)(baseAddr + g_currentCameraOff);
    if (!IsValidAddress(currentCameraPtr)) return false;
    uintptr_t currentCamera = *currentCameraPtr;
    if (!IsValidAddress((void*)currentCamera)) return false;

    BYTE* flagsPtr = (BYTE*)(currentCamera + g_cameraFlagsOff);
    BYTE flagsValue = *flagsPtr | 0x10;
    PatchMemory(flagsPtr, &flagsValue, sizeof(BYTE));

    float* fov1Ptr = (float*)(currentCamera + g_fovOff1);
    float* fov2Ptr = (float*)(currentCamera + g_fovOff2);
    PatchMemory(fov1Ptr, &radians, sizeof(float));
    PatchMemory(fov2Ptr, &radians, sizeof(float));

    return true;
}

bool ReadGameFOV(float& outFov) {
    uintptr_t* baseAddrPtr = (uintptr_t*)g_cameraManagerAddr;
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

void SmoothFOVLoop() {
    constexpr float MAX_STEP = 0.025f;

    while (g_initialized.load()) {
        if (g_fovOverrideEnabled.load()) {
            float current = g_currentFOV.load();
            float target = g_targetFOV.load();
            float delta = std::clamp(target - current, -MAX_STEP, MAX_STEP);
            float newFOV = current + delta;

            g_currentFOV.store(newFOV);
            ApplyFOVToGame(newFOV);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(0));
    }
}

void KeyPollLoop() {
    static bool togglePressedLastFrame = false;

    while (g_initialized.load()) {
        bool togglePressedThisFrame = (GetAsyncKeyState(TOGGLE_KEY) & 0x8000) != 0;

        if (togglePressedThisFrame && !togglePressedLastFrame) {
            bool currentToggleState = g_fovOverrideEnabled.load();
            g_fovOverrideEnabled.store(!currentToggleState);

            if (!currentToggleState) {
                float currentFov = DEFAULT_FOV;
                ReadGameFOV(currentFov);
                g_currentFOV.store(currentFov);

                g_targetFOV.store(g_lastFOV);

                PatchSetFovCall(true);
            }
            else {
                g_lastFOV = g_targetFOV.load();
                ReadGameFOV(g_gameFOV);
                g_targetFOV.store(g_gameFOV);
                PatchSetFovCall(false);
            }

            Log(std::string("FOV Override ") + (g_fovOverrideEnabled.load() ? "Enabled" : "Disabled"));
        }

        togglePressedLastFrame = togglePressedThisFrame;

        if (g_fovOverrideEnabled.load()) {
            if (GetAsyncKeyState(INCREASE_KEY) & 0x8000) {
                g_targetFOV.store((std::min)(MAX_FOV, g_targetFOV.load() + FOV_CH_RATE));
                SaveFOV(g_targetFOV.load());
                Log("FOV Increased: " + std::to_string(g_targetFOV.load()));
            }
            if (GetAsyncKeyState(DECREASE_KEY) & 0x8000) {
                g_targetFOV.store((std::max)(MIN_FOV, g_targetFOV.load() - FOV_CH_RATE));
                SaveFOV(g_targetFOV.load());
                Log("FOV Decreased: " + std::to_string(g_targetFOV.load()));
            }
            if (GetAsyncKeyState(RESET_KEY) & 0x8000) {
                g_targetFOV.store(DEFAULT_FOV);
                SaveFOV(DEFAULT_FOV);
                Log("FOV Reset to Default: " + std::to_string(DEFAULT_FOV));
            }
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}
