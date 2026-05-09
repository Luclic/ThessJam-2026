#pragma once
#include "Entities.h"
#include <iostream>

inline void ProcessImpact(Entity& e) {
    if (e.HasTag(TAG_FRAGILE) && !e.HasTag(TAG_BROKEN)) {
        e.AddTag(TAG_BROKEN); e.color = DARKGRAY; e.zHeight = 5.0f;   
    }
}

inline ChemResult CanProcessChemistry(int toolIdx, int targetIdx, const std::vector<Entity>& entities) {
    const Entity& tool = entities[toolIdx];
    const Entity& target = entities[targetIdx];

    // Tool Empty Check!
    if ((tool.HasTag(TAG_TAPE) || tool.HasTag(TAG_BUBBLE_WRAP)) && tool.stateValue <= 0) return CHEM_NONE;

    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_BROKEN)) return CHEM_USED_BUT_KEPT; 
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_SANDALS)) return CHEM_USED_BUT_KEPT; 
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_MUMMY)) return CHEM_USED_BUT_KEPT; 
    if (tool.HasTag(TAG_EYEWEAR) && target.HasTag(TAG_MEDUSA)) return CHEM_ATTACHED;
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_WATER_SOURCE)) return CHEM_USED_BUT_KEPT;
    if (tool.HasTag(TAG_CORK) && target.HasTag(TAG_WATER_SOURCE)) return CHEM_ATTACHED;
    if (tool.HasTag(TAG_SPHINX_NOSE) && target.HasTag(TAG_SPHINX)) return CHEM_ATTACHED; 
    if (tool.HasTag(TAG_SANDALS) && target.HasTag(TAG_MJOLNIR)) return CHEM_ATTACHED; 
    if (tool.HasTag(TAG_BUBBLE_WRAP) && target.HasTag(TAG_BANSHEE_STONE)) return CHEM_ATTACHED; 
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_BANSHEE_STONE)) {
        for(const auto& e : entities) if(e.attachedTo == targetIdx && e.HasTag(TAG_BUBBLE_WRAP)) return CHEM_USED_BUT_KEPT;
    }
    
    // NEW: Hole Covering
    if (tool.HasTag(TAG_WET_SIGN) && target.HasTag(TAG_HOLE)) return CHEM_ATTACHED; // Drops sign on hole
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_HOLE)) return CHEM_USED_BUT_KEPT; // Tape web over hole

    return CHEM_NONE;
}

inline ChemResult ProcessChemistry(int toolIdx, int targetIdx, std::vector<Entity>& entities) {
    Entity& tool = entities[toolIdx];
    Entity& target = entities[targetIdx];

    if (tool.HasTag(TAG_TAPE)) tool.stateValue -= 1.0f; // Consume 1 Tape Use!
    if (tool.HasTag(TAG_BUBBLE_WRAP)) tool.stateValue -= 1.0f; // Consume 1 Wrap Use!

    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_BROKEN)) {
        target.RemoveTag(TAG_BROKEN);
        if (target.HasTag(TAG_EYEWEAR)) { target.color = BLACK; target.zHeight = 10.0f; } else { target.color = YELLOW; target.zHeight = 30.0f; }
        return CHEM_USED_BUT_KEPT; 
    }
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_SANDALS)) { target.isGlitching = false; target.stateTimer = 0.0f; return CHEM_USED_BUT_KEPT; }
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_MUMMY)) { target.isGlitching = false; target.color = WHITE; return CHEM_USED_BUT_KEPT; } 
    if (tool.HasTag(TAG_EYEWEAR) && target.HasTag(TAG_MEDUSA)) { tool.attachedTo = targetIdx; return CHEM_ATTACHED; }
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_WATER_SOURCE)) { target.isGlitching = false; target.stateTimer = 15.0f; return CHEM_USED_BUT_KEPT; }
    if (tool.HasTag(TAG_CORK) && target.HasTag(TAG_WATER_SOURCE)) { target.isGlitching = false; target.stateTimer = 0.0f; tool.attachedTo = targetIdx; return CHEM_ATTACHED; }
    if (tool.HasTag(TAG_SPHINX_NOSE) && target.HasTag(TAG_SPHINX)) { tool.attachedTo = targetIdx; return CHEM_ATTACHED; } 
    if (tool.HasTag(TAG_SANDALS) && target.HasTag(TAG_MJOLNIR)) { tool.attachedTo = targetIdx; target.canGrab = true; target.isGlitching = false; target.zHeight += 40.0f; return CHEM_ATTACHED; }
    if (tool.HasTag(TAG_BUBBLE_WRAP) && target.HasTag(TAG_BANSHEE_STONE)) { tool.attachedTo = targetIdx; return CHEM_ATTACHED; }
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_BANSHEE_STONE)) { target.isGlitching = false; return CHEM_USED_BUT_KEPT; } 

    // NEW: Hole Covering
    if (tool.HasTag(TAG_WET_SIGN) && target.HasTag(TAG_HOLE)) { tool.attachedTo = targetIdx; return CHEM_ATTACHED; }
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_HOLE)) { target.stateValue = 1.0f; return CHEM_USED_BUT_KEPT; } // 1.0f means taped up!

    return CHEM_NONE;
}