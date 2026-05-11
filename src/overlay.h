#pragma once
#include "raylib.h"
#include <string>

inline void UpdateAndRenderOverlay(bool& isOverlayActive, Music& currentMainMusic, Music& overlayMusic, int& tutorialStep, std::string docName) {
    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    DrawRectangle(0, 0, screenW, screenH, {0, 0, 0, 200});

    int bookW = 600; int bookH = 700;
    int startX = screenW / 2 - bookW / 2;
    int startY = screenH / 2 - bookH / 2;

    DrawRectangle(startX, startY, bookW, bookH, RAYWHITE);
    DrawRectangleLines(startX, startY, bookW, bookH, BLACK);

    // --- DYNAMIC CONTENT BASED ON WHAT WE GRABBED ---
    if (docName.find("Handbook") != std::string::npos) {
        DrawText("EMPLOYEE HANDBOOK", screenW/2 - MeasureText("EMPLOYEE HANDBOOK", 30)/2, startY + 40, 30, BLACK);
        DrawText("Welcome to the Museum of Time!", startX + 40, startY + 120, 20, DARKGRAY);
        DrawText("- Your shift ends at 8:00 AM (3 Minutes).", startX + 40, startY + 170, 20, DARKGRAY);
        DrawText("- The artifacts are unstable. Fix glitches quickly.", startX + 40, startY + 210, 20, DARKGRAY);
        DrawText("- DO NOT break the artifacts, or you will be fined.", startX + 40, startY + 250, 20, DARKGRAY);
        DrawText("- Duct tape and tools are your best friends.", startX + 40, startY + 290, 20, DARKGRAY);
    } 
    else if (docName.find("Brochure 1") != std::string::npos) {
        DrawText("EXHIBIT 1: GREEK MYTHOLOGY", screenW/2 - MeasureText("EXHIBIT 1: GREEK MYTHOLOGY", 30)/2, startY + 40, 30, BLACK);
        DrawText("- Do NOT look directly at Medusa. Wear protection.", startX + 40, startY + 120, 20, DARKGRAY);
        DrawText("- Keep water away from the Sisyphus Boulder.", startX + 40, startY + 170, 20, DARKGRAY);
        DrawText("- The Chalice is prone to leaking.", startX + 40, startY + 220, 20, DARKGRAY);
    }
    else if (docName.find("Brochure 2") != std::string::npos) {
        DrawText("EXHIBIT 2: THE HALL OF WINDS", screenW/2 - MeasureText("EXHIBIT 2: THE HALL OF WINDS", 30)/2, startY + 40, 30, BLACK);
        DrawText("- Aeolus's Bag contains Gale-Force winds.", startX + 40, startY + 120, 20, DARKGRAY);
        DrawText("- Secure heavy items, or plug the bag.", startX + 40, startY + 170, 20, DARKGRAY);
        DrawText("- Hermes's Sandals are known to take flight.", startX + 40, startY + 220, 20, DARKGRAY);
    }
    else if (docName.find("Brochure 3") != std::string::npos) {
        DrawText("EXHIBIT 3: EGYPTIAN SANDS", screenW/2 - MeasureText("EXHIBIT 3: EGYPTIAN SANDS", 30)/2, startY + 40, 30, BLACK);
        DrawText("- Ra's Sun Disk reaches extreme temperatures.", startX + 40, startY + 120, 20, DARKGRAY);
        DrawText("- The Mummy is restless and seeks electricity.", startX + 40, startY + 170, 20, DARKGRAY);
        DrawText("- Zeus's Lightning is highly volatile when wet.", startX + 40, startY + 220, 20, DARKGRAY);
    }
    else if (docName.find("Brochure 4") != std::string::npos) {
        DrawText("EXHIBIT 4: NORDIC LEGENDS", screenW/2 - MeasureText("EXHIBIT 4: NORDIC LEGENDS", 30)/2, startY + 40, 30, BLACK);
        DrawText("- Mjolnir cannot be lifted by mortals.", startX + 40, startY + 120, 20, DARKGRAY);
        DrawText("- Gleipnir's Ribbon will bind anything it catches.", startX + 40, startY + 170, 20, DARKGRAY);
        DrawText("- The Banshee Stone emits chaotic energy pulses.", startX + 40, startY + 220, 20, DARKGRAY);
    }
    else if (docName.find("Brochure 5") != std::string::npos) {
        DrawText("EXHIBIT 5: PANDORA'S BOX", screenW/2 - MeasureText("EXHIBIT 5: PANDORA'S BOX", 30)/2, startY + 40, 30, RED);
        DrawText("- WARNING: DO NOT OPEN.", startX + 40, startY + 120, 20, DARKGRAY);
        DrawText("- If the seal breaks, tape it immediately.", startX + 40, startY + 170, 20, DARKGRAY);
        DrawText("- Prepare for all artifacts to destabilize.", startX + 40, startY + 220, 20, DARKGRAY);
    }
    else {
        DrawText("MUSEUM BROCHURE", screenW/2 - MeasureText("MUSEUM BROCHURE", 30)/2, startY + 40, 30, BLACK);
        DrawText("Fascinating historical facts inside.", startX + 40, startY + 120, 20, DARKGRAY);
    }

    const char* closeText = "Press 'ESC' or 'E' to close and resume shift.";
    DrawText(closeText, screenW/2 - MeasureText(closeText, 20)/2, startY + bookH - 50, 20, GRAY);

    // --- OVERLAY LOGIC ---
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_E)) {
        isOverlayActive = false;
        StopMusicStream(overlayMusic);
        ResumeMusicStream(currentMainMusic); 
        if (tutorialStep == 2) tutorialStep = 3; // Advance tutorial upon closing!
    }
}