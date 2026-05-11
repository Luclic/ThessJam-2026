#pragma once
#include "raylib.h"
#include "raymath.h"
#include "Entities.h"
#include "Level.h"
#include "GameSystems.h"
#include "Hitboxes_Export.h" // This provides GetGlobalTweaks() and ModelTweak!
#include <vector>
#include <algorithm>
#include <unordered_map>
#include <string>

inline Color DarkenColor(Color c, float amount) { return { (unsigned char)(c.r - c.r*amount), (unsigned char)(c.g - c.g*amount), (unsigned char)(c.b - c.b*amount), c.a }; }

inline void RenderWorld(RenderTexture2D renderTarget, Camera2D& camera, float dt, const std::vector<Entity>& entities, const Entity& player, int grabbedEntityIndex, const HazardVisuals& hazVis, const std::unordered_map<std::string, Model>& models) {
    int currentRoomX = (int)std::floor((player.position.x) / GAME_WIDTH); 
    int currentRoomY = (int)std::floor((player.position.z) / GAME_HEIGHT); 
    camera.target.x += (currentRoomX * GAME_WIDTH - camera.target.x) * 5.0f * dt; 
    camera.target.y += (currentRoomY * GAME_HEIGHT - camera.target.y) * 5.0f * dt;

    std::vector<Entity*> renderList; 
    for (auto& e : const_cast<std::vector<Entity>&>(entities)) renderList.push_back(&e);
    std::sort(renderList.begin(), renderList.end(), [](const Entity* a, const Entity* b) { return (a->position.y + (a->isGrabbed ? 0.1f : 0.0f)) < (b->position.y + (b->isGrabbed ? 0.1f : 0.0f)); });

    BeginTextureMode(renderTarget); 
    ClearBackground({35, 35, 40, 255}); 
    
    BeginMode2D(camera);
    for (int i = -4000; i < 8000; i += 200) { DrawLine(i, -4000, i, 8000, {55, 55, 60, 255}); DrawLine(-4000, i, 8000, i, {55, 55, 60, 255}); }
    EndMode2D();

    Camera3D cam3D = { 0 };
    cam3D.position = (Vector3){ (camera.target.x + 50) + GAME_WIDTH/2.0f, 1300.0f, camera.target.y + GAME_HEIGHT/2.0f + 700.0f }; 
    cam3D.target = (Vector3){ (camera.target.x + 50) + GAME_WIDTH/2.0f, 0.0f, camera.target.y + GAME_HEIGHT/2.0f };      
    cam3D.up = (Vector3){ 0.0f, 1.0f, 0.0f }; 
    cam3D.fovy = 50.0f; 
    cam3D.projection = CAMERA_PERSPECTIVE;

    std::vector<std::pair<Vector2, std::string>> floatingTexts;

    BeginMode3D(cam3D);
    Vector2 camCenter = { camera.target.x + GAME_WIDTH/2.0f, camera.target.y + GAME_HEIGHT/2.0f };
    
    // Fetch the tweaks exported from your editor!
    auto globalTweaks = GetGlobalTweaks();

    for (const Entity* e : renderList) {
        if (e->color.a < 255) continue; 
        if (Vector2Distance({e->position.x, e->position.z}, camCenter) > 2500.0f) continue;

        std::string baseName = GetBaseModelName(e->name);
        auto modIt = models.find(e->name); // Still load the specific color model!
        if (modIt != models.end()) {
            ModelTweak tweak = globalTweaks[baseName];
            float rad = e->stateValue * DEG2RAD;
            float cosR = cos(rad); float sinR = sin(rad);
            Vector3 rotatedOffset = { tweak.offset.x * cosR + tweak.offset.z * sinR, tweak.offset.y, -tweak.offset.x * sinR + tweak.offset.z * cosR };
            Vector3 pos3D = Vector3Add(e->position, rotatedOffset);
            float finalRot = tweak.rot + e->stateValue; 
            
            // ✅ REPLACE WITH:
            Vector3 finalScale = (Vector3){tweak.scale, tweak.scale, tweak.scale};
            DrawModelEx(modIt->second, pos3D, {0, 1, 0}, finalRot, finalScale, WHITE);
        } else {
            // PLACEHOLDER SYSTEM
            Color drawCol = e->color;
            std::vector<BoundingBox> projected = e->GetWorldBounds();
            
            for(const auto& b : projected) {
                Vector3 size = { b.max.x - b.min.x, b.max.y - b.min.y, b.max.z - b.min.z };
                if (size.x > 0 && size.y > 0 && size.z > 0) {
                    Vector3 center = { b.min.x + size.x/2.0f, b.min.y + size.y/2.0f, b.min.z + size.z/2.0f };
                    DrawCubeV(center, size, Fade(drawCol, 0.8f));
                    DrawCubeWiresV(center, size, BLACK);
                    
                    // NEW: Capture the 2D screen coordinate for the text!
                    // NEW: Only draw text if the object is IN FRONT of the camera!
                    Vector3 toObj = Vector3Subtract(center, cam3D.position);
                    Vector3 camForward = Vector3Subtract(cam3D.target, cam3D.position);
                    if (Vector3DotProduct(toObj, camForward) > 0.0f) {
                        floatingTexts.push_back({ GetWorldToScreenEx({center.x, b.max.y + 20.0f, center.z}, cam3D, renderTarget.texture.width, renderTarget.texture.height), e->name });
                    }                }
            }
        }

        // --- NEW CONCENTRIC PUDDLE RENDERER (PERFORMANCE & VISUAL FIX) ---
        if (e->HasTag(TAG_WATER_SOURCE) && e->stateValue > 0.0f) {
            float radius = e->stateValue;
            float timeSec = (float)GetTime();

            // 1. Draw Concentric 2D Rings (layered slightly on Y-axis to prevent Z-fighting)
            
            // Outer Ring - Shallow Water (Light Blue, Highly Transparent)
            DrawCylinder({e->position.x, 50.0f, e->position.z}, radius, radius, 0.1f, 24, {0, 150, 255, 80});
            
            // Middle Ring - Deepening Water (Medium Blue)
            float midRadius = radius * 0.9f; // 70% of total size
            if (midRadius > 0) {
                DrawCylinder({e->position.x, 55.0f, e->position.z}, midRadius, midRadius, 0.1f, 24, {0, 100, 200, 140});
            }

            // Inner Ring - Deep Water (Dark Ocean Blue)
            float innerRadius = radius * 0.85f; // 40% of total size
            if (innerRadius > 0) {
                DrawCylinder({e->position.x, 60.0f, e->position.z}, innerRadius, innerRadius, 0.1f, 24, {0, 50, 150, 200});
            }
        }
        // -----------------------------------------------------------------

        if (e->HasTag(TAG_BANSHEE_STONE) && e->stateValue > 0.0f) DrawCylinderWires({e->position.x, e->position.y, e->position.z}, e->stateValue, e->stateValue, 1.0f, 16, {200, 100, 255, (unsigned char)(std::max(0.0f, 255.0f - (e->stateValue / 800.0f) * 255.0f))});
        
        if (e->isUsing && e->HasTag(TAG_FLASHLIGHT)) {
            Vector3 dir = player.facingDir; Vector3 perp = { -dir.z, 0.0f, dir.x }; 
            Vector3 fl2 = { e->position.x + dir.x * 600.0f + perp.x * 300.0f, e->position.y, e->position.z + dir.z * 600.0f + perp.z * 300.0f }; 
            Vector3 fl3 = { e->position.x + dir.x * 600.0f - perp.x * 300.0f, e->position.y, e->position.z + dir.z * 600.0f - perp.z * 300.0f };
            DrawTriangle3D(e->position, fl2, fl3, { 255, 255, 200, 100 }); DrawTriangle3D(e->position, fl3, fl2, { 255, 255, 200, 100 }); 
        }

        // Draw Tape if the Wind Bag is sealed
        if (e->HasTag(TAG_WIND_BAG) && e->HasTag(TAG_BROKEN)) {
            DrawCubeWiresV(e->position, {60, 20, 60}, GRAY); 
        }

        // Draw Zeus Lightning Sparks
        if (e->HasTag(TAG_ZEUS) && e->isGlitching) {
            if ((int)(GetTime() * 10) % 2 == 0) { // Flickers every other frame
                DrawSphereWires({e->position.x, e->position.y + 40.0f, e->position.z}, 60.0f, 4, 4, YELLOW);
            }
        }
    }

    if (hazVis.drawingBeam) { DrawTriangle3D(hazVis.beamP1, hazVis.beamP2, hazVis.beamP3, { 0, 255, 0, 80 }); DrawTriangle3D(hazVis.beamP1, hazVis.beamP3, hazVis.beamP2, { 0, 255, 0, 80 }); }
    if (hazVis.drawingExtinguisher) { DrawTriangle3D(hazVis.extP1, hazVis.extP2, hazVis.extP3, { 255, 255, 255, 150 }); DrawTriangle3D(hazVis.extP1, hazVis.extP3, hazVis.extP2, { 255, 255, 255, 150 }); }
    EndMode3D();
    // NEW: Draw all the saved floating text over the 3D scene
    for(const auto& ft : floatingTexts) {
        DrawText(ft.second.c_str(), ft.first.x - MeasureText(ft.second.c_str(), 20)/2, ft.first.y, 20, RAYWHITE);
    }
    BeginMode2D(camera);
    for (const auto& e : entities) {
        if (e.HasTag(TAG_LIGHTSWITCH) && e.stateValue < 0.5f) {
            int rx = (int)std::floor(e.position.x / GAME_WIDTH); int ry = (int)std::floor(e.position.z / GAME_HEIGHT);
            bool hasFlashlight = false;
            for(const auto& item : entities) if(item.isGrabbed && item.isUsing && item.HasTag(TAG_FLASHLIGHT)) hasFlashlight = true;
            DrawRectangle(rx * GAME_WIDTH, ry * GAME_HEIGHT, GAME_WIDTH, GAME_HEIGHT, {0, 0, 0, (unsigned char)(hasFlashlight ? 180 : 230)});
        }
    }
    EndMode2D(); 
    EndTextureMode();
}

