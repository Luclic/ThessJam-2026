#pragma once
#include "raylib.h"
#include <algorithm>

inline void UpdateAndRenderMenu(int& currentState, float& mainMusicVolume, bool& triggerShiftStart) {
    Vector2 mousePos = GetMousePosition();
    
    // Check both pressed and released to ensure we catch fast clicks
    bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Subtle vignette overlay
    DrawRectangle(0, 0, screenW, screenH, {0, 0, 0, 100});

    const char* title = "MUSEUM TECH SUPPORT";
    int titleW = MeasureText(title, 50);
    DrawText(title, screenW/2 - titleW/2 + 3, screenH/3 - 50 + 3, 50, BLACK); // Drop shadow
    DrawText(title, screenW/2 - titleW/2, screenH/3 - 50, 50, WHITE);

    if (currentState == 0) { // STATE_MENU
        
        // --- KEYBOARD FALLBACK ---
        if (IsKeyPressed(KEY_ENTER)) {
            triggerShiftStart = true;
        }

        // --- START BUTTON ---
        // Made the button slightly taller (60) to ensure easy clicking
        Rectangle startBtn = { (float)screenW/2 - 150, (float)screenH/2, 300, 60 };
        bool startHover = CheckCollisionPointRec(mousePos, startBtn);
        
        // Debug border: If you don't see this box around the text, your screen scaling is off!
        DrawRectangleLinesEx(startBtn, 2, startHover ? GREEN : Fade(LIGHTGRAY, 0.3f));

        Color startColor = startHover ? GREEN : LIGHTGRAY;
        DrawText("START SHIFT", screenW/2 - MeasureText("START SHIFT", 30)/2, startBtn.y + 15, 30, startColor);
        
        if (startHover && clicked) {
            triggerShiftStart = true; 
        }

        // --- OPTIONS BUTTON ---
        Rectangle optBtn = { (float)screenW/2 - 150, (float)screenH/2 + 80, 300, 60 };
        bool optHover = CheckCollisionPointRec(mousePos, optBtn);
        
        DrawRectangleLinesEx(optBtn, 2, optHover ? SKYBLUE : Fade(LIGHTGRAY, 0.3f));

        Color optColor = optHover ? SKYBLUE : LIGHTGRAY;
        DrawText("OPTIONS", screenW/2 - MeasureText("OPTIONS", 30)/2, optBtn.y + 15, 30, optColor);
        
        if (optHover && clicked) {
            currentState = 1; // STATE_OPTIONS
        }
        
        DrawText("Press ENTER to Start | Use WASD to run around the lobby!", 15, screenH - 30, 16, GRAY);
    } 
    else if (currentState == 1) { // STATE_OPTIONS
        const char* optTitle = "- OPTIONS -";
        DrawText(optTitle, screenW/2 - MeasureText(optTitle, 30)/2, screenH/2 - 40, 30, LIGHTGRAY);

        const char* volText = TextFormat("Main Volume: %d%%", (int)(mainMusicVolume * 100));
        DrawText(volText, screenW/2 - MeasureText(volText, 25)/2, screenH/2 + 20, 25, YELLOW);
        
        Rectangle volDown = { (float)screenW/2 - MeasureText(volText, 25)/2 - 50, (float)screenH/2 + 15, 40, 40 };
        bool downHover = CheckCollisionPointRec(mousePos, volDown);
        DrawRectangleLinesEx(volDown, 2, downHover ? WHITE : Fade(GRAY, 0.5f));
        DrawText("<", volDown.x + 10, volDown.y + 5, 30, downHover ? WHITE : GRAY);
        if (downHover && clicked) mainMusicVolume = std::max(0.0f, mainMusicVolume - 0.1f);

        Rectangle volUp = { (float)screenW/2 + MeasureText(volText, 25)/2 + 10, (float)screenH/2 + 15, 40, 40 };
        bool upHover = CheckCollisionPointRec(mousePos, volUp);
        DrawRectangleLinesEx(volUp, 2, upHover ? WHITE : Fade(GRAY, 0.5f));
        DrawText(">", volUp.x + 10, volUp.y + 5, 30, upHover ? WHITE : GRAY);
        if (upHover && clicked) mainMusicVolume = std::min(1.0f, mainMusicVolume + 0.1f);

        Rectangle backBtn = { (float)screenW/2 - 150, (float)screenH/2 + 100, 300, 60 };
        bool backHover = CheckCollisionPointRec(mousePos, backBtn);
        
        DrawRectangleLinesEx(backBtn, 2, backHover ? RED : Fade(LIGHTGRAY, 0.3f));

        Color backColor = backHover ? RED : LIGHTGRAY;
        DrawText("BACK", screenW/2 - MeasureText("BACK", 30)/2, backBtn.y + 15, 30, backColor);
        if (backHover && clicked) currentState = 0; 
    }
}