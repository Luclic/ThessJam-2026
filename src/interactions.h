#pragma once
#include "Entities.h"
#include <vector>

inline void ProcessImpact(Entity& e) {
    if (e.HasTag(TAG_FRAGILE) && !e.HasTag(TAG_BROKEN)) {
        e.AddTag(TAG_BROKEN); e.color = DARKGRAY; 
        if (!e.boundsList.empty()) e.boundsList[0].max.y = 5.0f; // SAFETY CHECK
    }
}

inline ChemResult CanProcessChemistry(int toolIdx, int targetIdx, const std::vector<Entity>& entities) {
    const Entity& tool = entities[toolIdx]; const Entity& target = entities[targetIdx];
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_BROKEN)) return CHEM_USED_BUT_KEPT;
    if (tool.HasTag(TAG_SPONGE) && target.HasTag(TAG_SPHINX_NOSE) && target.color.r == GOLD.r) return CHEM_USED_BUT_KEPT;
    if (tool.HasTag(TAG_SPHINX_NOSE) && target.HasTag(TAG_SPHINX)) return CHEM_ATTACHED;
    if (tool.HasTag(TAG_GLOVES) && target.HasTag(TAG_ELECTRIC)) return CHEM_ATTACHED;
    if (tool.HasTag(TAG_CORK) && target.HasTag(TAG_WIND_BAG) && target.isGlitching) return CHEM_ATTACHED;
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_EYEWEAR) && target.HasTag(TAG_BROKEN)) return CHEM_USED_BUT_KEPT;
    if (tool.HasTag(TAG_SANDALS) && target.HasTag(TAG_MJOLNIR)) return CHEM_ATTACHED;
    return CHEM_NONE;
}

inline ChemResult ProcessChemistry(int toolIdx, int targetIdx, std::vector<Entity>& entities) {
    Entity& tool = entities[toolIdx]; Entity& target = entities[targetIdx];
    
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_BROKEN)) {
        tool.stateValue -= 1.0f;
        target.RemoveTag(TAG_BROKEN);
        if (target.HasTag(TAG_FRAGILE)) { target.color = ORANGE; if (!target.boundsList.empty()) target.boundsList[0].max.y = 30.0f; }
        if (target.HasTag(TAG_MEDUSA)) { target.color = GREEN; if (!target.boundsList.empty()) target.boundsList[0].max.y = 30.0f; }
        if (tool.stateValue <= 0) { tool.position = {-9999, -9999, -9999}; tool.isSolid = false; tool.canGrab = false; }
        return CHEM_USED_BUT_KEPT;
    }
    
    if (tool.HasTag(TAG_SPONGE) && target.HasTag(TAG_SPHINX_NOSE)) {
        target.color = BEIGE; return CHEM_USED_BUT_KEPT;
    }
    
    if (tool.HasTag(TAG_SPHINX_NOSE) && target.HasTag(TAG_SPHINX) && tool.color.r == BEIGE.r) {
        tool.attachedTo = targetIdx; tool.color = GOLD; return CHEM_ATTACHED;
    }
    
    if (tool.HasTag(TAG_GLOVES) && target.HasTag(TAG_ELECTRIC)) {
        tool.attachedTo = targetIdx; target.canGrab = true; return CHEM_ATTACHED;
    }
    
    if (tool.HasTag(TAG_CORK) && target.HasTag(TAG_WIND_BAG) && target.isGlitching) {
        tool.attachedTo = targetIdx; target.isGlitching = false; return CHEM_ATTACHED;
    }
    
    if (tool.HasTag(TAG_TAPE) && target.HasTag(TAG_EYEWEAR) && target.HasTag(TAG_BROKEN)) {
        tool.stateValue -= 1.0f; target.RemoveTag(TAG_BROKEN);
        if (target.HasTag(TAG_EYEWEAR)) { target.color = BLACK; if (!target.boundsList.empty()) target.boundsList[0].max.y = 10.0f; } 
        else { target.color = YELLOW; if (!target.boundsList.empty()) target.boundsList[0].max.y = 30.0f; }
        if (tool.stateValue <= 0) { tool.position = {-9999, -9999, -9999}; tool.isSolid = false; tool.canGrab = false; }
        return CHEM_USED_BUT_KEPT;
    }
    
    if (tool.HasTag(TAG_SANDALS) && target.HasTag(TAG_MJOLNIR)) { 
        tool.attachedTo = targetIdx; target.canGrab = true; target.isGlitching = false; 
        if (!target.boundsList.empty()) target.boundsList[0].max.y += 40.0f; 
        return CHEM_ATTACHED; 
    }
    
    return CHEM_NONE;
}