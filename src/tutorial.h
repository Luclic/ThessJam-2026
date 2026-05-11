#pragma once
#include "raylib.h"
#include "Entities.h"
#include <vector>

inline void UpdateAndRenderTutorial(int& currentState, const std::vector<Entity>& entities, int grabbedEntityIndex, float& shiftTimer, bool& tutorialFinished, int& tutorialStep, bool playerJumpedThisFrame) {
    
    // Debug Skip
    if (IsKeyPressed(KEY_ONE)) { tutorialStep = 5; }

    if (tutorialStep == 0) {
        if (playerJumpedThisFrame) tutorialStep = 1;
    } 
    else if (tutorialStep == 1) {
        if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_HANDBOOK)) tutorialStep = 2;
    }
    // Step 2 is advanced in Overlay.h when they close the book!
    else if (tutorialStep == 3) {
        for (const auto& e : entities) {
            if (e.HasTag(TAG_EYEWEAR) && e.HasTag(TAG_BROKEN)) { tutorialStep = 4; break; }
        }
    }
    else if (tutorialStep == 4) {
        bool brokenExists = false;
        for (const auto& e : entities) if (e.HasTag(TAG_EYEWEAR) && e.HasTag(TAG_BROKEN)) brokenExists = true;
        if (!brokenExists) tutorialStep = 5;
    }
    else if (tutorialStep == 5) {
        currentState = 3; // STATE_PLAYING
        shiftTimer = 0.0f;
        tutorialFinished = true; 
        return; 
    }

    int screenW = GetScreenWidth();
    DrawRectangle(0, 0, screenW, 60, {0, 0, 0, 200});
    DrawRectangle(0, 60, screenW, 5, {255, 255, 255, 100}); 
    
    const char* inst = "";
    switch(tutorialStep) {
        case 0: inst = "TUTORIAL: Use WASD to move. Press SPACE to jump."; break;
        case 1: inst = "TUTORIAL: Good. Now approach the table and press 'E' to grab the Employee Handbook."; break;
        case 2: inst = "TUTORIAL: Hold the Handbook and press 'F' to read it."; break;
        case 3: inst = "TUTORIAL: Now grab the Sunglasses. Press 'SPACE' to throw and break them!"; break;
        case 4: inst = "TUTORIAL: Oops! Grab the Painters Tape and approach the broken glasses. Press 'F' to repair."; break;
    }
    
    DrawText(inst, screenW/2 - MeasureText(inst, 20)/2, 20, 20, YELLOW);
    DrawText("WASD: Move | E: Grab/Drop | HOLD F: Use | SPACE: Throw/Jump | 1: Skip Tutorial", 15, GetScreenHeight() - 30, 16, LIGHTGRAY);
}