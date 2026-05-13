#pragma once
#include "raylib.h"
#include "Evaluation.h"
#include <algorithm>

inline void UpdateAndRenderNews(int& currentState, const ShiftReport& lastReport, bool& triggerRetryNight, Font fontMuseum, Font fontEmployee) {
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float scale = std::min(screenW / 1280.0f, screenH / 960.0f);

    BeginDrawing();
    ClearBackground({235, 235, 230, 255}); // Old newspaper off-white

    // --- BREAKING NEWS BANNER ---
    DrawRectangle(0, 40 * scale, screenW, 100 * scale, {150, 20, 20, 255}); // Maroon-ish red
    const char* header = "BREAKING NEWS";
    Vector2 headerDims = MeasureTextEx(fontMuseum, header, 70 * scale, 2);
    DrawTextEx(fontMuseum, header, {screenW/2.0f - headerDims.x/2.0f, 55 * scale}, 70 * scale, 2, RAYWHITE);

    // --- MAIN HEADLINE ---
    const char* title = "YOU'RE FIRED.";
    Vector2 titleDims = MeasureTextEx(fontMuseum, title, 100 * scale, 2);
    DrawTextEx(fontMuseum, title, {screenW/2.0f - titleDims.x/2.0f, 170 * scale}, 100 * scale, 2, BLACK);

    // --- SUBTITLE (Final Verdict) ---
    const char* sub = lastReport.finalVerdict.c_str();
    Vector2 subDims = MeasureTextEx(fontEmployee, sub, 30 * scale, 1);
    DrawTextEx(fontEmployee, sub, {screenW/2.0f - subDims.x/2.0f, 280 * scale}, 30 * scale, 1, DARKGRAY);

    DrawLineEx({screenW * 0.1f, 330 * scale}, {screenW * 0.9f, 330 * scale}, 4 * scale, BLACK);

    // --- THE ARTICLES (Your Failure Conditions) ---
    float yOffset = 360 * scale;
    for (const auto& rev : lastReport.reviews) {
        // Publisher Name (e.g., "The Daily Myth")
        DrawTextEx(fontMuseum, rev.artifactName.c_str(), { screenW * 0.15f, yOffset }, 35 * scale, 1, {150, 20, 20, 255});
        
        // Headline (e.g., "BREAKING: LOCAL MUSEUM GUESTS PETRIFIED...")
        DrawTextEx(fontEmployee, rev.reviewText.c_str(), { screenW * 0.15f, yOffset + (45 * scale) }, 25 * scale, 1, BLACK);
        
        // Divider line between articles
        DrawLineEx({screenW * 0.15f, yOffset + (90 * scale)}, {screenW * 0.85f, yOffset + (90 * scale)}, 2 * scale, LIGHTGRAY);
        
        yOffset += 110 * scale;
    }

    // --- FOOTER PROMPT ---
    const char* enterText = "PRESS ENTER TO RETRY THE NIGHT";
    Vector2 textDims = MeasureTextEx(fontEmployee, enterText, 30 * scale, 0);
    
    // Blinking effect
    if ((int)(GetTime() * 2) % 2 == 0) {
        DrawTextEx(fontEmployee, enterText, {screenW / 2.0f - textDims.x / 2.0f, screenH - (80 * scale)}, 30 * scale, 0, {150, 20, 20, 255});
    }

    EndDrawing();

    // --- INPUT HANDLING ---
    if (IsKeyPressed(KEY_ENTER)) {
        triggerRetryNight = true;
    }
}