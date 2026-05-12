#pragma once
#include "raylib.h"
#include <algorithm>

template <typename T>
inline void UpdateAndRenderMenu(T& currentState, float& mainMusicVolume, bool& triggerShiftStart, Font fontMuseum, Font fontEmployee, int currentNight) {
    Vector2 mousePos = GetMousePosition();
    bool clicked = IsMouseButtonPressed(MOUSE_LEFT_BUTTON) || IsMouseButtonReleased(MOUSE_LEFT_BUTTON);

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // --- DYNAMIC SCALING MULTIPLIERS ---
    // Calculate sizes as a percentage of the screen height (mostly) and width
    float leftMargin = screenW * 0.08f; 
    float titleY = screenH * 0.15f;
    float buttonsY = screenH * 0.60f;

    float title1Size = screenH * 0.14f;  // Large title
    float title2Size = screenH * 0.05f;  // Subtitle
    float fontSizeBtn = screenH * 0.06f; // Buttons
    float footerSize = screenH * 0.025f; // Bottom hint text

    float btnHeight = screenH * 0.08f;
    float btnWidth = screenW * 0.25f;    // Button width scales with screen width
    float btnSpacing = screenH * 0.03f;
    float strokeWidth = screenW * 0.003f; // The highlight lines on buttons

    // --- UI BACKDROP ---
    DrawRectangleGradientH(0, 0, screenW * 0.50f, screenH, Fade(BLACK, 0.8f), BLANK);
    
    Color warningOrange = { 255, 150, 0, 255 }; 

    if (currentState == 0) { // STATE_MENU
        
        if (IsKeyPressed(KEY_ENTER)) {
            triggerShiftStart = true;
        }

        // --- TITLE: MYTHIC ---
        const char* title1 = "Mythic";
        Vector2 title1Pos = { leftMargin, titleY };
        Vector2 title1Shadow = { leftMargin + (screenW * 0.003f), titleY + (screenH * 0.005f) };
        
        DrawTextEx(fontMuseum, title1, title1Shadow, title1Size, 2, Fade(BLACK, 0.8f));
        DrawTextEx(fontMuseum, title1, title1Pos, title1Size, 2, RAYWHITE);

        // --- TITLE: MAINTENANCE ---
        const char* title2 = "MAINTENANCE";
        Vector2 title2Pos = { leftMargin + (screenW * 0.01f), titleY + title1Size - (screenH * 0.02f) };
        Vector2 title2Shadow = { title2Pos.x + (screenW * 0.002f), title2Pos.y + (screenH * 0.003f) };
        
        Vector2 title2Dims = MeasureTextEx(fontEmployee, title2, title2Size, 0);
        float padding = screenH * 0.01f;
        DrawRectangle(title2Pos.x - padding, title2Pos.y + padding, title2Dims.x + (padding*2), title2Dims.y - (padding*2), Fade(BLACK, 0.7f));
        
        DrawTextEx(fontEmployee, title2, title2Shadow, title2Size, 0, Fade(BLACK, 0.9f));
        DrawTextEx(fontEmployee, title2, title2Pos, title2Size, 0, warningOrange);

        // --- BUTTONS ---
        
        // START BUTTON
        Rectangle startBtn = { leftMargin, buttonsY, btnWidth, btnHeight };
        bool startHover = CheckCollisionPointRec(mousePos, startBtn);
        
        if (startHover) {
            DrawRectangleRec({startBtn.x - (padding*2), startBtn.y, startBtn.width, startBtn.height}, Fade(warningOrange, 0.2f));
            DrawRectangleRec({startBtn.x - (padding*2), startBtn.y, strokeWidth, startBtn.height}, warningOrange);
        }
        
        Color startColor = startHover ? RAYWHITE : LIGHTGRAY;
        const char* startText;
        if (currentNight > 1) {
            startText = TextFormat("CONTINUE (NIGHT %d)", currentNight);
        } else {
            startText = "START SHIFT";
        }
        DrawTextEx(fontMuseum, startText, { startBtn.x, startBtn.y }, fontSizeBtn, 1, startColor);
        
        if (startHover && clicked) triggerShiftStart = true; 

        // OPTIONS BUTTON
        Rectangle optBtn = { leftMargin, buttonsY + btnHeight + btnSpacing, btnWidth, btnHeight };
        bool optHover = CheckCollisionPointRec(mousePos, optBtn);
        
        if (optHover) {
            DrawRectangleRec({optBtn.x - (padding*2), optBtn.y, optBtn.width, optBtn.height}, Fade(SKYBLUE, 0.2f));
            DrawRectangleRec({optBtn.x - (padding*2), optBtn.y, strokeWidth, optBtn.height}, SKYBLUE);
        }

        Color optColor = optHover ? RAYWHITE : LIGHTGRAY;
        DrawTextEx(fontMuseum, "Options", { optBtn.x, optBtn.y }, fontSizeBtn, 1, optColor);
        
        if (optHover && clicked) currentState = (T)1; 
        
        // FOOTER
        DrawTextEx(fontEmployee, "Press ENTER to Start | WASD to move", { leftMargin, (float)screenH - (screenH * 0.05f) }, footerSize, 0, GRAY);
    } 
    else if (currentState == (T)1) { // STATE_OPTIONS
        
        float optTitleSize = screenH * 0.09f;
        const char* optTitle = "Options";
        Vector2 optTitleDims = MeasureTextEx(fontMuseum, optTitle, optTitleSize, 2);
        DrawTextEx(fontMuseum, optTitle, { screenW/2.0f - optTitleDims.x/2.0f, screenH * 0.25f }, optTitleSize, 2, RAYWHITE);

        float volTextSize = screenH * 0.045f;
        const char* volText = TextFormat("Main Volume: %d%%", (int)(mainMusicVolume * 100));
        Vector2 volDims = MeasureTextEx(fontEmployee, volText, volTextSize, 0);
        float volY = screenH/2.0f - (screenH * 0.02f);
        DrawTextEx(fontEmployee, volText, { screenW/2.0f - volDims.x/2.0f, volY }, volTextSize, 0, warningOrange);
        
        // Volume Controls
        float ctrlSize = screenH * 0.05f;
        Rectangle volDown = { screenW/2.0f - volDims.x/2.0f - (screenW * 0.05f), volY - (screenH * 0.005f), ctrlSize, ctrlSize };
        bool downHover = CheckCollisionPointRec(mousePos, volDown);
        DrawRectangleLinesEx(volDown, 2, downHover ? warningOrange : Fade(GRAY, 0.5f));
        // Centering the < and > inside the boxes dynamically
        Vector2 arrowDownDims = MeasureTextEx(fontEmployee, "<", volTextSize, 0);
        DrawTextEx(fontEmployee, "<", { volDown.x + (ctrlSize/2) - (arrowDownDims.x/2), volDown.y + (ctrlSize/2) - (arrowDownDims.y/2) }, volTextSize, 0, downHover ? warningOrange : GRAY);
        if (downHover && clicked) mainMusicVolume = std::max(0.0f, mainMusicVolume - 0.1f);

        Rectangle volUp = { screenW/2.0f + volDims.x/2.0f + (screenW * 0.02f), volY - (screenH * 0.005f), ctrlSize, ctrlSize };
        bool upHover = CheckCollisionPointRec(mousePos, volUp);
        DrawRectangleLinesEx(volUp, 2, upHover ? warningOrange : Fade(GRAY, 0.5f));
        Vector2 arrowUpDims = MeasureTextEx(fontEmployee, ">", volTextSize, 0);
        DrawTextEx(fontEmployee, ">", { volUp.x + (ctrlSize/2) - (arrowUpDims.x/2), volUp.y + (ctrlSize/2) - (arrowUpDims.y/2) }, volTextSize, 0, upHover ? warningOrange : GRAY);
        if (upHover && clicked) mainMusicVolume = std::min(1.0f, mainMusicVolume + 0.1f);

        // BACK BUTTON
        float backBtnW = screenW * 0.15f;
        Rectangle backBtn = { screenW/2.0f - backBtnW/2.0f, screenH/2.0f + (screenH * 0.1f), backBtnW, btnHeight };
        bool backHover = CheckCollisionPointRec(mousePos, backBtn);
        
        if (backHover) {
            DrawRectangleRec({backBtn.x - (screenW * 0.01f), backBtn.y, strokeWidth, backBtn.height}, RED);
            DrawRectangleRec(backBtn, Fade(RED, 0.2f));
        }

        Color backColor = backHover ? RAYWHITE : LIGHTGRAY;
        Vector2 backDims = MeasureTextEx(fontMuseum, "Return", fontSizeBtn, 1);
        DrawTextEx(fontMuseum, "Return", { screenW/2.0f - backDims.x/2.0f, backBtn.y }, fontSizeBtn, 1, backColor);
        
        if (backHover && clicked) currentState = (T)0; 
    }
}