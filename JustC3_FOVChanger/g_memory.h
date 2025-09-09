#pragma once
#include <cstddef>

bool IsValidAddress(const void* address);
bool PatchMemory(void* address, const void* data, size_t size);
bool PatchSetFovCall(bool enable);
