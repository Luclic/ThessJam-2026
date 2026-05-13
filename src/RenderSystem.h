#pragma once
#include "raylib.h"
#include "raymath.h"
#include "Entities.h"
#include "Level.h"
#include "GameSystems.h"
#include "Hitboxes_Export.h" // This provides GetGlobalTweaks() and ModelTweak!
#include <vector>
#include <string>
#include <algorithm>
#include <unordered_map>

inline Color DarkenColor(Color c, float amount) { return { (unsigned char)(c.r - c.r*amount), (unsigned char)(c.g - c.g*amount), (unsigned char)(c.b - c.b*amount), c.a }; }

inline void RenderWorld(RenderTexture2D renderTarget, Camera2D& camera, float dt, const std::vector<Entity>& entities, const Entity& player, int grabbedEntityIndex, const std::unordered_map<std::string, Model>& models, const HazardVisuals& hazVis) {
    int currentRoomX = (int)std::floor((player.position.x) / GAME_WIDTH); 
    int currentRoomY = (int)std::floor((player.position.z) / GAME_HEIGHT); 
    camera.target.x += (currentRoomX * GAME_WIDTH - camera.target.x) * 5.0f * dt; 
    camera.target.y += (currentRoomY * GAME_HEIGHT - camera.target.y) * 5.0f * dt;

    std::vector<const Entity*> renderList; 
    for (auto& e : const_cast<std::vector<Entity>&>(entities)) renderList.push_back(&e);
    std::sort(renderList.begin(), renderList.end(), [](const Entity* a, const Entity* b) { return (a->position.y + (a->isGrabbed ? 0.1f : 0.0f)) < (b->position.y + (b->isGrabbed ? 0.1f : 0.0f)); });

    Camera3D cam3D = { 0 };
    cam3D.position = (Vector3){ (camera.target.x + 50) + GAME_WIDTH/2.0f, 1300.0f, camera.target.y + GAME_HEIGHT/2.0f + 700.0f }; 
    cam3D.target = (Vector3){ (camera.target.x + 50) + GAME_WIDTH/2.0f, 0.0f, camera.target.y + GAME_HEIGHT/2.0f };      
    cam3D.up = (Vector3){ 0.0f, 1.0f, 0.0f }; 
    cam3D.fovy = 50.0f; 
    cam3D.projection = CAMERA_PERSPECTIVE;

    // =========================================================================================
    // --- PHASE 2: SHADOW MAPPING SETUP ---
    // =========================================================================================
    static RenderTexture2D shadowMap = { 0 };
    static Shader depthShader = { 0 };
    static Shader clayShader = { 0 }; // NEW: We must cache your beautiful shader!
    static bool isShadowMapInit = false;

    if (!isShadowMapInit) {
        shadowMap = LoadRenderTexture(2048, 2048); 
        depthShader = LoadShader(0, 0); 
        depthShader.locs[SHADER_LOC_MATRIX_MVP] = GetShaderLocation(depthShader, "mvp");
        
        // CACHE THE CLAY SHADER SO WE DON'T LOSE IT!
        if (!models.empty()) clayShader = models.begin()->second.materials[0].shader;
        
        isShadowMapInit = true;
    }

    // 1. Setup the Sun's "Camera"
    Camera3D lightCam = { 0 };
    // The sun sits high above the current room and slightly to the left/front
    lightCam.position = (Vector3){ cam3D.target.x - 1000.0f, 2500.0f, cam3D.target.y - 1000.0f }; 
    lightCam.target = cam3D.target;
    lightCam.up = (Vector3){ 0.0f, 1.0f, 0.0f };
    lightCam.fovy = 90.0f; // Wide angle to capture the whole room
    lightCam.projection = CAMERA_ORTHOGRAPHIC; // Orthographic makes sunlight parallel!

    // 2. Calculate the Light Space Matrix (Projection * View)
    Matrix lightView = MatrixLookAt(lightCam.position, lightCam.target, lightCam.up);
    // --- FIX 4: TIGHTEN ORTHO BOUNDS FOR HIGHER RESOLUTION SHADOWS ---
    Matrix lightProj = MatrixOrtho(-900, 900, -900, 900, 100, 5000); 
    Matrix lightSpaceMatrix = MatrixMultiply(lightView, lightProj);

    auto globalTweaks = GetGlobalTweaks();

    // =========================================================================================
    // PASS 1: DRAW TO THE SHADOW MAP
    // =========================================================================================
    BeginTextureMode(shadowMap);
    ClearBackground(WHITE); // White means "maximum distance" (no shadow)
    BeginMode3D(lightCam);
    
    // We temporarily override the default shader to just record geometry depth
    for (const Entity* e : renderList) {
        if (e->color.a < 255 || e->HasTag(TAG_WATER_SOURCE)) continue; // Don't cast shadows for invisible or flat puddle objects
        
        std::string baseName = GetBaseModelName(e->name);
        auto modIt = models.find(e->name); 
        if (modIt != models.end()) {
            ModelTweak tweak = globalTweaks[baseName];
            float rotationAngle = (!e->HasTag(TAG_BANSHEE_STONE)) ? e->stateValue : 0.0f;
            float rad = rotationAngle * DEG2RAD;
            Vector3 rotatedOffset = { tweak.offset.x * cos(rad) + tweak.offset.z * sin(rad), tweak.offset.y, -tweak.offset.x * sin(rad) + tweak.offset.z * cos(rad) };
            Vector3 pos3D = Vector3Add(e->position, rotatedOffset);
            
            // Force the model to draw with the depth shader for this pass
            for (int i = 0; i < modIt->second.materialCount; i++) modIt->second.materials[i].shader = depthShader;
            DrawModelEx(modIt->second, pos3D, {0, 1, 0}, tweak.rot + rotationAngle, (Vector3){tweak.scale, tweak.scale, tweak.scale}, WHITE);
        } else {
            // Draw placeholder cubes into the shadow map
            std::vector<BoundingBox> projected = e->GetWorldBounds();
            for(const auto& b : projected) {
                Vector3 size = { b.max.x - b.min.x, b.max.y - b.min.y, b.max.z - b.min.z };
                if (size.x > 0 && size.y > 0 && size.z > 0) {
                    Vector3 center = { b.min.x + size.x/2.0f, b.min.y + size.y/2.0f, b.min.z + size.z/2.0f };
                    DrawCubeV(center, size, WHITE);
                }
            }
        }
    }
    EndMode3D();
    EndTextureMode();

    // =========================================================================================
    // PASS 2: RENDER THE ACTUAL GAME WORLD
    // =========================================================================================
    
    // Send the Light Space Matrix to our safely cached clayShader!
    int lightSpaceMatLoc = GetShaderLocation(clayShader, "lightSpaceMatrix");
    SetShaderValueMatrix(clayShader, lightSpaceMatLoc, lightSpaceMatrix);

    std::vector<std::pair<Vector2, std::string>> floatingTexts;

    BeginTextureMode(renderTarget); 
    ClearBackground({35, 35, 40, 255}); 
    
    BeginMode2D(camera);
    for (int i = -4000; i < 8000; i += 200) { DrawLine(i, -4000, i, 8000, {55, 55, 60, 255}); DrawLine(-4000, i, 8000, i, {55, 55, 60, 255}); }
    EndMode2D();

    BeginMode3D(cam3D);
    Vector2 camCenter = { camera.target.x + GAME_WIDTH/2.0f, camera.target.y + GAME_HEIGHT/2.0f };
    
    for (const Entity* e : renderList) {
        if (e->color.a < 255) continue; 
        if (Vector2Distance({e->position.x, e->position.z}, camCenter) > 2500.0f) continue;

        std::string baseName = GetBaseModelName(e->name);
        auto modIt = models.find(e->name); 
        if (modIt != models.end()) {
            ModelTweak tweak = globalTweaks[baseName];

            float rotationAngle = 0.0f;
            if (!e->HasTag(TAG_BANSHEE_STONE) && !e->HasTag(TAG_WATER_SOURCE)) {
                rotationAngle = e->stateValue; 
            }

            float rad = rotationAngle * DEG2RAD;
            float cosR = cos(rad); float sinR = sin(rad);
            Vector3 rotatedOffset = { tweak.offset.x * cosR + tweak.offset.z * sinR, tweak.offset.y, -tweak.offset.x * sinR + tweak.offset.z * cosR };
            Vector3 pos3D = Vector3Add(e->position, rotatedOffset);
            float finalRot = tweak.rot + rotationAngle; 

            // --- THE MISSING FIX: ACTUALLY RESTORE THE SHADER! ---
            for (int i = 0; i < modIt->second.materialCount; i++) {
                modIt->second.materials[i].shader = clayShader;
            }
            
            // Bind the shadow map texture to the shader just before drawing!
            SetShaderValueTexture(clayShader, GetShaderLocation(clayShader, "shadowMap"), shadowMap.texture);

            Vector3 finalScale = (Vector3){tweak.scale, tweak.scale, tweak.scale};
            DrawModelEx(modIt->second, pos3D, {0, 1, 0}, finalRot, finalScale, WHITE);
        } else {
            // --- NEW FIX: BRIGHT PINK PLACEHOLDER SYSTEM ---
            // Force the color to MAGENTA so it's impossible to miss!
            Color drawCol = MAGENTA; 
            std::vector<BoundingBox> projected = e->GetWorldBounds();
            
            for(const auto& b : projected) {
                Vector3 size = { b.max.x - b.min.x, b.max.y - b.min.y, b.max.z - b.min.z };
                if (size.x > 0 && size.y > 0 && size.z > 0) {
                    Vector3 center = { b.min.x + size.x/2.0f, b.min.y + size.y/2.0f, b.min.z + size.z/2.0f };
                    
                    // Draw a semi-transparent bright pink cube with solid pink wires
                    DrawCubeV(center, size, Fade(drawCol, 0.8f));
                    DrawCubeWiresV(center, size, PINK);
                    
                    Vector3 toObj = Vector3Subtract(center, cam3D.position);
                    Vector3 camForward = Vector3Subtract(cam3D.target, cam3D.position);
                    if (Vector3DotProduct(toObj, camForward) > 0.0f) {
                        floatingTexts.push_back({ GetWorldToScreenEx({center.x, b.max.y + 20.0f, center.z}, cam3D, renderTarget.texture.width, renderTarget.texture.height), e->name });
                    }                
                }
            }
        }

        // --- CONCENTRIC PUDDLE RENDERER ---
        if (e->HasTag(TAG_WATER_SOURCE) && e->stateValue > 0.0f) {
            float radius = e->stateValue;
            DrawCylinder({e->position.x, 50.0f, e->position.z}, radius, radius, 0.1f, 24, {0, 150, 255, 80});
            
            float midRadius = radius * 0.9f; 
            if (midRadius > 0) DrawCylinder({e->position.x, 55.0f, e->position.z}, midRadius, midRadius, 0.1f, 24, {0, 100, 200, 140});

            float innerRadius = radius * 0.85f; 
            if (innerRadius > 0) DrawCylinder({e->position.x, 60.0f, e->position.z}, innerRadius, innerRadius, 0.1f, 24, {0, 50, 150, 200});
        }
        // -----------------------------------------------------------------

        // --- NEW FIX: AEOLUS TORNADO VISUAL EFFECT (Solid & Smooth) ---
        if (e->HasTag(TAG_WIND_BAG) && e->isGlitching && !e->HasTag(TAG_BROKEN)) {
            float timeSec = (float)GetTime();
            
            // Use 3 solid, semi-transparent cones layered together.
            // Slowing down the animation speed significantly (0.4f instead of 1.5f) to prevent dizziness.
            for (int r = 0; r < 3; r++) {
                // Slower expansion phase
                float t = fmod((timeSec * 0.4f) + (r * 0.333f), 1.0f); 
                
                // Smoother, less aggressive expansion
                float topRadius = 50.0f + (t * 200.0f);
                float bottomRadius = 10.0f + (t * 15.0f);
                float height = 100.0f + (t * 200.0f);
                
                // Sine wave for smooth pulsating opacity (peaks in the middle, fades gently at start/end)
                float fade = sinf(t * PI); 
                
                Vector3 tornadoPos = {e->position.x, e->position.y + (height / 2.0f), e->position.z};
                
                // 1. Draw solid, translucent geometry! No more spiderwebs.
                // 12 slices gives it a nice stylized, low-poly "gale" look.
                DrawCylinder(tornadoPos, topRadius, bottomRadius, height, 12, {150, 200, 255, (unsigned char)(60 * fade)});
                
                // 2. Add a very minimal, low-opacity thick outline (only 6 slices) to give it a slow swirling texture
                DrawCylinderWires(tornadoPos, topRadius + 5.0f, bottomRadius + 5.0f, height, 6, {200, 230, 255, (unsigned char)(30 * fade)});
            }
        }
        // --------------------------------------------------------------
        if (e->HasTag(TAG_BANSHEE_STONE) && e->stateValue > 0.0f) DrawCylinderWires({e->position.x, e->position.y, e->position.z}, e->stateValue, e->stateValue, 1.0f, 16, {200, 100, 255, (unsigned char)(std::max(0.0f, 255.0f - (e->stateValue / 800.0f) * 255.0f))});
        
        if (e->isUsing && e->HasTag(TAG_FLASHLIGHT)) {
            Vector3 dir = player.facingDir; Vector3 perp = { -dir.z, 0.0f, dir.x }; 
            Vector3 fl2 = { e->position.x + dir.x * 600.0f + perp.x * 300.0f, e->position.y, e->position.z + dir.z * 600.0f + perp.z * 300.0f }; 
            Vector3 fl3 = { e->position.x + dir.x * 600.0f - perp.x * 300.0f, e->position.y, e->position.z + dir.z * 600.0f - perp.z * 300.0f };
            DrawTriangle3D(e->position, fl2, fl3, { 255, 255, 200, 100 }); DrawTriangle3D(e->position, fl3, fl2, { 255, 255, 200, 100 }); 
        }

        if (e->HasTag(TAG_WIND_BAG) && e->HasTag(TAG_BROKEN)) DrawCubeWiresV(e->position, {60, 20, 60}, GRAY); 

        if (e->HasTag(TAG_ZEUS) && e->isGlitching) {
            if ((int)(GetTime() * 10) % 2 == 0) DrawSphereWires({e->position.x, e->position.y + 40.0f, e->position.z}, 60.0f, 4, 4, YELLOW);
        }
    }

    if (hazVis.drawingBeam) { DrawTriangle3D(hazVis.beamP1, hazVis.beamP2, hazVis.beamP3, { 0, 255, 0, 80 }); DrawTriangle3D(hazVis.beamP1, hazVis.beamP3, hazVis.beamP2, { 0, 255, 0, 80 }); }
    if (hazVis.drawingExtinguisher) { DrawTriangle3D(hazVis.extP1, hazVis.extP2, hazVis.extP3, { 255, 255, 255, 150 }); DrawTriangle3D(hazVis.extP1, hazVis.extP3, hazVis.extP2, { 255, 255, 255, 150 }); }
    
    // --- NEW: DRAW THE SUN DISK BEAMS ---
    if (hazVis.drawingSunBeams) {
        for (int j = 0; j < 4; j++) {
            float angleRad = (hazVis.sunAngle + j * 90.0f) * DEG2RAD;
            Vector3 dir = {cos(angleRad), 0.0f, sin(angleRad)};
            Vector3 perp = {-dir.z, 0.0f, dir.x}; // Perpendicular vector for thickness

            Vector3 startCenter = hazVis.sunCenter;
            startCenter.y += 10.0f; // Hover slightly off the floor

            // Beam reaches out 800 units
            Vector3 endCenter = {startCenter.x + dir.x * 800.0f, startCenter.y, startCenter.z + dir.z * 800.0f};

            // Calculate the 4 corners of the thick rectangle (30 units each side = 60 total width)
            Vector3 p1 = {startCenter.x + perp.x * 30.0f, startCenter.y, startCenter.z + perp.z * 30.0f};
            Vector3 p2 = {startCenter.x - perp.x * 30.0f, startCenter.y, startCenter.z - perp.z * 30.0f};
            Vector3 p3 = {endCenter.x + perp.x * 30.0f, endCenter.y, endCenter.z + perp.z * 30.0f};
            Vector3 p4 = {endCenter.x - perp.x * 30.0f, endCenter.y, endCenter.z - perp.z * 30.0f};

            // Draw the thick beam using two triangles
            DrawTriangle3D(p1, p3, p2, { 255, 100, 0, 150 });
            DrawTriangle3D(p2, p3, p4, { 255, 100, 0, 150 });
            
            // Draw backfaces so it renders from both sides
            DrawTriangle3D(p1, p2, p3, { 255, 100, 0, 150 });
            DrawTriangle3D(p2, p4, p3, { 255, 100, 0, 150 });
        }
    }
    
    EndMode3D();

    for(const auto& ft : floatingTexts) {
        DrawText(ft.second.c_str(), ft.first.x - MeasureText(ft.second.c_str(), 20)/2, ft.first.y, 20, RAYWHITE);
    }

    BeginMode2D(camera);
    for (const auto& e : entities) {
        // --- NEW FIX: Darken screen for BOTH Lightswitches and Fuseboxes! ---
        if ((e.HasTag(TAG_LIGHTSWITCH) || e.HasTag(TAG_FUSEBOX)) && e.stateValue < 0.5f) {
            int rx = (int)std::floor(e.position.x / GAME_WIDTH); 
            int ry = (int)std::floor(e.position.z / GAME_HEIGHT);
            bool hasFlashlight = false;
            for(const auto& item : entities) if(item.isGrabbed && item.isUsing && item.HasTag(TAG_FLASHLIGHT)) hasFlashlight = true;
            
            // 252 is almost entirely pitch black! 
            // 190 leaves a dim, spooky cone if you have your flashlight turned on.
            unsigned char alphaValue = hasFlashlight ? 190 : 210;
            DrawRectangle(rx * GAME_WIDTH*8, ry * GAME_HEIGHT*8, GAME_WIDTH, GAME_HEIGHT, {0, 0, 0, alphaValue});
        }
    }
    EndMode2D(); 
    EndTextureMode();
}

