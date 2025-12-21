#include "pch.h"

bool IsValidAddress(const void* address) {
    if (!address) return false;
    MEMORY_BASIC_INFORMATION mbi;
    return VirtualQuery(address, &mbi, sizeof(mbi)) &&
        mbi.State == MEM_COMMIT &&
        !(mbi.Protect & PAGE_GUARD) &&
        !(mbi.Protect & PAGE_NOACCESS) &&
        (mbi.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_READWRITE | PAGE_READONLY));
}

bool PatchMemory(void* address, const void* data, size_t size) {
    if (!IsValidAddress(address)) return false;
    DWORD oldProtect;
    if (VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        memcpy(address, data, size);
        VirtualProtect(address, size, oldProtect, &oldProtect);
        return true;
    }
    return false;
}

bool NopMemory(uintptr_t address, size_t size) {
    if (!IsValidAddress((void*)address)) return false;
    BYTE* nops = new BYTE[size];
    memset(nops, NOP_INSTRUCTION, size);
    bool result = PatchMemory((void*)address, nops, size);
    delete[] nops;
    return result;
}

uintptr_t GetModuleSize(HMODULE hModule) {
    MODULEINFO mi;
    if (GetModuleInformation(GetCurrentProcess(), hModule, &mi, sizeof(mi))) {
        return (uintptr_t)mi.SizeOfImage;
    }
    return 0;
}

uintptr_t FindPattern(const std::vector<BYTE>& pattern, const std::string& mask) {
    uintptr_t base = g_gameBaseAddr;
    uintptr_t size = GetModuleSize((HMODULE)base);

    for (uintptr_t i = base; i < base + size - pattern.size(); i++) {
        bool found = true;
        for (size_t j = 0; j < pattern.size(); j++) {
            if (mask[j] != '?' && pattern[j] != *(BYTE*)(i + j)) {
                found = false;
                break;
            }
        }
        if (found) return i;
    }
    return 0;
}