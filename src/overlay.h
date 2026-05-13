#pragma once
#include "raylib.h"
#include <string>

// Global texture cache for the overlay
static Texture2D handbookTexs[3] = { 0 }; 
static Texture2D brochureTexs[5] = { 0 };

inline void InitOverlayAssets() {
    for (int i = 0; i < 3; i++) {
        handbookTexs[i] = LoadTexture(TextFormat("resources/img/handbook%d.png", i + 1));
    }
    for (int i = 0; i < 5; i++) {
        brochureTexs[i] = LoadTexture(TextFormat("resources/img/brochure%d.png", i + 1));
    }
}

inline void UnloadOverlayAssets() {
    for (int i = 0; i < 3; i++) UnloadTexture(handbookTexs[i]);
    for (int i = 0; i < 5; i++) UnloadTexture(brochureTexs[i]);
}

inline void UpdateAndRenderOverlay(bool& isOverlayActive, Music& currentMainMusic, Music& overlayMusic, int& tutorialStep, std::string docName, float itemID) {
    static int currentHandbookPage = 0;

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();

    // Dim the background
    DrawRectangle(0, 0, (int)screenW, (int)screenH, {0, 0, 0, 200});

    // --- DYNAMIC 16:9 SCALING MATH ---
    float targetAspect = 16.0f / 9.0f; // Google Slides standard ratio
    float screenAspect = screenW / screenH;
    float padding = 0.85f; // Take up 85% of the screen so it feels like a nice popup

    float bookW, bookH;

    if (screenAspect > targetAspect) {
        // Screen is wider than 16:9 (Ultra-wide monitor) -> Height is the limit
        bookH = screenH * padding;
        bookW = bookH * targetAspect;
    } else {
        // Screen is taller than 16:9 (Square/Vertical monitor) -> Width is the limit
        bookW = screenW * padding;
        bookH = bookW / targetAspect;
    }

    // Perfectly center the calculated dimensions
    float startX = (screenW - bookW) / 2.0f;
    float startY = (screenH - bookH) / 2.0f;
    // ---------------------------------

    Texture2D* textureToDraw = nullptr;
    bool isHandbook = (docName == "Open Book" || docName.find("Handbook") != std::string::npos);

    if (isHandbook) {
        if (IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D)) currentHandbookPage++;
        if (IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A)) currentHandbookPage--;
        
        if (currentHandbookPage < 0) currentHandbookPage = 0;
        if (currentHandbookPage > 2) currentHandbookPage = 2;

        textureToDraw = &handbookTexs[currentHandbookPage];
    } 
    else if (docName == "Magazine" || docName.find("Brochure") != std::string::npos) {
        int index = ((int)itemID - 1) % 5; 
        if (index < 0) index = 0;          
        textureToDraw = &brochureTexs[index];
    }

    // --- DRAW THE IMAGE ---
    if (textureToDraw != nullptr && textureToDraw->id > 0) {
        Rectangle src = { 0.0f, 0.0f, (float)textureToDraw->width, (float)textureToDraw->height };
        Rectangle dest = { startX, startY, bookW, bookH };
        DrawTexturePro(*textureToDraw, src, dest, { 0, 0 }, 0.0f, WHITE);
    } else {
        // Fallback dynamically centered
        DrawRectangle((int)startX, (int)startY, (int)bookW, (int)bookH, RAYWHITE);
        DrawRectangleLines((int)startX, (int)startY, (int)bookW, (int)bookH, BLACK);
        
        const char* errText = "DOCUMENT NOT FOUND";
        DrawText(errText, (int)(startX + bookW/2 - MeasureText(errText, 20)/2), (int)(startY + bookH/2 - 20), 20, RED);

        DrawText(TextFormat("DEBUG INFO -> Name: %s | ID: %d", docName.c_str(), (int)itemID), (int)startX + 50, (int)(startY + bookH/2 + 20), 16, DARKGRAY);
        DrawText(TextFormat("TEXTURE ID: %d", textureToDraw != nullptr ? textureToDraw->id : -1), (int)startX + 50, (int)(startY + bookH/2 + 50), 16, DARKGRAY);
    }

    // --- PAGINATION UI VISUALS ---
    if (isHandbook) {
        const char* pageText = TextFormat("Page %d of 3", currentHandbookPage + 1);
        DrawText(pageText, (int)(screenW/2 - MeasureText(pageText, 20)/2), (int)(startY + bookH - 40), 20, DARKGRAY);
        
        // Dynamically place arrows outside the image bounds
        if (currentHandbookPage > 0) DrawText("< (A)", (int)(startX - 70), (int)(screenH/2 - 10), 20, LIGHTGRAY);
        if (currentHandbookPage < 2) DrawText("(D) >", (int)(startX + bookW + 20), (int)(screenH/2 - 10), 20, LIGHTGRAY);
    }

    const char* closeText = "Press 'E' to close and resume shift.";
    DrawText(closeText, (int)(screenW/2 - MeasureText(closeText, 20)/2), (int)(startY + bookH + 20), 20, WHITE);

    // --- OVERLAY LOGIC ---
    if (IsKeyPressed(KEY_E)) {
        isOverlayActive = false;
        currentHandbookPage = 0; 
        StopMusicStream(overlayMusic);
        ResumeMusicStream(currentMainMusic); 
        if (tutorialStep == 2) tutorialStep = 3; 
    }
}