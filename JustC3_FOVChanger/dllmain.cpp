#include "pch.h"

constexpr bool ENABLE_CONSOLE = false;

constexpr float DEFAULT_FOV = 34.89f;
constexpr float MIN_FOV = 1.0f;
constexpr float MAX_FOV = 120.0f;
constexpr float FOV_CH_RATE = 0.2f;

constexpr int TOGGLE_KEY = VK_F7;
constexpr int INCREASE_KEY = VK_F5;
constexpr int DECREASE_KEY = VK_F6;
constexpr int RESET_KEY = VK_F8;

constexpr size_t INSTRUCTION_PATCH_SIZE = 5;
constexpr BYTE NOP_INSTRUCTION = 0x90;

static std::thread g_keyThread;
static std::thread g_smoothThread;
static std::atomic<float> g_currentFOV = DEFAULT_FOV;
static std::atomic<float> g_targetFOV = DEFAULT_FOV;
static float g_lastFOV = DEFAULT_FOV;
static std::atomic<bool> g_fovOverrideEnabled = true;
static std::atomic<bool> g_initialized = false;
static BYTE* g_originalCallBytes = nullptr;
static float g_gameFOV = DEFAULT_FOV;

constexpr uintptr_t g_cameraManagerAddr = 0x142ED0E20;
constexpr uintptr_t g_setFovFunc = 0x143AEFF41;
constexpr int g_currentCameraOff = 0x5c0;
constexpr int g_cameraFlagsOff = 0x55e;
constexpr int g_fovOff1 = 0x580;
constexpr int g_fovOff2 = 0x584;

static void Log(const std::string& msg) {
    if (ENABLE_CONSOLE) {
        std::cout << msg << std::endl;
    }
}

static void SaveFOV(float fov) {
    std::ofstream out("JustC3-FOVChanger.cfg", std::ios::trunc);
    if (out) out << "FOV value = " << fov;
}

static float LoadFOV() {
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

static void InitConsole() {
    if (!ENABLE_CONSOLE) return;
    if (AllocConsole()) {
        FILE* fDummy;
        freopen_s(&fDummy, "CONIN$", "r", stdin);
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        std::ios::sync_with_stdio(true);
    }
}

static void FreeConsoleIfEnabled() {
    if (ENABLE_CONSOLE) FreeConsole();
}

static float DegToRad(float degrees) { return degrees * (3.14159265359f / 180.0f); }

static bool IsValidAddress(const void* address) {
    if (!address) return false;
    MEMORY_BASIC_INFORMATION mbi;
    return VirtualQuery(address, &mbi, sizeof(mbi)) &&
        mbi.State == MEM_COMMIT &&
        !(mbi.Protect & PAGE_GUARD) &&
        !(mbi.Protect & PAGE_NOACCESS) &&
        (mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_READWRITE | PAGE_READONLY));
}

static bool PatchMemory(void* address, const void* data, size_t size) {
    if (!IsValidAddress(address)) return false;
    DWORD oldProtect;
    if (VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(address, data, size);
        VirtualProtect(address, size, oldProtect, &oldProtect);
        return true;
    }
    return false;
}

static bool PatchSetFovCall(bool enable) {
    void* callAddress = (void*)g_setFovFunc;
    if (enable) {
        BYTE nops[INSTRUCTION_PATCH_SIZE];
        memset(nops, NOP_INSTRUCTION, INSTRUCTION_PATCH_SIZE);
        return PatchMemory(callAddress, nops, INSTRUCTION_PATCH_SIZE);
    }
    else if (g_originalCallBytes) {
        return PatchMemory(callAddress, g_originalCallBytes, INSTRUCTION_PATCH_SIZE);
    }
    return false;
}

static bool ApplyFOVToGame(float newFOV) {
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

static void SmoothFOVLoop() {
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



static bool ReadGameFOV(float& outFov) {
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

static void KeyPollLoop() {
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

static BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID) {
    if (reason == DLL_PROCESS_ATTACH) {
        DisableThreadLibraryCalls(hModule);
        std::thread(InitializeFOVHook).detach();
    }
    else if (reason == DLL_PROCESS_DETACH) {
        CleanupFOVHook();
    }
    return TRUE;
}
