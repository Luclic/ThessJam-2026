#pragma once
#include "Entities.h"
#include <vector>


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
        else if (e.name == "Sandal") { e.AddTag(TAG_SANDALS); e.canGrab = true; e.canThrow = true; } // Name already matches
        else if (e.name == "Banshee Stone") { e.AddTag(TAG_BANSHEE_STONE); e.canGrab = true; e.canThrow = true; e.name = "rocks"; }
        else if (e.name == "Gleipnir Ribbon") { e.AddTag(TAG_GLEIPNIR); e.name = "Gleipnir"; } // Placeholder

        // --- EGYPTIAN ROOM ---
        else if (e.name == "Sun Disk") { e.AddTag(TAG_SUN_DISK); e.AddTag(TAG_HEAT_SOURCE); e.canGrab = false; e.name = "Coin"; }
        else if (e.name == "Anubis") { e.AddTag(TAG_MUMMY); e.name = "Anubis Statue"; }
        else if (e.name == "Mummy") { e.AddTag(TAG_MUMMY); } // Placeholder
        else if (e.name == "Sphinx") { e.AddTag(TAG_SPHINX); } // Placeholder
        else if (e.name == "Sphinx Nose") { e.AddTag(TAG_SPHINX_NOSE); e.canGrab = true; } // Placeholder

        // --- GREEK ROOM ---
        else if (e.name == "Medusa") { e.AddTag(TAG_MEDUSA); } // Placeholder
        else if (e.name == "Zeus Statue") { e.AddTag(TAG_ZEUS); e.AddTag(TAG_ELECTRIC); } // Placeholder
        else if (e.name == "Display Vase") { e.AddTag(TAG_FRAGILE); e.canGrab = true; e.canThrow = true; e.name = "Tall Vase"; }
        else if (e.name == "Chalice") { e.AddTag(TAG_WATER_SOURCE); e.canGrab = true; e.canThrow = true; } // Water leak trigger!
        else if (e.name == "Sisyphus Boulder") { e.AddTag(TAG_BOULDER); e.AddTag(TAG_HEAVY); e.name = "Rock"; }
        else if (e.name == "Aeolus Bag") { e.AddTag(TAG_WIND_BAG); e.canGrab = true; e.canThrow = true; e.name = "Coin Pouch"; }

        // --- TOOLS & JANITOR ARSENAL ---
        else if (e.name == "Extinguisher") { e.AddTag(TAG_EXTINGUISHER); e.canGrab = true; e.canThrow = true; e.name = "Fire Extinguisher"; }
        else if (e.name == "Flashlight") { e.AddTag(TAG_FLASHLIGHT); e.canGrab = true; e.name = "Time Hotel 7.07"; }
        else if (e.name == "Sunglasses") { e.AddTag(TAG_EYEWEAR); e.canGrab = true; }
        else if (e.name == "Pixel Glasses") { e.AddTag(TAG_EYEWEAR); e.canGrab = true; e.name = "Pixel Sunglasses"; }
        else if (e.name == "Glove") { e.AddTag(TAG_GLOVES); e.canGrab = true; }
        else if (e.name == "Painters Tape") { e.AddTag(TAG_TAPE); e.stateValue = 10.0f; e.canGrab = true; e.name = "Time Hotel 5.25 Painters Tape"; }
        else if (e.name == "Sponge") { e.AddTag(TAG_SPONGE); e.canGrab = true; }
        else if (e.name == "Bag") { e.AddTag(TAG_SANDBAG); e.AddTag(TAG_HEAVY); e.canGrab = true; e.name = "Bag"; }
        else if (e.name == "Broom") { e.AddTag(TAG_MOP); e.canGrab = true; }
        else if (e.name == "Saw") { e.AddTag(TAG_HOLE_SAW); e.canGrab = true; }
        else if (e.name == "Cardboard Boxes") { e.AddTag(TAG_BUBBLE_WRAP); e.stateValue = 3.0f; e.canGrab = true; }
        else if (e.name == "Giant Cork") { e.AddTag(TAG_CORK); e.canGrab = true; e.name = "Cork"; }
        
        // --- SIGNS (Legal Excuses) ---
        else if (e.name == "Sign 1") { e.AddTag(TAG_WET_SIGN); e.canGrab = true; e.name = "sign1"; }
        else if (e.name == "Sign 2") { e.AddTag(TAG_WET_SIGN); e.canGrab = true; e.name = "sign2"; }
        else if (e.name == "Sign 3") { e.AddTag(TAG_WET_SIGN); e.canGrab = true; e.name = "sign3"; }
        else if (e.name == "Artifact Sign 1") { e.AddTag(TAG_WET_SIGN); e.canGrab = true; e.name = "artifactsign1"; }
        else if (e.name == "Artifact Sign 2") { e.AddTag(TAG_WET_SIGN); e.canGrab = true; e.name = "artifactsign2"; }
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
    
    return CHEM_NONE;
}

// --- 4. APPLY CHEMISTRY ---
inline ChemResult ProcessChemistry(int toolIdx, int targetIdx, std::vector<Entity>& entities) {
    Entity& tool = entities[toolIdx]; Entity& target = entities[targetIdx];
    
    // Duct tape fixing fragile props
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_BROKEN)) {
        tool.stateValue -= 1.0f;
        target.RemoveTag(TAG_BROKEN);
        if (target.HasTag(TAG_FRAGILE)) { target.color = ORANGE; if (!target.boundsList.empty()) target.boundsList[0].max.y = 30.0f; }
        if (target.HasTag(TAG_MEDUSA)) { target.color = GREEN; if (!target.boundsList.empty()) target.boundsList[0].max.y = 30.0f; }
        if (tool.stateValue <= 0) { tool.position = {-9999, -9999, -9999}; tool.isSolid = false; tool.canGrab = false; }
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
    
    // Taping Broken Eyewear
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_EYEWEAR) && target.HasTag(TAG_BROKEN)) {
        tool.stateValue -= 1.0f; target.RemoveTag(TAG_BROKEN);
        if (target.HasTag(TAG_EYEWEAR)) { target.color = BLACK; if (!target.boundsList.empty()) target.boundsList[0].max.y = 10.0f; } 
        else { target.color = YELLOW; if (!target.boundsList.empty()) target.boundsList[0].max.y = 30.0f; }
        if (tool.stateValue <= 0) { tool.position = {-9999, -9999, -9999}; tool.isSolid = false; tool.canGrab = false; }
        return CHEM_USED_BUT_KEPT;
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