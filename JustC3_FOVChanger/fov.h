#pragma once

void SmoothFOVLoop();
void KeyPollLoop();
bool ApplyFOVToGame(float newFOV);
bool ReadGameFOV(float& outFov);
float LoadFOV();
void SaveFOV(float fov);
float DegToRad(float degrees);
