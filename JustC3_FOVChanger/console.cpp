#include "pch.h"

void Log(const std::string& msg) {
    if (ENABLE_CONSOLE) {
        std::cout << msg << std::endl;
    }
}

void InitConsole() {
    if (!ENABLE_CONSOLE) return;
    if (AllocConsole()) {
        FILE* fDummy;
        freopen_s(&fDummy, "CONIN$", "r", stdin);
        freopen_s(&fDummy, "CONOUT$", "w", stdout);
        freopen_s(&fDummy, "CONOUT$", "w", stderr);
        std::ios::sync_with_stdio(true);
    }
}

void FreeConsoleIfEnabled() {
    if (ENABLE_CONSOLE) FreeConsole();
}
