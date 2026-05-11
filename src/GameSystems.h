#pragma once
#include "Entities.h"
#include "Interactions.h"
#include "raymath.h"
#include <vector>

struct HazardVisuals {
    bool drawingBeam = false;
    Vector3 beamP1, beamP2, beamP3;
    bool drawingExtinguisher = false;
    Vector3 extP1, extP2, extP3;
};

inline HazardVisuals UpdatePhysicsAndHazards(std::vector<Entity>& entities, float dt, int grabbedEntityIndex, int equippedEyewear, int equippedGloves) {
    Entity& player = entities[0];
    HazardVisuals visuals;

    for (size_t i = 0; i < entities.size(); ++i) {
        Entity& e = entities[i];
        
        // --- QUALITY OF LIFE: DISABLE COLLISION FOR HELD/WORN ITEMS ---
        if ((int)i == grabbedEntityIndex || (int)i == equippedEyewear || (int)i == equippedGloves || e.attachedTo != -1) {
            e.isSolid = false; 
        }
        
        if ((int)i == grabbedEntityIndex) {
            // Push the item 60 units forward so it sits in front of the player
            e.position.x = player.position.x + player.facingDir.x * 60.0f;
            e.position.y = player.position.y + 70.0f; // Lift it to chest height
            e.position.z = player.position.z + player.facingDir.z * 60.0f; 
            e.velocity = {0,0,0}; continue;
        }
        
        if (e.attachedTo != -1) {
            Entity& parent = entities[e.attachedTo];
            e.position = parent.position; 
            
            // --- SAFETY CHECK: Prevent segfault if parent has no hitboxes ---
            float parentHeight = parent.boundsList.empty() ? 10.0f : parent.boundsList[0].max.y;
            e.position.y = parent.position.y + parentHeight; 
            
            if (parent.HasTag(TAG_ZEUS)) { e.position.x -= 40.0f; e.position.y += 40.0f; } 
            if (parent.HasTag(TAG_FUSEBOX)) { e.position.z += 20.0f; e.position.y -= 20.0f; } 
            e.velocity = {0,0,0}; continue; 
        }
        if ((int)i == equippedEyewear) {
            // Move glasses to head height and slightly forward
            e.position.x = player.position.x + player.facingDir.x * 15.0f;
            e.position.y = player.position.y + 135.0f; // Head height
            e.position.z = player.position.z + player.facingDir.z * 15.0f;
            e.velocity = {0,0,0}; continue;
        }

        if ((int)i == equippedGloves) {
            // Move gloves to waist/hand height
            e.position.x = player.position.x + player.facingDir.x * 25.0f;
            e.position.y = player.position.y + 80.0f; 
            e.position.z = player.position.z + player.facingDir.z * 25.0f;
            e.velocity = {0,0,0}; continue;
        }

        float currentFriction = 6.0f;
        for (auto& w : entities) {
            if (w.HasTag(TAG_WATER_SOURCE) && w.isStone && Vector3Distance(e.position, w.position) < w.stateValue) {
                currentFriction = 0.5f; 
            }
        }
        e.velocity.x -= e.velocity.x * currentFriction * dt; 
        e.velocity.z -= e.velocity.z * currentFriction * dt;

        Vector3 horizVel = {e.velocity.x, 0.0f, e.velocity.z};
        if (&e == &player && Vector3Length(horizVel) > 700.0f) {
            horizVel = Vector3Scale(Vector3Normalize(horizVel), 700.0f);
            e.velocity.x = horizVel.x; e.velocity.z = horizVel.z;
        }

        if (e.HasTag(TAG_SANDALS) && e.isGlitching && !e.isStone && !e.isGrabbed) {
            e.position.y = 70.0f; 
            if (GetRandomValue(0, 100) < 10) { e.velocity.x = (GetRandomValue(-100, 100) / 100.0f) * 1000.0f; e.velocity.z = (GetRandomValue(-100, 100) / 100.0f) * 1000.0f; }
        }
        if (e.HasTag(TAG_SANDALS) && !e.isGlitching && !e.isGrabbed && !e.isStone && e.position.y < 20.0f) {
            if (e.stateTimer > 0.0f) { e.stateTimer -= dt; if (e.stateTimer <= 0.0f) e.isGlitching = true; }
        }

        if (!e.isStatic) { 
            float prevYVel = e.velocity.y;
            e.velocity.y -= 2400.0f * dt; 
            e.position.y += e.velocity.y * dt;
            
            std::vector<BoundingBox> myBoxY = e.GetWorldBounds();
            bool hitGround = false;
            
            // NEW: Are we currently inside a hole?
            bool inHole = false;
            for (const auto& hole : entities) {
                if (hole.HasTag(TAG_HOLE) && CheckCollisionLists(myBoxY, hole.GetWorldBounds())) {
                    inHole = true; break;
                }
            }
            
            // If we are at the floor (0.0f) AND not in a hole, stop falling.
            if (e.position.y <= 0.1f && !inHole) { 
                e.position.y = 0.0f;
                hitGround = true;
            } else if (!inHole) {
                // ... (Keep your existing solid-object collision loop here)
                for (const auto& other : entities) {
                    if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1) {
                        if (CheckCollisionLists(myBoxY, other.GetWorldBounds())) {
                            if (e.velocity.y <= 0.0f) { 
                                float highestY = -99999.0f;
                                for (const auto& b : other.GetWorldBounds()) {
                                    if (b.max.y > highestY) highestY = b.max.y;
                                }
                                e.position.y = highestY + 0.1f;
                                hitGround = true; 
                                break;
                            }
                        }
                    }
                }
            }
            
            // NEW: Clean up items that fall into the abyss so they don't lag the game
            if (e.position.y < -300.0f) {
                e.velocity = {0,0,0}; e.isGlitching = false;
            }

            if (hitGround) {
                e.velocity.y = 0.0f; 
                e.velocity.x *= 0.8f; 
                e.velocity.z *= 0.8f;
                
                if (prevYVel < -600.0f) { 
                    bool safe = false;
                    for (const auto& mat : entities) if (mat.HasTag(TAG_MAT) && CheckCollisionLists(e.GetWorldBounds(), mat.GetWorldBounds())) safe = true;
                    if (!safe) ProcessImpact(e); 
                }
            }
        }

        if (abs(e.velocity.x) > 0.1f && !e.isGrabbed) {
            e.position.x += e.velocity.x * dt;
            std::vector<BoundingBox> nextX = e.GetWorldBounds();
            for (auto& other : entities) {
                if (e.HasTag(TAG_BOULDER) && other.name == "stand2") continue; 
                if (e.HasTag(TAG_BOULDER) && other.name == "Player") continue; 
                if (e.name == "Player" && other.HasTag(TAG_BOULDER)) continue; 

                if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1) {
                    // SEAM FIX: Only collide horizontally if obstacle is taller than our step offset
                    bool isObstacleTallEnough = false;
                    for (const auto& ob : other.GetWorldBounds()) {
                        if (ob.max.y > e.position.y + 5.0f) isObstacleTallEnough = true;
                    }
                    if (isObstacleTallEnough && CheckCollisionLists(nextX, other.GetWorldBounds())) {
                        e.position.x -= e.velocity.x * dt; e.velocity.x *= -0.5f; break;
                    }
                }
            }
        }
        
        if (abs(e.velocity.z) > 0.1f && !e.isGrabbed) {
            e.position.z += e.velocity.z * dt;
            std::vector<BoundingBox> nextZ = e.GetWorldBounds();
            for (auto& other : entities) {
                if (e.HasTag(TAG_BOULDER) && other.name == "stand2") continue; 

                if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1) {
                    // SEAM FIX: Step Offset
                    bool isObstacleTallEnough = false;
                    for (const auto& ob : other.GetWorldBounds()) {
                        if (ob.max.y > e.position.y + 5.0f) isObstacleTallEnough = true;
                    }
                    if (isObstacleTallEnough && CheckCollisionLists(nextZ, other.GetWorldBounds())) {
                        e.position.z -= e.velocity.z * dt; e.velocity.z *= -0.5f; break;
                    }
                }
            }
        }

        if (e.HasTag(TAG_WATER_SOURCE)) {
            if (e.isGlitching) {
                float maxRadius = 9999.0f;
                for (const auto& sp : entities) if (sp.HasTag(TAG_SPONGE) && !sp.isGrabbed && sp.position.y < 20.0f) { 
                    float dist = Vector3Distance(e.position, sp.position); 
                    if (dist - 40.0f < maxRadius) maxRadius = dist - 40.0f; 
                }
                if (e.stateValue < maxRadius) { e.stateValue += 150.0f * dt; if (e.stateValue > maxRadius) e.stateValue = maxRadius; }
            }
            if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_MOP) && entities[grabbedEntityIndex].isUsing) {
                if (Vector3Distance(player.position, e.position) < e.stateValue + 100.0f) { e.stateValue -= 400.0f * dt; if (e.stateValue < 0) e.stateValue = 0; }
            }
            if (e.stateTimer > 0.0f) { e.stateTimer -= dt; if (e.stateTimer <= 0.0f) e.isGlitching = true; }
        }
        if (e.HasTag(TAG_SANDALS) && e.isGlitching) {
            for (const auto& w : entities) if (w.HasTag(TAG_WATER_SOURCE) && w.stateValue > 0.0f && Vector3Distance(e.position, w.position) < w.stateValue) {
                e.isGlitching = false; e.color = DARKBLUE; e.stateTimer = 0.0f; break; 
            }
        }
        
        if (e.HasTag(TAG_BOULDER) && e.isGlitching) {
            e.velocity.x += 120.0f * dt; 

            for (const auto& other : entities) {
                if (other.HasTag(TAG_SANDBAG) && !other.isGrabbed && other.position.y < 20.0f) {
                    if (CheckCollisionLists(e.GetWorldBounds(), other.GetWorldBounds()) && other.position.x > e.position.x) {
                        e.velocity.x = 0; 
                    }
                }
            }

            // NEW: Proper physical separation for the Boulder
            if (CheckCollisionLists(e.GetWorldBounds(), player.GetWorldBounds())) {
                if (player.position.x > e.position.x) { 
                    // Player is on the right, push them right!
                    player.position.x = e.position.x + 80.0f; 
                    if (player.velocity.x < -10.0f) e.velocity.x = -200.0f; // Player is pushing against it
                    else player.velocity.x = e.velocity.x; // Player is being steamrolled
                } else {
                    // Player is on the left, push them left!
                    player.position.x = e.position.x - 80.0f;
                    if (player.velocity.x > 10.0f) e.velocity.x = 200.0f; // Player is pushing it
                }
            }

            for (auto& other : entities) {
                if (&e != &other && (other.HasTag(TAG_FRAGILE) || other.HasTag(TAG_MEDUSA))) {
                    if (CheckCollisionLists(e.GetWorldInteractBounds(), other.GetWorldInteractBounds()) && !other.HasTag(TAG_BROKEN)) {
                        other.AddTag(TAG_BROKEN);
                        other.color = DARKGRAY;
                        if (!other.boundsList.empty()) other.boundsList[0].max.y = 5.0f; // SAFETY CHECK
                        other.isGlitching = false; 
                        other.attachedTo = -1; 
                    }
                }
            }
        }

        if (e.HasTag(TAG_SUN_DISK)) {
            if (e.isGlitching) {
                e.color = ORANGE; e.canGrab = false; 
                for (auto& w : entities) {
                    if (w.HasTag(TAG_WATER_SOURCE) && w.stateValue > 0.0f && Vector3Distance(e.position, w.position) < w.stateValue) {
                        e.isGlitching = false; break;
                    }
                }
            } else { e.color = BLACK; e.canGrab = true; }
        }

        if (e.HasTag(TAG_MUMMY) && e.isGlitching && !e.isStone && !e.isGrabbed) {
            Vector3 targetPos = e.position;
            bool targetFound = false;

            // Target the nearest Fusebox
            Entity* targetFusebox = nullptr;
            float closestDist = 9999.0f;

            for (auto& other : entities) {
                if (other.HasTag(TAG_FUSEBOX)) {
                    float d = Vector3Distance(e.position, other.position);
                    if (d < closestDist) {
                        closestDist = d;
                        targetFusebox = &other;
                        targetFound = true;
                    }
                }
            }

            if (targetFound && targetFusebox) {
                Vector3 dir = Vector3Normalize(Vector3Subtract(targetFusebox->position, e.position));
                e.velocity.x += dir.x * 250.0f * dt; // Slightly faster mummy
                e.velocity.z += dir.z * 250.0f * dt;

                // If close to fusebox, SABOTAGE IT (Turn it off)
                if (Vector3Distance(e.position, targetFusebox->position) < 80.0f) {
                    targetFusebox->stateValue = 0.0f; // OFF
                }
            }

            for (auto& other : entities) {
                if (&e != &other && (other.HasTag(TAG_FRAGILE) || other.HasTag(TAG_MEDUSA)) && !other.HasTag(TAG_BROKEN)) {
                    if (CheckCollisionLists(e.GetWorldBounds(), other.GetWorldBounds())) {
                        other.AddTag(TAG_BROKEN); other.color = DARKGRAY; 
                        if (!other.boundsList.empty()) other.boundsList[0].max.y = 5.0f; // SAFETY CHECK
                        other.attachedTo = -1;
                    }
                }
            }
        }
    }

    bool playerInWater = false;
    for (const auto& w : entities) if (w.HasTag(TAG_WATER_SOURCE) && w.stateValue > 0.0f && Vector3Distance(player.position, w.position) < w.stateValue) playerInWater = true;
    if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_ELECTRIC) && !entities[grabbedEntityIndex].isStone) {
        if (equippedGloves == -1 || playerInWater) player.isDead = true; 
    }

    for (int i = 0; i < entities.size(); ++i) {
        Entity& e = entities[i];
        if (e.HasTag(TAG_MEDUSA) && e.isGlitching) {
            bool isBlocked = false;
            for (const auto& other : entities) if (other.attachedTo == i && other.HasTag(TAG_EYEWEAR) && !other.HasTag(TAG_BROKEN)) isBlocked = true;
            if (!isBlocked) {
                visuals.drawingBeam = true; 
                visuals.beamP1 = e.position; 
                visuals.beamP2 = { e.position.x - 300.0f, e.position.y, e.position.z + 600.0f }; 
                visuals.beamP3 = { e.position.x + 300.0f, e.position.y, e.position.z + 600.0f };
                
                Vector2 bp1 = {visuals.beamP1.x, visuals.beamP1.z}; Vector2 bp2 = {visuals.beamP2.x, visuals.beamP2.z}; Vector2 bp3 = {visuals.beamP3.x, visuals.beamP3.z};
                
                for (auto& target : entities) {
                    if (&target != &e && !target.isStone && (target.canGrab || target.name == "Player")) {
                        Vector2 targetPos2D = {target.position.x, target.position.z};
                        if (CheckCollisionPointTriangle(targetPos2D, bp1, bp2, bp3)) {
                            bool immune = (target.name == "Player" && equippedEyewear != -1 && !entities[equippedEyewear].HasTag(TAG_BROKEN));
                            if (!immune) { target.isStone = true; target.color = GRAY; target.isGlitching = false; }
                        }
                    }
                }
            }
        }
        
        if (e.isUsing && e.HasTag(TAG_EXTINGUISHER)) {
            visuals.drawingExtinguisher = true; 
            Vector3 dir = player.facingDir; 
            Vector3 perp = { -dir.z, 0.0f, dir.x }; 
            visuals.extP1 = e.position; 
            visuals.extP2 = { e.position.x + dir.x * 400.0f + perp.x * 200.0f, e.position.y, e.position.z + dir.z * 400.0f + perp.z * 200.0f }; 
            visuals.extP3 = { e.position.x + dir.x * 400.0f - perp.x * 200.0f, e.position.y, e.position.z + dir.z * 400.0f - perp.z * 200.0f };
            
            player.velocity.x -= dir.x * 1500.0f * dt;
            player.velocity.z -= dir.z * 1500.0f * dt;

            Vector2 ep1 = {visuals.extP1.x, visuals.extP1.z}; Vector2 ep2 = {visuals.extP2.x, visuals.extP2.z}; Vector2 ep3 = {visuals.extP3.x, visuals.extP3.z};

            for (auto& target : entities) {
                if (&target != &player) {
                    Vector2 targetPos2D = {target.position.x, target.position.z};
                    if(CheckCollisionPointTriangle(targetPos2D, ep1, ep2, ep3)) {
                        if (target.HasTag(TAG_SUN_DISK) && target.isGlitching) target.isGlitching = false; 
                        if (target.HasTag(TAG_WATER_SOURCE) && target.stateValue > 0) { target.isStone = true; target.isGlitching = false; } 
                        if (target.canGrab && !target.HasTag(TAG_HEAVY)) {
                            target.velocity.x += dir.x * 2500.0f * dt;
                            target.velocity.z += dir.z * 2500.0f * dt;
                        }
                    }
                }
            }
        }

        if (e.HasTag(TAG_HOLE)) {
            for (auto& target : entities) {
                if (&target != &e && CheckCollisionLists(e.GetWorldBounds(), target.GetWorldBounds())) {
                    if (target.HasTag(TAG_WATER_SOURCE)) target.isGlitching = false; 
                    if (target.HasTag(TAG_BOULDER) && target.isGlitching) {
                        target.isGlitching = false; target.velocity = {0,0,0}; target.position = e.position; target.position.y = -20.0f; 
                    }
                    if (target.HasTag(TAG_MJOLNIR)) target.isGlitching = false; 
                }
            }
        }

        if (e.HasTag(TAG_MJOLNIR) && !e.canGrab) {
            for (const auto& w : entities) {
                if (w.HasTag(TAG_BUBBLE_WRAP) && w.isGrabbed) {
                    if (CheckCollisionLists(e.GetWorldBounds(), w.GetWorldInteractBounds())) {
                        e.position = w.position; 
                    }
                }
            }
        }

        if (e.HasTag(TAG_GLEIPNIR) && e.isGlitching && !e.isStone) {
            e.position.y = 5.0f; 
            Vector3 dir = Vector3Normalize(Vector3Subtract(player.position, e.position));
            e.velocity.x += dir.x * 250.0f * dt;
            e.velocity.z += dir.z * 250.0f * dt;

            for (auto& other : entities) {
                if (&e != &other && !other.isGrabbed && CheckCollisionLists(e.GetWorldBounds(), other.GetWorldBounds())) {
                    if (other.HasTag(TAG_SANDBAG)) {
                        e.attachedTo = &other - &entities[0]; e.isGlitching = false; break; 
                    }
                    if (other.HasTag(TAG_MUMMY) && other.isGlitching) {
                        e.attachedTo = &other - &entities[0]; e.isGlitching = false; 
                        other.isGlitching = false; other.color = WHITE; break; 
                    }
                }
            }
        }

        if (e.HasTag(TAG_BANSHEE_STONE) && e.isGlitching && !e.isStone) {
            e.stateValue += 300.0f * dt; 
            if (e.stateValue > 800.0f) {
                e.stateValue = 0.0f; 
                if (GetRandomValue(0, 100) < 5) {
                    int randIdx = GetRandomValue(1, entities.size()-1);
                    if (!entities[randIdx].isGrabbed && !entities[randIdx].isStatic) {
                        entities[randIdx].isGlitching = true;
                    }
                }
            }
        }
        
        if (e.HasTag(TAG_WIND_BAG) && e.isGlitching) {
            for (auto& target : entities) {
                if (&target != &e && target.attachedTo == -1 && !target.isStone && !target.isStatic) {
                    float dist = Vector3Distance(e.position, target.position);
                    if (dist < 600.0f && dist > 5.0f) {
                        Vector3 pushDir = Vector3Normalize(Vector3Subtract(target.position, e.position));
                        float force = (600.0f - dist) * 4.0f; 
                        if (target.HasTag(TAG_HEAVY)) force *= 0.1f; 
                        if (target.name == "Player") force *= 0.6f;  
                        target.velocity.x += pushDir.x * force * dt; 
                        target.velocity.z += pushDir.z * force * dt;
                    }
                }
            }
        }
    }

    return visuals;
}

