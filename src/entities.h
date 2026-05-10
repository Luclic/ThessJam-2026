#pragma once
#include "raylib.h"
#include "raymath.h"
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
    
    // --- TRUE 3D PHYSICS DATA ---
    Vector3 position;          // X (Left/Right), Y (Up/Down), Z (Forward/Back)
    Vector3 velocity;          
    Vector3 facingDir;         
    
    // --- TRUE 3D PHYSICS DATA ---
    std::vector<BoundingBox> boundsList;         // Supports multiple hitboxes per object!
    std::vector<BoundingBox> interactBoundsList; // For interactions
    
    Color color; 
    bool canGrab;       
    bool canThrow;      
    bool canInteract;   
    bool isSolid;              
    bool isGrabbed;
    bool castsShadow;
    bool is3DBlock; 
    bool isStatic = false; 
    
    int attachedTo = -1; 
    bool isGlitching; 
    bool isStone;     
    bool isUsing = false; 
    bool isDead = false; 
    
    float stateTimer = 0.0f; 
    float stateValue = 0.0f; // Stores Rotation in Degrees!
    std::vector<Tag> tags;

    bool HasTag(Tag t) const { return std::find(tags.begin(), tags.end(), t) != tags.end(); }
    void AddTag(Tag t) { if (!HasTag(t)) tags.push_back(t); }
    void RemoveTag(Tag t) { tags.erase(std::remove(tags.begin(), tags.end(), t), tags.end()); }

    // --- TRUE AABB MATRIX ROTATION (SYNCED TO RAYLIB) ---
    std::vector<BoundingBox> GetProjectedBounds(Vector3 offsetPos) const { 
        std::vector<BoundingBox> result;
        float rad = stateValue * DEG2RAD;
        float cosR = cos(rad); float sinR = sin(rad);

        for (const auto& b : boundsList) {
            Vector3 corners[8] = {
                {b.min.x, b.min.y, b.min.z}, {b.max.x, b.min.y, b.min.z},
                {b.min.x, b.max.y, b.min.z}, {b.max.x, b.max.y, b.min.z},
                {b.min.x, b.min.y, b.max.z}, {b.max.x, b.min.y, b.max.z},
                {b.min.x, b.max.y, b.max.z}, {b.max.x, b.max.y, b.max.z}
            };

            Vector3 newMin = {99999, 99999, 99999};
            Vector3 newMax = {-99999, -99999, -99999};

            for(int i=0; i<8; ++i) {
                // FIXED: Signs swapped to match Raylib's internal OpenGL rotation!
                float rx = corners[i].x * cosR + corners[i].z * sinR;
                float rz = -corners[i].x * sinR + corners[i].z * cosR;
                
                if(rx < newMin.x) newMin.x = rx; if(rx > newMax.x) newMax.x = rx;
                if(corners[i].y < newMin.y) newMin.y = corners[i].y; if(corners[i].y > newMax.y) newMax.y = corners[i].y;
                if(rz < newMin.z) newMin.z = rz; if(rz > newMax.z) newMax.z = rz;
            }
            result.push_back({
                {offsetPos.x + newMin.x, offsetPos.y + newMin.y, offsetPos.z + newMin.z},
                {offsetPos.x + newMax.x, offsetPos.y + newMax.y, offsetPos.z + newMax.z}
            });
        }
        return result;
    }
    
    std::vector<BoundingBox> GetWorldBounds() const { return GetProjectedBounds(position); }
    
    std::vector<BoundingBox> GetWorldInteractBounds() const { 
        std::vector<BoundingBox> result;
        float rad = stateValue * DEG2RAD;
        float cosR = cos(rad); float sinR = sin(rad);

        for (const auto& b : interactBoundsList) {
            Vector3 corners[8] = {
                {b.min.x, b.min.y, b.min.z}, {b.max.x, b.min.y, b.min.z},
                {b.min.x, b.max.y, b.min.z}, {b.max.x, b.max.y, b.min.z},
                {b.min.x, b.min.y, b.max.z}, {b.max.x, b.min.y, b.max.z},
                {b.min.x, b.max.y, b.max.z}, {b.max.x, b.max.y, b.max.z}
            };

            Vector3 newMin = {99999, 99999, 99999};
            Vector3 newMax = {-99999, -99999, -99999};

            for(int i=0; i<8; ++i) {
                // FIXED: Signs swapped here too!
                float rx = corners[i].x * cosR + corners[i].z * sinR;
                float rz = -corners[i].x * sinR + corners[i].z * cosR;
                
                if(rx < newMin.x) newMin.x = rx; if(rx > newMax.x) newMax.x = rx;
                if(corners[i].y < newMin.y) newMin.y = corners[i].y; if(corners[i].y > newMax.y) newMax.y = corners[i].y;
                if(rz < newMin.z) newMin.z = rz; if(rz > newMax.z) newMax.z = rz;
            }
            result.push_back({
                {position.x + newMin.x, position.y + newMin.y, position.z + newMin.z},
                {position.x + newMax.x, position.y + newMax.y, position.z + newMax.z}
            });
        }
        return result;
    }
};

