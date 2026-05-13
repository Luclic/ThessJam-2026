#pragma once
#include "raylib.h"
#include "Entities.h"
#include <vector>
#include <algorithm>

inline void UpdateAndRenderTutorial(int& currentState, const std::vector<Entity>& entities, int grabbedEntityIndex, int equippedEyewear, float& shiftTimer, bool& tutorialFinished, int& tutorialStep, bool playerJumpedThisFrame, Font fontMuseum, Font fontEmployee) {
    
    // Debug Skip
    if (IsKeyPressed(KEY_ONE)) { tutorialStep = 5; }

    if (tutorialStep == 0) {
        if (playerJumpedThisFrame) tutorialStep = 1;
    } 
    else if (tutorialStep == 1) {
        if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_HANDBOOK)) tutorialStep = 2;
    }
    else if (tutorialStep == 2) {
        // BUG FIX: If they drop or throw the handbook before reading it, revert to step 1!
        if (grabbedEntityIndex == -1 || !entities[grabbedEntityIndex].HasTag(TAG_HANDBOOK)) {
            tutorialStep = 1;
        }
        // Note: Step 2 successfully advances to 3 inside Overlay.h when they close the book.
    }
    else if (tutorialStep == 3) {
        // Wait for the player to WEAR the sunglasses (F)
        if (equippedEyewear != -1) {
            tutorialStep = 4;
        }
    }
    else if (tutorialStep == 4) {
        // INSTANT COMPLETION: No waiting for an extra frame.
        if (IsKeyPressed(KEY_ENTER)) {
            currentState = 3; // STATE_PLAYING
            shiftTimer = 0.0f;
            tutorialFinished = true; 
            return; 
        }
    }
    else if (tutorialStep == 5) {
        currentState = 3; // STATE_PLAYING
        shiftTimer = 0.0f;
        tutorialFinished = true; 
        return; 
    }

    // --- DYNAMIC SCALING ---
    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float scale = std::min(screenW / 1280.0f, screenH / 960.0f);
    
    float barHeight = 80.0f * scale;
    float titleSize = 30.0f * scale;
    float instSize = 24.0f * scale;
    float bottomSize = 18.0f * scale;

    // --- TOP BAR RENDERING ---
    DrawRectangle(0, 0, (int)screenW, (int)barHeight, {15, 15, 20, 230}); 
    DrawRectangle(0, (int)barHeight, (int)screenW, (int)(4.0f * scale), {218, 165, 32, 255}); // Golden accent
    
    const char* titleText = "Tutorial";
    Vector2 titleVec = MeasureTextEx(fontMuseum, titleText, titleSize, 1);
    DrawTextEx(fontMuseum, titleText, { 30.0f * scale, barHeight/2.0f - titleVec.y/2.0f }, titleSize, 1, {218, 165, 32, 255}); 

    const char* inst = "";
    switch(tutorialStep) {
        case 0: inst = "Use WASD to move around the museum. Press SPACE to jump."; break;
        case 1: inst = "Good. Now approach the table and press 'E' to grab the Employee Handbook."; break;
        case 2: inst = "Press 'F' to read it. (Time freezes while reading. Find more magazines about the museum around)"; break;
        case 3: inst = "Excellent. Now grab the Sunglasses and press 'F' to wear them."; break;
        case 4: inst = "You are now ready. Press ENTER to start your first night shift! Your shift is 3 mins: 12 AM to 8 AM"; break;
    }
    
    Vector2 instVec = MeasureTextEx(fontEmployee, inst, instSize, 1);
    DrawTextEx(fontEmployee, inst, { screenW/2.0f - instVec.x/2.0f, barHeight/2.0f - instVec.y/2.0f }, instSize, 1, WHITE);

    // --- BOTTOM CONTROLS RENDERING ---
    const char* bottomText = "WASD: Move  |  E: Grab/Drop  |  F: Use/Read  |  SPACE: Throw/Jump  |  1: Skip Tutorial";
    Vector2 bottomVec = MeasureTextEx(fontEmployee, bottomText, bottomSize, 1);
    
    float pillPaddingX = 30.0f * scale, pillPaddingY = 15.0f * scale;
    Rectangle pillRec = { screenW/2.0f - bottomVec.x/2.0f - pillPaddingX, screenH - bottomVec.y - (40.0f * scale) - pillPaddingY, bottomVec.x + (pillPaddingX * 2.0f), bottomVec.y + (pillPaddingY * 2.0f) };
    DrawRectangleRounded(pillRec, 0.5f, 10, {10, 10, 15, 200});
    
    DrawTextEx(fontEmployee, bottomText, { screenW/2.0f - bottomVec.x/2.0f, pillRec.y + pillPaddingY }, bottomSize, 1, LIGHTGRAY);
}