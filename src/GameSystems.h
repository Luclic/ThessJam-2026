#pragma once
#include "Entities.h"
#include "Interactions.h"
#include "raymath.h"
#include <vector>

struct HazardVisuals {
    bool drawingBeam = false;
    Vector2 beamP1, beamP2, beamP3;
    bool drawingExtinguisher = false;
    Vector2 extP1, extP2, extP3;
};

inline HazardVisuals UpdatePhysicsAndHazards(std::vector<Entity>& entities, float dt, int grabbedEntityIndex, int equippedEyewear, int equippedGloves) {
    Entity& player = entities[0];
    HazardVisuals visuals;

    // --- 1. PHYSICS & KINEMATICS ---
    for (size_t i = 0; i < entities.size(); ++i) {
        Entity& e = entities[i];
        
        if ((int)i == grabbedEntityIndex) {
            e.position = player.position; e.z = 100.0f; e.velocity = {0,0}; e.zVelocity = 0; continue;
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

        // --- Ice Rink Physics ---
        float currentFriction = 6.0f;
        for (auto& w : entities) {
            if (w.HasTag(TAG_WATER_SOURCE) && w.isStone && Vector2Distance(e.position, w.position) < w.stateValue) {
                currentFriction = 0.5f; // SLIPPERY ICE!
            }
        }
        e.velocity.x -= e.velocity.x * currentFriction * dt; 
        e.velocity.y -= e.velocity.y * currentFriction * dt;

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
                if (prevZVel < -300.0f) {
                    // BUBBLE MAT CHECK!
                    bool safe = false;
                    for (const auto& mat : entities) if (mat.HasTag(TAG_MAT) && CheckCollisionRecs(e.GetWorldMovementBox(), mat.GetWorldMovementBox())) safe = true;
                    if (!safe) ProcessImpact(e); // Only break if no mat!
                }
                if (abs(e.zVelocity) < 30.0f) e.zVelocity = 0.0f; e.velocity = Vector2Scale(e.velocity, 0.4f);
            }
        }

        // --- X Axis Collision ---
        if (abs(e.velocity.x) > 0.1f && !e.isGrabbed) {
            e.position.x += e.velocity.x * dt;
            Rectangle nextX = e.GetWorldMovementBox();
            for (auto& other : entities) {
                // EXEMPTIONS: Let the Boulder crush Pedestals, and let manual pushing handle the Player
                if (e.HasTag(TAG_BOULDER) && other.name == "Pedestal") continue; 
                if (e.HasTag(TAG_BOULDER) && other.name == "Player") continue; 
                if (e.name == "Player" && other.HasTag(TAG_BOULDER)) continue; 

                if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1 && CheckCollisionRecs(nextX, other.GetWorldMovementBox()) && e.z < other.zHeight) {
                    e.position.x -= e.velocity.x * dt; e.velocity.x *= -0.5f; break;
                }
            }
        }
        
        // --- Y Axis Collision ---
        if (abs(e.velocity.y) > 0.1f && !e.isGrabbed) {
            e.position.y += e.velocity.y * dt;
            Rectangle nextY = e.GetWorldMovementBox();
            for (auto& other : entities) {
                // EXEMPTIONS: Let the Boulder crush Pedestals. 
                // Note: We DO NOT exempt the Player here, so the player can't walk vertically through the boulder!
                if (e.HasTag(TAG_BOULDER) && other.name == "Pedestal") continue; 

                if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1 && CheckCollisionRecs(nextY, other.GetWorldMovementBox()) && e.z < other.zHeight) {
                    e.position.y -= e.velocity.y * dt; e.velocity.y *= -0.5f; break;
                }
            }
        }

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
        
        // --- Sisyphus Boulder Logic ---
        if (e.HasTag(TAG_BOULDER) && e.isGlitching) {
            e.velocity.x += 120.0f * dt; // Constantly accelerates right!

            // 1. Sandbag Block Check
            for (const auto& other : entities) {
                if (other.HasTag(TAG_SANDBAG) && !other.isGrabbed && other.z < 20.0f) {
                    if (CheckCollisionRecs(e.GetWorldMovementBox(), other.GetWorldMovementBox()) && other.position.x > e.position.x) {
                        e.velocity.x = 0; // The sandbag holds it back
                    }
                }
            }

            // 2. Player Push Logic (MANUAL RESOLUTION)
            Rectangle bBox = e.GetWorldMovementBox();
            Rectangle pBox = player.GetWorldMovementBox();
            if (CheckCollisionRecs(bBox, pBox)) {
                if (player.position.x > e.position.x) { 
                    // Player is on the Right side of the boulder
                    if (player.velocity.x < -10.0f) {
                        // Player is walking left, overpowering the boulder!
                        e.velocity.x = player.velocity.x;
                    } else {
                        // Player is standing still, boulder pushes them right!
                        player.velocity.x = e.velocity.x;
                        player.position.x += e.velocity.x * dt; 
                    }
                } else {
                    // Player is on the Left side, trying to push it right
                    if (player.velocity.x > 10.0f) e.velocity.x = player.velocity.x;
                }
            }

            // 3. Crush Artifacts in its path!
            for (auto& other : entities) {
                if (&e != &other && (other.HasTag(TAG_FRAGILE) || other.HasTag(TAG_MEDUSA))) {
                    if (CheckCollisionRecs(e.GetWorldInteractionBox(), other.GetWorldInteractionBox()) && !other.HasTag(TAG_BROKEN)) {
                        other.AddTag(TAG_BROKEN);
                        other.color = DARKGRAY;
                        other.zHeight = 5.0f; // Flattens it
                        other.isGlitching = false; // Stops standard glitch logic permanently
                        other.attachedTo = -1; // Knocks it off pedestal
                    }
                }
            }
        }

        // --- Egyptian Wing Logic ---
        if (e.HasTag(TAG_SUN_DISK)) {
            if (e.isGlitching) {
                e.color = ORANGE; 
                e.canGrab = false; // Too hot to touch!
                
                // Chemistry: Cool down automatically if dropped in water
                for (auto& w : entities) {
                    if (w.HasTag(TAG_WATER_SOURCE) && w.stateValue > 0.0f && Vector2Distance(e.position, w.position) < w.stateValue) {
                        e.isGlitching = false; break;
                    }
                }
            } else {
                e.color = BLACK; // Obsidian rock
                e.canGrab = true; // Safe to carry
            }
        }

        if (e.HasTag(TAG_MUMMY) && e.isGlitching && !e.isStone && !e.isGrabbed) {
            Vector2 targetPos = e.position;
            bool targetFound = false;

            int myRoomX = (int)std::floor(e.position.x / GAME_WIDTH);
            int myRoomY = (int)std::floor(e.position.y / GAME_HEIGHT);

            bool lightOn = true;
            Entity* mySwitch = nullptr;
            for (auto& other : entities) {
                if (other.HasTag(TAG_LIGHTSWITCH) && (int)std::floor(other.position.x / GAME_WIDTH) == myRoomX && (int)std::floor(other.position.y / GAME_HEIGHT) == myRoomY) {
                    mySwitch = &other; lightOn = (other.stateValue > 0.5f); break;
                }
            }

            if (lightOn && mySwitch) {
                targetPos = mySwitch->position; targetFound = true;
            } else {
                for (auto& other : entities) if (other.HasTag(TAG_SARCOPHAGUS)) { targetPos = other.position; targetFound = true; break; }
            }

            if (targetFound) {
                Vector2 dir = Vector2Normalize(Vector2Subtract(targetPos, e.position));
                e.velocity.x += dir.x * 200.0f * dt; // Slow shamble
                e.velocity.y += dir.y * 200.0f * dt;

                // Turn off switch if close!
                if (lightOn && Vector2Distance(e.position, targetPos) < 100.0f) mySwitch->stateValue = 0.0f; 
            }

            // Knock over artifacts blindly
            for (auto& other : entities) {
                if (&e != &other && (other.HasTag(TAG_FRAGILE) || other.HasTag(TAG_MEDUSA)) && !other.HasTag(TAG_BROKEN)) {
                    if (CheckCollisionRecs(e.GetWorldMovementBox(), other.GetWorldMovementBox())) {
                        other.AddTag(TAG_BROKEN); other.color = DARKGRAY; other.zHeight = 5.0f; other.attachedTo = -1;
                    }
                }
            }
        }
    }

    // --- 2. LIGHTNING DEATH CHECK ---
    bool playerInWater = false;
    for (const auto& w : entities) if (w.HasTag(TAG_WATER_SOURCE) && w.stateValue > 0.0f && Vector2Distance(player.position, w.position) < w.stateValue) playerInWater = true;
    if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_ELECTRIC) && !entities[grabbedEntityIndex].isStone) {
        if (equippedGloves == -1 || playerInWater) player.isDead = true; 
    }

    // --- 3. HAZARD LOGIC ---
    for (int i = 0; i < entities.size(); ++i) {
        Entity& e = entities[i];
        if (e.HasTag(TAG_MEDUSA) && e.isGlitching) {
            bool isBlocked = false;
            for (const auto& other : entities) if (other.attachedTo == i && other.HasTag(TAG_EYEWEAR) && !other.HasTag(TAG_BROKEN)) isBlocked = true;
            if (!isBlocked) {
                visuals.drawingBeam = true; visuals.beamP1 = { e.position.x, e.position.y - e.z }; visuals.beamP2 = { e.position.x - 300.0f, e.position.y + 600.0f }; visuals.beamP3 = { e.position.x + 300.0f, e.position.y + 600.0f };
                for (auto& target : entities) {
                    if (&target != &e && !target.isStone && (target.canGrab || target.name == "Player") && CheckCollisionPointTriangle(target.position, visuals.beamP1, visuals.beamP2, visuals.beamP3)) {
                        bool immune = (target.name == "Player" && equippedEyewear != -1 && !entities[equippedEyewear].HasTag(TAG_BROKEN));
                        if (!immune) { target.isStone = true; target.color = GRAY; target.isGlitching = false; }
                    }
                }
            }
        }
        
        if (e.isUsing && e.HasTag(TAG_EXTINGUISHER)) {
            visuals.drawingExtinguisher = true; Vector2 dir = player.facingDir; Vector2 perp = { -dir.y, dir.x }; 
            visuals.extP1 = e.position; visuals.extP2 = { e.position.x + dir.x * 400.0f + perp.x * 200.0f, e.position.y + dir.y * 400.0f + perp.y * 200.0f }; visuals.extP3 = { e.position.x + dir.x * 400.0f - perp.x * 200.0f, e.position.y + dir.y * 400.0f - perp.y * 200.0f };
            
            // Recoil/Boost Player!
            player.velocity.x -= dir.x * 1500.0f * dt;
            player.velocity.y -= dir.y * 1500.0f * dt;

            for (auto& target : entities) {
                if (&target != &player && CheckCollisionPointTriangle(target.position, visuals.extP1, visuals.extP2, visuals.extP3)) {
                    if (target.HasTag(TAG_SUN_DISK) && target.isGlitching) target.isGlitching = false; // Cools Disk
                    if (target.HasTag(TAG_WATER_SOURCE) && target.stateValue > 0) { target.isStone = true; target.isGlitching = false; } // Freezes Water
                    if (target.canGrab && !target.HasTag(TAG_HEAVY)) {
                        // Knockback light items!
                        target.velocity.x += dir.x * 2500.0f * dt;
                        target.velocity.y += dir.y * 2500.0f * dt;
                    }
                }
            }
        }

        // Hole Logic
        if (e.HasTag(TAG_HOLE)) {
            for (auto& target : entities) {
                if (&target != &e && CheckCollisionRecs(e.GetWorldMovementBox(), target.GetWorldMovementBox())) {
                    if (target.HasTag(TAG_WATER_SOURCE)) target.isGlitching = false; // Drains water
                    if (target.HasTag(TAG_BOULDER) && target.isGlitching) {
                        target.isGlitching = false; target.velocity = {0,0}; target.position = e.position; target.zHeight = 20.0f; // Swallows boulder
                    }
                    if (target.HasTag(TAG_MJOLNIR)) target.isGlitching = false; // Mjolnir is in the hole, safe!
                }
            }
        }

        // --- Nordic & Celtic Wing Logic ---
        
        // Mjolnir Bubble Wrap Dragging
        if (e.HasTag(TAG_MJOLNIR) && !e.canGrab) {
            for (const auto& w : entities) {
                if (w.HasTag(TAG_BUBBLE_WRAP) && w.isGrabbed) {
                    if (CheckCollisionRecs(e.GetWorldMovementBox(), w.GetWorldInteractionBox())) {
                        e.position = w.position; // Drags along the mat!
                    }
                }
            }
        }

        // Gleipnir Slithering
        if (e.HasTag(TAG_GLEIPNIR) && e.isGlitching && !e.isStone) {
            e.z = 5.0f; // Slithers low
            Vector2 dir = Vector2Normalize(Vector2Subtract(player.position, e.position));
            e.velocity.x += dir.x * 250.0f * dt;
            e.velocity.y += dir.y * 250.0f * dt;

            for (auto& other : entities) {
                if (&e != &other && !other.isGrabbed && CheckCollisionRecs(e.GetWorldMovementBox(), other.GetWorldMovementBox())) {
                    if (other.HasTag(TAG_SANDBAG)) {
                        e.attachedTo = &other - &entities[0]; e.isGlitching = false; break; // Distracted!
                    }
                    if (other.HasTag(TAG_MUMMY) && other.isGlitching) {
                        e.attachedTo = &other - &entities[0]; e.isGlitching = false; 
                        other.isGlitching = false; other.color = WHITE; break; // Ties up mummy!
                    }
                }
            }
        }

        // Banshee Stone Screaming
        if (e.HasTag(TAG_BANSHEE_STONE) && e.isGlitching && !e.isStone) {
            e.stateValue += 300.0f * dt; // StateValue acts as the visual pulse radius
            if (e.stateValue > 800.0f) {
                e.stateValue = 0.0f; // Reset pulse
                // 5% chance per pulse to wake up a random artifact in the museum!
                if (GetRandomValue(0, 100) < 5) {
                    int randIdx = GetRandomValue(1, entities.size()-1);
                    if (!entities[randIdx].isGrabbed && entities[randIdx].name != "Wall" && entities[randIdx].name != "Door" && entities[randIdx].name != "Pedestal") {
                        entities[randIdx].isGlitching = true;
                    }
                }
            }
        }
        
        if (e.HasTag(TAG_WIND_BAG) && e.isGlitching) {
            for (auto& target : entities) {
                if (&target != &e && target.attachedTo == -1 && !target.isStone && target.name != "Wall" && target.name != "Door" && target.name != "Pedestal") {
                    float dist = Vector2Distance(e.position, target.position);
                    if (dist < 600.0f && dist > 5.0f) {
                        Vector2 pushDir = Vector2Normalize(Vector2Subtract(target.position, e.position));
                        float force = (600.0f - dist) * 4.0f; 
                        if (target.HasTag(TAG_HEAVY)) force *= 0.1f; 
                        if (target.name == "Player") force *= 0.6f;  
                        target.velocity.x += pushDir.x * force * dt; target.velocity.y += pushDir.y * force * dt;
                    }
                }
            }
        }
    }

    return visuals;
}