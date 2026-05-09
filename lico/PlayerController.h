#pragma once
#include "raylib.h"

// Handles movement, grabbing, throwing, and environment interaction (E, F, Space)
void UpdatePlayerInput(float dt);

// Handles the debug keys and door toggles (1-6, I, O, P, etc.)
void UpdateEnvironmentTriggers();