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
    
    if (hazVis.drawingBeam) DrawTriangle(hazVis.beamP1, hazVis.beamP2, hazVis.beamP3, { 0, 255, 0, 80 }); 
    if (hazVis.drawingExtinguisher) DrawTriangle(hazVis.extP1, hazVis.extP2, hazVis.extP3, { 255, 255, 255, 150 }); 

    for (const Entity* e : renderList) {
        if (e->color.a < 255) continue; 
        if (e->castsShadow && e->movementBox.width > 0 && !e->isGrabbed && e->attachedTo == -1) DrawEllipse(e->position.x, e->position.y, e->movementBox.width * 0.6f, e->movementBox.height * 0.8f, {0, 0, 0, 80});

        Color drawCol = e->color;
        if (e->HasTag(TAG_FUSEBOX)) drawCol = (e->stateValue > 0.5f) ? GREEN : RED; 

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