// --- HELPER FOR COLLISION LOOPS ---
inline bool CheckCollisionLists(const std::vector<BoundingBox>& listA, const std::vector<BoundingBox>& listB) {
    for (const auto& a : listA) {
        for (const auto& b : listB) {
            if (CheckCollisionBoxes(a, b)) return true;
        }
    }
    return false;
}
// ... [End of your Entity struct] ...

const int GAME_WIDTH = 1600;
const int GAME_HEIGHT = 1200; 

// Placed here so main.cpp can use it to spawn holes and bubble mats dynamically!
inline Entity MakeProp(std::string name, Vector3 pos, BoundingBox b, Color col, std::vector<Tag> tags) { 
    return { name, pos, {0,0,0}, {0,0,1}, {b}, {b}, col, true, true, false, false, false, true, false, false, -1, false, false, false, false, 0.0f, 0.0f, tags }; 
}

// --- COLOR VARIANT LOGIC ---
inline std::string GetBaseModelName(const std::string& name) {
    if (name=="wall1"||name=="wall2"||name=="wall3"||name=="wall4") return "wall1";
    if (name=="wall1corner"||name=="wall2corner"||name=="wall3corner"||name=="wall4corner") return "wall1corner";
    if (name=="arch1"||name=="arch2"||name=="arch3") return "arch1";
    if (name=="archarch1"||name=="archarch2"||name=="archarch3") return "archarch1";
    if (name=="floor11"||name=="floor21"||name=="floor31"||name=="floor41") return "floor11";
    if (name=="floor12"||name=="floor22"||name=="floor32"||name=="floor42") return "floor12";
    return name;
}

inline std::string GetNextStyle(const std::string& name) {
    if (name == "wall1") return "wall2"; if (name == "wall2") return "wall3"; if (name == "wall3") return "wall4"; if (name == "wall4") return "wall1";
    if (name == "wall1corner") return "wall2corner"; if (name == "wall2corner") return "wall3corner"; if (name == "wall3corner") return "wall4corner"; if (name == "wall4corner") return "wall1corner";
    if (name == "arch1") return "arch2"; if (name == "arch2") return "arch3"; if (name == "arch3") return "arch1";
    if (name == "archarch1") return "archarch2"; if (name == "archarch2") return "archarch3"; if (name == "archarch3") return "archarch1";
    if (name == "floor11") return "floor21"; if (name == "floor21") return "floor31"; if (name == "floor31") return "floor41"; if (name == "floor41") return "floor11";
    if (name == "floor12") return "floor22"; if (name == "floor22") return "floor32"; if (name == "floor32") return "floor42"; if (name == "floor42") return "floor12";
    return name; // Return unchanged if not a variant object
}