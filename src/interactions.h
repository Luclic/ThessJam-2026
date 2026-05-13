#pragma once
#include "Entities.h"
#include <vector>
#include <string>

enum ChemResult { CHEM_NONE, CHEM_ATTACHED, CHEM_USED_BUT_KEPT, CHEM_FIXED };

inline void AssignEntityRules(std::vector<Entity>& entities) {
    for (auto& e : entities) {
        // Quality of Life: Items on the floor are not solid so we don't trip on them
        if (e.canGrab) e.isSolid = false; 

        // --- DOORS ---
        if (e.name == "Door 1 (Greek 1)") { e.AddTag(TAG_DOOR_1); e.name = "wall4"; }
        else if (e.name == "Door 2 (Greek 2)") { e.AddTag(TAG_DOOR_2); e.name = "wall4"; }
        else if (e.name == "Door 3 (Egyptian)") { e.AddTag(TAG_DOOR_3); e.name = "wall4"; }
        else if (e.name == "Door 4 (Nordic)") { e.AddTag(TAG_DOOR_4); e.name = "wall4"; }
        else if (e.name == "Door 5 (Boss)") { e.AddTag(TAG_DOOR_5); e.name = "wall4"; }

        // --- NORDIC ROOM ---
        else if (e.name == "Mjolnir") { e.AddTag(TAG_MJOLNIR); e.AddTag(TAG_HEAVY); e.canGrab = false; e.name = "mjolner"; }
        else if (e.name == "Sandal") { e.AddTag(TAG_SANDALS); e.canGrab = true; e.canThrow = true; e.name = "Sandal"; } 
        else if (e.name == "Banshee Stone") { e.AddTag(TAG_BANSHEE_STONE); e.canGrab = true; e.canThrow = true; e.name = "rocks"; }
        else if (e.name == "Gleipnir Ribbon") { e.AddTag(TAG_GLEIPNIR); e.canGrab = true; e.canThrow = true; e.name = "Snake"; } 

        // --- EGYPTIAN ROOM ---
        else if (e.name == "Sun Disk") { e.AddTag(TAG_SUN_DISK); e.AddTag(TAG_HEAT_SOURCE); e.canGrab = false; e.name = "Coin"; }
        else if (e.name == "Anubis" || e.name == "Anubis Statue") { e.name = "Anubis Statue"; } // Matches both names!
        else if (e.name == "Mummy") { e.AddTag(TAG_MUMMY); e.name = "mummy"; } 
        else if (e.name == "Sphinx") { e.AddTag(TAG_SPHINX); } 
        else if (e.name == "Sphinx Nose") { e.AddTag(TAG_SPHINX_NOSE); e.canGrab = true; } 
        else if (e.name == "Coffin") { e.name = "Coffin"; }

        // --- GREEK ROOM ---
        else if (e.name == "Medusa") { e.AddTag(TAG_MEDUSA); e.name = "medusa"; } 
        else if (e.name == "Zeus Statue") { e.AddTag(TAG_ZEUS); e.isStatic = true; e.canGrab = false; e.isSolid = true; e.name = "zeus"; } 
// --- NEW FIX: Force alpha to 255 so the graphics engine doesn't skip it! ---
        else if (e.name == "lightning") { 
            e.AddTag(TAG_LIGHTNING); 
            e.canGrab = true; 
            e.name = "lightning"; 
            e.color.a = 255; // Force 100% opacity
        }        else if (e.name == "Display Vase") { e.AddTag(TAG_FRAGILE); e.canGrab = true; e.canThrow = true; e.name = "Tall Vase"; }
        else if (e.name == "Chalice") { e.AddTag(TAG_WATER_SOURCE); e.canGrab = false; e.canThrow = true; e.name = "Chalice"; } 
        else if (e.name == "Sisyphus Boulder") { e.AddTag(TAG_BOULDER); e.AddTag(TAG_HEAVY); e.name = "Rock"; }
        else if (e.name == "Aeolus Bag") { e.AddTag(TAG_WIND_BAG); e.canGrab = true; e.canThrow = true; e.name = "Coin Pouch"; }

        // --- BOSS ROOM ---
        else if (e.name == "Pandora's Box" || e.name == "PandoraBox" || e.name == "pandora") { 
            e.AddTag(TAG_PANDORA); 
            e.canGrab = false; 
            e.isSolid = true; 
            e.name = "pandora";
        }

        // --- TOOLS & JANITOR ARSENAL ---
        else if (e.name == "Fire Extinguisher") { e.AddTag(TAG_EXTINGUISHER); e.canGrab = true; e.canThrow = true; e.name = "Fire Extinguisher"; }
        else if (e.name == "Flashlight") { e.AddTag(TAG_FLASHLIGHT); e.canGrab = true; e.name = "Time Hotel 7.07"; }
        else if (e.name == "Sunglasses") { e.AddTag(TAG_EYEWEAR); e.canGrab = true; e.name = "Sunglasses"; }
        else if (e.name == "Pixel Sunglasses") { e.AddTag(TAG_EYEWEAR); e.canGrab = true; e.name = "Pixel Sunglasses"; }
        else if (e.name == "Glove") { e.AddTag(TAG_GLOVES); e.canGrab = true; e.name = "Glove"; }
        else if (e.name == "Painters Tape"||e.name == "Time Hotel 5.25 Painters Tape") { e.AddTag(TAG_TAPE); e.stateTimer = 3.0f; e.canGrab = true; e.name = "Painters Tape"; }
        else if (e.name == "Sponge") { e.AddTag(TAG_SPONGE); e.canGrab = true; e.name = "Sponge"; }
        else if (e.name == "Bag") { e.AddTag(TAG_SANDBAG); e.AddTag(TAG_HEAVY); e.canGrab = true; e.name = "Bag"; }
        else if (e.name == "Broom" || e.name == "Mop") { 
            e.AddTag(TAG_MOP); 
            e.canGrab = true; 
            e.name = "Broom"; 
            if (!e.interactBoundsList.empty()) {
                e.interactBoundsList[0].min.x -= 30.0f; e.interactBoundsList[0].max.x += 30.0f;
                e.interactBoundsList[0].min.z -= 30.0f; e.interactBoundsList[0].max.z += 30.0f;
                e.interactBoundsList[0].max.y += 50.0f; 
            }
        }
        else if (e.name == "Saw") { e.AddTag(TAG_HOLE_SAW); e.stateValue = 3.0f; e.canGrab = true; e.name = "Saw"; }
        else if (e.name == "Cardboard Boxes") { e.AddTag(TAG_BUBBLE_WRAP); e.stateValue = 3.0f; e.canGrab = true; e.name = "Cardboard Boxes"; }
        else if (e.name == "Giant Cork") { e.AddTag(TAG_CORK); e.canGrab = true; e.name = "Cork"; }

        // --- READABLES (YOUR EXACT CODE + ID FIX) ---
        else if (e.name == "Employee Handbook" || e.name == "Handbook") { 
            e.AddTag(TAG_HANDBOOK); 
            e.canGrab = true; 
            e.name = "Open Book"; // Maps to Open Book.glb
        }
        else if (e.name.find("Brochure") != std::string::npos || e.name == "Magazine") { 
            e.AddTag(TAG_HANDBOOK); 
            e.canGrab = true; 
            
            // --- THE INITIALIZATION FIX ---
            // Store the ID in stateTimer so we don't destroy the Y-Rotation (stateValue)!
            if (e.name.find("1") != std::string::npos) e.stateTimer = 1.0f;
            else if (e.name.find("2") != std::string::npos) e.stateTimer = 2.0f;
            else if (e.name.find("3") != std::string::npos) e.stateTimer = 3.0f;
            else if (e.name.find("4") != std::string::npos) e.stateTimer = 4.0f;
            else if (e.name.find("5") != std::string::npos) e.stateTimer = 5.0f;
            else {
                // If the Map Editor exported a generic "Magazine", auto-assign it a number 1-5!
                static float autoID = 1.0f;
                e.stateTimer = autoID;
                autoID += 1.0f; if (autoID > 5.0f) autoID = 1.0f;
            }

            e.name = "Magazine"; // Mapped to Magazine.glb
            
            // Bulletproof fallback for the thin book shape
            if (e.boundsList.empty()) {
                e.boundsList.push_back({{-15, 0, -20}, {15, 5, 20}});
                e.interactBoundsList.push_back({{-15, 0, -20}, {15, 5, 20}});
            }
        }
        
        // --- SIGNS (Legal Excuses) ---
        else if (e.name == "Sign 1") { e.AddTag(TAG_WET_SIGN); e.canGrab = false; e.name = "sign1"; }
        else if (e.name == "Sign 2") { e.AddTag(TAG_WET_SIGN); e.canGrab = false; e.name = "sign2"; }
        else if (e.name == "Sign 3") { e.AddTag(TAG_WET_SIGN); e.canGrab = false; e.name = "sign3"; }
        else if (e.name == "Artifact Sign 1") { e.AddTag(TAG_WET_SIGN); e.canGrab = false; e.name = "artifactsign1"; }
        else if (e.name == "Artifact Sign 2") { e.AddTag(TAG_WET_SIGN); e.canGrab = false; e.name = "artifactsign2"; }
        
        // --- EXHAUSTIVE DECORATIONS & ARCHITECTURE MAPPING ---
        // This explicitly maps any UI names or weird exports to the exact lowercase/underscore .glb name!
        else if (e.name == "Wall (Press C)") e.name = "wall1";
        else if (e.name == "Wall Corner (Press C)") e.name = "wall1corner";
        else if (e.name == "Floor A (Press C)") e.name = "floor11";
        else if (e.name == "Floor B (Press C)") e.name = "floor12";
        else if (e.name == "Arch (Press C)") e.name = "arch1";
        else if (e.name == "ArchArch (Press C)") e.name = "archarch1";
        else if (e.name == "Pedestal 1") { e.name = "stand1"; e.isStatic = true; }
        else if (e.name == "Pedestal 2") { e.name = "stand2"; e.isStatic = true; }
        else if (e.name == "Pedestal 3") { e.name = "stand3"; e.isStatic = true; }
        else if (e.name == "Pedestal 4") { e.name = "stand4"; e.isStatic = true; }
        else if (e.name == "Bench 1") { e.name = "bench1"; e.isStatic = true;}
        else if (e.name == "Bench 2") { e.name = "bench2"; e.isStatic = true;}
        else if (e.name == "Bench 3" || e.name == "bench3") { e.name = "bench3"; e.isStatic = true;}
        else if (e.name == "Desk") {e.name = "desk"; e.isSolid = true;}
        else if (e.name == "Glass Case") e.name = "glasscase";
        else if (e.name == "Pillar 1") e.name = "pillar1";
        else if (e.name == "Pillar 2" || e.name == "pillar2") e.name = "pillar2";
        else if (e.name == "Rope 1") e.name = "rope1";
        else if (e.name == "Rope 2") e.name = "rope2";
        else if (e.name == "Fence" || e.name == "fence") e.name = "fence";
        else if (e.name == "Light 1") e.name = "light1";
        else if (e.name == "Light 2") e.name = "light2";
        else if (e.name == "Painting 1" || e.name == "painting1") e.name = "painting1";
        else if (e.name == "Painting 2" || e.name == "painting2") e.name = "painting2";
        else if (e.name == "Painting Light") e.name = "paintinglight";
        else if (e.name == "Time Hotel Sign") e.name = "Time Hotel 7.07";
        else if (e.name == "Ticket Stand") { e.name = "ticketstand"; e.isStatic = true;}
        else if (e.name == "Ticket Seat") e.name = "ticketstandseat";
        else if (e.name == "Modern Art Sticker") e.name = "Sticker";
        else if (e.name == "Info Button") { e.isSolid = true; e.name = "Info Button"; }

        // --- NEW FINAL MODELS ---
        else if (e.name == "Sarcophagus") { 
            e.name = "sarcophagus"; 
            e.AddTag(TAG_SARCOPHAGUS); // Adding the tag you already have in Entities.h!
            e.isSolid = true; 
        }
        else if (e.name == "Viking Boat") { 
            e.name = "Viking Boat"; 
            e.isSolid = true; 
            e.isStatic = true; 
        }
        else if (e.name == "Shield Round") { 
            e.name = "Shield Round"; 
            e.isSolid = true; 
            e.isStatic = true; 
        }

        else if (e.name == "Fuse Box") { 
            e.name = "Fuse Box"; 
            e.AddTag(TAG_FUSEBOX);
            e.isSolid = true; 
            e.isStatic = true; 
        }
        
        // --- THE ULTIMATE BULLETPROOF HITBOX FALLBACK ---
        // If an item was exported with NO hitboxes, it becomes invisible to physics and falls through the floor!
        // This forces a default 40x40 hitbox so NOTHING ever goes missing again.
        if (e.boundsList.empty()) {
            e.boundsList.push_back({{-20, 0, -20}, {20, 40, 20}});
        }
        if (e.interactBoundsList.empty()) {
            e.interactBoundsList = e.boundsList;
        }
    }
}

