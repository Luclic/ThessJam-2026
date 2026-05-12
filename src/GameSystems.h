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

inline HazardVisuals UpdatePhysicsAndHazards(std::vector<Entity>& entities, float dt, int grabbedEntityIndex, int equippedEyewear, int equippedGloves, int currentNight, float shiftTimer) {
    Entity& player = entities[0];
    HazardVisuals visuals;

    // --- 🕒 THE 5-NIGHT SHIFT TIMELINE DIRECTOR ---
    static bool e_start = false;
    static bool e_15s = false;
    static bool e_30s = false;
    static bool e_90s = false;
    
    if (shiftTimer < 0.1f) {
        e_start = false; e_15s = false; e_30s = false; e_90s = false;
    }

    if (!e_start && shiftTimer >= 0.1f) {
        e_start = true;
        // Night 2-4: Boulder standard rolling. Night 5: Reserved for Boss fight.
        if (currentNight >= 2 && currentNight < 5) { for(auto& e: entities) if(e.HasTag(TAG_BOULDER)) e.isGlitching = true; }
        // Night 3: Egyptian Room opens
        if (currentNight >= 3) { 
            for(auto& e: entities) {
                if(e.HasTag(TAG_MUMMY)) e.isGlitching = true;
                if(e.HasTag(TAG_ZEUS)) e.isGlitching = true; 
            }
        }
        // Night 4: Nordic Room opens
        if (currentNight >= 4) { for(auto& e: entities) if(e.HasTag(TAG_BANSHEE_STONE)) { e.isGlitching = true; e.stateValue = 0.0f; } }
        // Night 5: The Boss Room opens
        if (currentNight >= 5) { for(auto& e: entities) if(e.HasTag(TAG_PANDORA)) e.isGlitching = true; }
    }
    
    if (!e_15s && shiftTimer >= 15.0f) {
        e_15s = true;
        if (currentNight >= 1) { for(auto& e: entities) if(e.HasTag(TAG_MEDUSA) && e.position.y > -50.0f) e.isGlitching = true; }
    }
    
    if (!e_30s && shiftTimer >= 30.0f) {
        e_30s = true;
        if (currentNight >= 2) { for(auto& e: entities) if(e.HasTag(TAG_SANDALS) && e.position.y > -50.0f) e.isGlitching = true; }
    }
    
    if (!e_90s && shiftTimer >= 90.0f) {
        e_90s = true;
        if (currentNight >= 1) { for(auto& e: entities) if(e.HasTag(TAG_WATER_SOURCE) && e.position.y > -50.0f) e.isGlitching = true; }
        if (currentNight >= 2) { for(auto& e: entities) if(e.HasTag(TAG_WIND_BAG) && e.position.y > -50.0f) e.isGlitching = true; }
        if (currentNight >= 3) { for(auto& e: entities) if(e.HasTag(TAG_SUN_DISK) && e.position.y > -50.0f) e.isGlitching = true; }
        if (currentNight >= 4) { for(auto& e: entities) if(e.HasTag(TAG_GLEIPNIR) && e.position.y > -50.0f) e.isGlitching = true; }
    }
    // ---------------------------------------

    // Check if Pandora's Box is currently active for Wall Phasing
    bool isPandoraActive = false;
    for (auto& p : entities) {
        if (p.HasTag(TAG_PANDORA) && p.isGlitching && p.stateTimer <= 0.0f && !p.HasTag(TAG_BROKEN)) {
            isPandoraActive = true;
            break;
        }
    }

    for (size_t i = 0; i < entities.size(); ++i) {
        Entity& e = entities[i];
        
        // --- QUALITY OF LIFE: DISABLE COLLISION FOR HELD/WORN ITEMS ---
        if ((int)i == grabbedEntityIndex || (int)i == equippedEyewear || (int)i == equippedGloves || e.attachedTo != -1) {
            e.isSolid = false; 
        }
        
        if ((int)i == grabbedEntityIndex) {
            e.position.x = player.position.x + player.facingDir.x * 60.0f;
            e.position.y = player.position.y + 70.0f; 
            e.position.z = player.position.z + player.facingDir.z * 60.0f; 
            e.velocity = {0,0,0}; continue;
        }
        
        if (e.attachedTo != -1) {
            Entity& parent = entities[e.attachedTo];
            
            // --- NEW: THE SISYPHUS BLENDER ORBIT ---
            if (e.HasTag(TAG_BOULDER) && parent.HasTag(TAG_PANDORA)) {
                e.stateTimer += dt * 8.0f; // Orbit speed
                // Mathematically lock the boulder to Pandora's exact position
                e.position.x = parent.position.x + cos(e.stateTimer) * 120.0f; 
                e.position.y = parent.position.y + 10.0f; 
                e.position.z = parent.position.z + sin(e.stateTimer) * 120.0f;
                e.velocity = {0,0,0};
                
                // The boulder still smashes fragile things while orbiting!
                for (auto& f : entities) {
                    if (&e != &f && (f.HasTag(TAG_FRAGILE) || f.HasTag(TAG_MEDUSA)) && !f.HasTag(TAG_BROKEN)) {
                        if (CheckCollisionLists(e.GetWorldBounds(), f.GetWorldBounds())) {
                            f.AddTag(TAG_BROKEN); f.color = DARKGRAY; f.attachedTo = -1;
                        }
                    }
                }
                continue; 
            }

            float localMinY = e.boundsList.empty() ? 0.0f : e.boundsList[0].min.y;
            e.position = parent.position; 
            
            if (parent.name.find("stand") != std::string::npos) {
                // PEDESTALS: Uses the parent's actual height hitbox so it sits beautifully on top
                float parentHeight = parent.boundsList.empty() ? 40.0f : parent.boundsList[0].max.y;
                e.position.y = parent.position.y + parentHeight - localMinY + 90.0f; 
            } 
            else if (parent.HasTag(TAG_ZEUS)) {
                // ZEUS: Totally custom coordinates. You can fine tune these!
                e.position.x -= 40.0f; 
                e.position.y += 120.0f - localMinY; 
                e.position.z += 10.0f; 
            } 
            else if (parent.HasTag(TAG_FUSEBOX)) {
                // FUSEBOX: Snaps into the socket
                e.position.z += 20.0f; 
                e.position.y -= 20.0f - localMinY; 
            } 
            else {
                // FALLBACK
                float parentHeight = parent.boundsList.empty() ? 10.0f : parent.boundsList[0].max.y;
                e.position.y = parent.position.y + parentHeight - localMinY; 
            }
            
            e.velocity = {0,0,0}; continue;
        }
        
        if ((int)i == equippedEyewear) {
            e.position.x = player.position.x + player.facingDir.x * 15.0f;
            e.position.y = player.position.y + 135.0f; 
            e.position.z = player.position.z + player.facingDir.z * 15.0f;
            e.velocity = {0,0,0}; continue;
        }
        
        if ((int)i == equippedGloves) {
            e.position.x = player.position.x + player.facingDir.x * 25.0f;
            e.position.y = player.position.y + 80.0f; 
            e.position.z = player.position.z + player.facingDir.z * 25.0f;
            e.velocity = {0,0,0}; continue;
        }

        float currentFriction = 6.0f;

        if (e.HasTag(TAG_MJOLNIR) && e.stateTimer > 0.0f) {
            currentFriction = 0.2f; // Slippery ice physics!
            e.stateTimer -= dt; 
        }
        
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

        // --- HERMES SANDALS SWEEPING FLIGHT ---
        if (e.HasTag(TAG_SANDALS) && e.isGlitching && !e.isStone && !e.isGrabbed) {
            float timeSec = (float)GetTime();
            e.position.y = 70.0f + sinf(timeSec * 4.0f) * 15.0f; 
            e.velocity.x = cosf(timeSec * 0.5f) * 500.0f;
            e.velocity.z = sinf(timeSec * 0.35f) * 500.0f;
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
            
            bool inHole = false;
            for (const auto& hole : entities) {
                if (hole.HasTag(TAG_HOLE) && CheckCollisionLists(myBoxY, hole.GetWorldBounds())) {
                    inHole = true; break;
                }
            }
            
            if (e.position.y <= 0.1f && !inHole) { 
                e.position.y = 0.0f;
                hitGround = true;
            } else if (!inHole) {
                for (const auto& other : entities) {
                    if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1) {
                        if (fabs(e.position.x - other.position.x) > 300.0f || fabs(e.position.z - other.position.z) > 300.0f) continue;
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

        // --- X/Z AXIS COLLISION (Smooth Sliding Fix) ---
        if (abs(e.velocity.x) > 0.1f && !e.isGrabbed) {
            float oldX = e.position.x;
            e.position.x += e.velocity.x * dt;
            std::vector<BoundingBox> nextX = e.GetWorldBounds();
            bool collided = false;
            for (auto& other : entities) {
                if (e.HasTag(TAG_BOULDER) && other.name.find("stand") != std::string::npos) continue; 
                if (e.HasTag(TAG_BOULDER) && other.name == "Player") continue; 
                if (e.name == "Player" && other.HasTag(TAG_BOULDER)) continue; 

                if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1) {
                    if (fabs(e.position.x - other.position.x) > 300.0f || fabs(e.position.z - other.position.z) > 300.0f) continue;
                    bool isObstacleTallEnough = false;
                    for (const auto& ob : other.GetWorldBounds()) {
                        if (ob.max.y > e.position.y + 5.0f) { isObstacleTallEnough = true; break; }
                    }
                    if (isObstacleTallEnough && CheckCollisionLists(nextX, other.GetWorldBounds())) {
                        collided = true; break;
                    }
                }
            }
            if (collided) {
                e.position.x = oldX; 
                e.velocity.x = 0.0f; // Stops the input-feedback sticking loop
            }
        }
        
        if (abs(e.velocity.z) > 0.1f && !e.isGrabbed) {
            float oldZ = e.position.z;
            e.position.z += e.velocity.z * dt;
            std::vector<BoundingBox> nextZ = e.GetWorldBounds();
            bool collided = false;
            for (auto& other : entities) {
                if (e.HasTag(TAG_BOULDER) && other.name.find("stand") != std::string::npos) continue; 

                if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1) {
                    if (fabs(e.position.x - other.position.x) > 300.0f || fabs(e.position.z - other.position.z) > 300.0f) continue;
                    bool isObstacleTallEnough = false;
                    for (const auto& ob : other.GetWorldBounds()) {
                        if (ob.max.y > e.position.y + 5.0f) { isObstacleTallEnough = true; break; }
                    }
                    if (isObstacleTallEnough && CheckCollisionLists(nextZ, other.GetWorldBounds())) {
                        collided = true; break;
                    }
                }
            }
            if (collided) {
                e.position.z = oldZ; 
                e.velocity.z = 0.0f; // Stops the input-feedback sticking loop
            }
        }

        if (e.HasTag(TAG_WATER_SOURCE)) {
            if (e.isGlitching) {
                float maxRadius = 9999.0f;
                for (const auto& sp : entities) if (sp.HasTag(TAG_SPONGE) && !sp.isGrabbed && sp.position.y < 20.0f) { 
                    float dist = Vector3Distance(e.position, sp.position); 
                    if (dist - 40.0f < maxRadius) maxRadius = dist - 40.0f; 
                }
                if (e.stateValue < maxRadius) { e.stateValue += 40.0f * dt; if (e.stateValue > maxRadius) e.stateValue = maxRadius; }
            }
            if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_MOP) && entities[grabbedEntityIndex].isUsing) {
                if (Vector3Distance(player.position, e.position) < e.stateValue + 100.0f) { e.stateValue -= 400.0f * dt; if (e.stateValue < 0) e.stateValue = 0; }
            }
            if (e.stateTimer > 0.0f) { e.stateTimer -= dt; if (e.stateTimer <= 0.0f) e.isGlitching = true; }
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
            if (CheckCollisionLists(e.GetWorldBounds(), player.GetWorldBounds())) {
                if (player.position.x > e.position.x) { 
                    player.position.x = e.position.x + 80.0f; 
                    if (player.velocity.x < -10.0f) e.velocity.x = -200.0f; 
                    else player.velocity.x = e.velocity.x; 
                } else {
                    player.position.x = e.position.x - 80.0f;
                    if (player.velocity.x > 10.0f) e.velocity.x = 200.0f; 
                }
            }
            for (auto& other : entities) {
                if (&e != &other && (other.HasTag(TAG_FRAGILE) || other.HasTag(TAG_MEDUSA))) {
                    if (CheckCollisionLists(e.GetWorldInteractBounds(), other.GetWorldInteractBounds()) && !other.HasTag(TAG_BROKEN)) {
                        other.AddTag(TAG_BROKEN);
                        other.color = DARKGRAY;
                        if (!other.boundsList.empty()) other.boundsList[0].max.y = 5.0f; 
                        other.isGlitching = false; 
                        other.attachedTo = -1; 
                    }
                }
            }
        }

        // --- RA'S SUN DISK LOGIC ---
        if (e.HasTag(TAG_SUN_DISK)) {
            if (e.isGlitching) {
                e.color = ORANGE; 
                e.canGrab = false; 
                for (auto& w : entities) {
                    if (w.HasTag(TAG_WATER_SOURCE) && w.stateValue > 0.0f && Vector3Distance(e.position, w.position) < w.stateValue) {
                        e.isGlitching = false; 
                        break;
                    }
                }
                for (auto& other : entities) {
                    if (&e != &other && other.HasTag(TAG_BUBBLE_WRAP) && !other.isGrabbed) {
                        if (CheckCollisionLists(e.GetWorldBounds(), other.GetWorldBounds())) {
                            other.position = {-9999, -9999, -9999}; 
                            other.isSolid = false;
                        }
                    }
                }
            } else { 
                e.color = BLACK; 
                e.canGrab = true; 
            }
        }

        // --- RESTLESS MUMMY LOGIC ---
        if (e.HasTag(TAG_MUMMY) && e.isGlitching && !e.HasTag(TAG_BROKEN) && !e.isStone && !e.isGrabbed) {
            Entity* targetFusebox = nullptr;
            float closestDist = 9999.0f;

            for (auto& other : entities) {
                if (other.HasTag(TAG_FUSEBOX) && other.stateValue > 0.5f) { 
                    float d = Vector3Distance(e.position, other.position);
                    if (d < closestDist) {
                        closestDist = d; targetFusebox = &other;
                    }
                }
            }
            if (targetFusebox) {
                Vector3 dir = Vector3Normalize(Vector3Subtract(targetFusebox->position, e.position));
                e.velocity.x += dir.x * 250.0f * dt; 
                e.velocity.z += dir.z * 250.0f * dt;
                if (closestDist < 80.0f) targetFusebox->stateValue = 0.0f; 
            } else {
                if (GetRandomValue(0, 100) < 2) { 
                    e.facingDir.x = (GetRandomValue(-100, 100) / 100.0f);
                    e.facingDir.z = (GetRandomValue(-100, 100) / 100.0f);
                    if (Vector3Length(e.facingDir) > 0.0f) e.facingDir = Vector3Normalize(e.facingDir);
                }
                e.velocity.x += e.facingDir.x * 120.0f * dt;
                e.velocity.z += e.facingDir.z * 120.0f * dt;
            }
            for (auto& other : entities) {
                if (&e != &other && (other.HasTag(TAG_FRAGILE) || other.HasTag(TAG_MEDUSA)) && !other.HasTag(TAG_BROKEN)) {
                    if (CheckCollisionLists(e.GetWorldBounds(), other.GetWorldBounds())) {
                        other.AddTag(TAG_BROKEN); other.color = DARKGRAY; 
                        if (!other.boundsList.empty()) other.boundsList[0].max.y = 5.0f; 
                        other.attachedTo = -1;
                    }
                }
            }
        }

    } // END OF MAIN ENTITY LOOP

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
                            if (!immune) { 
                                target.isStone = true; 
                                target.color = GRAY; 
                                target.isGlitching = false; 
                                target.AddTag(TAG_HEAVY);   
                                target.velocity.y = -500.0f; 
                            }            
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
                        
                        if (target.HasTag(TAG_MJOLNIR)) {
                            target.velocity.x += dir.x * 4000.0f * dt;
                            target.velocity.z += dir.z * 4000.0f * dt;
                            target.stateTimer = 1.5f; 
                        }

                        if (target.HasTag(TAG_SUN_DISK) && target.isGlitching) {
                            target.isGlitching = false; 
                            target.color = BLACK;  
                            target.canGrab = true; 
                        } 
                        if (target.HasTag(TAG_WATER_SOURCE) && target.stateValue > 0) { target.isStone = true; target.isGlitching = false; } 
                        if (target.canGrab && !target.HasTag(TAG_HEAVY)) {
                            target.velocity.x += dir.x * 2500.0f * dt;
                            target.velocity.z += dir.z * 2500.0f * dt;
                        }

                        if (target.HasTag(TAG_PANDORA) && target.isGlitching) {
                            target.stateTimer = 15.0f; // Thermal Shock freezes it for 15s
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
                    if (target.HasTag(TAG_MUMMY) && target.isGlitching) {
                        target.isGlitching = false; 
                        target.velocity = {0,0,0}; 
                        target.position = e.position; 
                        target.position.y = -30.0f; 
                    }
                    // --- CURE 4: THE HANDSAW (Drop it in a hole) ---
                    if (target.HasTag(TAG_PANDORA)) {
                        target.isGlitching = false; 
                        target.velocity = {0,0,0}; 
                        target.position = e.position; 
                        target.position.y = -50.0f; // Swallowed by the floor!
                    }
                }
            }
        }

        if (e.HasTag(TAG_MJOLNIR)) {
            if (e.canGrab && !e.isGrabbed) {
                float timeSec = (float)GetTime();
                e.position.y = 40.0f + sinf(timeSec * 3.0f + e.position.x) * 10.0f; 
                e.velocity.y = 0.0f; 
            }
            if (!e.canGrab) {
                for (const auto& w : entities) {
                    if (w.HasTag(TAG_BUBBLE_WRAP) && w.isGrabbed) {
                        if (CheckCollisionLists(e.GetWorldBounds(), w.GetWorldInteractBounds())) {
                            e.position.x = w.position.x;
                            e.position.z = w.position.z;
                        }
                    }
                }
            }
        }

        if (e.HasTag(TAG_GLEIPNIR) && e.isGlitching && !e.isStone) {
            e.position.y = 5.0f; 
            Vector3 toPlayer = Vector3Subtract(player.position, e.position);
            float dist = Vector3Length(toPlayer);
            
            if (dist > 10.0f) { 
                Vector3 dir = { toPlayer.x / dist, 0.0f, toPlayer.z / dist }; 
                Vector3 perp = { -dir.z, 0.0f, dir.x }; 
                
                float timeSec = (float)GetTime();
                float wiggle = sinf(timeSec * 8.0f) * 350.0f; 
                
                e.velocity.x = dir.x * 250.0f + perp.x * wiggle;
                e.velocity.z = dir.z * 250.0f + perp.z * wiggle;
            }

            for (size_t j = 0; j < entities.size(); ++j) {
                auto& other = entities[j];
                if (&e != &other && !other.isGrabbed) {
                    if (fabs(e.position.x - other.position.x) > 200.0f || fabs(e.position.z - other.position.z) > 200.0f) continue;
                    if (CheckCollisionLists(e.GetWorldBounds(), other.GetWorldBounds())) {
                        if (other.HasTag(TAG_SANDBAG)) {
                            e.attachedTo = j; e.isGlitching = false; e.position.y = other.position.y + 10.0f; break; 
                        }
                        if (other.HasTag(TAG_MUMMY) && other.isGlitching) {
                            e.attachedTo = j; e.isGlitching = false; other.isGlitching = false; other.color = WHITE; e.position.y = other.position.y + 30.0f; break; 
                        }
                        if (other.name == "Player") { player.isDead = true; }
                    }
                }
            }
        }

        if (e.HasTag(TAG_BANSHEE_STONE) && e.isGlitching && !e.isStone) {
            float growthSpeed = (currentNight == 4) ? 17.77f : 400.0f;
            e.stateValue += growthSpeed * dt; 
            
            if (e.stateValue > 800.0f) {
                if (currentNight == 4) {
                    int effect = GetRandomValue(0, 2);
                    if (effect == 0) {
                        for(auto& t: entities) if(t.HasTag(TAG_SUN_DISK) && t.position.y > -50.0f) t.isGlitching = true;
                    } else if (effect == 1) {
                        for(auto& t: entities) if(t.HasTag(TAG_MUMMY) && t.position.y > -50.0f) { t.isGlitching = true; t.RemoveTag(TAG_BROKEN); t.color = WHITE; }
                    } else {
                        for(auto& t: entities) if(t.HasTag(TAG_SANDALS) && t.position.y > -50.0f) { t.isGlitching = true; t.stateTimer = 0.0f; }
                    }
                } else {
                    for (auto& target : entities) {
                        if (&target != &e && !target.isStatic && !target.isGrabbed && target.name != "Player" && target.position.y > -50.0f) {
                            if (Vector3Distance(e.position, target.position) < 800.0f) target.isGlitching = true;
                        }
                    }
                }
                e.stateValue = 0.0f; 
            }
        }
        
        if (e.HasTag(TAG_WIND_BAG) && e.isGlitching && !e.HasTag(TAG_BROKEN)) {
            for (auto& item : entities) {
                if (item.HasTag(TAG_CORK) && !item.isGrabbed && Vector3Distance(e.position, item.position) < 60.0f) { e.isGlitching = false; }
            }
            if (e.isGlitching) { 
                for (auto& target : entities) {
                    if (&target != &e && target.attachedTo == -1 && !target.isStone && !target.isStatic && target.position.y > -50.0f) {
                        float dist = Vector3Distance(e.position, target.position);
                        if (dist < 600.0f && dist > 5.0f) {
                            bool isBlocked = false;
                            for (auto& blocker : entities) {
                                if (blocker.HasTag(TAG_SANDBAG) && !blocker.isGrabbed && blocker.position.y < 20.0f) {
                                    float blockerDist = Vector3Distance(e.position, blocker.position);
                                    if (blockerDist < dist && Vector3Distance(target.position, blocker.position) < 150.0f) {
                                        isBlocked = true; break;
                                    }
                                }
                            }
                            if (!isBlocked) {
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
        }

        if (e.HasTag(TAG_ZEUS) && e.isGlitching) {
            float zapRadius = 150.0f;
            if (Vector3Distance(player.position, e.position) < zapRadius) {
                if (equippedGloves == -1) player.isDead = true;
            }
            for (auto& puddle : entities) {
                if (puddle.HasTag(TAG_WATER_SOURCE) && puddle.stateValue > 0.0f) {
                    if (Vector3Distance(e.position, puddle.position) < puddle.stateValue) {
                        if (Vector3Distance(player.position, puddle.position) < puddle.stateValue) {
                            if (equippedGloves == -1) player.isDead = true;
                        }
                    }
                }
            }
        }

        // --- PANDORA'S BOX (THE BLACK HOLE & THE CHAOS SYNERGIES) ---
        if (e.HasTag(TAG_PANDORA) && e.isGlitching && !e.HasTag(TAG_BROKEN)) {
            
            // CURE 1: Fire Extinguisher Thermal Shock
            if (e.stateTimer > 0.0f) {
                e.stateTimer -= dt;
                e.color = SKYBLUE; 
                continue; // Box is frozen, gravity pauses
            } else { e.color = PURPLE; }

            // Define Mutation Flags (Bitmask)
            enum Mutations { MUT_WIND=1, MUT_MEDUSA=2, MUT_SANDALS=4, MUT_ZEUS=8, MUT_MUMMY=16, MUT_WATER=32, MUT_GLEIPNIR=64, MUT_BANSHEE=128 };
            int currentMutations = (int)e.stateValue;

            float pullStrength = 850.0f; 
            float eventHorizon = 60.0f;  
            float gravityRange = 99999.0f; // INFINITE RANGE

            if (currentMutations & MUT_BANSHEE) { pullStrength *= 2.0f; eventHorizon = 100.0f; }
            
            if (currentMutations & MUT_SANDALS) {
                e.position.x += sin((float)GetTime() * 2.0f) * 150.0f * dt;
                e.position.z += cos((float)GetTime() * 1.5f) * 150.0f * dt;
            }

            for (auto& other : entities) {
                // EXPLICIT CHECK: Ensure floors/walls/static architecture are completely ignored!
                if (other.name == "Floor" || other.name == "Wall" || other.isStatic) continue;

                if (&e == &other || other.attachedTo != -1 || other.isGrabbed || other.position.y < -50.0f) continue;
                float dist = Vector3Distance(e.position, other.position);
                
                if (dist > gravityRange) continue;
                Vector3 dir = Vector3Normalize(Vector3Subtract(e.position, other.position));
                
                if (dist < eventHorizon) {
                    // CURE 2: Mjolnir (The Absolute Clog)
                    if (other.HasTag(TAG_MJOLNIR)) { e.isGlitching = false; e.AddTag(TAG_BROKEN); e.color = DARKGRAY; continue; }
                    // CURE 3: The Cork (Temporary Plug)
                    if (other.HasTag(TAG_CORK)) { e.stateTimer = 10.0f; other.position.y = -1000.0f; continue; }

                    if (other.name != "Player" && !other.HasTag(TAG_BOULDER)) {
                        // CONSUME AND MUTATE!
                        if (other.HasTag(TAG_WIND_BAG)) currentMutations |= MUT_WIND;
                        if (other.HasTag(TAG_MEDUSA)) currentMutations |= MUT_MEDUSA;
                        if (other.HasTag(TAG_SANDALS)) currentMutations |= MUT_SANDALS;
                        if (other.HasTag(TAG_ZEUS)) currentMutations |= MUT_ZEUS;
                        if (other.HasTag(TAG_MUMMY)) {
                            currentMutations |= MUT_MUMMY;
                            for (auto& f : entities) if (f.HasTag(TAG_FUSEBOX)) f.stateValue = 0.0f; 
                        }
                        if (other.HasTag(TAG_WATER_SOURCE) || other.HasTag(TAG_SPONGE)) currentMutations |= MUT_WATER;
                        if (other.HasTag(TAG_GLEIPNIR)) currentMutations |= MUT_GLEIPNIR;
                        if (other.HasTag(TAG_BANSHEE_STONE)) currentMutations |= MUT_BANSHEE;

                        e.stateValue = (float)currentMutations; 
                        other.position.y = -1000.0f; 
                        other.velocity = {0,0,0};
                        other.isGlitching = false;
                        other.canGrab = false; 
                    }
                } else {
                    // --- THE TRAP: CATCH THE BOULDER ---
                    if (other.HasTag(TAG_BOULDER)) {
                        if (dist < 180.0f) { 
                            other.attachedTo = (int)(&e - &entities[0]); 
                            other.stateTimer = 0.0f; 
                            continue; 
                        }
                    }

                    float applyForce = pullStrength;
                    if (other.HasTag(TAG_HEAVY) && other.stateTimer <= 0.0f) {
                        if (other.HasTag(TAG_MJOLNIR)) continue; 
                        applyForce *= 0.1f;
                    }
                    if (other.name == "Player") applyForce *= 1.2f; 
                    
                    other.velocity.x += dir.x * applyForce * dt;
                    other.velocity.z += dir.z * applyForce * dt;

                    if ((currentMutations & MUT_WIND) && !other.HasTag(TAG_MJOLNIR)) {
                        Vector3 vortexDir = {-dir.z, 0.0f, dir.x}; 
                        other.velocity.x += vortexDir.x * 1500.0f * dt;
                        other.velocity.z += vortexDir.z * 1500.0f * dt;
                    }

                    if (currentMutations & MUT_WATER) {
                        other.velocity.x += other.velocity.x * 5.0f * dt; 
                        other.velocity.z += other.velocity.z * 5.0f * dt;
                    }

                    if ((currentMutations & MUT_MEDUSA) && dist < 250.0f) {
                        bool immune = (other.name == "Player" && equippedEyewear != -1 && !entities[equippedEyewear].HasTag(TAG_BROKEN));
                        if (!immune && (other.canGrab || other.name == "Player")) {
                            other.isStone = true; other.color = GRAY; other.AddTag(TAG_HEAVY);
                        }
                    }

                    if ((currentMutations & MUT_ZEUS) && dist < 300.0f) {
                        if (other.name == "Player" && equippedGloves == -1) {
                            if (GetRandomValue(0, 100) < 3) player.isDead = true; 
                        }
                    }

                    if ((currentMutations & MUT_GLEIPNIR) && dist < 500.0f && other.name == "Player") {
                        if (GetRandomValue(0, 100) < 5) {
                            other.velocity.x += dir.x * 3000.0f * dt; 
                            other.velocity.z += dir.z * 3000.0f * dt;
                        }
                    }
                }
            }
        }
    }

    return visuals;
}

inline void SetupNightHazards(int currentNight, std::vector<Entity>& entities) {
    // 1. THE CLEAN SLATE 
    for (auto& e : entities) {
        e.isGlitching = false;
        if (e.HasTag(TAG_LIGHTSWITCH)) e.stateValue = 1.0f; // Lights ON
        
        // Ensure Zeus Lightning starts safe until Night 2 or 3
        if (e.HasTag(TAG_LIGHTNING)) {
            if (currentNight < 2) { e.RemoveTag(TAG_ELECTRIC); e.canGrab = false; }
            else { e.AddTag(TAG_ELECTRIC); }
        }
    }

    // 2. NIGHT 4: Nordic Room Setup
    if (currentNight >= 4) {
        for (auto& e : entities) {
            if (e.HasTag(TAG_MJOLNIR)) {
                e.position.y = 0.0f; // Drop Mjolnir to the floor at shift start
            }
        }
    }

    // 3. NIGHT 5: Boss Room (The Blackout)
    if (currentNight >= 5) {
        for (auto& e : entities) {
            if (e.HasTag(TAG_LIGHTSWITCH)) e.stateValue = 0.0f; // Start in the dark!
        }
    }
}