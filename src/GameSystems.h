#pragma once
#include "Entities.h"
#include "Interactions.h"
#include "raymath.h"
#include <vector>
#include <unordered_map>

// Access the global sounds map defined over in main.cpp
extern std::unordered_map<std::string, Sound> sounds;

struct HazardVisuals {
    bool drawingBeam = false;
    Vector3 beamP1, beamP2, beamP3;
    bool drawingExtinguisher = false;
    Vector3 extP1, extP2, extP3;
    bool pandoraWarning = false; 

    bool drawingSunBeams = false;
    Vector3 sunCenter;
    float sunAngle = 0.0f;

    bool qteActive = false;
    float qteProgress = 0.0f;
    float qteTimeLeft = 0.0f;
};

// ====================================================================================
// --- MAIN PHYSICS & HAZARD LOOP ---
// ====================================================================================
inline HazardVisuals UpdatePhysicsAndHazards(std::vector<Entity>& entities, float dt, int grabbedEntityIndex, int equippedEyewear, int equippedGloves, int currentNight, float shiftTimer, bool isPlaying) {
    Entity& player = entities[0];
    HazardVisuals visuals;
    visuals.pandoraWarning = false;

    // ---------------------------------------------------------
    // 1. GLOBAL STATES & TIMELINE
    // ---------------------------------------------------------
    bool isPandoraOpen = false;
    visuals.pandoraWarning = false;

    static bool e_start = false;
    static int gleipnirDefeats = 0;
    if (shiftTimer < 0.1f) { e_start = false; gleipnirDefeats = 0;}

    if (!e_start && shiftTimer >= 0.1f) {
        e_start = true;
        if (currentNight == 4) {
            for(auto& e: entities) {
                if (e.HasTag(TAG_BANSHEE_STONE)) { e.stateValue = 0.0f; }
                if (e.HasTag(TAG_MJOLNIR)) {
                    e.attachedTo = -1;         
                    e.position.y = 0.0f;       
                    e.position.x += 400.0f;    
                    e.position.z += 400.0f;
                    e.canGrab = false;         
                }
            }
        }
    }

    // ---------------------------------------------------------
    // 2. MAIN ENTITY PROCESSOR
    // ---------------------------------------------------------
    for (size_t i = 0; i < entities.size(); ++i) {
        Entity& e = entities[i];
        bool skipPhysics = false; 

        // --- PHASE A: TRANSFORMS & ATTACHMENTS ---
        if ((int)i == grabbedEntityIndex || (int)i == equippedEyewear || (int)i == equippedGloves || e.attachedTo != -1) {
            e.isSolid = false; 
        }
        
        if ((int)i == grabbedEntityIndex) {
            e.position.x = player.position.x + player.facingDir.x * 60.0f;
            e.position.y = player.position.y + 70.0f; 
            e.position.z = player.position.z + player.facingDir.z * 60.0f; 
            e.stateValue = player.stateValue;
            e.velocity = {0,0,0}; skipPhysics = true;
        }
        else if (e.attachedTo != -1) {
            Entity& parent = entities[e.attachedTo];
            float localMinY = e.boundsList.empty() ? 0.0f : e.boundsList[0].min.y;
            e.position = parent.position; 
            
            if (parent.name == "ticketstand") {
                // Use the absolute top of the ticketstand model
                float parentHeight = parent.boundsList.empty() ? 100.0f : parent.boundsList[0].max.y;
                
                // We sit it directly on the surface (no +90f floating offset)
                // We add +5.0f just to prevent Z-fighting with the desk surface
                e.position.y = parent.position.y + parentHeight - localMinY + 150.0f; 
            } 

            else if (parent.name.find("artifactsign") != std::string::npos) {
                // Use the highest point of the sign
                float parentHeight = parent.boundsList.empty() ? 30.0f : parent.boundsList[0].max.y;
                e.position.y = parent.position.y + parentHeight - localMinY; 
                
                // Push the brochure slightly forward and rotate it to match the sign perfectly
                float rad = parent.stateValue * DEG2RAD;
                e.position.x += sin(rad) * 10.0f; 
                e.position.z += cos(rad) * 10.0f; 
                e.stateValue = parent.stateValue; 
            }
            // --- Existing Pedestal Logic ---
            else if (parent.name.find("stand") != std::string::npos) {
                float parentHeight = parent.boundsList.empty() ? 40.0f : parent.boundsList[0].max.y;
                e.position.y = parent.position.y + parentHeight - localMinY + 90.0f; 
            } 
            else if (parent.HasTag(TAG_ZEUS)) {
                e.position = { 2260.0f, 279.0f, 470.0f };            
            } 
            else if (parent.HasTag(TAG_FUSEBOX)) {
                e.position.z += 20.0f; e.position.y -= 20.0f - localMinY; 
            } 
            else if (parent.HasTag(TAG_PANDORA)) {
                float parentHeight = parent.boundsList.empty() ? 20.0f : parent.boundsList[0].max.y;
                e.position.y = parent.position.y + parentHeight + 5.0f;
            }
            else if (parent.HasTag(TAG_MEDUSA)) {
                float parentHeight = parent.boundsList.empty() ? 40.0f : parent.boundsList[0].max.y;
                e.position.y = parent.position.y + parentHeight - localMinY - 25.0f; 
                float rad = parent.stateValue * DEG2RAD;
                e.position.x += sin(rad) * 12.0f; 
                e.position.z += cos(rad) * 12.0f; 
                e.stateValue = parent.stateValue; 
            }
            else { 
                float parentHeight = parent.boundsList.empty() ? 10.0f : parent.boundsList[0].max.y;
                e.position.y = parent.position.y + parentHeight - localMinY; 
            }
            
            e.velocity = {0,0,0}; skipPhysics = true;
        }
        else if ((int)i == equippedEyewear) {
            float rad = player.stateValue * DEG2RAD;
            e.position.x = player.position.x + sin(rad) * 12.0f; 
            e.position.y = player.position.y + 135.0f; 
            e.position.z = player.position.z + cos(rad) * 12.0f;
            e.stateValue = player.stateValue; 
            e.velocity = {0,0,0}; skipPhysics = true; 
        }
        else if ((int)i == equippedGloves) {
            e.position.x = player.position.x + player.facingDir.x * 25.0f;
            e.position.y = player.position.y + 80.0f; 
            e.position.z = player.position.z + player.facingDir.z * 25.0f;
            e.velocity = {0,0,0}; skipPhysics = true;
        }

        // --- PHASE B: PHYSICS (Friction, Gravity, Collisions) ---
        if (!skipPhysics) {
            float currentFriction = 6.0f;
            if (e.HasTag(TAG_MJOLNIR) && e.stateTimer > 0.0f) {
                currentFriction = 0.2f; e.stateTimer -= dt; 
            }
            for (auto& w : entities) {
                if (w.HasTag(TAG_WATER_SOURCE) && w.isStone && Vector3Distance(e.position, w.position) < w.stateValue) currentFriction = 0.5f; 
            }

            e.velocity.x -= e.velocity.x * currentFriction * dt; 
            e.velocity.z -= e.velocity.z * currentFriction * dt;

            Vector3 horizVel = {e.velocity.x, 0.0f, e.velocity.z};
            if (&e == &player && Vector3Length(horizVel) > 700.0f) {
                horizVel = Vector3Scale(Vector3Normalize(horizVel), 700.0f);
                e.velocity.x = horizVel.x; e.velocity.z = horizVel.z;
            }

            if (!e.isStatic) { 
                float prevYVel = e.velocity.y;
                e.velocity.y -= 2400.0f * dt; 
                e.position.y += e.velocity.y * dt;
                
                std::vector<BoundingBox> myBoxY = e.GetWorldBounds();
                bool hitGround = false;
                bool inHole = false;
                
                // --- NEW FIX: Make sure the hole doesn't check against itself! ---
                for (const auto& hole : entities) {
                    if (&e != &hole && hole.HasTag(TAG_HOLE) && CheckCollisionLists(myBoxY, hole.GetWorldBounds())) { 
                        inHole = true; break; 
                    }
                }
                
                if (e.position.y <= 0.1f && !inHole) { 
                    e.position.y = 0.0f; hitGround = true;
                } else if (!inHole) {
                    for (const auto& other : entities) {
                        if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1) {
                            if (fabs(e.position.x - other.position.x) > 300.0f || fabs(e.position.z - other.position.z) > 300.0f) continue;
                            if (CheckCollisionLists(myBoxY, other.GetWorldBounds())) {
                                if (e.velocity.y <= 0.0f) { 
                                    float highestY = -99999.0f;
                                    for (const auto& b : other.GetWorldBounds()) if (b.max.y > highestY) highestY = b.max.y;
                                    e.position.y = highestY + 0.1f; hitGround = true; break;
                                }
                            }
                        }
                    }
                }
                
                if (e.position.y < -300.0f) { e.velocity = {0,0,0}; e.isGlitching = false; }

                if (hitGround) {
                    e.velocity.y = 0.0f; e.velocity.x *= 0.8f; e.velocity.z *= 0.8f;
                    if (prevYVel < -600.0f) { 
                        bool safe = false;
                        for (const auto& mat : entities) if (mat.HasTag(TAG_MAT) && CheckCollisionLists(e.GetWorldBounds(), mat.GetWorldBounds())) safe = true;
                        if (!safe) ProcessImpact(e); 
                    }
                }
            }

            // X/Z Axis Smooth Collision
            if (abs(e.velocity.x) > 0.1f) {
                float oldX = e.position.x; e.position.x += e.velocity.x * dt;
                bool collided = false;
                for (auto& other : entities) {
                    if (e.HasTag(TAG_BOULDER) && (other.name.find("stand") != std::string::npos || other.name == "Player")) continue; 
                    if (e.name == "Player" && other.HasTag(TAG_BOULDER)) continue; 
                    if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1) {
                        if (fabs(e.position.x - other.position.x) > 300.0f || fabs(e.position.z - other.position.z) > 300.0f) continue;
                        bool tallEnough = false;
                        for (const auto& ob : other.GetWorldBounds()) if (ob.max.y > e.position.y + 5.0f) { tallEnough = true; break; }
                        if (tallEnough && CheckCollisionLists(e.GetWorldBounds(), other.GetWorldBounds())) { collided = true; break; }
                    }
                }
                if (collided) { e.position.x = oldX; e.velocity.x = 0.0f; }
            }
            
            if (abs(e.velocity.z) > 0.1f) {
                float oldZ = e.position.z; e.position.z += e.velocity.z * dt;
                bool collided = false;
                for (auto& other : entities) {
                    if (e.HasTag(TAG_BOULDER) && other.name.find("stand") != std::string::npos) continue; 
                    if (&e != &other && other.isSolid && !other.isGrabbed && e.attachedTo == -1) {
                        if (fabs(e.position.x - other.position.x) > 300.0f || fabs(e.position.z - other.position.z) > 300.0f) continue;
                        bool tallEnough = false;
                        for (const auto& ob : other.GetWorldBounds()) if (ob.max.y > e.position.y + 5.0f) { tallEnough = true; break; }
                        if (tallEnough && CheckCollisionLists(e.GetWorldBounds(), other.GetWorldBounds())) { collided = true; break; }
                    }
                }
                if (collided) { e.position.z = oldZ; e.velocity.z = 0.0f; }
            }
        } // End Physics Phase

        // --- PHASE C: CONTINUOUS HAZARD LOGIC & REACTIVATION ---
        
        if (e.HasTag(TAG_PANDORA) && e.isGlitching) {
            int sealCount = 0;
            for (size_t j = 0; j < entities.size(); ++j) {
                if (entities[j].attachedTo == (int)i) sealCount++;
                else if (!entities[j].isGrabbed && entities[j].attachedTo == -1 && CheckCollisionLists(e.GetWorldInteractBounds(), entities[j].GetWorldInteractBounds())) {
                    if (entities[j].HasTag(TAG_MJOLNIR) || entities[j].HasTag(TAG_GLEIPNIR) || entities[j].HasTag(TAG_TAPE)) {
                        entities[j].attachedTo = i; sealCount++;
                    }
                }
            }
            if (sealCount >= 2) { e.isGlitching = false; isPandoraOpen = false; }
        }

        // DYNAMIC EVALUATION: Medusa
        if (e.HasTag(TAG_MEDUSA)) {
            bool activeTime = isPlaying && ((currentNight < 5) && ((currentNight == 1 && shiftTimer >= 5.0f) || currentNight > 1));
            if (activeTime && e.position.y > -50.0f) {
                if (isPandoraOpen) {
                    e.isGlitching = false;
                } else {
                    bool hasGlasses = false;
                    for (const auto& other : entities) {
                        if (other.attachedTo == (int)i && other.HasTag(TAG_EYEWEAR) && !other.HasTag(TAG_BROKEN)) {
                            hasGlasses = true; break;
                        }
                    }
                    if (e.HasTag(TAG_BROKEN) || hasGlasses) e.isGlitching = false;
                    else e.isGlitching = true; 
                }
            } else if (e.position.y <= -50.0f) {
                e.isGlitching = false;
            }
        }

        // DYNAMIC EVALUATION: Aiolus Wind Bag
        if (e.HasTag(TAG_WIND_BAG)) {
            bool activeTime = isPlaying && ((currentNight < 5) && ((currentNight == 2 && shiftTimer >= 90.0f) || currentNight > 2));            
            if (activeTime && e.position.y > -50.0f) {
                if (isPandoraOpen) {
                    e.isGlitching = false;
                } else {
                    bool hasCork = false;

                    // Check for cork and handle Tape Application
                    for (auto& item : entities) {
                        if (item.HasTag(TAG_CORK) && !item.isGrabbed && Vector3Distance(e.position, item.position) < 60.0f) hasCork = true;
                        if (item.HasTag(TAG_TAPE) && item.isUsing && item.stateValue > 0.0f && Vector3Distance(e.position, item.position) < 150.0f) {
                            if (!hasCork && !e.HasTag(TAG_BROKEN)) { e.AddTag(TAG_BROKEN); item.stateTimer -= 1.0f; item.isUsing = false; }
                        }
                    }

                    if (hasCork || e.HasTag(TAG_BROKEN)) e.isGlitching = false;
                    else e.isGlitching = true; 
                }
            } else if (e.position.y <= -50.0f) {
                e.isGlitching = false;
            }
            
            // Push Force
            if (e.isGlitching) { 
                if (!IsSoundPlaying(sounds["sound-of-wind"])) PlaySound(sounds["sound-of-wind"]);
                float dist = Vector3Distance(e.position, player.position);
                if (dist < 600.0f && dist > 10.0f) {
                    Vector3 pushDir = Vector3Normalize(Vector3Subtract(player.position, e.position));
                    float force = (600.0f - dist) * 8.0f;; 
                    player.velocity.x += pushDir.x * force * dt;
                    player.velocity.z += pushDir.z * force * dt;
                }
            } else {
                if (IsSoundPlaying(sounds["sound-of-wind"])) StopSound(sounds["sound-of-wind"]);
            }
        }

        // DYNAMIC EVALUATION: Water Source
        if (e.HasTag(TAG_WATER_SOURCE)) {
            bool activeTime = isPlaying && ((currentNight < 5) && ((currentNight == 1 && shiftTimer >= 90.0f) || currentNight > 1));            
            if (activeTime && e.position.y > -50.0f) {
                if (isPandoraOpen || e.isStone) {
                    e.isGlitching = false;
                } else {
                    bool hasCork = false;

                    for (auto& item : entities) {
                        if (item.HasTag(TAG_CORK) && !item.isGrabbed && Vector3Distance(e.position, item.position) < 60.0f) hasCork = true;
                        if (item.HasTag(TAG_TAPE) && item.isUsing && item.stateTimer > 0.0f && Vector3Distance(e.position, item.position) < 150.0f) {
                            if (!hasCork && !e.HasTag(TAG_BROKEN)) { e.AddTag(TAG_BROKEN); item.stateTimer -= 1.0f; item.isUsing = false; }
                        }
                    }

                    if (hasCork || e.HasTag(TAG_BROKEN)) e.isGlitching = false;
                    else e.isGlitching = true;
                }
            } else if (e.position.y <= -50.0f) {
                e.isGlitching = false;
            }
            
            if (e.isGlitching) {
                if (!IsSoundPlaying(sounds["poseidon-water"])) PlaySound(sounds["poseidon-water"]);
                float maxRadius = 9999.0f;
                for (const auto& sp : entities) if (sp.HasTag(TAG_SPONGE) && !sp.isGrabbed && sp.position.y < 20.0f) { 
                    float dist = Vector3Distance(e.position, sp.position); 
                    if (dist - 40.0f < maxRadius) maxRadius = dist - 40.0f; 
                }
                if (e.stateValue < maxRadius) { e.stateValue += 40.0f * dt; if (e.stateValue > maxRadius) e.stateValue = maxRadius; }
            } else {
                if (IsSoundPlaying(sounds["poseidon-water"])) StopSound(sounds["poseidon-water"]);
            }
            if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_MOP) && entities[grabbedEntityIndex].isUsing) {
                if (Vector3Distance(player.position, e.position) < e.stateValue + 100.0f) { e.stateValue -= 400.0f * dt; if (e.stateValue < 0) e.stateValue = 0; }
            }
        }

        // DYNAMIC EVALUATION: Sandals
        if (e.HasTag(TAG_SANDALS)) {
            bool activeTime = isPlaying && ((currentNight < 5) && ((currentNight == 2 && shiftTimer >= 30.0f) || currentNight > 2));            
            if (activeTime && e.position.y > -50.0f) {

                // Handle Tape Application
                for (size_t j = 0; j < entities.size(); ++j) {
                    auto& item = entities[j];
                    if (item.HasTag(TAG_TAPE) && item.isUsing && item.stateValue > 0.0f && Vector3Distance(e.position, item.position) < 150.0f) {
                        if (!e.HasTag(TAG_BROKEN)) { e.AddTag(TAG_BROKEN); item.stateValue -= 1.0f; item.isUsing = false; }
                    }
                }

                // e.isStone will be triggered seamlessly if Medusa shoots them!
                if (isPandoraOpen || e.stateTimer == -999.0f || e.isStone || e.HasTag(TAG_BROKEN)) {
                    e.isGlitching = false;
                } else {
                    e.isGlitching = true;
                }
            } else if (e.position.y <= -50.0f) {
                e.isGlitching = false;
            }

            if (e.isGrabbed && e.isGlitching) {
                e.stateTimer = 5.0f; if (IsSoundPlaying(sounds["wing-flap"])) StopSound(sounds["wing-flap"]);
            } 
            else if (e.isGlitching && !e.isStone) {
                if (!IsSoundPlaying(sounds["wing-flap"])) PlaySound(sounds["wing-flap"]);
                if (e.stateTimer > 0.0f) {
                    e.stateTimer -= dt; 
                } else {
                    e.attachedTo = -1; 
                    float timeSec = (float)GetTime();
                    e.position.y = 70.0f + sinf(timeSec * 4.0f) * 15.0f;
                    e.velocity.x = cosf(timeSec * 0.5f) * 500.0f;
                    e.velocity.z = sinf(timeSec * 0.35f) * 500.0f;
                }
            } else {
                if (IsSoundPlaying(sounds["wing-flap"])) StopSound(sounds["wing-flap"]);
            }
            
            if (e.isGlitching && e.position.y < 20.0f) {
                for (const auto& w : entities) {
                    if (w.HasTag(TAG_WATER_SOURCE) && w.stateValue > 0.0f && Vector3Distance(e.position, w.position) < w.stateValue) {
                        if (!isPandoraOpen) {
                            e.isGlitching = false;
                            e.stateTimer = -999.0f; // Permanently ruined
                        }
                    }
                }
            }
        }

        // DYNAMIC EVALUATION: Boulder (PROXIMITY FIX - No more physics jitter!)
        if (e.HasTag(TAG_BOULDER)) {
            bool activeTime = isPlaying && ((currentNight < 5) && ((currentNight == 2 && shiftTimer >= 0.1f) || currentNight > 2));            if (activeTime && e.position.y > -50.0f) {
                bool isBlocked = false;
                float rollDir = -1.0f; 
                
                for (const auto& other : entities) {
                    if (other.HasTag(TAG_SANDBAG) && !other.isGrabbed && other.position.y < 20.0f) {
                        float distX = fabs(e.position.x - other.position.x);
                        float distZ = fabs(e.position.z - other.position.z);
                        if (distX < 120.0f && distZ < 100.0f) {
                            if ((rollDir > 0 && other.position.x > e.position.x) || 
                                (rollDir < 0 && other.position.x < e.position.x)) {
                                isBlocked = true;
                                break;
                            }
                        }
                    }
                }

                if (isPandoraOpen || e.HasTag(TAG_BROKEN) || isBlocked) e.isGlitching = false;
                else e.isGlitching = true;
            } else if (e.position.y <= -50.0f) {
                e.isGlitching = false;
            }

            if (e.isGlitching) {
                float rollDir = -1.0f; 
                e.velocity.x += 120.0f * rollDir * dt; 

                if (CheckCollisionLists(e.GetWorldBounds(), player.GetWorldBounds())) {
                    PlaySound(sounds["breaking-vase"]);
                    if (player.position.x > e.position.x) { 
                        player.position.x = e.position.x + 80.0f; 
                        if (player.velocity.x < -10.0f) e.velocity.x = -200.0f; 
                        else e.velocity.x = player.velocity.x; 
                    } else {
                        player.position.x = e.position.x - 80.0f;
                        if (player.velocity.x > 10.0f) e.velocity.x = 200.0f; 
                        else e.velocity.x = player.velocity.x; 
                    }
                } 

                for (auto& other : entities) {
                    if (&e != &other && (other.HasTag(TAG_FRAGILE) || other.HasTag(TAG_MEDUSA))) {
                        if (CheckCollisionLists(e.GetWorldInteractBounds(), other.GetWorldInteractBounds()) && !other.HasTag(TAG_BROKEN)) {
                            other.AddTag(TAG_BROKEN); 
                            other.color = DARKGRAY; 
                            if (!other.boundsList.empty()) other.boundsList[0].max.y = 5.0f; 
                            if (!isPandoraOpen) other.isGlitching = false; 
                            other.attachedTo = -1; 
                            other.velocity.x = 400.0f * rollDir; 
                            other.velocity.y = 300.0f;           
                        }
                    }
                }
            } 
        }

       // DYNAMIC EVALUATION: Sun Disk
        if (e.HasTag(TAG_SUN_DISK)) {
            bool activeTime = isPlaying && ((currentNight < 5) && ((currentNight == 3 && shiftTimer >= 90.0f) || currentNight > 3));            if (activeTime && e.position.y > -50.0f) {
                if (isPandoraOpen || (e.color.r == BLACK.r && e.color.g == BLACK.g && e.color.b == BLACK.b)) {
                    e.isGlitching = false;
                } else {
                    e.isGlitching = true;
                }
            } else if (e.position.y <= -50.0f) {
                e.isGlitching = false;
            }

            if (e.isGlitching) {
                e.color = ORANGE; e.canGrab = false; 
                
                // 1. ROTATION LOGIC
                // e.stateValue holds the current angle in degrees. It spins 45 degrees per second!
                e.stateValue += 45.0f * dt; 
                if (e.stateValue >= 360.0f) e.stateValue -= 360.0f;

                // --- NEW FIX: Beams instantly activate without the 3-second timer! ---
                visuals.drawingSunBeams = true;
                visuals.sunCenter = e.position;
                visuals.sunAngle = e.stateValue;

                // 2. DEADLY BEAM COLLISION
                Vector2 pPos = {player.position.x, player.position.z};
                Vector2 sPos = {e.position.x, e.position.z};
                
                // Check all 4 directions (0, 90, 180, 270 degrees from current rotation)
                for (int j = 0; j < 4; j++) {
                    float angleRad = (e.stateValue + j * 90.0f) * DEG2RAD;
                    Vector2 beamDir = {cos(angleRad), sin(angleRad)};
                    
                    // Check if player is in the beam (Length: 800 units, Width: 60 units)
                    Vector2 toPlayer = {pPos.x - sPos.x, pPos.y - sPos.y};
                    float projection = toPlayer.x * beamDir.x + toPlayer.y * beamDir.y; 
                    
                    if (projection > 0.0f && projection < 800.0f) { // If player is in front of this specific beam
                        float perpDist = fabs(toPlayer.x * (-beamDir.y) + toPlayer.y * beamDir.x); // Distance from center line
                        if (perpDist < 30.0f) { // If within the 60 unit width
                            player.isDead = true; 
                        }
                    }
                }

                // 3. THE PERMANENT FIX (Water)
                for (auto& w : entities) {
                    if (w.HasTag(TAG_WATER_SOURCE) && w.stateValue > 0.0f && Vector3Distance(e.position, w.position) < w.stateValue) {
                        if (!isPandoraOpen) {
                            e.isGlitching = false; 
                            e.color = BLACK;     // Fixes it permanently!
                        }
                        break;
                    }
                }
                
                // (Optional) Melts bubble wrap instantly
                for (auto& other : entities) {
                    if (&e != &other && other.HasTag(TAG_BUBBLE_WRAP) && !other.isGrabbed && CheckCollisionLists(e.GetWorldBounds(), other.GetWorldBounds())) {
                        other.position = {-9999, -9999, -9999}; other.isSolid = false;
                    }
                }
            } else { 
                e.color = BLACK; e.canGrab = true; 
            }
        }

        
        // DYNAMIC EVALUATION: Mummy
        if (e.HasTag(TAG_MUMMY)) {
            bool activeTime = isPlaying && ((currentNight < 5) && ((currentNight == 3 && shiftTimer >= 0.1f) || currentNight > 3));            if (activeTime && e.position.y > -50.0f) {
                for (size_t j = 0; j < entities.size(); ++j) {
                    auto& item = entities[j];
                    if (item.HasTag(TAG_TAPE) && item.isUsing && item.stateTimer > 0.0f && Vector3Distance(e.position, item.position) < 150.0f) {
                        if (!e.HasTag(TAG_BROKEN)) { 
                            e.AddTag(TAG_BROKEN); 
                            e.color = LIGHTGRAY;       
                            e.velocity = {0, 0, 0};    
                            item.stateTimer -= 1.0f;   
                            item.isUsing = false;      
                        }
                    }
                }

                bool isTiedUpByGleipnir = (e.color.r == WHITE.r && e.color.g == WHITE.g && e.color.b == WHITE.b);
                if (isPandoraOpen || e.HasTag(TAG_BROKEN) || e.isStone || isTiedUpByGleipnir) e.isGlitching = false;
                else e.isGlitching = true;
            } else if (e.position.y <= -50.0f) {
                e.isGlitching = false; 
            }

            // 3. Mummy AI Behavior: Advanced Right-Angle Waypoints
            if (e.isGlitching && !e.isGrabbed) {
                Entity* targetFusebox = nullptr;
                int targetFuseboxIdx = -1;
                
                for (size_t k = 0; k < entities.size(); ++k) {
                    if (entities[k].HasTag(TAG_FUSEBOX) || entities[k].name == "Fuse Box") { 
                        targetFusebox = &entities[k]; 
                        targetFuseboxIdx = (int)k;
                        break; 
                    }
                }
                
                if (targetFusebox) {
                    // --- NEW FIX: Check if Lightning is guarding the Fusebox ---
                    bool fuseboxDefended = false;
                    for (const auto& item : entities) {
                        if (item.attachedTo == targetFuseboxIdx && item.HasTag(TAG_LIGHTNING)) {
                            fuseboxDefended = true; break;
                        }
                    }

                    // EXACT 8-step route to avoid the Pyramid, Fences, AND Brochure 3!
                    static const Vector3 waypoints[8] = {
                        { 2350.0f, 0.0f, -1140.0f }, // 0: Start (Sarcophagus)
                        { 2350.0f, 0.0f, -850.0f },  // 1: Move out of the corner
                        { 2180.0f, 0.0f, -850.0f },  // 2: Move Left (dodge pyramid & brochure)
                        { 2180.0f, 0.0f, -200.0f },  // 3: Move Forward (past pyramid safely)
                        { 2340.0f, 0.0f, -200.0f },  // 4: Move Right (align with Door 3)
                        { 2340.0f, 0.0f, 100.0f },   // 5: Move Forward (through Door 3)
                        { 2050.0f, 0.0f, 100.0f },   // 6: Move Left (align with Fusebox)
                        { 2050.0f, 0.0f, 10.0f }     // 7: The Fusebox!
                    };

                    int numWP = 8;
                    int currentWP = (int)e.stateTimer;
                    if (currentWP < 0) { currentWP = 0; e.stateTimer = 0.0f; }
                    if (currentWP >= numWP) { currentWP = numWP - 1; e.stateTimer = (float)(numWP - 1); }

                    // --- NEW FIX: Mummy retreats if the Fusebox is defended! ---
                    bool isLightOn = (targetFusebox->stateValue > 0.5f) && !fuseboxDefended;
                    int targetIndex = currentWP;

                    // If light is ON (and unprotected), advance through the waypoints
                    if (isLightOn) {
                        targetIndex = currentWP + 1;
                        if (targetIndex >= numWP) targetIndex = numWP - 1; 
                        
                        // Increased tolerance to 50 so it smoothly triggers the next waypoint
                        if (Vector3Distance(e.position, waypoints[targetIndex]) < 50.0f) {
                            if (currentWP < numWP - 1) e.stateTimer += 1.0f; 
                        }
                    } 
                    // If light is OFF (or protected by lightning), walk backward through the waypoints
                    else {
                        targetIndex = currentWP; 
                        if (Vector3Distance(e.position, waypoints[targetIndex]) < 50.0f) {
                            if (currentWP > 0) e.stateTimer -= 1.0f;
                        }
                    }

                    Vector3 targetPos = waypoints[targetIndex];
                    Vector3 diff = { targetPos.x - e.position.x, 0.0f, targetPos.z - e.position.z };
                    
                    // --- NEW FIX: Increased hit-tolerance to 40.0f so it doesn't get stuck on hitboxes! ---
                    if (Vector3Length(diff) > 40.0f) { 
                        Vector3 dir = Vector3Normalize(diff);
                        e.facingDir = dir;
                        e.stateValue = atan2(e.facingDir.x, e.facingDir.z) * RAD2DEG; 
                        e.velocity.x += dir.x * 1200.0f * dt; 
                        e.velocity.z += dir.z * 1200.0f * dt;
                    } else {
                        // Action at the end of the line
                        if (!isLightOn && targetIndex == 0) {
                            e.stateValue = 270.0f; // Lay back down
                            e.velocity = {0,0,0};  // Stop walking
                        }
                        if (isLightOn && targetIndex == numWP - 1) targetFusebox->stateValue = 0.0f; // Smash it!
                    }
                }
            }
        }

        // DYNAMIC EVALUATION: Zeus
        if (e.HasTag(TAG_ZEUS)) {
            bool activeTime = isPlaying && ((currentNight < 5) && ((currentNight == 3 && shiftTimer >= 0.1f) || currentNight > 3));            if (activeTime && e.position.y > -50.0f) {
                if (isPandoraOpen) e.isGlitching = false;
                else e.isGlitching = true;
            } else if (e.position.y <= -50.0f) {
                e.isGlitching = false;
            }
        }

        // DYNAMIC EVALUATION: Mjolnir
        if (e.HasTag(TAG_MJOLNIR)) {
            bool activeTime = isPlaying && ((currentNight < 5) && ((currentNight == 4 && shiftTimer >= 0.1f) || currentNight > 4));            
            // --- NEW FIX: Check if Hermes Sandals are attached to the hammer! ---
            bool hasSandals = false;
            for (const auto& item : entities) {
                if (item.HasTag(TAG_SANDALS) && item.attachedTo == (int)i) {
                    hasSandals = true; break;
                }
            }

            if (activeTime && e.position.y > -50.0f) {
                // If it's active time, Mjolnir gets heavy and breaks the pedestal!
                if (e.attachedTo != -1 && entities[e.attachedTo].name.find("stand") != std::string::npos) {
                    e.attachedTo = -1; // Crash to the floor!
                    
                }
                
                if (isPandoraOpen || hasSandals) {
                    e.isGlitching = false;
                } else {
                    e.isGlitching = true; 
                }
            } else if (e.position.y <= -50.0f) {
                e.isGlitching = false;
            }

            // --- NEW FIX: Hovering & Grab Logic ---
            if (hasSandals) {
                e.canGrab = true; // Floating, so mortals can push/grab it easily!
                
                // Only hover if you aren't currently holding it
                if (e.attachedTo == -1 && !e.isGrabbed) {
                    float timeSec = (float)GetTime();
                    e.position.y = 40.0f + sinf(timeSec * 3.0f + e.position.x) * 10.0f; 
                    e.velocity.y = 0.0f; 
                }
            } else {
                e.canGrab = false; // Only the worthy can lift it without sandals!
            }
        }
        
        
        // DYNAMIC EVALUATION: Banshee Stone
        if (e.HasTag(TAG_BANSHEE_STONE)) {
            bool activeTime = isPlaying && ((currentNight < 5) && ((currentNight == 4 && shiftTimer >= 0.1f) || currentNight > 4));            if (activeTime && e.position.y > -50.0f) {
                if (isPandoraOpen) e.isGlitching = false;
                else e.isGlitching = true;
            } else if (e.position.y <= -50.0f) {
                e.isGlitching = false;
            }

            

            if (e.isGlitching && !e.isStone) {
                // Start the screaming sound if it isn't already playing!
                if (!IsSoundPlaying(sounds["scream"])) {
                    PlaySound(sounds["scream"]);
                }

                e.stateValue += ((currentNight == 4) ? 17.77f : 400.0f) * dt; 
                if (e.stateValue > 800.0f) {
                    if (currentNight == 4) {
                        int effect = GetRandomValue(0, 2);
                        if (effect == 0) {
                            for(auto& t: entities) if(t.HasTag(TAG_SUN_DISK)) { t.color = ORANGE; t.canGrab = false; }
                        } else if (effect == 1) {
                            for(auto& t: entities) if(t.HasTag(TAG_MUMMY)) { t.RemoveTag(TAG_BROKEN); t.color = DARKGRAY; }
                        } else {
                            for(auto& t: entities) if(t.HasTag(TAG_SANDALS)) { t.stateTimer = 0.0f; }
                        }
                    } else {
                        for (auto& target : entities) if (&target != &e && !target.isStatic && !target.isGrabbed && target.name != "Player" && target.position.y > -50.0f) {
                            if (Vector3Distance(e.position, target.position) < 800.0f) target.isGlitching = true;
                        }
                    }
                    e.stateValue = 0.0f; 
                }
            } else {
                // Immediately cut the scream off if petrified, suppressed by Pandora, or thrown in a hole
                if (IsSoundPlaying(sounds["scream"])) {
                    StopSound(sounds["scream"]);
                }
            }
        }

        // DYNAMIC EVALUATION: Gleipnir's Ribbon
        if (e.HasTag(TAG_GLEIPNIR)) {
            // 1. TIMELINE ACTIVATION (Night 4 @ 90s, or Night 5)
            bool activeTime = isPlaying && ((currentNight < 5) && ((currentNight == 4 && shiftTimer >= 90.0f) || currentNight > 4));            
            if (activeTime && e.position.y > -50.0f) {
                // Stay dormant if Pandora is open, if you've beaten it 3 times, or if it's petrified
                if (isPandoraOpen || gleipnirDefeats >= 3 || e.isStone) {
                    e.isGlitching = false;
                } else {
                    e.isGlitching = true;
                }
            } else if (e.position.y <= -50.0f) {
                e.isGlitching = false;
            }

            // Always allow grabbing if it has been petrified or successfully deactivated
            if (e.isStone || !e.isGlitching) {
                e.canGrab = true;
                e.isUsing = false; // Cancel QTE if petrified mid-attack
            }

            // 2. HUNTING & QTE LOGIC
            if (e.isGlitching && !e.isStone) {
                // --- QTE ACTIVE STATE (STRUGGLING WITH PLAYER) ---
                if (e.isUsing) {
                    e.position = player.position; 
                    e.position.y += 60.0f; // Wrap around the upper body
                    e.velocity = {0,0,0};
                    e.canGrab = false;

                    e.stateTimer -= dt; // 5-second countdown
                    
                    if (IsKeyPressed(KEY_SPACE)) {
                        e.stateValue += 1.0f; // Count spacebar mashes
                    }

                    visuals.qteActive = true;
                    visuals.qteProgress = e.stateValue / 20.0f;
                    visuals.qteTimeLeft = std::max(0.0f, e.stateTimer);

                    if (e.stateTimer <= 0.0f) {
                        player.isDead = true; // Choked to death!
                    } 
                    else if (e.stateValue >= 20.0f) {
                        // QTE WON!
                        gleipnirDefeats++;
                        e.isUsing = false;

                        if (gleipnirDefeats >= 3) {
                            e.isGlitching = false; // Permanently dead
                            e.canGrab = true;
                            e.position.x += player.facingDir.x * 100.0f;
                            e.position.z += player.facingDir.z * 100.0f;
                        } else {
                            // Throw it back and stun it for 2 seconds
                            e.position.x += player.facingDir.x * 400.0f;
                            e.position.z += player.facingDir.z * 400.0f;
                            e.stateTimer = 2.0f; // Repurpose timer for stun duration
                        }
                    }
                } 
                // --- FREE ROAMING & HUNTING STATE ---
                else {
                    // Stun recovery (after being thrown off)
                    if (e.stateTimer > 0.0f) {
                        e.stateTimer -= dt;
                        e.velocity.x *= 0.9f; e.velocity.z *= 0.9f; // Slide to a halt
                    } 
                    else {
                        // Break free from display stands
                        if (e.attachedTo != -1 && entities[e.attachedTo].name.find("stand") != std::string::npos) {
                            e.attachedTo = -1; e.position.y = 5.0f; 
                        }

                        if (e.attachedTo == -1) {
                            e.canGrab = false;   
                            e.position.y = 5.0f; 

                            Vector3 targetPos = player.position;
                            float closestDist = Vector3Distance(player.position, e.position);

                            // Scan the room for Sandbags or Glitching Mummies
                            for (size_t j = 1; j < entities.size(); ++j) {
                                auto& other = entities[j];
                                if (&e != &other && (other.HasTag(TAG_SANDBAG) || (other.HasTag(TAG_MUMMY) && other.isGlitching))) {
                                    if (!other.isGrabbed) { 
                                        float d = Vector3Distance(other.position, e.position);
                                        if (d < closestDist) { closestDist = d; targetPos = other.position; }
                                    }
                                }
                            }

                            // Slither towards the closest target
                            if (closestDist > 10.0f) { 
                                Vector3 dir = { (targetPos.x - e.position.x) / closestDist, 0.0f, (targetPos.z - e.position.z) / closestDist }; 
                                Vector3 perp = { -dir.z, 0.0f, dir.x }; 
                                float timeSec = (float)GetTime();
                                float wiggle = sinf(timeSec * 8.0f) * 350.0f; 
                                e.velocity.x = dir.x * 250.0f + perp.x * wiggle; 
                                e.velocity.z = dir.z * 250.0f + perp.z * wiggle;
                            }

                            // COLLISION LOGIC
                            for (size_t j = 0; j < entities.size(); ++j) {
                                auto& other = entities[j];
                                if (&e != &other && !other.isGrabbed && e.attachedTo == -1 && CheckCollisionLists(e.GetWorldBounds(), other.GetWorldBounds())) {
                                    if (other.HasTag(TAG_SANDBAG)) {
                                        e.attachedTo = j; if (!isPandoraOpen) e.isGlitching = false; e.position.y = other.position.y + 10.0f; break; 
                                    }
                                    if (other.HasTag(TAG_MUMMY) && other.isGlitching) {
                                        e.attachedTo = j; 
                                        if (!isPandoraOpen) { e.isGlitching = false; other.isGlitching = false; other.color = WHITE; }
                                        e.position.y = other.position.y + 30.0f; break; 
                                    }
                                    
                                    // --- TRIGGER QTE ---
                                    if (other.name == "Player") { 
                                        e.isUsing = true; 
                                        e.stateTimer = 5.0f; // 5 seconds to live
                                        e.stateValue = 0.0f; // 0 spacebar mashes
                                    } 
                                }
                            }
                        }
                    }
                }
            }
        }
        // Hole functionality (Black Hole)
        if (e.HasTag(TAG_HOLE)) {
            for (auto& target : entities) {
                if (&target != &e && CheckCollisionLists(e.GetWorldBounds(), target.GetWorldBounds())) {
                    if (target.HasTag(TAG_WATER_SOURCE)) { if (!isPandoraOpen) target.isGlitching = false; }
                    if (target.HasTag(TAG_BOULDER) && target.isGlitching) {
                        if (!isPandoraOpen) target.isGlitching = false; 
                        target.velocity = {0,0,0}; target.position = e.position; target.position.y = -20.0f; 
                    }
                    if (target.HasTag(TAG_MJOLNIR)) { 
                        if (!isPandoraOpen) target.isGlitching = false; 
                        // --- NEW FIX: Teleport it down into the hole! ---
                        target.velocity = {0,0,0}; target.position = e.position; target.position.y = -40.0f;
                        }                    
                    if (target.HasTag(TAG_MUMMY) && target.isGlitching) {
                        if (!isPandoraOpen) target.isGlitching = false; 
                        target.velocity = {0,0,0}; target.position = e.position; target.position.y = -30.0f; 
                    }
                    if (target.HasTag(TAG_BANSHEE_STONE) && target.isGlitching) {
                        if (!isPandoraOpen) target.isGlitching = false; 
                        target.velocity = {0,0,0}; target.position = e.position; target.position.y = -25.0f; 
                    }
                }
            }
        }
    } // END ENTITY LOOP

    // ---------------------------------------------------------
    // 3. POST-LOOP HAZARDS (Beams, Extinguishers, Electric)
    // ---------------------------------------------------------
    bool playerInWater = false;
    for (const auto& w : entities) if (w.HasTag(TAG_WATER_SOURCE) && w.stateValue > 0.0f && Vector3Distance(player.position, w.position) < w.stateValue) playerInWater = true;
    if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_ELECTRIC) && !entities[grabbedEntityIndex].isStone) {
        if (equippedGloves == -1 || playerInWater) player.isDead = true; 
    }

    for (int i = 0; i < entities.size(); ++i) {
        Entity& e = entities[i];
        
        if (e.HasTag(TAG_MEDUSA) && e.isGlitching) {
            visuals.drawingBeam = true; 
            visuals.beamP1 = e.position; 
            visuals.beamP2 = { e.position.x - 300.0f, e.position.y, e.position.z + 600.0f }; 
            visuals.beamP3 = { e.position.x + 300.0f, e.position.y, e.position.z + 600.0f };
            Vector2 bp1 = {visuals.beamP1.x, visuals.beamP1.z}, bp2 = {visuals.beamP2.x, visuals.beamP2.z}, bp3 = {visuals.beamP3.x, visuals.beamP3.z};
            
            for (auto& target : entities) {
                if (&target != &e && !target.isStone && (target.canGrab || target.name == "Player")) {
                    if (CheckCollisionPointTriangle({target.position.x, target.position.z}, bp1, bp2, bp3)) {
                        bool immune = (target.name == "Player" && equippedEyewear != -1 && !entities[equippedEyewear].HasTag(TAG_BROKEN));
                        if (!immune) { 
                            if (!target.isStone) PlaySound(sounds["petrification"]);
                            target.isStone = true; target.color = GRAY; target.isGlitching = false; 
                            target.AddTag(TAG_HEAVY); target.velocity.y = -500.0f; 
                        }            
                    }
                }
            }
        }
        
        if (e.isUsing && e.HasTag(TAG_EXTINGUISHER)) {
            visuals.drawingExtinguisher = true; 
            Vector3 dir = player.facingDir, perp = { -dir.z, 0.0f, dir.x }; 
            visuals.extP1 = e.position; 
            visuals.extP2 = { e.position.x + dir.x * 400.0f + perp.x * 200.0f, e.position.y, e.position.z + dir.z * 400.0f + perp.z * 200.0f }; 
            visuals.extP3 = { e.position.x + dir.x * 400.0f - perp.x * 200.0f, e.position.y, e.position.z + dir.z * 400.0f - perp.z * 200.0f };
            
            player.velocity.x -= dir.x * 1500.0f * dt; player.velocity.z -= dir.z * 1500.0f * dt;
            Vector2 ep1 = {visuals.extP1.x, visuals.extP1.z}, ep2 = {visuals.extP2.x, visuals.extP2.z}, ep3 = {visuals.extP3.x, visuals.extP3.z};
            
            for (auto& target : entities) {
                if (&target != &player && CheckCollisionPointTriangle({target.position.x, target.position.z}, ep1, ep2, ep3)) {
                    if (target.HasTag(TAG_MJOLNIR)) { target.velocity.x += dir.x * 4000.0f * dt; target.velocity.z += dir.z * 4000.0f * dt; target.stateTimer = 1.5f; }
                    // --- NEW FIX: Permanently extinguish the Sun Disk! ---
                    if (target.HasTag(TAG_SUN_DISK) && target.isGlitching && !isPandoraOpen) { 
                        target.isGlitching = false; 
                        target.color = BLACK; 
                        target.stateTimer = 0.0f; 
                    }                    if (target.HasTag(TAG_WATER_SOURCE) && target.stateValue > 0) { target.isStone = true; if (!isPandoraOpen) target.isGlitching = false; } 
                    if (target.canGrab && !target.HasTag(TAG_HEAVY)) { target.velocity.x += dir.x * 2500.0f * dt; target.velocity.z += dir.z * 2500.0f * dt; }
                    if (target.HasTag(TAG_PANDORA) && target.isGlitching) target.stateTimer = 15.0f;
                }
            }
        }

        if (e.HasTag(TAG_ZEUS) && e.isGlitching) {
            if (Vector3Distance(player.position, e.position) < 150.0f && equippedGloves == -1) player.isDead = true;
            for (auto& puddle : entities) {
                if (puddle.HasTag(TAG_WATER_SOURCE) && puddle.stateValue > 0.0f) {
                    if (Vector3Distance(e.position, puddle.position) < puddle.stateValue && Vector3Distance(player.position, puddle.position) < puddle.stateValue && equippedGloves == -1) {
                        if (!player.isDead) PlaySound(sounds["electric_shock"]);
                        player.isDead = true;
                    }
                }
            }
        }
    }

    return visuals;
}

