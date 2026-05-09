#include "PlayerController.h"
#include "GameState.h"
#include "raymath.h"
#include "Interactions.h" // For ProcessChemistry
#include <algorithm>

void UpdateEnvironmentTriggers() {
    // Debug & Anomalies
    if (IsKeyPressed(KEY_I)) { for (auto& e : entities) if (e.HasTag(TAG_MEDUSA)) e.isGlitching = !e.isGlitching; }
    if (IsKeyPressed(KEY_O)) { for (auto& e : entities) if (e.HasTag(TAG_WATER_SOURCE)) e.isGlitching = !e.isGlitching; }
    if (IsKeyPressed(KEY_P)) { for (auto& e : entities) if (e.HasTag(TAG_SANDALS)) e.isGlitching = true; }
    if (IsKeyPressed(KEY_K)) { for (auto& e : entities) if (e.HasTag(TAG_WIND_BAG)) e.isGlitching = !e.isGlitching; }
    if (IsKeyPressed(KEY_L)) { for (auto& e : entities) if (e.HasTag(TAG_BOULDER)) e.isGlitching = !e.isGlitching; }
    if (IsKeyPressed(KEY_SIX)) { shiftTimer = 8 * SECONDS_PER_HOUR; } 

    // Doors
    if (IsKeyPressed(KEY_ONE)) doorsOpen[0] = !doorsOpen[0]; 
    if (IsKeyPressed(KEY_TWO)) doorsOpen[1] = !doorsOpen[1];
    if (IsKeyPressed(KEY_THREE)) doorsOpen[2] = !doorsOpen[2]; 
    if (IsKeyPressed(KEY_FOUR)) doorsOpen[3] = !doorsOpen[3];
    if (IsKeyPressed(KEY_FIVE)) doorsOpen[4] = !doorsOpen[4];

    if (currentNight >= 1) doorsOpen[0] = doorsOpen[1] = true; 
    if (currentNight >= 2) doorsOpen[3] = true;                
    if (currentNight >= 3) doorsOpen[4] = true;                
    
    for(auto& e : entities) {
        if (e.HasTag(TAG_DOOR_1)) { e.isSolid = !doorsOpen[0]; e.color.a = doorsOpen[0] ? 30 : 255; }
        if (e.HasTag(TAG_DOOR_2)) { e.isSolid = !doorsOpen[1]; e.color.a = doorsOpen[1] ? 30 : 255; }
        if (e.HasTag(TAG_DOOR_3)) { e.isSolid = !doorsOpen[2]; e.color.a = doorsOpen[2] ? 30 : 255; }
        if (e.HasTag(TAG_DOOR_4)) { e.isSolid = !doorsOpen[3]; e.color.a = doorsOpen[3] ? 30 : 255; }
        if (e.HasTag(TAG_DOOR_5)) { e.isSolid = !doorsOpen[4]; e.color.a = doorsOpen[4] ? 30 : 255; }
    }
}

