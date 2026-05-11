#pragma once
#include "raylib.h"

inline void UpdateAndRenderOverlay(bool& isOverlayActive, Music& currentMainMusic, Music& overlayMusic, int& tutorialStep) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    DrawRectangle(0, 0, screenW, screenH, {0, 0, 0, 200});

    int bookW = 600; int bookH = 700;
    int startX = screenW / 2 - bookW / 2;
    int startY = screenH / 2 - bookH / 2;

    DrawRectangle(startX, startY, bookW, bookH, RAYWHITE);
    DrawRectangleLines(startX, startY, bookW, bookH, BLACK);

    const char* title = "EMPLOYEE HANDBOOK";
    DrawText(title, screenW/2 - MeasureText(title, 30)/2, startY + 40, 30, BLACK);
    
    DrawText("Welcome to the Museum of Time!", startX + 40, startY + 120, 20, DARKGRAY);
    DrawText("- Your shift ends at 8:00 AM (3 Minutes).", startX + 40, startY + 170, 20, DARKGRAY);
    DrawText("- The artifacts are unstable. Fix glitches quickly.", startX + 40, startY + 210, 20, DARKGRAY);
    DrawText("- DO NOT break the artifacts, or you will be fined.", startX + 40, startY + 250, 20, DARKGRAY);
    DrawText("- Duct tape and tools are your best friends.", startX + 40, startY + 290, 20, DARKGRAY);

    const char* closeText = "Press 'F' or 'ESC' to close and resume shift.";
    DrawText(closeText, screenW/2 - MeasureText(closeText, 20)/2, startY + bookH - 50, 20, GRAY);

    // --- OVERLAY LOGIC ---
    if (IsKeyPressed(KEY_F) || IsKeyPressed(KEY_ESCAPE)) {
        isOverlayActive = false;
        StopMusicStream(overlayMusic);
        ResumeMusicStream(currentMainMusic); 
        if (tutorialStep == 2) tutorialStep = 3; // Advance tutorial upon closing!
    }
}