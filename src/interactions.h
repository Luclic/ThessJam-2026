#pragma once
#include "Entities.h"
#include <iostream>

inline void ProcessImpact(Entity& e) {
    if (e.HasTag(TAG_FRAGILE) && !e.HasTag(TAG_BROKEN)) {
        e.AddTag(TAG_BROKEN);
        e.color = DARKGRAY; 
        e.zHeight = 5.0f;   
        std::cout << e.name << " shattered!" << std::endl;
    }
}

// Processes Tool + Target interactions. 
inline ChemResult ProcessChemistry(int toolIdx, int targetIdx, std::vector<Entity>& entities) {
    Entity& tool = entities[toolIdx];
    Entity& target = entities[targetIdx];

    // 1. TAPE + BROKEN = REPAIR
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_BROKEN)) {
        target.RemoveTag(TAG_BROKEN);
        if (target.HasTag(TAG_EYEWEAR)) { target.color = BLACK; } 
        else { target.color = YELLOW; target.zHeight = 30.0f; }
        return CHEM_USED_BUT_KEPT; 
    }

    // 2. SUNGLASSES + MEDUSA = MERGE & BLOCK BEAM
    if (tool.HasTag(TAG_EYEWEAR) && target.HasTag(TAG_MEDUSA)) {
        tool.attachedTo = targetIdx; // Merge!
        return CHEM_ATTACHED;
    }

    // 3. TAPE + HOLY CUP = TEMPORARY FIX
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_WATER_SOURCE)) {
        target.isGlitching = false;
        target.stateTimer = 15.0f; // Tape holds for 15 seconds
        return CHEM_USED_BUT_KEPT;
    }

    // 4. CORK + HOLY CUP = PERMANENT FIX
    if (tool.HasTag(TAG_CORK) && target.HasTag(TAG_WATER_SOURCE)) {
        target.isGlitching = false;
        target.stateTimer = 0.0f; // Stop the tape-breaking timer if it was running
        tool.attachedTo = targetIdx; // Merge!
        return CHEM_ATTACHED;
    }

    return CHEM_NONE;
}