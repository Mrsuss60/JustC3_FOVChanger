#pragma once
#include <vector>
#include <windows.h>
#include <psapi.h> 

bool IsValidAddress(const void* address);
bool PatchMemory(void* address, const void* data, size_t size);
bool NopMemory(uintptr_t address, size_t size);

uintptr_t FindPattern(const std::vector<BYTE>& pattern, const std::string& mask);
uintptr_t GetModuleSize(HMODULE hModule);