inline void RenderHUD(RenderTexture2D renderTarget, float shiftTimer, float secondsPerHour, int currentNight, const Entity& player, bool showInteractMenu, bool isDropMenu, const std::vector<int>& interactTargets, int interactSelectedIndex, const std::vector<Entity>& entities) {
    ClearBackground(BLACK);
    float scale = std::min((float)GetScreenWidth() / GAME_WIDTH, (float)GetScreenHeight() / GAME_HEIGHT);
    Rectangle sourceRec = { 0.0f, 0.0f, (float)renderTarget.texture.width, (float)-renderTarget.texture.height };
    Rectangle destRec = { (GetScreenWidth() - ((float)GAME_WIDTH * scale)) * 0.5f, (GetScreenHeight() - ((float)GAME_HEIGHT * scale)) * 0.5f, (float)GAME_WIDTH * scale, (float)GAME_HEIGHT * scale };
    DrawTexturePro(renderTarget.texture, sourceRec, destRec, { 0, 0 }, 0.0f, WHITE);

    DrawFPS(GetScreenWidth() - 100, 10);

    int elapsedHours = (int)(shiftTimer / secondsPerHour);
    int displayHour = (12 + elapsedHours) % 12;
    if (displayHour == 0) displayHour = 12;
    DrawText(TextFormat("%02d:00 AM", displayHour), GetScreenWidth() - 150, 70, 30, YELLOW);

    DrawText("WASD: Move | 1-5: Doors | E: Grab | HOLD F: Use | PRESS F: Equip | SPACE: Throw | P/I/O/K: Triggers", 10, 10, 20, WHITE);
    
    if (player.isStone) DrawText("PETRIFIED!", 10, 40, 30, RED);
    if (player.isDead) DrawText("ZAPPED TO DEATH!", 10, 70, 30, ORANGE);

    for (const auto& e : entities) {
        if (e.isGrabbed && (e.HasTag(TAG_TAPE) || e.HasTag(TAG_BUBBLE_WRAP))) {
            DrawText(TextFormat("%s USES LEFT: %d", e.name.c_str(), (int)e.stateValue), GetScreenWidth() / 2 - 100, GetScreenHeight() - 50, 20, (e.stateValue > 0) ? GREEN : RED);
        }
    }

    if (showInteractMenu) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(), {0, 0, 0, 150});
        float menuW = 400; float itemH = 50;
        float menuH = 60 + interactTargets.size() * itemH;
        float menuX = GetScreenWidth() / 2.0f - menuW / 2.0f;
        float menuY = GetScreenHeight() / 2.0f - menuH / 2.0f;

        DrawRectangle(menuX, menuY, menuW, menuH, {40, 40, 40, 240});
        DrawRectangleLines(menuX, menuY, menuW, menuH, WHITE);
        DrawText(isDropMenu ? "SELECT TARGET:" : "SELECT ITEM:", menuX + 20, menuY + 20, 20, RAYWHITE);

        for (size_t i = 0; i < interactTargets.size(); ++i) {
            Rectangle itemRec = { menuX + 20, menuY + 60 + i * itemH, menuW - 40, itemH - 10 };
            Color btnColor = (interactSelectedIndex == i) ? DARKGRAY : GRAY;
            if (CheckCollisionPointRec(GetMousePosition(), itemRec) && IsMouseButtonDown(MOUSE_LEFT_BUTTON)) btnColor = BLACK;

            DrawRectangleRec(itemRec, btnColor);
            DrawRectangleLinesEx(itemRec, 2, (interactSelectedIndex == i) ? WHITE : LIGHTGRAY);
            DrawText(entities[interactTargets[i]].name.c_str(), itemRec.x + 20, itemRec.y + 10, 20, WHITE);
        }
        DrawText("W/S: Move | SPACE/CLICK: Select | E: Cancel", menuX + 10, menuY + menuH + 10, 16, LIGHTGRAY);
    }
}