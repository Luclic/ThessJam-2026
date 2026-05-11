#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <vector>
#include "Entities.h"
#include "Level.h"
#include "Interactions.h"
#include "Evaluation.h"
#include "SaveSystem.h"
#include "GameSystems.h"
#include "RenderSystem.h"
#include <unordered_map>
#include <string>

// --- Animation Index Mapping (Quaternius Pack) ---
const int idxIdle = 4;
const int idxInteract = 10;
const int idxRoll = 15;
const int idxRun = 24;
const int idxWalk = 30;

// --- 1. Enums & Constants ---
enum GameState { STATE_MENU, STATE_PLAYING, STATE_REVIEW, STATE_GAMEOVER_FIRED, STATE_GAMEOVER_DEATH };
const float SECONDS_PER_HOUR = 30.0f;

// --- 2. Global Game State ---
GameState currentState = STATE_MENU;
Camera2D camera = { 0 };
std::vector<Entity> entities;

std::unordered_map<std::string, Model> models;

int grabbedEntityIndex = -1;
int equippedEyewear = -1; 
int equippedGloves = -1; 

int currentNight = 1;
float shiftTimer = 0.0f;
ShiftReport lastReport;

// --- 3. UI & Environment State ---
bool showInteractMenu = false;
bool isDropMenu = false;
std::vector<int> interactTargets;
int interactSelectedIndex = 0;
bool doorsOpen[5] = {false, false, false, false, false};