// --- 2. IMPACT LOGIC ---
inline void ProcessImpact(Entity& e) {
    if (e.HasTag(TAG_FRAGILE) && !e.HasTag(TAG_BROKEN)) {
        e.AddTag(TAG_BROKEN); e.color = DARKGRAY; 
        if (!e.boundsList.empty()) e.boundsList[0].max.y = 5.0f; // SAFETY CHECK
    }
}

// --- 3. CHEMISTRY PRE-CHECK ---
inline ChemResult CanProcessChemistry(int toolIdx, int targetIdx, const std::vector<Entity>& entities) {
    const Entity& tool = entities[toolIdx]; const Entity& target = entities[targetIdx];
    
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_BROKEN)) return CHEM_USED_BUT_KEPT;
    if (tool.HasTag(TAG_SPONGE) && target.HasTag(TAG_SPHINX_NOSE) && target.color.r == GOLD.r) return CHEM_USED_BUT_KEPT;
    if (tool.HasTag(TAG_SPHINX_NOSE) && target.HasTag(TAG_SPHINX)) return CHEM_ATTACHED;
    if (tool.HasTag(TAG_GLOVES) && target.HasTag(TAG_ELECTRIC)) return CHEM_ATTACHED;
    if (tool.HasTag(TAG_CORK) && target.HasTag(TAG_WIND_BAG) && target.isGlitching) return CHEM_ATTACHED;
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_EYEWEAR) && target.HasTag(TAG_BROKEN)) return CHEM_USED_BUT_KEPT;
    if (tool.HasTag(TAG_SANDALS) && target.HasTag(TAG_MJOLNIR)) return CHEM_ATTACHED;
    
    // New GDD Synergies
    if (tool.HasTag(TAG_EXTINGUISHER) && target.HasTag(TAG_SUN_DISK) && target.isGlitching) return CHEM_FIXED;
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_SANDALS)) return CHEM_USED_BUT_KEPT; // Tape the flying shoes
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_MUMMY)) return CHEM_USED_BUT_KEPT;   // Wrap the mummy
    if (tool.HasTag(TAG_CORK) && target.HasTag(TAG_WATER_SOURCE)) return CHEM_ATTACHED; // Plug the trident puddle
    if (tool.HasTag(TAG_EYEWEAR) && target.HasTag(TAG_MEDUSA)) return CHEM_ATTACHED;
    if (tool.HasTag(TAG_HANDBOOK) && target.name.find("artifactsign") != std::string::npos) return CHEM_ATTACHED;
    // ZEUS SOCKET LOGIC: Can put Lightning OR any Petrified item into Zeus's hand
    if (target.HasTag(TAG_ZEUS) && (tool.HasTag(TAG_LIGHTNING) || tool.isStone)) return CHEM_ATTACHED;
    if (tool.HasTag(TAG_LIGHTNING) && target.HasTag(TAG_FUSEBOX)) return CHEM_ATTACHED;
    
    return CHEM_NONE;
}

