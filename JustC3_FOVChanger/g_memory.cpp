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

bool PatchSetFovCall(bool enable) {
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
