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

// --- Interaction UI State ---
bool showInteractMenu = false;
bool isDropMenu = false;
std::vector<int> interactTargets;
int interactSelectedIndex = 0;

Color DarkenColor(Color c, float amount) { return { (unsigned char)(c.r - c.r*amount), (unsigned char)(c.g - c.g*amount), (unsigned char)(c.b - c.b*amount), c.a }; }

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 960, "Museum Tech Support - Phase 11");
    SetTargetFPS(60);

    RenderTexture2D renderTarget = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_BILINEAR);

    InitLevel(entities);
    camera.zoom = 1.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();
        Entity& player = entities[0];

        // --- Execute Actions Queue ---
        bool executeAction = false;
        int actionTargetIdx = -1;

        // --- 0. Global Triggers ---
        if (IsKeyPressed(KEY_I)) { for (auto& e : entities) if (e.HasTag(TAG_MEDUSA)) e.isGlitching = !e.isGlitching; }
        if (IsKeyPressed(KEY_O)) { for (auto& e : entities) if (e.HasTag(TAG_WATER_SOURCE)) e.isGlitching = !e.isGlitching; }
        if (IsKeyPressed(KEY_P)) { for (auto& e : entities) if (e.HasTag(TAG_SANDALS)) e.isGlitching = true; }
        if (IsKeyPressed(KEY_K)) { for (auto& e : entities) if (e.HasTag(TAG_WIND_BAG)) e.isGlitching = !e.isGlitching; }
        
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

        // --- 1. Input Context Switcher (Menu vs Player) ---
        if (showInteractMenu) {
            // Menu Controls
            if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) interactSelectedIndex = (interactSelectedIndex - 1 + interactTargets.size()) % interactTargets.size();
            if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) interactSelectedIndex = (interactSelectedIndex + 1) % interactTargets.size();

            bool clicked = false;
            Vector2 mousePos = GetMousePosition();
            float menuW = 400; float itemH = 50;
            float menuH = 60 + interactTargets.size() * itemH;
            float menuX = GetScreenWidth() / 2.0f - menuW / 2.0f;
            float menuY = GetScreenHeight() / 2.0f - menuH / 2.0f;

            for (size_t i = 0; i < interactTargets.size(); ++i) {
                Rectangle itemRec = { menuX + 20, menuY + 60 + i * itemH, menuW - 40, itemH - 10 };
                if (CheckCollisionPointRec(mousePos, itemRec)) {
                    if (Vector2Length(GetMouseDelta()) > 0.1f) interactSelectedIndex = i; // Highlight on move
                    if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) clicked = true;
                }
            }

            if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || clicked) {
                executeAction = true;
                actionTargetIdx = interactTargets[interactSelectedIndex];
                showInteractMenu = false;
            }
            if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ESCAPE)) showInteractMenu = false;

        } else if (!player.isStone && !player.isDead) {
            // Standard Player Movement & Actions
            Vector2 input = { 0.0f, 0.0f };
            float currentAccel = 3500.0f;
            float currentMaxSpeed = 700.0f;
            if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_HEAVY)) { currentAccel = 1500.0f; currentMaxSpeed = 250.0f; }

            if (IsKeyDown(KEY_W)) input.y -= 1.0f; if (IsKeyDown(KEY_S)) input.y += 1.0f;
            if (IsKeyDown(KEY_A)) input.x -= 1.0f; if (IsKeyDown(KEY_D)) input.x += 1.0f;
            if (Vector2Length(input) > 0.0f) {
                input = Vector2Normalize(input); player.facingDir = input;
                player.velocity.x += input.x * currentAccel * dt; player.velocity.y += input.y * currentAccel * dt;
            }

            if (grabbedEntityIndex != -1) entities[grabbedEntityIndex].isUsing = IsKeyDown(KEY_F);

            if (IsKeyPressed(KEY_F)) {
                bool environmentF = false;
                Rectangle pIntBox = player.GetWorldInteractionBox(); pIntBox.width += 80; pIntBox.height += 80; 
                
                // 1. Fusebox Toggle
                for (auto& e : entities) {
                    if (e.HasTag(TAG_FUSEBOX) && CheckCollisionRecs(pIntBox, e.GetWorldInteractionBox())) {
                        e.stateValue = (e.stateValue > 0.5f) ? 0.0f : 1.0f; environmentF = true; break;
                    }
                }
                
                // 2. Chemistry Tool Use (Tape -> Vase, Cork -> Cup, Glasses -> Medusa)
                if (!environmentF && grabbedEntityIndex != -1) {
                    for (size_t i = 1; i < entities.size(); ++i) {
                        if (i != grabbedEntityIndex && CheckCollisionRecs(pIntBox, entities[i].GetWorldInteractionBox())) {
                            ChemResult res = ProcessChemistry(grabbedEntityIndex, i, entities);
                            if (res != CHEM_NONE) {
                                environmentF = true;
                                if (res == CHEM_ATTACHED) { entities[grabbedEntityIndex].isGrabbed = false; grabbedEntityIndex = -1; }
                                break;
                            }
                        }
                    }
                }

                // 3. Equip Glasses/Gloves to self
                if (!environmentF) {
                    if (grabbedEntityIndex != -1) {
                        if (entities[grabbedEntityIndex].HasTag(TAG_EYEWEAR)) { equippedEyewear = grabbedEntityIndex; entities[grabbedEntityIndex].isGrabbed = false; grabbedEntityIndex = -1; }
                        else if (entities[grabbedEntityIndex].HasTag(TAG_GLOVES)) { equippedGloves = grabbedEntityIndex; entities[grabbedEntityIndex].isGrabbed = false; grabbedEntityIndex = -1; }
                    } else {
                        if (equippedGloves != -1) { grabbedEntityIndex = equippedGloves; entities[equippedGloves].isGrabbed = true; equippedGloves = -1; }
                        else if (equippedEyewear != -1) { grabbedEntityIndex = equippedEyewear; entities[equippedEyewear].isGrabbed = true; equippedEyewear = -1; }
                    }
                }
            }

            // GATHER TARGETS FOR MENU/EXECUTION
            if (IsKeyPressed(KEY_E)) {
                interactTargets.clear();
                
                if (grabbedEntityIndex != -1) {
                    // Gather Env Snapping Targets (Pedestals, Zeus, Fusebox)
                    Entity& item = entities[grabbedEntityIndex];
                    Vector2 dropPos = { player.position.x + player.facingDir.x * 60.0f, player.position.y + player.facingDir.y * 60.0f };
                    Rectangle dropBox = item.movementBox; dropBox.x += dropPos.x; dropBox.y += dropPos.y;

                    for(size_t i = 0; i < entities.size(); ++i) {
                        if(i != grabbedEntityIndex && CheckCollisionRecs(dropBox, entities[i].GetWorldInteractionBox())) {
                            if (entities[i].name == "Pedestal" || entities[i].HasTag(TAG_ZEUS) || (entities[i].HasTag(TAG_FUSEBOX) && entities[i].stateValue > 0.5f && item.HasTag(TAG_ELECTRIC))) {
                                interactTargets.push_back(i);
                            }
                        }
                    }

                    if (interactTargets.empty()) {
                        // Free drop on floor!
                        float targetZ = 0.0f;
                        for(size_t i = 0; i < entities.size(); ++i) if(i != grabbedEntityIndex && entities[i].isSolid && CheckCollisionRecs(dropBox, entities[i].GetWorldMovementBox()) && entities[i].zHeight > targetZ) targetZ = entities[i].zHeight; 
                        item.position = dropPos; item.z = targetZ; 
                        if (item.HasTag(TAG_SANDALS)) item.stateTimer = 4.0f;
                        item.isGrabbed = false; item.isUsing = false; item.velocity = {0,0}; item.zVelocity = 0; grabbedEntityIndex = -1;
                    } else if (interactTargets.size() == 1) {
                        executeAction = true; actionTargetIdx = interactTargets[0]; isDropMenu = true;
                    } else {
                        showInteractMenu = true; isDropMenu = true; interactSelectedIndex = 0;
                    }

                } else {
                    // Gather Grab Targets (Tightened hitbox to stop spam!)
                    Rectangle pIntBox = player.GetWorldInteractionBox(); 
                    pIntBox.x -= 40; pIntBox.y -= 100; pIntBox.width += 80; pIntBox.height += 150; 

                    for (size_t i = 1; i < entities.size(); ++i) {
                        if (i == equippedEyewear || i == equippedGloves) continue; 
                        if (!entities[i].isGrabbed && entities[i].canGrab && CheckCollisionRecs(pIntBox, entities[i].GetWorldInteractionBox())) interactTargets.push_back(i);
                    }

                    std::sort(interactTargets.begin(), interactTargets.end(), [&](int a, int b) {
                        return Vector2Distance(player.position, entities[a].position) < Vector2Distance(player.position, entities[b].position);
                    });

                    if (interactTargets.size() == 1) {
                        executeAction = true; actionTargetIdx = interactTargets[0]; isDropMenu = false;
                    } else if (interactTargets.size() > 1) {
                        showInteractMenu = true; isDropMenu = false; interactSelectedIndex = 0;
                    }
                }
            }

            if (IsKeyPressed(KEY_SPACE) && grabbedEntityIndex != -1 && entities[grabbedEntityIndex].canThrow) {
                entities[grabbedEntityIndex].isGrabbed = false; entities[grabbedEntityIndex].isUsing = false; 
                entities[grabbedEntityIndex].velocity = Vector2Scale(player.facingDir, 1200.0f); entities[grabbedEntityIndex].zVelocity = 450.0f; 
                if (entities[grabbedEntityIndex].HasTag(TAG_SANDALS)) entities[grabbedEntityIndex].isGlitching = true;
                grabbedEntityIndex = -1;
            }
        }

        // --- 1.5 DECOUPLED ACTION EXECUTION ---
        if (executeAction) {
            if (!isDropMenu) {
                // Grab Execution
                grabbedEntityIndex = actionTargetIdx;
                entities[grabbedEntityIndex].isGrabbed = true;
                entities[grabbedEntityIndex].attachedTo = -1;
            } else {
                // Drop Snapping Execution
                Entity& item = entities[grabbedEntityIndex];
                item.attachedTo = actionTargetIdx; // Snap directly to Pedestal/Zeus!
                item.isGrabbed = false; item.isUsing = false; item.velocity = {0,0}; item.zVelocity = 0; grabbedEntityIndex = -1;
            }
        }

        // --- 2. Physics & Kinematics ---
        for (size_t i = 0; i < entities.size(); ++i) {
            Entity& e = entities[i];
            
            // FIXED: If the item is grabbed, lock it to the player and bypass gravity entirely!
            if ((int)i == grabbedEntityIndex) {
                e.position = player.position; e.z = 100.0f; e.velocity = {0,0}; e.zVelocity = 0;
                continue;
            }
            
            if (e.attachedTo != -1) {
                Entity& parent = entities[e.attachedTo];
                e.position = parent.position; e.z = parent.z + parent.zHeight; 
                if (parent.HasTag(TAG_ZEUS)) { e.position.x -= 40.0f; e.z += 40.0f; } 
                if (parent.HasTag(TAG_FUSEBOX)) { e.position.y += 20.0f; e.z -= 20.0f; } 
                e.velocity = {0,0}; e.zVelocity = 0; continue; 
            }
            if ((int)i == equippedEyewear || (int)i == equippedGloves) {
                e.position = player.position; e.z = player.z + ((int)i == equippedEyewear ? 60.0f : 40.0f); e.velocity = {0,0}; e.zVelocity = 0; continue;
            }

            e.velocity.x -= e.velocity.x * 6.0f * dt; e.velocity.y -= e.velocity.y * 6.0f * dt;
            if (&e == &player && Vector2Length(e.velocity) > 700.0f) e.velocity = Vector2Scale(Vector2Normalize(e.velocity), 700.0f);

            if (e.HasTag(TAG_SANDALS) && e.isGlitching && !e.isStone && !e.isGrabbed) {
                e.z = 150.0f; 
                if (GetRandomValue(0, 100) < 10) { e.velocity.x = (GetRandomValue(-100, 100) / 100.0f) * 1000.0f; e.velocity.y = (GetRandomValue(-100, 100) / 100.0f) * 1000.0f; }
            }
            if (e.HasTag(TAG_SANDALS) && !e.isGlitching && !e.isGrabbed && !e.isStone && e.z < 20.0f) {
                if (e.stateTimer > 0.0f) { e.stateTimer -= dt; if (e.stateTimer <= 0.0f) e.isGlitching = true; }
            }

            float groundZ = 0.0f;
            Rectangle myFeet = e.GetWorldMovementBox();
            for (const auto& other : entities) {
                if (&e != &other && other.isSolid && !other.isGrabbed && CheckCollisionRecs(myFeet, other.GetWorldMovementBox()) && e.z >= other.zHeight - 5.0f && other.zHeight > groundZ) groundZ = other.zHeight;
            }

            if (e.z > groundZ || e.zVelocity != 0.0f) {
                float prevZVel = e.zVelocity; 
                e.zVelocity -= (e.HasTag(TAG_SANDALS) && e.isGlitching ? 0.0f : 1200.0f) * dt; 
                e.z += e.zVelocity * dt;
                if (e.z <= groundZ) {
                    e.z = groundZ; e.zVelocity *= -0.3f;
                    if (prevZVel < -300.0f) ProcessImpact(e);
                    if (abs(e.zVelocity) < 30.0f) e.zVelocity = 0.0f; e.velocity = Vector2Scale(e.velocity, 0.4f);
                }
            }

            if (abs(e.velocity.x) > 0.1f && !e.isGrabbed) {
                e.position.x += e.velocity.x * dt;
                Rectangle nextX = e.GetWorldMovementBox();
                for (auto& other : entities) {
                    if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1 && CheckCollisionRecs(nextX, other.GetWorldMovementBox()) && e.z < other.zHeight) {
                        e.position.x -= e.velocity.x * dt; e.velocity.x *= -0.5f; break;
                    }
                }
            }
            if (abs(e.velocity.y) > 0.1f && !e.isGrabbed) {
                e.position.y += e.velocity.y * dt;
                Rectangle nextY = e.GetWorldMovementBox();
                for (auto& other : entities) {
                    if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1 && CheckCollisionRecs(nextY, other.GetWorldMovementBox()) && e.z < other.zHeight) {
                        e.position.y -= e.velocity.y * dt; e.velocity.y *= -0.5f; break;
                    }
                }
            }

            // Water Processing
            if (e.HasTag(TAG_WATER_SOURCE)) {
                if (e.isGlitching) {
                    float maxRadius = 9999.0f;
                    for (const auto& sp : entities) if (sp.HasTag(TAG_SPONGE) && !sp.isGrabbed && sp.z < 20.0f) { float dist = Vector2Distance(e.position, sp.position); if (dist - 40.0f < maxRadius) maxRadius = dist - 40.0f; }
                    if (e.stateValue < maxRadius) { e.stateValue += 150.0f * dt; if (e.stateValue > maxRadius) e.stateValue = maxRadius; }
                }
                if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_MOP) && entities[grabbedEntityIndex].isUsing) {
                    if (Vector2Distance(player.position, e.position) < e.stateValue + 100.0f) { e.stateValue -= 400.0f * dt; if (e.stateValue < 0) e.stateValue = 0; }
                }
                if (e.stateTimer > 0.0f) { e.stateTimer -= dt; if (e.stateTimer <= 0.0f) e.isGlitching = true; }
            }
            if (e.HasTag(TAG_SANDALS) && e.isGlitching) {
                for (const auto& w : entities) if (w.HasTag(TAG_WATER_SOURCE) && w.stateValue > 0.0f && Vector2Distance(e.position, w.position) < w.stateValue) {
                    e.isGlitching = false; e.color = DARKBLUE; e.stateTimer = 0.0f; break; 
                }
            }
        }

        // Lightning Death Checking
        bool playerInWater = false;
        for (const auto& w : entities) if (w.HasTag(TAG_WATER_SOURCE) && w.stateValue > 0.0f && Vector2Distance(player.position, w.position) < w.stateValue) playerInWater = true;
        if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_ELECTRIC) && !entities[grabbedEntityIndex].isStone) {
            if (equippedGloves == -1 || playerInWater) player.isDead = true; 
        }

        // --- Render Data Preparation ---
        Vector2 beamP1, beamP2, beamP3; bool drawingBeam = false;
        Vector2 extP1, extP2, extP3; bool drawingExtinguisher = false;

        for (int i = 0; i < entities.size(); ++i) {
            Entity& e = entities[i];
            if (e.HasTag(TAG_MEDUSA) && e.isGlitching) {
                bool isBlocked = false;
                for (const auto& other : entities) if (other.attachedTo == i && other.HasTag(TAG_EYEWEAR) && !other.HasTag(TAG_BROKEN)) isBlocked = true;
                if (!isBlocked) {
                    drawingBeam = true; beamP1 = { e.position.x, e.position.y - e.z }; beamP2 = { e.position.x - 300.0f, e.position.y + 600.0f }; beamP3 = { e.position.x + 300.0f, e.position.y + 600.0f };
                    for (auto& target : entities) {
                        if (&target != &e && !target.isStone && (target.canGrab || target.name == "Player") && CheckCollisionPointTriangle(target.position, beamP1, beamP2, beamP3)) {
                            bool immune = (target.name == "Player" && equippedEyewear != -1 && !entities[equippedEyewear].HasTag(TAG_BROKEN));
                            if (!immune) { target.isStone = true; target.color = GRAY; target.isGlitching = false; }
                        }
                    }
                }
            }
            if (e.isUsing && e.HasTag(TAG_EXTINGUISHER)) {
                drawingExtinguisher = true; Vector2 dir = player.facingDir; Vector2 perp = { -dir.y, dir.x }; 
                extP1 = e.position; extP2 = { e.position.x + dir.x * 400.0f + perp.x * 200.0f, e.position.y + dir.y * 400.0f + perp.y * 200.0f }; extP3 = { e.position.x + dir.x * 400.0f - perp.x * 200.0f, e.position.y + dir.y * 400.0f - perp.y * 200.0f };
            }
            // Bag of Winds Push Logic
            if (e.HasTag(TAG_WIND_BAG) && e.isGlitching) {
                for (auto& target : entities) {
                    if (&target != &e && target.attachedTo == -1 && !target.isStone && target.name != "Wall" && target.name != "Door" && target.name != "Pedestal") {
                        float dist = Vector2Distance(e.position, target.position);
                        if (dist < 600.0f && dist > 5.0f) {
                            Vector2 pushDir = Vector2Normalize(Vector2Subtract(target.position, e.position));
                            float force = (600.0f - dist) * 4.0f; 
                            if (target.HasTag(TAG_HEAVY)) force *= 0.1f; // Heavy objects barely move
                            if (target.name == "Player") force *= 0.6f;  // Player fights the wind
                            
                            target.velocity.x += pushDir.x * force * dt;
                            target.velocity.y += pushDir.y * force * dt;
                        }
                    }
                }
            }
        }

        // --- Camera & World Rendering ---
        int currentRoomX = (int)std::floor((player.position.x) / GAME_WIDTH); int currentRoomY = (int)std::floor((player.position.y) / GAME_HEIGHT);
        camera.target.x += (currentRoomX * GAME_WIDTH - camera.target.x) * 5.0f * dt; camera.target.y += (currentRoomY * GAME_HEIGHT - camera.target.y) * 5.0f * dt;

        std::vector<Entity*> renderList; for (auto& e : entities) renderList.push_back(&e);
        std::sort(renderList.begin(), renderList.end(), [](const Entity* a, const Entity* b) { return (a->position.y + (a->isGrabbed ? 0.1f : 0.0f)) < (b->position.y + (b->isGrabbed ? 0.1f : 0.0f)); });

        BeginTextureMode(renderTarget); ClearBackground(RAYWHITE); BeginMode2D(camera);
        for (int i = -1600; i < 4800; i += 100) DrawLine(i, -1200, i, 2400, LIGHTGRAY);
        for (int i = -1200; i < 2400; i += 100) DrawLine(-1600, i, 4800, i, LIGHTGRAY);

        for (const auto& e : entities) if (e.HasTag(TAG_WATER_SOURCE) && e.stateValue > 0.0f) DrawEllipse(e.position.x, e.position.y, e.stateValue, e.stateValue * 0.6f, {0, 121, 241, 150});
        
        if (drawingBeam) DrawTriangle(beamP1, beamP2, beamP3, { 0, 255, 0, 80 }); 
        if (drawingExtinguisher) DrawTriangle(extP1, extP2, extP3, { 255, 255, 255, 150 }); 

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

        EndMode2D(); EndTextureMode();
        
        // --- Screen UI Rendering ---
        BeginDrawing(); ClearBackground(BLACK);
        float scale = std::min((float)GetScreenWidth() / GAME_WIDTH, (float)GetScreenHeight() / GAME_HEIGHT);
        Rectangle sourceRec = { 0.0f, 0.0f, (float)renderTarget.texture.width, (float)-renderTarget.texture.height };
        Rectangle destRec = { (GetScreenWidth() - ((float)GAME_WIDTH * scale)) * 0.5f, (GetScreenHeight() - ((float)GAME_HEIGHT * scale)) * 0.5f, (float)GAME_WIDTH * scale, (float)GAME_HEIGHT * scale };
        DrawTexturePro(renderTarget.texture, sourceRec, destRec, { 0, 0 }, 0.0f, WHITE);
        
        if (player.isStone) DrawText("PETRIFIED!", 10, 40, 30, RED);
        if (player.isDead) DrawText("ZAPPED TO DEATH!", 10, 70, 30, ORANGE);

        DrawText("WASD: Move | 1-5: Doors | E: Grab | HOLD F: Use | PRESS F: Equip | SPACE: Throw | P/I/O: Triggers", 10, 10, 20, WHITE);

        // --- Interaction UI Overlay ---
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
    UnloadRenderTexture(renderTarget); CloseWindow(); return 0;
}