void ResetNight() {
    entities.clear();
    InitLevel(entities);
    
    // INJECT ALL TAGS AND LOGIC HERE!
    AssignEntityRules(entities); 

    grabbedEntityIndex = -1;
    equippedEyewear = -1; 
    equippedGloves = -1; 
    shiftTimer = 0.0f;
    showInteractMenu = false;
    
    if (currentNight > 1) LoadGame(currentNight, entities); 
    SetupNightHazards(currentNight, entities);
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    InitWindow(1280, 960, "Museum Tech Support - Phase 15");
    SetTargetFPS(60);

    // --- LOAD ENVIRONMENT MODELS ---
    const char* modelNames[] = {
        "artifactsign2", "bench1", "bench2", "bench3", "desk", "fence", "floor11", "floor12", "floor21", "floor22", "floor31", "floor32", "floor41", "floor42", 
        "glasscase", "light1", "light2", "painting1", "painting2", "paintinglight", "pillar1", "pillar2", "rope1", "rope2", "sign1", "sign2", "sign3", 
        "stand1", "stand2", "stand3", "stand4", "ticketstand", "ticketstandseat", "wall1", "wall1corner", "wall2", "wall2corner", "wall3", "wall3corner", "wall4", "wall4corner", 
        "arch1", "arch2", "arch3", "archarch1", "archarch2", "archarch3", "artifactsign1", "Waterfall", "Time Hotel 7.07", "Saw", "Wall Shelf", "Shelves", 
        "Cardboard Boxes", "Time Hotel 5.25 Painters Tape", "Pixel Sunglasses", "Sunglasses", "Glove", "Broom", "Sponge", "Bag", "Fire Extinguisher", "rocks", 
        "Ocean", "Coin", "Sandal", "Greek Temple", "Chalice", "Coin Pouch", "Pyramid", "Anubis Statue", "mjolner", "Rock", "Tall Vase", "Generic"
    };
    for (const char* mn : modelNames) {
        models[mn] = LoadModel(TextFormat("resources/%s.glb", mn));
    }
    
    // --- LOAD PLAYER MODEL, TEXTURE, & ANIMATIONS ---
    models["Player"] = LoadModel("resources/worker.glb"); 

    int animCount = 0;
    // Load the full animation array from the single worker.glb file
    ModelAnimation* anims = LoadModelAnimations("resources/worker.glb", &animCount);

    float animTimer = 0.0f;
    int currentAnimState = 0; // 0: Idle, 1: Walk, 2: Run, 3: Roll, 4: Interact
        
    RenderTexture2D renderTarget = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_BILINEAR);

    InitLevel(entities);
    if (!LoadGame(currentNight, entities)) currentNight = 1; 
    camera.zoom = 1.0f;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (currentState == STATE_MENU) {
            BeginDrawing(); ClearBackground(DARKGRAY);
            DrawText("MUSEUM TECH SUPPORT", GetScreenWidth()/2 - 250, GetScreenHeight()/2 - 100, 40, WHITE);
            DrawText(TextFormat("CURRENT SHIFT: NIGHT %d", currentNight), GetScreenWidth()/2 - 120, GetScreenHeight()/2, 20, LIGHTGRAY);
            DrawText("PRESS ENTER TO START SHIFT", GetScreenWidth()/2 - 150, GetScreenHeight()/2 + 80, 20, GREEN);
            if (IsKeyPressed(KEY_ENTER)) { ResetNight(); currentState = STATE_PLAYING; }
            EndDrawing(); continue;
        }

        if (currentState == STATE_REVIEW) {
            BeginDrawing(); ClearBackground(RAYWHITE);
            DrawText("SHIFT COMPLETE - 8:00 AM", GetScreenWidth()/2 - 200, 100, 30, BLACK);
            DrawText(lastReport.finalVerdict.c_str(), GetScreenWidth()/2 - MeasureText(lastReport.finalVerdict.c_str(), 20)/2, 160, 20, DARKBLUE);
            int yOffset = 250;
            for (const auto& rev : lastReport.reviews) {
                DrawText(TextFormat("- %s: %s", rev.artifactName.c_str(), rev.reviewText.c_str()), 100, yOffset, 20, DARKGRAY); yOffset += 40;
            }
            DrawText("PRESS ENTER TO SAVE & CONTINUE TO NEXT NIGHT", GetScreenWidth()/2 - 250, GetScreenHeight() - 100, 20, GREEN);
            if (IsKeyPressed(KEY_ENTER)) { SaveGame(currentNight, entities); currentState = STATE_MENU; }
            EndDrawing(); continue;
        }

        if (currentState == STATE_GAMEOVER_FIRED) {
            BeginDrawing(); ClearBackground(MAROON);
            DrawText("YOU'RE FIRED.", GetScreenWidth()/2 - 150, 200, 50, WHITE);
            DrawText(lastReport.finalVerdict.c_str(), GetScreenWidth()/2 - MeasureText(lastReport.finalVerdict.c_str(), 20)/2, 300, 20, LIGHTGRAY);
            DrawText("PRESS ENTER TO RETRY THE NIGHT", GetScreenWidth()/2 - 200, GetScreenHeight() - 100, 20, YELLOW);
            if (IsKeyPressed(KEY_ENTER)) { ResetNight(); currentState = STATE_PLAYING; }
            EndDrawing(); continue;
        }

        if (currentState == STATE_GAMEOVER_DEATH) {
            BeginDrawing(); ClearBackground(RED);
            DrawText("YOU DIED.", GetScreenWidth()/2 - 120, 200, 50, BLACK);
            DrawText(lastReport.finalVerdict.c_str(), GetScreenWidth()/2 - MeasureText(lastReport.finalVerdict.c_str(), 20)/2, 300, 20, LIGHTGRAY);
            DrawText("PRESS ENTER TO RETRY THE NIGHT", GetScreenWidth()/2 - 200, GetScreenHeight() - 100, 20, YELLOW);
            if (IsKeyPressed(KEY_ENTER)) { ResetNight(); currentState = STATE_PLAYING; }
            EndDrawing(); continue;
        }

        if (currentState == STATE_PLAYING) {
            Entity& player = entities[0];
            
            shiftTimer += dt;
            if (shiftTimer >= 8 * SECONDS_PER_HOUR) {
                lastReport = EvaluateMuseum(entities);
                if (lastReport.isFired) currentState = STATE_GAMEOVER_FIRED;
                else { currentNight++; currentState = STATE_REVIEW; }
            }
            bool isSprinting = IsKeyDown(KEY_LEFT_SHIFT);
            bool executeAction = false;
            int actionTargetIdx = -1;

            if (IsKeyPressed(KEY_I)) { for (auto& e : entities) if (e.HasTag(TAG_MEDUSA)) e.isGlitching = !e.isGlitching; }
            if (IsKeyPressed(KEY_O)) { for (auto& e : entities) if (e.HasTag(TAG_WATER_SOURCE)) e.isGlitching = !e.isGlitching; }
            if (IsKeyPressed(KEY_P)) { for (auto& e : entities) if (e.HasTag(TAG_SANDALS)) e.isGlitching = true; }
            if (IsKeyPressed(KEY_K)) { for (auto& e : entities) if (e.HasTag(TAG_WIND_BAG)) e.isGlitching = !e.isGlitching; }
            if (IsKeyPressed(KEY_L)) { for (auto& e : entities) if (e.HasTag(TAG_BOULDER)) e.isGlitching = !e.isGlitching; }
            if (IsKeyPressed(KEY_SIX)) { shiftTimer = 8 * SECONDS_PER_HOUR; }

            // TO BE JUST THIS (So your debug keys work!):
            if (IsKeyPressed(KEY_ONE)) doorsOpen[0] = !doorsOpen[0]; 
            if (IsKeyPressed(KEY_TWO)) doorsOpen[1] = !doorsOpen[1];
            if (IsKeyPressed(KEY_THREE)) doorsOpen[2] = !doorsOpen[2]; 
            if (IsKeyPressed(KEY_FOUR)) doorsOpen[3] = !doorsOpen[3];
            if (IsKeyPressed(KEY_FIVE)) doorsOpen[4] = !doorsOpen[4];

            // 4. Update the actual Entity properties so the physics engine respects the doors
            for(auto& e : entities) {
                if (e.HasTag(TAG_DOOR_1)) { e.isSolid = !doorsOpen[0]; e.color.a = doorsOpen[0] ? 30 : 255; }
                if (e.HasTag(TAG_DOOR_2)) { e.isSolid = !doorsOpen[1]; e.color.a = doorsOpen[1] ? 30 : 255; }
                if (e.HasTag(TAG_DOOR_3)) { e.isSolid = !doorsOpen[2]; e.color.a = doorsOpen[2] ? 30 : 255; }
                if (e.HasTag(TAG_DOOR_5)) { e.isSolid = !doorsOpen[3]; e.color.a = doorsOpen[3] ? 30 : 255; }
                if (e.HasTag(TAG_DOOR_4)) { e.isSolid = !doorsOpen[4]; e.color.a = doorsOpen[4] ? 30 : 255; }
            }

            if (showInteractMenu) {
                if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) interactSelectedIndex = (interactSelectedIndex - 1 + interactTargets.size()) % interactTargets.size();
                if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) interactSelectedIndex = (interactSelectedIndex + 1) % interactTargets.size();

                bool clicked = false;
                Vector2 mousePos = GetMousePosition();
                float menuW = 400; float itemH = 50; float menuH = 60 + interactTargets.size() * itemH;
                float menuX = GetScreenWidth() / 2.0f - menuW / 2.0f; float menuY = GetScreenHeight() / 2.0f - menuH / 2.0f;

                for (size_t i = 0; i < interactTargets.size(); ++i) {
                    if (CheckCollisionPointRec(mousePos, { menuX + 20, menuY + 60 + i * itemH, menuW - 40, itemH - 10 })) {
                        if (Vector2Length(GetMouseDelta()) > 0.1f) interactSelectedIndex = i; 
                        if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) clicked = true;
                    }
                }

                if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || clicked) {
                    executeAction = true; actionTargetIdx = interactTargets[interactSelectedIndex]; showInteractMenu = false;
                }
                if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ESCAPE)) showInteractMenu = false;

            } else if (!player.isStone && !player.isDead) {
                Vector3 input = { 0.0f, 0.0f, 0.0f };

                float baseAccel = 6000.0f; 
                // Sprinting increases max speed from 1200 to 1800
                float maxSpeed = isSprinting ? 1200.0f : 300.0f; 
                float currentAccel = isSprinting ? baseAccel * 1.5f : baseAccel;

                if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_HEAVY)) {
                    currentAccel = 3000.0f; 
                    maxSpeed = 200.0f;
                }

                if (IsKeyDown(KEY_W)) input.z -= 1.0f; if (IsKeyDown(KEY_S)) input.z += 1.0f;
                if (IsKeyDown(KEY_A)) input.x -= 1.0f; if (IsKeyDown(KEY_D)) input.x += 1.0f;
                
                if (Vector3Length(input) > 0.0f) {
                    input = Vector3Normalize(input); player.facingDir = input;
                    player.velocity.x += input.x * currentAccel * dt; 
                    player.velocity.z += input.z * currentAccel * dt;
                    
                    // Update visual rotation of the character model
                    player.stateValue = atan2(player.facingDir.x, player.facingDir.z) * RAD2DEG;
                }

                Vector2 horizVel = { player.velocity.x, player.velocity.z };
                if (Vector2Length(horizVel) > maxSpeed) {
                    horizVel = Vector2Scale(Vector2Normalize(horizVel), maxSpeed);
                    player.velocity.x = horizVel.x;
                    player.velocity.z = horizVel.y;
                }

                if (IsKeyPressed(KEY_SPACE) && player.velocity.y == 0.0f && grabbedEntityIndex == -1) {
                    player.velocity.y = 800.0f; 
                }

                if (grabbedEntityIndex != -1) entities[grabbedEntityIndex].isUsing = IsKeyDown(KEY_F);

                // ---------------------------------------------------------
                // 1. USE LOGIC (KEY_F)
                // ---------------------------------------------------------
                if (IsKeyPressed(KEY_F)) {
                    bool environmentF = false;
                    
                    std::vector<BoundingBox> pIntBoxList = player.GetWorldInteractBounds(); 
                    for (auto& b : pIntBoxList) {
                        // Push the box forward in the direction we are facing
                        float reach = 60.0f;
                        b.min.x += player.facingDir.x * reach; b.max.x += player.facingDir.x * reach;
                        b.min.z += player.facingDir.z * reach; b.max.z += player.facingDir.z * reach;
                        
                        // Expand into a 3D box (Crucial: Expanded Y-axis to reach pedestals/switches)
                        b.min.x -= 50; b.max.x += 50; 
                        b.min.z -= 50; b.max.z += 50; 
                        b.min.y -= 20; b.max.y += 120; 
                    }
                    
                    for (auto& e : entities) {
                        if ((e.HasTag(TAG_FUSEBOX) || e.HasTag(TAG_LIGHTSWITCH)) && CheckCollisionLists(pIntBoxList, e.GetWorldInteractBounds())) {
                            e.stateValue = (e.stateValue > 0.5f) ? 0.0f : 1.0f; environmentF = true; break;
                        }
                    }
                    if (!environmentF && grabbedEntityIndex != -1) {
                        for (size_t i = 1; i < entities.size(); ++i) {
                            if (i != grabbedEntityIndex && CheckCollisionLists(pIntBoxList, entities[i].GetWorldInteractBounds())) {
                                ChemResult res = ProcessChemistry(grabbedEntityIndex, i, entities);
                                if (res != CHEM_NONE) {
                                    environmentF = true;
                                    if (res == CHEM_ATTACHED) { entities[grabbedEntityIndex].isGrabbed = false; grabbedEntityIndex = -1; }
                                    break;
                                }
                            }
                        }
                    }
                    if (!environmentF) {
                        if (grabbedEntityIndex != -1) {
                            if (entities[grabbedEntityIndex].HasTag(TAG_EYEWEAR) && !entities[grabbedEntityIndex].HasTag(TAG_BROKEN)) { equippedEyewear = grabbedEntityIndex; entities[grabbedEntityIndex].isGrabbed = false; grabbedEntityIndex = -1; }
                            else if (entities[grabbedEntityIndex].HasTag(TAG_GLOVES)) { equippedGloves = grabbedEntityIndex; entities[grabbedEntityIndex].isGrabbed = false; grabbedEntityIndex = -1; }
                        } else {
                            if (equippedGloves != -1) { grabbedEntityIndex = equippedGloves; entities[equippedGloves].isGrabbed = true; equippedGloves = -1; }
                            else if (equippedEyewear != -1) { grabbedEntityIndex = equippedEyewear; entities[equippedEyewear].isGrabbed = true; equippedEyewear = -1; }
                        }
                    }
                }
                
                if (IsKeyPressed(KEY_F) && grabbedEntityIndex != -1) {
                    Entity& held = entities[grabbedEntityIndex];
                    if (held.HasTag(TAG_HOLE_SAW)) {
                        BoundingBox holeBox = {{-30, 0, -30}, {30, 5, 30}};
                        Entity hole = MakeProp("Floor Hole", {player.position.x + player.facingDir.x * 70.0f, 0.0f, player.position.z + player.facingDir.z * 70.0f}, holeBox, BLACK, {TAG_HOLE});
                        hole.isSolid = false; hole.canGrab = false; entities.push_back(hole);
                    }
                    if (held.HasTag(TAG_BUBBLE_WRAP) && held.stateValue > 0) {
                        held.stateValue -= 1.0f;
                        BoundingBox matBox = {{-50, 0, -50}, {50, 5, 50}};
                        Entity mat = MakeProp("Bubble Mat", player.position, matBox, SKYBLUE, {TAG_MAT});
                        mat.isSolid = false; mat.canGrab = false; entities.push_back(mat);
                    }
                }
                
                // ---------------------------------------------------------
                // 2. GRAB/DROP LOGIC (KEY_E)
                // ---------------------------------------------------------
                // ---------------------------------------------------------
                // 2. GRAB/DROP LOGIC (KEY_E)
                // ---------------------------------------------------------
                if (IsKeyPressed(KEY_E)) {
                    interactTargets.clear();
                    if (grabbedEntityIndex != -1) {
                        Entity& item = entities[grabbedEntityIndex];
                        // Drop slightly forward and HIGHER UP so it can land on pedestals
                        Vector3 dropPos = { player.position.x + player.facingDir.x * 70.0f, player.position.y + 100.0f, player.position.z + player.facingDir.z * 70.0f };
                        
                        std::vector<BoundingBox> dropBoxList = item.boundsList; 
                        for (auto& b : dropBoxList) {
                            b.min.x += dropPos.x; b.max.x += dropPos.x; 
                            b.min.y += dropPos.y; b.max.y += dropPos.y; 
                            b.min.z += dropPos.z; b.max.z += dropPos.z;
                        }

                        // ... (Keep your drop interaction checks here, they are fine) ...
                        
                        for(size_t i = 0; i < entities.size(); ++i) {
                            if(i != grabbedEntityIndex && CheckCollisionLists(dropBoxList, entities[i].GetWorldInteractBounds())) {
                                if (entities[i].name == "stand2" || entities[i].HasTag(TAG_ZEUS) || (entities[i].HasTag(TAG_FUSEBOX) && entities[i].stateValue > 0.5f && item.HasTag(TAG_ELECTRIC))) {
                                    interactTargets.push_back(i);
                                } else if (CanProcessChemistry(grabbedEntityIndex, i, entities) != CHEM_NONE) {
                                    interactTargets.push_back(i);
                                }
                            }
                        }

                        if (interactTargets.empty()) {
                            float targetY = 0.0f;
                            for(size_t i = 0; i < entities.size(); ++i) {
                                // Only collide with solids for drop height
                                if(i != grabbedEntityIndex && entities[i].isSolid && CheckCollisionLists(dropBoxList, entities[i].GetWorldBounds())) {
                                    for (const auto& b : entities[i].GetWorldBounds()) {
                                        if (b.max.y > targetY) targetY = b.max.y;
                                    }
                                }
                            }
                            item.position = dropPos; item.position.y = targetY; 
                            if (item.HasTag(TAG_SANDALS)) item.stateTimer = 4.0f;
                            item.isGrabbed = false; item.isUsing = false; item.velocity = {0,0,0}; grabbedEntityIndex = -1;
                        } else if (interactTargets.size() == 1) {
                            executeAction = true; actionTargetIdx = interactTargets[0]; isDropMenu = true;
                        } else {
                            showInteractMenu = true; isDropMenu = true; interactSelectedIndex = 0;
                        }

                    } else {
                        // WE ARE EMPTY HANDED - TRY TO GRAB SOMETHING
                        std::vector<BoundingBox> pIntBoxList = player.GetWorldInteractBounds(); 
                        for (auto& b : pIntBoxList) {
                            // Push 3D hit volume forward
                            float reach = 60.0f;
                            b.min.x += player.facingDir.x * reach; b.max.x += player.facingDir.x * reach;
                            b.min.z += player.facingDir.z * reach; b.max.z += player.facingDir.z * reach;
                            
                            // Expand to reach floor and pedestals!
                            b.min.x -= 50; b.max.x += 50; 
                            b.min.z -= 50; b.max.z += 50; 
                            b.min.y -= 20; b.max.y += 120; // FIX: Y-AXIS EXPANSION
                        }

                        for (size_t i = 1; i < entities.size(); ++i) {
                            if (i == equippedEyewear || i == equippedGloves) continue; 
                            if (!entities[i].isGrabbed && entities[i].canGrab && CheckCollisionLists(pIntBoxList, entities[i].GetWorldInteractBounds())) {
                                interactTargets.push_back(i);
                            }
                        }
                        
                        // Sort by proximity so we grab what is closest to the player's center
                        std::sort(interactTargets.begin(), interactTargets.end(), [&](int a, int b) { return Vector3Distance(player.position, entities[a].position) < Vector3Distance(player.position, entities[b].position); });

                        if (interactTargets.size() == 1) { executeAction = true; actionTargetIdx = interactTargets[0]; isDropMenu = false; } 
                        else if (interactTargets.size() > 1) { showInteractMenu = true; isDropMenu = false; interactSelectedIndex = 0; }
                    }
                }

                if (IsKeyPressed(KEY_SPACE) && grabbedEntityIndex != -1 && entities[grabbedEntityIndex].canThrow) {
                    Entity& held = entities[grabbedEntityIndex];
                    held.isGrabbed = false; 
                    held.isUsing = false; 
                    
                    // BUFFED THROW: Increased horizontal speed and vertical arc
                    float throwStrength = 2200.0f; 
                    float upwardArc = 800.0f;     
                    
                    held.velocity = Vector3Scale(player.facingDir, throwStrength); 
                    held.velocity.y = upwardArc; 
                    
                    if (held.HasTag(TAG_SANDALS)) held.isGlitching = true;
                    grabbedEntityIndex = -1;
                }
            }

            if (executeAction) {
                if (!isDropMenu) {
                    grabbedEntityIndex = actionTargetIdx; entities[grabbedEntityIndex].isGrabbed = true; entities[grabbedEntityIndex].attachedTo = -1;
                } else {
                    Entity& item = entities[grabbedEntityIndex];
                    ChemResult res = ProcessChemistry(grabbedEntityIndex, actionTargetIdx, entities);
                    if (res != CHEM_NONE) {
                        if (res == CHEM_ATTACHED) { item.attachedTo = actionTargetIdx; item.isGrabbed = false; item.isUsing = false; item.velocity = {0,0,0}; grabbedEntityIndex = -1; }
                    } else {
                        item.attachedTo = actionTargetIdx; item.isGrabbed = false; item.isUsing = false; item.velocity = {0,0,0}; grabbedEntityIndex = -1;
                    }
                }
            }

            // --- UPDATED ANIMATION STATE MACHINE ---
            int targetAnimState = 0; // Default to Idle
            float speed = Vector2Length({player.velocity.x, player.velocity.z});

            if (abs(player.velocity.y) > 5.0f) {
                targetAnimState = 3; // Rolling (Jump)
            } else if (speed > 20.0f) {
                targetAnimState = isSprinting ? 2 : 1; // Run or Walk
            } else if (IsKeyDown(KEY_F) || IsKeyDown(KEY_E)) {
                targetAnimState = 4; // Interact
            }

            // If we changed states, reset the timer so the animation starts at frame 0
            if (currentAnimState != targetAnimState) {
                currentAnimState = targetAnimState;
                animTimer = 0.0f;
            }

            // Map our internal state to the index numbers you found
            int activeIdx = idxIdle;
            if (currentAnimState == 1) activeIdx = idxWalk;
            else if (currentAnimState == 2) activeIdx = idxRun;
            else if (currentAnimState == 3) activeIdx = idxRoll;
            else if (currentAnimState == 4) activeIdx = idxInteract;

            // --- Inside the Animation Update block ---
            if (anims != nullptr && activeIdx < animCount) {
                
                // 1. Define a playback speed. 
                // 30.0f is "real time". Try 45.0f or 60.0f for more energy.
                float playbackSpeed = 45.0f; 

                // Optional: Make the animation speed match the movement speed!
                if (currentAnimState == 2) playbackSpeed = 60.0f; // Fast run
                if (currentAnimState == 1) playbackSpeed = 50.0f; // Normal walk
                if (currentAnimState == 3) playbackSpeed = 90.0f; // Fast roll

                animTimer += dt * playbackSpeed; 
                
                float maxFrames = (float)anims[activeIdx].frameCount;
                animTimer = fmod(animTimer, maxFrames); 
                
                UpdateModelAnimation(models["Player"], anims[activeIdx], (int)animTimer);
            }

            HazardVisuals hazVis = UpdatePhysicsAndHazards(entities, dt, grabbedEntityIndex, equippedEyewear, equippedGloves);
            
            if (player.isStone || player.isDead) {
                lastReport = EvaluateMuseum(entities); 
                currentState = STATE_GAMEOVER_DEATH;
            }

            RenderWorld(renderTarget, camera, dt, entities, player, grabbedEntityIndex, hazVis, models);
            RenderHUD(renderTarget, shiftTimer, SECONDS_PER_HOUR, currentNight, player, showInteractMenu, isDropMenu, interactTargets, interactSelectedIndex, entities);
        }
    }

    // --- CLEANUP ---
    if (anims) UnloadModelAnimations(anims, animCount);

    for (auto& pair : models) {
        UnloadModel(pair.second);
    }

    UnloadRenderTexture(renderTarget); 
    CloseWindow(); 
    return 0;
}