inline void SetupNightHazards(int currentNight, std::vector<Entity>& entities) {
    // 1. Reset everything to a safe state first
    for (auto& e : entities) {
        e.isGlitching = false;
        if (e.HasTag(TAG_LIGHTSWITCH)) e.stateValue = 1.0f; // Lights ON
    }

    // 2. Night 1: Orientation (Water & Boulder)
    if (currentNight >= 1) {
        for (auto& e : entities) {
            if (e.HasTag(TAG_WATER_SOURCE) || e.HasTag(TAG_BOULDER)) e.isGlitching = true;
        }
    }

    // 3. Night 2: Cross-Breeze (Wind & Sparks)
    if (currentNight >= 2) {
        for (auto& e : entities) {
            if (e.HasTag(TAG_WIND_BAG) || e.HasTag(TAG_ZEUS)) e.isGlitching = true;
        }
    }

    // 4. Night 3: Graveyard Shift (Heat & Mummy)
    if (currentNight >= 3) {
        for (auto& e : entities) {
            if (e.HasTag(TAG_SUN_DISK) || e.HasTag(TAG_MUMMY)) e.isGlitching = true;
        }
    }

    // 5. Night 4: Heavy Lifting (Nordic Room)
    if (currentNight >= 4) {
        for (auto& e : entities) {
            if (e.HasTag(TAG_MJOLNIR) || e.HasTag(TAG_GLEIPNIR)) {
                e.isGlitching = true;
                if (e.HasTag(TAG_MJOLNIR)) e.canGrab = false; // Make sure Thor's hammer is stuck!
            }
        }
    }

    // 6. Night 6: The Blackout
    if (currentNight == 6) {
        for (auto& e : entities) {
            if (e.HasTag(TAG_LIGHTSWITCH)) e.stateValue = 0.0f; // Force lights off at start
        }
    }

    // 7. Night 7: Boss Room (Pandora)
    // Note: Add your TAG_PANDORA to Entities.h when you are ready to code the spawner!
}