#pragma once
#include "raylib.h"
#include <vector>
#include <string>
#include <algorithm>

enum Tag { 
    TAG_NONE, TAG_CONTAINER, TAG_HEAVY, TAG_HAZARD_BEAM, TAG_FRAGILE,
    TAG_BROKEN, TAG_TAPE, TAG_EYEWEAR, TAG_MEDUSA, 
    TAG_WATER_SOURCE, TAG_CORK,
    TAG_DOOR_1, TAG_DOOR_2, TAG_DOOR_3, TAG_DOOR_4, TAG_DOOR_5,
    TAG_EXTINGUISHER, TAG_GLOVES, TAG_MOP, TAG_SANDBAG, TAG_SPONGE,
    TAG_ZEUS, TAG_FUSEBOX, TAG_ELECTRIC, TAG_SANDALS, TAG_WIND_BAG,
    TAG_WET_SIGN, TAG_BOULDER,
    TAG_LIGHTSWITCH, TAG_SUN_DISK, TAG_MUMMY, TAG_SARCOPHAGUS, TAG_SPHINX, TAG_SPHINX_NOSE,
    TAG_MJOLNIR, TAG_GLEIPNIR, TAG_BANSHEE_STONE, TAG_BUBBLE_WRAP,
    TAG_HOLE_SAW, TAG_HOLE, TAG_FLASHLIGHT, TAG_MAT // Phase 15 Tags
};

enum ChemResult { CHEM_NONE, CHEM_USED_BUT_KEPT, CHEM_ATTACHED };

struct Entity {
    std::string name;
    Vector2 position;          
    float z;                   
    float zHeight;             
    Vector2 velocity;          
    float zVelocity;           
    Vector2 facingDir;         
    Rectangle movementBox;     
    Rectangle interactionBox;  
    Color color;
    
    bool canGrab;       
    bool canThrow;      
    bool canInteract;   
    bool isSolid;              
    bool isGrabbed;
    bool castsShadow;
    bool is3DBlock;
    
    int attachedTo = -1; 
    bool isGlitching; 
    bool isStone;     
    bool isUsing = false; 
    bool isDead = false; // New! For player zap death
    
    float stateTimer = 0.0f; 
    float stateValue = 0.0f; // Multi-use (Water Radius, Fusebox Open/Closed state)
    std::vector<Tag> tags;

    bool HasTag(Tag t) const { return std::find(tags.begin(), tags.end(), t) != tags.end(); }
    void AddTag(Tag t) { if (!HasTag(t)) tags.push_back(t); }
    void RemoveTag(Tag t) { tags.erase(std::remove(tags.begin(), tags.end(), t), tags.end()); }

    Rectangle GetWorldMovementBox() const { return { position.x + movementBox.x, position.y + movementBox.y, movementBox.width, movementBox.height }; }
    Rectangle GetWorldInteractionBox() const { return { position.x + interactionBox.x, position.y + interactionBox.y - z, interactionBox.width, interactionBox.height }; }
};