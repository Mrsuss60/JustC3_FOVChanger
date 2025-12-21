#include "pch.h"

static std::string GetConfigPath() {
    char exePath[MAX_PATH];
    GetModuleFileNameA(nullptr, exePath, MAX_PATH);
    std::string path = exePath;
    size_t pos = path.find_last_of("\\/");
    if (pos != std::string::npos) path = path.substr(0, pos + 1);
    return path + "JustC3-FOVChanger.cfg";
}

static time_t g_lastConfigModTime = 0;

bool HasConfigChanged() {
    struct stat fileStat;
    if (stat(GetConfigPath().c_str(), &fileStat) == 0) {
        if (fileStat.st_mtime != g_lastConfigModTime) {
            return true;
        }
    }
    return false;
}

void SaveConfig() {
    std::string path = GetConfigPath();
    std::ofstream f(path);
    if (!f.is_open()) return;

    f << "# FOV can be set from " << MIN_FOV << " to " << MAX_FOV << "\n";
    f << "FOV=" << g_currentFOV.load() << "\n\n";

    f << "# Stepping Factor (0.1 - 2.5) FOV changing speed\n";
    f << "Step=" << g_Step << "\n\n";

    f << "# VK keys (you can find them here https://github.com/sepehrsohrabi/Decimal-Virtual-Key-Codes)\n";
    f << "TOGGLE_KEY=" << g_ToggleKey << "\n";
    f << "INCREASE_KEY=" << g_IncreaseKey << "\n";
    f << "DECREASE_KEY=" << g_DecreaseKey << "\n";
    f.close();

    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) == 0) {
        g_lastConfigModTime = fileStat.st_mtime;
    }
}

void LoadConfig() {
    std::string path = GetConfigPath();
    std::ifstream f(path);
    if (!f.is_open()) {
        SaveConfig();
        return;
    }

    std::string line;
    while (std::getline(f, line)) {
        if (line.empty() || line[0] == '#' || line[0] == '|') continue;
        size_t eq = line.find('=');
        if (eq == std::string::npos) continue;

        std::string key = line.substr(0, eq);
        std::string val = line.substr(eq + 1);

        key.erase(0, key.find_first_not_of(" \t"));
        if (key.find_last_not_of(" \t") != std::string::npos) key.erase(key.find_last_not_of(" \t") + 1);

        val.erase(0, val.find_first_not_of(" \t"));
        if (val.find_last_not_of(" \t") != std::string::npos) val.erase(val.find_last_not_of(" \t") + 1);

        try {
            if (key == "FOV") {
                float f = std::stof(val);
                g_currentFOV.store(std::clamp(f, MIN_FOV, MAX_FOV));
            }
            else if (key == "Step") {
                g_Step = std::clamp(std::stof(val), 0.1f, 2.5f);
            }
            else if (key == "TOGGLE_KEY") g_ToggleKey = std::stoi(val);
            else if (key == "INCREASE_KEY") g_IncreaseKey = std::stoi(val);
            else if (key == "DECREASE_KEY") g_DecreaseKey = std::stoi(val);
        }
        catch (...) {}
    }
    f.close();

    struct stat fileStat;
    if (stat(path.c_str(), &fileStat) == 0) {
        g_lastConfigModTime = fileStat.st_mtime;
    }
}