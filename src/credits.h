#pragma once
#include "raylib.h"
#include <string>
#include <vector>

inline void UpdateAndRenderCredits(int& currentState, Font fontMuseum, Font fontEmployee) {
    static float scrollY = 0.0f;
    
    // Reset scroll when entering the state for the first time
    static int lastState = -1;
    if (lastState != currentState) {
        scrollY = (float)GetScreenHeight();
        lastState = currentState;
    }

    float dt = GetFrameTime();
    scrollY -= 80.0f * dt; // Slow, cinematic upward scroll


    ClearBackground({15, 15, 20, 255}); // Match your sleek UI background

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float scale = std::min(screenW / 1280.0f, screenH / 960.0f);

    std::vector<std::string> lines = {
        "MYTHIC MAINTENANCE",
        "",
        "A game about cleaning up after the Gods.",
        "",
        "Created for Thess Game Jam 2026",
        "",
        "PROGRAMMING & SYSTEMS",
        "GRAPHICS & DESIGN",
        "AUDIO & MUSIC",
        "Lucas Lico & Michailidis Michail",
        "",
        "",
        "Thank you for playing."
    };

    float y = scrollY;
    for (size_t i = 0; i < lines.size(); i++) {
        float fontSize = (i == 0) ? 60.0f * scale : 30.0f * scale;
        Font f = (i == 0) ? fontMuseum : fontEmployee;
        Color c = (i == 0) ? Color{218, 165, 32, 255} : WHITE; // Gold title, white text

        if (!lines[i].empty()) {
            Vector2 textSize = MeasureTextEx(f, lines[i].c_str(), fontSize, 1);
            DrawTextEx(f, lines[i].c_str(), { screenW / 2.0f - textSize.x / 2.0f, y }, fontSize, 1, c);
        }
        y += fontSize + (20.0f * scale); // Spacing between lines
    }

    // Return to menu text (Blinking)
    const char* exitMsg = "PRESS ENTER TO RETURN TO MENU";
    float exitSize = 20.0f * scale;
    Vector2 exitVec = MeasureTextEx(fontEmployee, exitMsg, exitSize, 1);
    
    if ((int)(GetTime() * 2) % 2 == 0) {
        DrawTextEx(fontEmployee, exitMsg, { screenW / 2.0f - exitVec.x / 2.0f, screenH - (50.0f * scale) }, exitSize, 1, DARKGRAY);
    }

    // Go back to the main menu on Enter
    if (IsKeyPressed(KEY_ENTER)) {
        currentState = 0; // STATE_MENU is index 0
    }
}