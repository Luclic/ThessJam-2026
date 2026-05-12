#pragma once
#include "raylib.h"
#include <string>

// We added 'float itemID' so we can tell Brochure 1 from Brochure 2!
inline void UpdateAndRenderOverlay(bool& isOverlayActive, Music& currentMainMusic, Music& overlayMusic, int& tutorialStep, std::string docName, float itemID) {
    // --- STATIC TEXTURE CACHE ---
    static Texture2D handbookTex = { 0 };
    static Texture2D brochureTexs[5] = { 0 };
    static bool assetsLoaded = false;

    if (!assetsLoaded) {
        handbookTex = LoadTexture("resources/img/handbook.png");
        for (int i = 0; i < 5; i++) {
            //brochures[i] = LoadTexture(TextFormat("resources/img/brochure%d.png", i + 1));
            continue;
        }
        assetsLoaded = true;
    }

    int screenW = GetScreenWidth();
    int screenH = GetScreenHeight();

    // Dim the background
    DrawRectangle(0, 0, screenW, screenH, {0, 0, 0, 200});

    // Dimensions for the document display (Standard paper/brochure ratio)
    int bookW = 600; 
    int bookH = 800;
    int startX = screenW / 2 - bookW / 2;
    int startY = screenH / 2 - bookH / 2;

    Texture2D* textureToDraw = nullptr;

    // --- LOGIC TO SELECT THE CORRECT IMAGE ---
    if (docName == "Open Book" || docName.find("Handbook") != std::string::npos) {
        textureToDraw = &handbookTex;
    } 
    else if (docName == "Magazine" || docName.find("Brochure") != std::string::npos) {
        // Remember: in Interactions.h we stored the brochure number (1-5) in stateValue!
        int index = (int)itemID - 1; 
        if (index >= 0 && index < 5) textureToDraw = &brochureTexs[index];
    }

    // --- DRAWING ---
    if (textureToDraw != nullptr && textureToDraw->id > 0) {
        Rectangle src = { 0, 0, (float)textureToDraw->width, (float)textureToDraw->height };
        Rectangle dest = { (float)startX, (float)startY, (float)bookW, (float)bookH };
        DrawTexturePro(*textureToDraw, src, dest, { 0, 0 }, 0.0f, WHITE);
    } else {
        // Fallback if image fails to load
        DrawRectangle(startX, startY, bookW, bookH, RAYWHITE);
        DrawRectangleLines(startX, startY, bookW, bookH, BLACK);
        DrawText("DOCUMENT NOT FOUND", startX + 150, startY + 300, 20, RED);
    }

    const char* closeText = "Press 'ESC' or 'E' to close and resume shift.";
    DrawText(closeText, screenW/2 - MeasureText(closeText, 20)/2, startY + bookH + 20, 20, WHITE);

    // --- OVERLAY LOGIC ---
    if (IsKeyPressed(KEY_ESCAPE) || IsKeyPressed(KEY_E)) {
        isOverlayActive = false;
        StopMusicStream(overlayMusic);
        ResumeMusicStream(currentMainMusic); 
        if (tutorialStep == 2) tutorialStep = 3; 
    }
}