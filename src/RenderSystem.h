#pragma once
#include "raylib.h"
#include "Entities.h"
#include "Level.h"
#include "GameSystems.h"
#include <vector>
#include <algorithm>

inline Color DarkenColor(Color c, float amount) { return { (unsigned char)(c.r - c.r*amount), (unsigned char)(c.g - c.g*amount), (unsigned char)(c.b - c.b*amount), c.a }; }

inline void RenderWorld(RenderTexture2D renderTarget, Camera2D& camera, float dt, const std::vector<Entity>& entities, const Entity& player, int grabbedEntityIndex, const HazardVisuals& hazVis) {
    int currentRoomX = (int)std::floor((player.position.x) / GAME_WIDTH); 
    int currentRoomY = (int)std::floor((player.position.y) / GAME_HEIGHT);
    camera.target.x += (currentRoomX * GAME_WIDTH - camera.target.x) * 5.0f * dt; 
    camera.target.y += (currentRoomY * GAME_HEIGHT - camera.target.y) * 5.0f * dt;

    std::vector<Entity*> renderList; 
    for (auto& e : const_cast<std::vector<Entity>&>(entities)) renderList.push_back(&e);
    std::sort(renderList.begin(), renderList.end(), [](const Entity* a, const Entity* b) { return (a->position.y + (a->isGrabbed ? 0.1f : 0.0f)) < (b->position.y + (b->isGrabbed ? 0.1f : 0.0f)); });

    BeginTextureMode(renderTarget); 
    ClearBackground(RAYWHITE); 
    BeginMode2D(camera);
    
    for (int i = -1600; i < 4800; i += 100) DrawLine(i, -1200, i, 2400, LIGHTGRAY);
    for (int i = -1200; i < 2400; i += 100) DrawLine(-1600, i, 4800, i, LIGHTGRAY);

    for (const auto& e : entities) if (e.HasTag(TAG_WATER_SOURCE) && e.stateValue > 0.0f) DrawEllipse(e.position.x, e.position.y, e.stateValue, e.stateValue * 0.6f, {0, 121, 241, 150});
    for (const auto& e : entities) {
        if (e.HasTag(TAG_BANSHEE_STONE) && e.stateValue > 0.0f) {
            float alpha = std::max(0.0f, 255.0f - (e.stateValue / 800.0f) * 255.0f);
            DrawCircleLines(e.position.x, e.position.y - e.z, e.stateValue, {200, 100, 255, (unsigned char)alpha});
            DrawCircleLines(e.position.x, e.position.y - e.z, e.stateValue * 0.5f, {200, 100, 255, (unsigned char)alpha});
        }
    }
    
    // Flashlight Cone
    for (const auto& e : entities) {
        if (e.isUsing && e.HasTag(TAG_FLASHLIGHT)) {
            Vector2 dir = player.facingDir; Vector2 perp = { -dir.y, dir.x }; 
            Vector2 fl1 = e.position; 
            Vector2 fl2 = { e.position.x + dir.x * 600.0f + perp.x * 300.0f, e.position.y + dir.y * 600.0f + perp.y * 300.0f }; 
            Vector2 fl3 = { e.position.x + dir.x * 600.0f - perp.x * 300.0f, e.position.y + dir.y * 600.0f - perp.y * 300.0f };
            DrawTriangle(fl1, fl2, fl3, { 255, 255, 200, 100 }); 
        }
    }

    if (hazVis.drawingBeam) DrawTriangle(hazVis.beamP1, hazVis.beamP2, hazVis.beamP3, { 0, 255, 0, 80 }); 
    if (hazVis.drawingExtinguisher) DrawTriangle(hazVis.extP1, hazVis.extP2, hazVis.extP3, { 255, 255, 255, 150 }); 

    for (const Entity* e : renderList) {
        if (e->color.a < 255) continue; 
        if (e->castsShadow && e->movementBox.width > 0 && !e->isGrabbed && e->attachedTo == -1) DrawEllipse(e->position.x, e->position.y, e->movementBox.width * 0.6f, e->movementBox.height * 0.8f, {0, 0, 0, 80});

        Color drawCol = e->color;
        if (e->HasTag(TAG_FUSEBOX)) drawCol = (e->stateValue > 0.5f) ? GREEN : RED; 
        if (e->HasTag(TAG_WATER_SOURCE) && e->isStone) drawCol = SKYBLUE; // Frozen Ice!
        if (e->HasTag(TAG_HOLE) && e->stateValue > 0.5f) drawCol = LIGHTGRAY; // Taped Hole!

        if (e->is3DBlock && e->zHeight > 0) {
            Rectangle frontFace = { e->position.x + e->movementBox.x, e->position.y + e->movementBox.y - e->zHeight - e->z, e->movementBox.width, e->zHeight };
            DrawRectangleRec(frontFace, DarkenColor(drawCol, 0.4f)); DrawRectangleLinesEx(frontFace, 2.0f, BLACK);
            Rectangle topFace = { e->position.x + e->movementBox.x, e->position.y + e->movementBox.y - e->zHeight - e->z, e->movementBox.width, e->movementBox.height };
            DrawRectangleRec(topFace, drawCol); DrawRectangleLinesEx(topFace, 2.0f, BLACK);
        } else {
            Rectangle drawRec = e->GetWorldInteractionBox(); DrawRectangleRec(drawRec, drawCol); DrawRectangleLinesEx(drawRec, 2.0f, BLACK); 
        }
        if (e->name != "Wall" && e->name != "Door" && e->name != "Player" && e->name != "Pedestal") DrawText(e->name.c_str(), e->position.x - (MeasureText(e->name.c_str(), 10) / 2), e->GetWorldInteractionBox().y - 15, 10, BLACK);
    }

    // --- Render Room Darkness ---
    for (const auto& e : entities) {
        if (e.HasTag(TAG_LIGHTSWITCH) && e.stateValue < 0.5f) {
            int rx = (int)std::floor(e.position.x / GAME_WIDTH);
            int ry = (int)std::floor(e.position.y / GAME_HEIGHT);
            
            // Check if player is holding flashlight to cut through darkness
            bool hasFlashlight = false;
            for(const auto& item : entities) if(item.isGrabbed && item.isUsing && item.HasTag(TAG_FLASHLIGHT)) hasFlashlight = true;
            
            DrawRectangle(rx * GAME_WIDTH, ry * GAME_HEIGHT, GAME_WIDTH, GAME_HEIGHT, {0, 0, 0, (unsigned char)(hasFlashlight ? 180 : 230)});
        }
    }

    EndMode2D(); 
    EndTextureMode();
}

