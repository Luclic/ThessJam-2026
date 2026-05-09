#pragma once
#include "Entities.h"
#include <iostream>

inline void ProcessImpact(Entity& e) {
    if (e.HasTag(TAG_FRAGILE) && !e.HasTag(TAG_BROKEN)) {
        e.AddTag(TAG_BROKEN); e.color = DARKGRAY; e.zHeight = 5.0f;   
    }
}

// NEW: Pure check without side-effects for the UI Menu
inline ChemResult CanProcessChemistry(int toolIdx, int targetIdx, const std::vector<Entity>& entities) {
    const Entity& tool = entities[toolIdx];
    const Entity& target = entities[targetIdx];

    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_BROKEN)) return CHEM_USED_BUT_KEPT; 
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_SANDALS)) return CHEM_USED_BUT_KEPT; 
    if (tool.HasTag(TAG_EYEWEAR) && target.HasTag(TAG_MEDUSA)) return CHEM_ATTACHED;
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_WATER_SOURCE)) return CHEM_USED_BUT_KEPT;
    if (tool.HasTag(TAG_CORK) && target.HasTag(TAG_WATER_SOURCE)) return CHEM_ATTACHED;
    return CHEM_NONE;
}

// Executes the actual change
inline ChemResult ProcessChemistry(int toolIdx, int targetIdx, std::vector<Entity>& entities) {
    Entity& tool = entities[toolIdx];
    Entity& target = entities[targetIdx];

    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_BROKEN)) {
        target.RemoveTag(TAG_BROKEN);
        if (target.HasTag(TAG_EYEWEAR)) { target.color = BLACK; target.zHeight = 10.0f; } 
        else { target.color = YELLOW; target.zHeight = 30.0f; }
        return CHEM_USED_BUT_KEPT; 
    }
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_SANDALS)) {
        target.isGlitching = false; target.stateTimer = 0.0f; return CHEM_USED_BUT_KEPT; 
    }
    if (tool.HasTag(TAG_EYEWEAR) && target.HasTag(TAG_MEDUSA)) {
        tool.attachedTo = targetIdx; return CHEM_ATTACHED;
    }
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_WATER_SOURCE)) {
        target.isGlitching = false; target.stateTimer = 15.0f; return CHEM_USED_BUT_KEPT;
    }
    if (tool.HasTag(TAG_CORK) && target.HasTag(TAG_WATER_SOURCE)) {
        target.isGlitching = false; target.stateTimer = 0.0f; tool.attachedTo = targetIdx; return CHEM_ATTACHED;
    }
    return CHEM_NONE;
}