void UpdatePlayerInput(float dt) {
    if (entities.empty()) return;
    Entity& player = entities[0];

    // If player is dead or turned to stone, they can't move!
    if (player.isStone || player.isDead) return;

    // --- MOVEMENT ---
    Vector2 input = { 0.0f, 0.0f };
    float currentAccel = 3500.0f; 
    float currentMaxSpeed = 700.0f;
    
    // Slow down if carrying something heavy
    if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_HEAVY)) { 
        currentAccel = 1500.0f; 
        currentMaxSpeed = 250.0f; 
    }

    if (IsKeyDown(KEY_W)) input.y -= 1.0f; 
    if (IsKeyDown(KEY_S)) input.y += 1.0f;
    if (IsKeyDown(KEY_A)) input.x -= 1.0f; 
    if (IsKeyDown(KEY_D)) input.x += 1.0f;
    
    if (Vector2Length(input) > 0.0f) {
        input = Vector2Normalize(input); 
        player.facingDir = input;
        player.velocity.x += input.x * currentAccel * dt; 
        player.velocity.y += input.y * currentAccel * dt;
    }

    // --- GRABBING / USING (KEY_F) ---
    if (grabbedEntityIndex != -1) {
        entities[grabbedEntityIndex].isUsing = IsKeyDown(KEY_F);
    }

    if (IsKeyPressed(KEY_F)) {
        bool environmentF = false;
        Rectangle pIntBox = player.GetWorldInteractionBox(); 
        pIntBox.width += 80; pIntBox.height += 80; 
        
        for (auto& e : entities) {
            if ((e.HasTag(TAG_FUSEBOX) || e.HasTag(TAG_LIGHTSWITCH)) && CheckCollisionRecs(pIntBox, e.GetWorldInteractionBox())) {
                e.stateValue = (e.stateValue > 0.5f) ? 0.0f : 1.0f; 
                environmentF = true; 
                break;
            }
        }
        
        if (!environmentF && grabbedEntityIndex != -1) {
            for (size_t i = 1; i < entities.size(); ++i) {
                if (i != grabbedEntityIndex && CheckCollisionRecs(pIntBox, entities[i].GetWorldInteractionBox())) {
                    ChemResult res = ProcessChemistry(grabbedEntityIndex, i, entities);
                    if (res != CHEM_NONE) {
                        environmentF = true;
                        if (res == CHEM_ATTACHED) { 
                            entities[grabbedEntityIndex].isGrabbed = false; 
                            grabbedEntityIndex = -1; 
                        }
                        break;
                    }
                }
            }
        }
        
        if (!environmentF) {
            if (grabbedEntityIndex != -1) {
                if (entities[grabbedEntityIndex].HasTag(TAG_EYEWEAR) && !entities[grabbedEntityIndex].HasTag(TAG_BROKEN)) { 
                    equippedEyewear = grabbedEntityIndex; 
                    entities[grabbedEntityIndex].isGrabbed = false; 
                    grabbedEntityIndex = -1; 
                }
                else if (entities[grabbedEntityIndex].HasTag(TAG_GLOVES)) { 
                    equippedGloves = grabbedEntityIndex; 
                    entities[grabbedEntityIndex].isGrabbed = false; 
                    grabbedEntityIndex = -1; 
                }
            } else {
                if (equippedGloves != -1) { 
                    grabbedEntityIndex = equippedGloves; 
                    entities[equippedGloves].isGrabbed = true; 
                    equippedGloves = -1; 
                }
                else if (equippedEyewear != -1) { 
                    grabbedEntityIndex = equippedEyewear; 
                    entities[equippedEyewear].isGrabbed = true; 
                    equippedEyewear = -1; 
                }
            }
        }
    }

    // --- DROPPING / MENU (KEY_E) ---
    if (IsKeyPressed(KEY_E)) {
        interactTargets.clear();
        if (grabbedEntityIndex != -1) {
            Entity& item = entities[grabbedEntityIndex];
            Vector2 dropPos = { player.position.x + player.facingDir.x * 60.0f, player.position.y + player.facingDir.y * 60.0f };
            Rectangle dropBox = item.movementBox; 
            dropBox.x += dropPos.x; dropBox.y += dropPos.y;

            for(size_t i = 0; i < entities.size(); ++i) {
                if(i != grabbedEntityIndex && CheckCollisionRecs(dropBox, entities[i].GetWorldInteractionBox())) {
                    if (entities[i].name == "Pedestal" || entities[i].HasTag(TAG_ZEUS) || (entities[i].HasTag(TAG_FUSEBOX) && entities[i].stateValue > 0.5f && item.HasTag(TAG_ELECTRIC))) {
                        interactTargets.push_back(i);
                    } else if (CanProcessChemistry(grabbedEntityIndex, i, entities) != CHEM_NONE) {
                        interactTargets.push_back(i);
                    }
                }
            }

            if (interactTargets.empty()) {
                float targetZ = 0.0f;
                for(size_t i = 0; i < entities.size(); ++i) {
                    if(i != grabbedEntityIndex && entities[i].isSolid && CheckCollisionRecs(dropBox, entities[i].GetWorldMovementBox()) && entities[i].zHeight > targetZ) {
                        targetZ = entities[i].zHeight; 
                    }
                }
                item.position = dropPos; item.z = targetZ; 
                if (item.HasTag(TAG_SANDALS)) item.stateTimer = 4.0f;
                item.isGrabbed = false; item.isUsing = false; item.velocity = {0,0}; item.zVelocity = 0; grabbedEntityIndex = -1;
            } else if (interactTargets.size() == 1) {
                // Notice how we don't 'executeAction' here directly anymore. 
                // We let the main loop handle the menu logic for now.
                isDropMenu = true;
            } else {
                showInteractMenu = true; isDropMenu = true; interactSelectedIndex = 0;
            }

        } else {
            Rectangle pIntBox = player.GetWorldInteractionBox(); 
            pIntBox.x -= 40; pIntBox.y -= 100; pIntBox.width += 80; pIntBox.height += 150; 

            for (size_t i = 1; i < entities.size(); ++i) {
                if (i == equippedEyewear || i == equippedGloves) continue; 
                if (!entities[i].isGrabbed && entities[i].canGrab && CheckCollisionRecs(pIntBox, entities[i].GetWorldInteractionBox())) {
                    interactTargets.push_back(i);
                }
            }
            std::sort(interactTargets.begin(), interactTargets.end(), [&](int a, int b) { 
                return Vector2Distance(player.position, entities[a].position) < Vector2Distance(player.position, entities[b].position); 
            });

            if (interactTargets.size() == 1) { isDropMenu = false; } 
            else if (interactTargets.size() > 1) { showInteractMenu = true; isDropMenu = false; interactSelectedIndex = 0; }
        }
    }

    // --- THROWING (KEY_SPACE) ---
    if (IsKeyPressed(KEY_SPACE) && grabbedEntityIndex != -1 && entities[grabbedEntityIndex].canThrow) {
        entities[grabbedEntityIndex].isGrabbed = false; 
        entities[grabbedEntityIndex].isUsing = false; 
        entities[grabbedEntityIndex].velocity = Vector2Scale(player.facingDir, 1200.0f); 
        entities[grabbedEntityIndex].zVelocity = 450.0f; 
        if (entities[grabbedEntityIndex].HasTag(TAG_SANDALS)) entities[grabbedEntityIndex].isGlitching = true;
        grabbedEntityIndex = -1;
    }
}