inline void RenderHUD(RenderTexture2D renderTarget, float shiftTimer, float secondsPerHour, int currentNight, const Entity& player, bool showInteractMenu, bool isDropMenu, const std::vector<int>& interactTargets, int interactSelectedIndex, const std::vector<Entity>& entities, const HazardVisuals& hazVis, Font fontMuseum, Font fontEmployee) {
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

    // --- PANDORA WARNING TEXT ---
    if (hazVis.pandoraWarning) {
        int textWidth = MeasureText("WARNING: PANDORA'S BOX IS OPEN", 40);
        DrawText("WARNING: PANDORA'S BOX IS OPEN", (GetScreenWidth() - textWidth) / 2, 20, 40, RED);
        
        // Optional sub-text to remind the player how to close it!
        const char* subText = "SEAL IT WITH ANY TWO: DUCT TAPE, MJOLNIR, GLEIPNIR";
        DrawText(subText, (GetScreenWidth() - MeasureText(subText, 20)) / 2, 65, 20, ORANGE);
    }
    // ----------------------------

    // --- GLEIPNIR QTE VISUALS ---
    if (hazVis.qteActive) {
        float screenW = (float)GetScreenWidth();
        float screenH = (float)GetScreenHeight();
        float scale = std::min(screenW / 1280.0f, screenH / 960.0f);
        
        // Flashing dramatic red screen tint
        float flash = (sinf((float)GetTime() * 20.0f) + 1.0f) / 2.0f;
        DrawRectangle(0, 0, (int)screenW, (int)screenH, {255, 0, 0, (unsigned char)(60 * flash)});
        
        // Giant Warning Text
        const char* qteMsg = "MASH SPACEBAR TO THROW SNAKE AWAY!";
        float msgSize = 60.0f * scale;
        Vector2 msgVec = MeasureTextEx(fontMuseum, qteMsg, msgSize, 1);
        DrawTextEx(fontMuseum, qteMsg, {screenW/2.0f - msgVec.x/2.0f, screenH/2.0f - (150.0f * scale)}, msgSize, 1, RED);
        
        // Countdown Timer
        const char* timeMsg = TextFormat("%.1f SECONDS LEFT!", hazVis.qteTimeLeft);
        float tSize = 30.0f * scale;
        Vector2 tVec = MeasureTextEx(fontEmployee, timeMsg, tSize, 1);
        DrawTextEx(fontEmployee, timeMsg, {screenW/2.0f - tVec.x/2.0f, screenH/2.0f - (80.0f * scale)}, tSize, 1, WHITE);
        
        // Mashing Progress Bar
        float barW = 600.0f * scale;
        float barH = 40.0f * scale;
        Rectangle barBg = {screenW/2.0f - barW/2.0f, screenH/2.0f + (50.0f * scale), barW, barH};
        DrawRectangleRounded(barBg, 0.5f, 10, {50, 0, 0, 200});
        
        // Foreground progress fill
        Rectangle barFg = {barBg.x, barBg.y, barW * hazVis.qteProgress, barH};
        DrawRectangleRounded(barFg, 0.5f, 10, GREEN);
        DrawRectangleRoundedLines(barBg, 0.5f, 10, {255, 255, 255, 255});
    }

    for (const auto& ent : entities) {
        if (ent.isGrabbed && (ent.HasTag(TAG_TAPE) || ent.HasTag(TAG_BUBBLE_WRAP))) {
            DrawText(TextFormat("%s USES LEFT: %d", ent.name.c_str(), (int)ent.stateTimer), GetScreenWidth() / 2 - 100, GetScreenHeight() - 50, 20, (ent.stateTimer > 0) ? GREEN : RED);
        }
    }

    // Add fontMuseum and fontEmployee to your RenderHUD arguments!
    
    if (showInteractMenu) {
        float screenW = (float)GetScreenWidth();
        float screenH = (float)GetScreenHeight();
        float scale = std::min(screenW / 1280.0f, screenH / 960.0f);
        
        float menuW = 400.0f * scale; 
        float itemH = 60.0f * scale; 
        float menuH = (80.0f * scale) + (interactTargets.size() * itemH);
        float menuX = screenW / 2.0f - menuW / 2.0f; 
        float menuY = screenH / 2.0f - menuH / 2.0f;

        // --- DRAW BACKGROUND PANEL ---
        Rectangle panelRec = {menuX, menuY, menuW, menuH};
        DrawRectangleRounded(panelRec, 0.1f, 10, {15, 15, 20, 240}); // Sleek dark base
        DrawRectangleRoundedLines(panelRec, 0.1f, 10, {218, 165, 32, 255}); // Gold border
        
        // --- DRAW TITLE ---
        const char* title = isDropMenu ? "PLACE ITEM ON:" : "INTERACT WITH?";
        float titleSize = 28.0f * scale;
        Vector2 titleVec = MeasureTextEx(fontMuseum, title, titleSize, 1);
        DrawTextEx(fontMuseum, title, { menuX + menuW/2.0f - titleVec.x/2.0f, menuY + (25.0f * scale) }, titleSize, 1, {218, 165, 32, 255});

        // --- DRAW MENU ITEMS ---
        float textSize = 22.0f * scale;
        for (size_t i = 0; i < interactTargets.size(); ++i) {
            Rectangle itemRec = { menuX + (20.0f * scale), menuY + (80.0f * scale) + i * itemH, menuW - (40.0f * scale), itemH - (10.0f * scale) };
            
            bool isSelected = (i == interactSelectedIndex);
            
            if (isSelected) {
                DrawRectangleRounded(itemRec, 0.2f, 8, {218, 165, 32, 200}); // Golden Highlight
            } else {
                DrawRectangleRounded(itemRec, 0.2f, 8, {40, 40, 50, 200}); // Unselected gray
            }
            
            std::string targetName = entities[interactTargets[i]].name;
            Vector2 textVec = MeasureTextEx(fontEmployee, targetName.c_str(), textSize, 1);
            
            Color textColor = isSelected ? BLACK : WHITE;
            DrawTextEx(fontEmployee, targetName.c_str(), { itemRec.x + itemRec.width/2.0f - textVec.x/2.0f, itemRec.y + itemRec.height/2.0f - textVec.y/2.0f }, textSize, 1, textColor);
        }
    }
}