// ====================================================================================
// --- NIGHT INITIALIZATION ---
// ====================================================================================
inline void SetupNightHazards(int currentNight, std::vector<Entity>& entities) {
    for (auto& e : entities) {
        e.isGlitching = false;
        
        // Setup Lights to start fully bright
        if (e.HasTag(TAG_LIGHTSWITCH) || e.HasTag(TAG_FUSEBOX)) { 
            e.stateValue = 1.0f; 
            e.stateTimer = 1.0f; 
        }
        
        if (e.HasTag(TAG_LIGHTNING)) {
            if (currentNight < 3) { e.RemoveTag(TAG_ELECTRIC); e.canGrab = false; }
            else { e.AddTag(TAG_ELECTRIC); e.canGrab = true; } // <--- Added canGrab = true!
        }
    }

    if (currentNight >= 4) {
        for (auto& e : entities) if (e.HasTag(TAG_MJOLNIR)) e.position.y = 0.0f; 
    }

    // SNAPPING INITIALIZATION
    for (size_t i = 0; i < entities.size(); ++i) {
        
        // --- NEW FIX: Add TAG_ZEUS to the parent snapping check ---
        if (entities[i].name.find("stand") != std::string::npos || 
            entities[i].name.find("artifactsign") != std::string::npos ||
            entities[i].HasTag(TAG_ZEUS)) {
            
            bool occupied = false;
            for (const auto& e : entities) if (e.attachedTo == (int)i) { occupied = true; break; }

            if (!occupied) {
                for (size_t j = 1; j < entities.size(); ++j) { 
                    if (i == j || entities[j].attachedTo != -1 || entities[j].isStatic || entities[j].name.find("stand") != std::string::npos) continue;

                    // --- NEW FIX: Strict Lightning Bolt Rules ---
                    // 1. Zeus ONLY auto-snaps the Lightning Bolt
                    if (entities[j].HasTag(TAG_LIGHTNING) && !entities[i].HasTag(TAG_ZEUS)) continue;
                    // 2. If the parent is Zeus, he CANNOT attach anything except Lightning.
                    if (entities[i].HasTag(TAG_ZEUS) && !entities[j].HasTag(TAG_LIGHTNING)) continue;

                    if (currentNight >= 4 && entities[j].HasTag(TAG_MJOLNIR) && entities[i].name.find("stand") != std::string::npos) continue;

                    bool isArtifact = false;
                    for (Tag t : entities[j].tags) {
                        if (t == TAG_TAPE || t == TAG_MEDUSA || t == TAG_CORK || t == TAG_WATER_SOURCE || 
                            t == TAG_EXTINGUISHER || t == TAG_GLOVES || t == TAG_MOP || t == TAG_SANDBAG || 
                            t == TAG_SPONGE || t == TAG_ELECTRIC || t == TAG_SANDALS || t == TAG_WIND_BAG || 
                            t == TAG_WET_SIGN || t == TAG_SUN_DISK || t == TAG_MJOLNIR || t == TAG_GLEIPNIR || 
                            t == TAG_BANSHEE_STONE || t == TAG_BUBBLE_WRAP || t == TAG_HOLE_SAW || 
                            t == TAG_FLASHLIGHT || t == TAG_MAT || t == TAG_HEAT_SOURCE || t == TAG_LIGHTNING || 
                            t == TAG_PANDORA || t == TAG_HANDBOOK || t == TAG_EYEWEAR || t == TAG_FRAGILE || t == TAG_BOULDER) {
                            isArtifact = true; break;
                        }
                    }
                    if (!isArtifact) continue;

                    float distXZ = Vector2Distance({entities[i].position.x, entities[i].position.z}, {entities[j].position.x, entities[j].position.z});
                    float yDiff = entities[j].position.y - entities[i].position.y;

                    // --- NEW FIX: Expanded Snap Radius for Zeus ---
                    float snapDistXZ = entities[i].HasTag(TAG_ZEUS) ? 1250.0f : 80.0f;
                    float snapDistY  = entities[i].HasTag(TAG_ZEUS) ? 1300.0f : 150.0f;

                    if (distXZ < snapDistXZ && yDiff > -20.0f && yDiff < snapDistY) {
                        entities[j].attachedTo = i; break; 
                    }
                }
            }
        }
    }
}