#pragma once
#include "raylib.h"
#include "Evaluation.h"
#include <vector>
#include <string>

struct UIReview {
    std::string author;
    std::string text;
    Color authorColor;
};

inline void UpdateAndRenderReview(int& currentState, const ShiftReport& lastReport, bool& triggerNextNight, Font fontMuseum, Font fontEmployee) {
    static bool initialized = false;
    static std::vector<UIReview> mixedReviews;
    static float scrollY = 0.0f;

    if (!initialized) {
        mixedReviews.clear();
        scrollY = 0.0f;

        // 1. Add the REAL consequences from your shift!
        for (const auto& rev : lastReport.reviews) {
            // Gold for real, gameplay-impacting reviews so they stand out slightly
            mixedReviews.push_back({rev.artifactName, rev.reviewText, {218, 165, 32, 255}}); 
        }

        // 2. Add the DUMMY filler reviews!
        std::vector<std::pair<std::string, std::string>> fakeReviews = {
            {"@ArtLover99", "The lighting in the Greek exhibit is stunning! Great atmosphere."},
            {"@HistoryBuff", "Cleanest museum I've visited in years. Highly recommend."},
            {"@NightOwl", "Swear I heard something moving in the Nordic room... 5 stars for the spook factor."},
            {"@TiredDad", "Good place to tire out the kids. The gift shop is completely overpriced though."},
            {"@LostTourist", "I think I saw a janitor sprinting with a fire extinguisher? 10/10 interactive theater!"},
            {"@MythNerd", "Are those real artifacts? They look almost too pristine..."},
            {"@CoffeeAddict", "Cafeteria was closed. Exhibits were okay, but I needed caffeine. 2/5 stars."},
            {"@SpookyFan", "Place gave me the creeps right at closing time. Loved every second of it."}
        };

        // Pick 5 random fake reviews to pad out the feed
        for (int i = 0; i < 5; i++) {
            int r = GetRandomValue(0, fakeReviews.size() - 1);
            mixedReviews.push_back({fakeReviews[r].first, fakeReviews[r].second, {150, 150, 150, 255}});
        }

        // 3. Shuffle them so the real reviews are hidden among the public posts!
        for (size_t i = 0; i < mixedReviews.size(); i++) {
            int swapIdx = GetRandomValue(0, mixedReviews.size() - 1);
            std::swap(mixedReviews[i], mixedReviews[swapIdx]);
        }

        initialized = true;
    }

    // --- LOGIC ---
    float dt = GetFrameTime();
    scrollY += GetMouseWheelMove() * 60.0f; // Mouse wheel scrolling
    if (scrollY > 0) scrollY = 0;           // Cap scroll at the top

    // --- RENDERING ---
    BeginDrawing();
    ClearBackground({20, 20, 25, 255}); // Dark Mode UI background

    float screenW = (float)GetScreenWidth();
    float screenH = (float)GetScreenHeight();
    float scale = std::min(screenW / 1280.0f, screenH / 960.0f);

    // Draw the scrolling feed of cards FIRST so they slide under the header
    float startY = (160.0f * scale) + scrollY;
    float cardX = screenW * 0.15f;
    float cardW = screenW * 0.70f;

    for (const auto& rev : mixedReviews) {
        float cardH = 110.0f * scale; 
        
        // Social Media Card Background
        DrawRectangleRounded({cardX, startY, cardW, cardH}, 0.1f, 10, {30, 30, 35, 255});
        
        // Username
        DrawTextEx(fontEmployee, rev.author.c_str(), {cardX + (20 * scale), startY + (15 * scale)}, 24 * scale, 0, rev.authorColor);
        
        // Review Text
        DrawTextEx(fontEmployee, rev.text.c_str(), {cardX + (20 * scale), startY + (55 * scale)}, 20 * scale, 0, RAYWHITE);
        
        startY += cardH + (20.0f * scale); // Space between cards
    }

    // HEADER (Drawn on top so cards scroll under it)
    DrawRectangle(0, 0, screenW, 140 * scale, {15, 15, 20, 255}); // Header bar
    DrawRectangle(0, 140 * scale, screenW, 5 * scale, {218, 165, 32, 255}); // Gold trim
    
    DrawTextEx(fontMuseum, "SHIFT COMPLETE - 8:00 AM", { 40 * scale, 30 * scale }, 45 * scale, 1, RAYWHITE);
    DrawTextEx(fontEmployee, lastReport.finalVerdict.c_str(), { 40 * scale, 90 * scale }, 25 * scale, 0, {218, 165, 32, 255});

    // FOOTER (Drawn on top so cards scroll under it)
    DrawRectangle(0, screenH - (80 * scale), screenW, 80 * scale, {15, 15, 20, 255});
    
    const char* enterText = "PRESS ENTER TO CONTINUE";
    Vector2 textDims = MeasureTextEx(fontEmployee, enterText, 30 * scale, 0);
    
    // Blinking prompt
    if ((int)(GetTime() * 2) % 2 == 0) {
        DrawTextEx(fontEmployee, enterText, {screenW / 2.0f - textDims.x / 2.0f, screenH - (55 * scale)}, 30 * scale, 0, {218, 165, 32, 255});
    }

    EndDrawing();

    // --- INPUT HANDLING ---
    if (IsKeyPressed(KEY_ENTER)) {
        triggerNextNight = true;
        initialized = false; // Reset the state for tomorrow night!
    }
}