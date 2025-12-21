#pragma once
#include <vector>
#include <atomic>
#include <thread>

constexpr bool ENABLE_CONSOLE = false;

constexpr float MIN_FOV = 1.0f;
constexpr float MAX_FOV = 120.0f;
constexpr float DEFAULT_FOV = 34.89f;

inline float g_Step = 0.25f;
inline int g_ToggleKey = 118;
inline int g_IncreaseKey = 116;
inline int g_DecreaseKey = 117;

constexpr BYTE NOP_INSTRUCTION = 0x90;

inline std::thread g_keyThread;
inline std::atomic<float> g_currentFOV = DEFAULT_FOV;
inline float g_lastFOV = DEFAULT_FOV;
inline std::atomic<bool> g_fovOverrideEnabled = true;
inline std::atomic<bool> g_initialized = false;
inline float g_gameFOV = DEFAULT_FOV;

inline uintptr_t g_gameBaseAddr = 0;
inline uintptr_t g_Camera_FOV_Addr = 0;

inline uintptr_t g_patchAddr1 = 0;
inline uintptr_t g_patchAddr2 = 0;
inline uintptr_t g_patchAddr3 = 0;

inline std::vector<BYTE> g_backupBytes1;
inline std::vector<BYTE> g_backupBytes2;
inline std::vector<BYTE> g_backupBytes3;


inline std::vector<BYTE> PATTERN_1 = { 0xF3, 0x0F, 0x11, 0x88, 0x80, 0x05, 0x00, 0x00 };
inline std::string MASK_1 = "xxxxxxxx";

inline std::vector<BYTE> PATTERN_2 = { 0x44, 0x89, 0x9F, 0x80, 0x05, 0x00, 0x00 };
inline std::string MASK_2 = "xxxxxxx";

inline std::vector<BYTE> PATTERN_3 = { 0xF3, 0x0F, 0x11, 0x88, 0x84, 0x05, 0x00, 0x00 };
inline std::string MASK_3 = "xxxxxxxx";

constexpr int g_currentCameraOff = 0x5c0;
constexpr int g_cameraFlagsOff = 0x55e;
constexpr int g_fovOff1 = 0x580;
constexpr int g_fovOff2 = 0x584;

constexpr uintptr_t OFFSET_CAM_NEW = 0x2ED0E20;
constexpr uintptr_t OFFSET_CAM_OLD = 0x2ED0E20;