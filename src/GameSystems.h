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