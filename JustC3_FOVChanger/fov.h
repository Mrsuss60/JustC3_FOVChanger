#pragma once

void KeyPollLoop();
bool ApplyFOVToGame(float newFOV);
bool ReadGameFOV(float& outFov);
float DegToRad(float degrees);