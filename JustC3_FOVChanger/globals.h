#pragma once
#include <thread>
#include <atomic>
#include <string>
#include <fstream>
#include <iostream>
#include <chrono>
#include <Windows.h>

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

inline std::thread g_keyThread;
inline std::thread g_smoothThread;
inline std::atomic<float> g_currentFOV = DEFAULT_FOV;
inline std::atomic<float> g_targetFOV = DEFAULT_FOV;
inline float g_lastFOV = DEFAULT_FOV;
inline std::atomic<bool> g_fovOverrideEnabled = true;
inline std::atomic<bool> g_initialized = false;
inline BYTE* g_originalCallBytes = nullptr;
inline float g_gameFOV = DEFAULT_FOV;

constexpr uintptr_t g_cameraManagerAddr = 0x142ED0E20;
constexpr uintptr_t g_setFovFunc = 0x143AEFF41;
constexpr int g_currentCameraOff = 0x5c0;
constexpr int g_cameraFlagsOff = 0x55e;
constexpr int g_fovOff1 = 0x580;
constexpr int g_fovOff2 = 0x584;