// --- 4. APPLY CHEMISTRY ---
inline ChemResult ProcessChemistry(int toolIdx, int targetIdx, std::vector<Entity>& entities) {
    Entity& tool = entities[toolIdx]; Entity& target = entities[targetIdx];
    
    // Duct tape fixing fragile props
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_BROKEN)) {
        tool.stateTimer -= 1.0f;
        target.RemoveTag(TAG_BROKEN);
        if (target.HasTag(TAG_FRAGILE)) { target.color = ORANGE; if (!target.boundsList.empty()) target.boundsList[0].max.y = 30.0f; }
        if (target.HasTag(TAG_MEDUSA)) { target.color = GREEN; if (!target.boundsList.empty()) target.boundsList[0].max.y = 30.0f; }
        if (tool.stateTimer <= 0) { tool.position = {-9999, -9999, -9999}; tool.isSolid = false; tool.canGrab = true; }
        return CHEM_USED_BUT_KEPT;
    }
    
    

    // Sponge cleaning Sphinx Nose
    if (tool.HasTag(TAG_SPONGE) && target.HasTag(TAG_SPHINX_NOSE)) {
        target.color = BEIGE; return CHEM_USED_BUT_KEPT;
    }
    
    // Attaching Sphinx Nose
    if (tool.HasTag(TAG_SPHINX_NOSE) && target.HasTag(TAG_SPHINX) && tool.color.r == BEIGE.r) {
        tool.attachedTo = targetIdx; tool.color = GOLD; return CHEM_ATTACHED;
    }
    
    // Electrician Gloves on Zeus/Sparks
    if (tool.HasTag(TAG_GLOVES) && target.HasTag(TAG_ELECTRIC)) {
        tool.attachedTo = targetIdx; target.canGrab = true; return CHEM_ATTACHED;
    }
    
    // Corking the Wind Bag
    if (tool.HasTag(TAG_CORK) && target.HasTag(TAG_WIND_BAG) && target.isGlitching) {
        tool.attachedTo = targetIdx; target.isGlitching = false; return CHEM_ATTACHED;
    }
    
    // --- NEW FIX: Put Sunglasses on Medusa ---
    if (tool.HasTag(TAG_EYEWEAR) && target.HasTag(TAG_MEDUSA)) {
        tool.attachedTo = targetIdx; 
        target.isGlitching = false; // Cures the glitching immediately!
        
        // Offset the glasses slightly higher so they sit on her face/head
        if (!target.boundsList.empty()) {
            tool.position.y = target.position.y + target.boundsList[0].max.y;
        }
        return CHEM_ATTACHED;
    }

    // Tape the Sandals
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_SANDALS)) {
        tool.stateValue -= 1.0f; 
        target.isGlitching = false; 
        target.velocity = {0,0,0};
        target.stateTimer = -999.0f; // Permanently prevents them from flying again!
        if (tool.stateValue <= 0) { tool.position = {-9999, -9999, -9999}; tool.isSolid = false; tool.canGrab = true; }
        return CHEM_USED_BUT_KEPT;
    }
    
    // Tape the Mummy
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_MUMMY)) {
        tool.stateValue -= 1.0f; target.isGlitching = false; target.velocity = {0,0,0};
        if (tool.stateValue <= 0) { tool.position = {-9999, -9999, -9999}; tool.isSolid = false; tool.canGrab = true; }
        return CHEM_USED_BUT_KEPT;
    }

    // Cork the Water Source (Trident)
    if (tool.HasTag(TAG_CORK) && target.HasTag(TAG_WATER_SOURCE)) {
        tool.attachedTo = targetIdx; target.isGlitching = false; return CHEM_ATTACHED;
    }

    if (tool.HasTag(TAG_HANDBOOK) && target.name.find("artifactsign") != std::string::npos) {
        tool.attachedTo = targetIdx; 
        return CHEM_ATTACHED;
    }

    // Give item to Zeus
    if (target.HasTag(TAG_ZEUS) && (tool.HasTag(TAG_LIGHTNING) || tool.isStone)) {
        tool.attachedTo = targetIdx; 
        tool.position = { target.position.x - 40.0f, target.position.y + 120.0f, target.position.z }; // Place in hand
        return CHEM_ATTACHED;
    }

    if (tool.HasTag(TAG_LIGHTNING) && target.HasTag(TAG_FUSEBOX)) {
        tool.attachedTo = targetIdx; 
        return CHEM_ATTACHED;
    }
    
    // Hovering Mjolnir with Sandals
    if (tool.HasTag(TAG_SANDALS) && target.HasTag(TAG_MJOLNIR)) { 
        tool.attachedTo = targetIdx; target.canGrab = true; target.isGlitching = false; 
        if (!target.boundsList.empty()) target.boundsList[0].max.y += 40.0f; 
        return CHEM_ATTACHED; 
    }
    
    // Cooling the Sun Disk with Extinguisher
    if (tool.HasTag(TAG_EXTINGUISHER) && target.HasTag(TAG_SUN_DISK) && target.isGlitching) {
        target.isGlitching = false;
        target.color = BLACK;     // Cooled obsidian
        target.canGrab = true;    // Safe to touch
        return CHEM_FIXED; 
    }
    
    return CHEM_NONE;
}