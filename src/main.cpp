#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include "Entities.h"
#include "Level.h"
#include "Interactions.h"

std::vector<Entity> entities;
int grabbedEntityIndex = -1;
int equippedEyewear = -1; 
int equippedGloves = -1; 
Camera2D camera = { 0 };

bool doorsOpen[5] = {false, false, false, false, false}; 

Color DarkenColor(Color c, float amount) { return { (unsigned char)(c.r - c.r*amount), (unsigned char)(c.g - c.g*amount), (unsigned char)(c.b - c.b*amount), c.a }; }

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 960, "Museum Tech Support - Phase 8");
    SetTargetFPS(60);

    RenderTexture2D renderTarget = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_BILINEAR);

    InitLevel(entities);
    camera.zoom = 1.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Entity& player = entities[0];

        // --- 0. Global Triggers ---
        if (IsKeyPressed(KEY_I)) { for (auto& e : entities) if (e.HasTag(TAG_MEDUSA)) e.isGlitching = !e.isGlitching; }
        if (IsKeyPressed(KEY_O)) { for (auto& e : entities) if (e.HasTag(TAG_WATER_SOURCE)) e.isGlitching = !e.isGlitching; }

        if (IsKeyPressed(KEY_ONE)) doorsOpen[0] = !doorsOpen[0];
        if (IsKeyPressed(KEY_TWO)) doorsOpen[1] = !doorsOpen[1];
        if (IsKeyPressed(KEY_THREE)) doorsOpen[2] = !doorsOpen[2];
        if (IsKeyPressed(KEY_FOUR)) doorsOpen[3] = !doorsOpen[3];
        if (IsKeyPressed(KEY_FIVE)) doorsOpen[4] = !doorsOpen[4];
        
        for(auto& e : entities) {
            if (e.HasTag(TAG_DOOR_1)) { e.isSolid = !doorsOpen[0]; e.color.a = doorsOpen[0] ? 30 : 255; }
            if (e.HasTag(TAG_DOOR_2)) { e.isSolid = !doorsOpen[1]; e.color.a = doorsOpen[1] ? 30 : 255; }
            if (e.HasTag(TAG_DOOR_3)) { e.isSolid = !doorsOpen[2]; e.color.a = doorsOpen[2] ? 30 : 255; }
            if (e.HasTag(TAG_DOOR_4)) { e.isSolid = !doorsOpen[3]; e.color.a = doorsOpen[3] ? 30 : 255; }
            if (e.HasTag(TAG_DOOR_5)) { e.isSolid = !doorsOpen[4]; e.color.a = doorsOpen[4] ? 30 : 255; }
        }

        // --- 1. Player Input & Encumbrance ---
        Vector2 input = { 0.0f, 0.0f };
        float currentAccel = 3500.0f;
        float currentMaxSpeed = 700.0f;
        
        // HEAVY ITEM PENALTY
        if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_HEAVY)) {
            currentAccel = 1500.0f;
            currentMaxSpeed = 250.0f;
        }

        if (!player.isStone) {
            if (IsKeyDown(KEY_W)) input.y -= 1.0f; if (IsKeyDown(KEY_S)) input.y += 1.0f;
            if (IsKeyDown(KEY_A)) input.x -= 1.0f; if (IsKeyDown(KEY_D)) input.x += 1.0f;
            if (Vector2Length(input) > 0.0f) {
                input = Vector2Normalize(input); player.facingDir = input;
                player.velocity.x += input.x * currentAccel * dt; 
                player.velocity.y += input.y * currentAccel * dt;
            }
        }

        // --- 2. Physics & Kinematics ---
        for (size_t i = 0; i < entities.size(); ++i) {
            Entity& e = entities[i];
            
            if (e.attachedTo != -1) {
                e.position = entities[e.attachedTo].position; e.z = entities[e.attachedTo].z + entities[e.attachedTo].zHeight; e.velocity = {0,0}; e.zVelocity = 0;
                continue; 
            }
            if ((int)i == equippedEyewear || (int)i == equippedGloves) {
                e.position = player.position; 
                e.z = player.z + ((int)i == equippedEyewear ? 60.0f : 40.0f); // Render eyewear higher than gloves
                e.velocity = {0,0}; e.zVelocity = 0;
                continue;
            }

            e.velocity.x -= e.velocity.x * 6.0f * dt; e.velocity.y -= e.velocity.y * 6.0f * dt;
            if (&e == &player && Vector2Length(e.velocity) > currentMaxSpeed) e.velocity = Vector2Scale(Vector2Normalize(e.velocity), currentMaxSpeed);

            float groundZ = 0.0f;
            Rectangle myFeet = e.GetWorldMovementBox();
            for (const auto& other : entities) {
                if (&e != &other && other.isSolid && !other.isGrabbed && CheckCollisionRecs(myFeet, other.GetWorldMovementBox()) && e.z >= other.zHeight - 5.0f && other.zHeight > groundZ) {
                    groundZ = other.zHeight;
                }
            }

            if (e.z > groundZ || e.zVelocity != 0.0f) {
                float prevZVel = e.zVelocity; 
                e.zVelocity -= 1200.0f * dt; e.z += e.zVelocity * dt;
                if (e.z <= groundZ) {
                    e.z = groundZ; e.zVelocity *= -0.3f;
                    if (prevZVel < -300.0f) ProcessImpact(e);
                    if (abs(e.zVelocity) < 30.0f) e.zVelocity = 0.0f;
                    e.velocity = Vector2Scale(e.velocity, 0.4f);
                }
            }

            if (abs(e.velocity.x) > 0.1f && !e.isGrabbed) {
                e.position.x += e.velocity.x * dt;
                if (e.isSolid || &e == &player) {
                    Rectangle nextX = e.GetWorldMovementBox();
                    for (auto& other : entities) {
                        if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1 && CheckCollisionRecs(nextX, other.GetWorldMovementBox()) && e.z < other.zHeight) {
                            e.position.x -= e.velocity.x * dt; e.velocity.x *= -0.5f; break;
                        }
                    }
                }
            }
            if (abs(e.velocity.y) > 0.1f && !e.isGrabbed) {
                e.position.y += e.velocity.y * dt;
                if (e.isSolid || &e == &player) {
                    Rectangle nextY = e.GetWorldMovementBox();
                    for (auto& other : entities) {
                        if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1 && CheckCollisionRecs(nextY, other.GetWorldMovementBox()) && e.z < other.zHeight) {
                            e.position.y -= e.velocity.y * dt; e.velocity.y *= -0.5f; break;
                        }
                    }
                }
            }

            // --- Holy Cup & Water System ---
            if (e.HasTag(TAG_WATER_SOURCE)) {
                if (e.isGlitching) {
                    float maxRadius = 9999.0f;
                    // Check for Sandbag dams!
                    for (const auto& sb : entities) {
                        if (sb.HasTag(TAG_SANDBAG) && !sb.isGrabbed && sb.z < 20.0f) { // Must be on floor
                            float dist = Vector2Distance(e.position, sb.position);
                            if (dist - 40.0f < maxRadius) maxRadius = dist - 40.0f; 
                        }
                    }

                    if (e.stateValue < maxRadius) {
                        e.stateValue += 150.0f * dt;
                        if (e.stateValue > maxRadius) e.stateValue = maxRadius; // Cap at the sandbag
                    }
                }
                
                // Mop Cleaning (Only recedes if Mop is actively used)
                if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_MOP) && entities[grabbedEntityIndex].isUsing) {
                    float distToWater = Vector2Distance(player.position, e.position);
                    if (distToWater < e.stateValue + 100.0f) {
                        e.stateValue -= 400.0f * dt; // Scrub it away!
                        if (e.stateValue < 0) e.stateValue = 0;
                    }
                }

                if (e.stateTimer > 0.0f) {
                    e.stateTimer -= dt; 
                    if (e.stateTimer <= 0.0f) e.isGlitching = true; // Tape breaks
                }
            }
        }

        // --- 3. Interaction ---
        if (!player.isStone) {
            
            // USE CONTINUOUS TOOL (Holding F)
            if (grabbedEntityIndex != -1) {
                if (IsKeyDown(KEY_F)) entities[grabbedEntityIndex].isUsing = true;
                else entities[grabbedEntityIndex].isUsing = false;
            }

            // EQUIP / UNEQUIP (Pressing F)
            if (IsKeyPressed(KEY_F)) {
                if (grabbedEntityIndex != -1) {
                    if (entities[grabbedEntityIndex].HasTag(TAG_EYEWEAR)) { equippedEyewear = grabbedEntityIndex; entities[grabbedEntityIndex].isGrabbed = false; grabbedEntityIndex = -1; }
                    else if (entities[grabbedEntityIndex].HasTag(TAG_GLOVES)) { equippedGloves = grabbedEntityIndex; entities[grabbedEntityIndex].isGrabbed = false; grabbedEntityIndex = -1; }
                } else {
                    // Unequip logic (Pop gloves first, then glasses)
                    if (equippedGloves != -1) { grabbedEntityIndex = equippedGloves; entities[equippedGloves].isGrabbed = true; equippedGloves = -1; }
                    else if (equippedEyewear != -1) { grabbedEntityIndex = equippedEyewear; entities[equippedEyewear].isGrabbed = true; equippedEyewear = -1; }
                }
            }

            // GRAB / DROP (E)
            if (IsKeyPressed(KEY_E)) {
                if (grabbedEntityIndex != -1) {
                    Entity& item = entities[grabbedEntityIndex];
                    Vector2 dropPos = { player.position.x + player.facingDir.x * 60.0f, player.position.y + player.facingDir.y * 60.0f };
                    Rectangle dropBox = item.movementBox; dropBox.x += dropPos.x; dropBox.y += dropPos.y;
                    
                    bool chemApplied = false;
                    for(size_t i = 0; i < entities.size(); ++i) {
                        if(i != grabbedEntityIndex && CheckCollisionRecs(dropBox, entities[i].GetWorldInteractionBox())) {
                            ChemResult res = ProcessChemistry(grabbedEntityIndex, i, entities);
                            if (res != CHEM_NONE) {
                                chemApplied = true;
                                if (res == CHEM_ATTACHED) { item.isGrabbed = false; item.isUsing = false; grabbedEntityIndex = -1; }
                                break; 
                            }
                        }
                    }

                    if (!chemApplied) {
                        float targetZ = 0.0f;
                        for(size_t i = 0; i < entities.size(); ++i) {
                            if(i != grabbedEntityIndex && entities[i].isSolid && CheckCollisionRecs(dropBox, entities[i].GetWorldMovementBox())) {
                                if (entities[i].zHeight > targetZ) targetZ = entities[i].zHeight; 
                            }
                        }
                        item.position = dropPos; item.z = targetZ; 
                        item.isGrabbed = false; item.isUsing = false; item.velocity = {0,0}; item.zVelocity = 0;
                        grabbedEntityIndex = -1;
                    }
                    
                } else {
                    Rectangle pIntBox = player.GetWorldInteractionBox();
                    pIntBox.x -= 60; pIntBox.y -= 120; pIntBox.width += 120; pIntBox.height += 160; 
                    float bestDist = 99999.0f; int bestIndex = -1;

                    for (size_t i = 1; i < entities.size(); ++i) {
                        if (i == equippedEyewear || i == equippedGloves) continue; 
                        
                        if (!entities[i].isGrabbed && CheckCollisionRecs(pIntBox, entities[i].GetWorldInteractionBox())) {
                            float dist = Vector2Distance(player.position, entities[i].position);
                            if (dist < bestDist) { bestDist = dist; bestIndex = static_cast<int>(i); }
                        }
                    }
                    if (bestIndex != -1 && entities[bestIndex].canGrab) {
                        grabbedEntityIndex = bestIndex; entities[bestIndex].isGrabbed = true; entities[bestIndex].attachedTo = -1;
                    }
                }
            }

            if (IsKeyPressed(KEY_SPACE) && grabbedEntityIndex != -1 && entities[grabbedEntityIndex].canThrow) {
                entities[grabbedEntityIndex].isGrabbed = false; entities[grabbedEntityIndex].isUsing = false; 
                entities[grabbedEntityIndex].velocity = Vector2Scale(player.facingDir, 1200.0f); entities[grabbedEntityIndex].zVelocity = 450.0f; 
                grabbedEntityIndex = -1;
            }
        }

        if (grabbedEntityIndex != -1) { entities[grabbedEntityIndex].position = player.position; entities[grabbedEntityIndex].z = 100.0f; }

        // --- 4. Special Rendering & Logic Data ---
        Vector2 beamP1, beamP2, beamP3;
        bool drawingBeam = false;
        Vector2 extP1, extP2, extP3;
        bool drawingExtinguisher = false;

        for (int i = 0; i < entities.size(); ++i) {
            Entity& e = entities[i];
            
            // Medusa
            if (e.HasTag(TAG_MEDUSA) && e.isGlitching) {
                bool isBlocked = false;
                for (const auto& other : entities) if (other.attachedTo == i && other.HasTag(TAG_EYEWEAR) && !other.HasTag(TAG_BROKEN)) isBlocked = true;
                if (!isBlocked) {
                    drawingBeam = true; beamP1 = { e.position.x, e.position.y - e.z };
                    beamP2 = { e.position.x - 300.0f, e.position.y + 600.0f }; beamP3 = { e.position.x + 300.0f, e.position.y + 600.0f };
                    for (auto& target : entities) {
                        if (&target != &e && !target.isStone && target.name == "Player" && CheckCollisionPointTriangle(target.position, beamP1, beamP2, beamP3)) {
                            if (equippedEyewear == -1 || entities[equippedEyewear].HasTag(TAG_BROKEN)) { target.isStone = true; target.color = GRAY; }
                        }
                    }
                }
            }
            
            // Extinguisher Cone Math
            if (e.isUsing && e.HasTag(TAG_EXTINGUISHER)) {
                drawingExtinguisher = true;
                Vector2 dir = player.facingDir;
                Vector2 perp = { -dir.y, dir.x }; // Perpendicular vector for width
                extP1 = e.position;
                extP2 = { e.position.x + dir.x * 400.0f + perp.x * 200.0f, e.position.y + dir.y * 400.0f + perp.y * 200.0f };
                extP3 = { e.position.x + dir.x * 400.0f - perp.x * 200.0f, e.position.y + dir.y * 400.0f - perp.y * 200.0f };
                // We will add fire extinguishing logic here later!
            }
        }

        // --- 5. 2D Camera Scrolling & Rendering ---
        int currentRoomX = (int)std::floor((player.position.x) / GAME_WIDTH);
        int currentRoomY = (int)std::floor((player.position.y) / GAME_HEIGHT);
        camera.target.x += (currentRoomX * GAME_WIDTH - camera.target.x) * 5.0f * dt;
        camera.target.y += (currentRoomY * GAME_HEIGHT - camera.target.y) * 5.0f * dt;

        std::vector<Entity*> renderList; for (auto& e : entities) renderList.push_back(&e);
        std::sort(renderList.begin(), renderList.end(), [](const Entity* a, const Entity* b) { return (a->position.y + (a->isGrabbed ? 0.1f : 0.0f)) < (b->position.y + (b->isGrabbed ? 0.1f : 0.0f)); });

        BeginTextureMode(renderTarget); ClearBackground(RAYWHITE); BeginMode2D(camera);
        for (int i = -1600; i < 4800; i += 100) DrawLine(i, -1200, i, 2400, LIGHTGRAY);
        for (int i = -1200; i < 2400; i += 100) DrawLine(-1600, i, 4800, i, LIGHTGRAY);

        // Render Water
        for (const auto& e : entities) if (e.HasTag(TAG_WATER_SOURCE) && e.stateValue > 0.0f) DrawEllipse(e.position.x, e.position.y, e.stateValue, e.stateValue * 0.6f, {0, 121, 241, 150});
        
        // Render Effect Cones
        if (drawingBeam) DrawTriangle(beamP1, beamP2, beamP3, { 0, 255, 0, 80 }); 
        if (drawingExtinguisher) DrawTriangle(extP1, extP2, extP3, { 255, 255, 255, 150 }); 

        // Render Entities
        for (const Entity* e : renderList) {
            if (e->color.a < 255) continue; 
            if (e->castsShadow && e->movementBox.width > 0 && !e->isGrabbed && e->attachedTo == -1) DrawEllipse(e->position.x, e->position.y, e->movementBox.width * 0.6f, e->movementBox.height * 0.8f, {0, 0, 0, 80});

            if (e->is3DBlock && e->zHeight > 0) {
                Rectangle frontFace = { e->position.x + e->movementBox.x, e->position.y + e->movementBox.y - e->zHeight - e->z, e->movementBox.width, e->zHeight };
                DrawRectangleRec(frontFace, DarkenColor(e->color, 0.4f)); DrawRectangleLinesEx(frontFace, 2.0f, BLACK);
                Rectangle topFace = { e->position.x + e->movementBox.x, e->position.y + e->movementBox.y - e->zHeight - e->z, e->movementBox.width, e->movementBox.height };
                DrawRectangleRec(topFace, e->color); DrawRectangleLinesEx(topFace, 2.0f, BLACK);
            } else {
                Rectangle drawRec = e->GetWorldInteractionBox(); DrawRectangleRec(drawRec, e->color); DrawRectangleLinesEx(drawRec, 2.0f, BLACK); 
            }
            if (e->name != "Wall" && e->name != "Door" && e->name != "Player" && e->name != "Pedestal") {
                DrawText(e->name.c_str(), e->position.x - (MeasureText(e->name.c_str(), 10) / 2), e->GetWorldInteractionBox().y - 15, 10, BLACK);
            }
        }

        EndMode2D(); EndTextureMode();
        BeginDrawing(); ClearBackground(BLACK);
        float scale = std::min((float)GetScreenWidth() / GAME_WIDTH, (float)GetScreenHeight() / GAME_HEIGHT);
        Rectangle sourceRec = { 0.0f, 0.0f, (float)renderTarget.texture.width, (float)-renderTarget.texture.height };
        Rectangle destRec = { (GetScreenWidth() - ((float)GAME_WIDTH * scale)) * 0.5f, (GetScreenHeight() - ((float)GAME_HEIGHT * scale)) * 0.5f, (float)GAME_WIDTH * scale, (float)GAME_HEIGHT * scale };
        DrawTexturePro(renderTarget.texture, sourceRec, destRec, { 0, 0 }, 0.0f, WHITE);
        
        if (player.isStone) DrawText("PETRIFIED!", 10, 40, 30, RED);
        DrawText("WASD: Move | 1-5: Doors | E: Grab | HOLD F: Use (Mop/Ext) | PRESS F: Equip | SPACE: Throw", 10, 10, 20, WHITE);
        EndDrawing();
    }
    UnloadRenderTexture(renderTarget); CloseWindow(); return 0;
}