inline void RenderHUD(RenderTexture2D renderTarget, float shiftTimer, float secondsPerHour, int currentNight, const Entity& player, bool showInteractMenu, bool isDropMenu, const std::vector<int>& interactTargets, int interactSelectedIndex, const std::vector<Entity>& entities) {
    BeginDrawing(); 
    ClearBackground(BLACK);
    float scale = std::min((float)GetScreenWidth() / GAME_WIDTH, (float)GetScreenHeight() / GAME_HEIGHT);
    Rectangle sourceRec = { 0.0f, 0.0f, (float)renderTarget.texture.width, (float)-renderTarget.texture.height };
    Rectangle destRec = { (GetScreenWidth() - ((float)GAME_WIDTH * scale)) * 0.5f, (GetScreenHeight() - ((float)GAME_HEIGHT * scale)) * 0.5f, (float)GAME_WIDTH * scale, (float)GAME_HEIGHT * scale };
    DrawTexturePro(renderTarget.texture, sourceRec, destRec, { 0, 0 }, 0.0f, WHITE);

    int elapsedHours = (int)(shiftTimer / secondsPerHour);
    int displayHour = (12 + elapsedHours) % 12;
    if (displayHour == 0) displayHour = 12;
    DrawText(TextFormat("%02d:00 AM", displayHour), GetScreenWidth() - 150, 20, 30, YELLOW);

    DrawText("WASD: Move | 1-5: Doors | E: Grab | HOLD F: Use | PRESS F: Equip | SPACE: Throw | P/I/O/K: Triggers", 10, 10, 20, WHITE);
    
    if (player.isStone) DrawText("PETRIFIED!", 10, 40, 30, RED);
    if (player.isDead) DrawText("ZAPPED TO DEATH!", 10, 70, 30, ORANGE);

    // DRAW TOOL USES
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
    EndDrawing();
}