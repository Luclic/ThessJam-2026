#pragma once
#include "Entities.h"

const int GAME_WIDTH = 1600;
const int GAME_HEIGHT = 1200;

inline Entity MakeWallBox(float x, float y, float w, float h) {
    return { "Wall", {x, y}, 0, 200.0f, {0,0}, 0, {0,1}, {0,0,w,h}, {0,-150,w,h+150}, {40, 40, 50, 255}, false, false, false, true, false, false, true, -1, false, false, false, 0, 0, {} };
}
inline Entity MakeDoorBox(float x, float y, float w, float h, Tag doorTag) {
    return { "Door", {x, y}, 0, 200.0f, {0,0}, 0, {0,1}, {0,0,w,h}, {0,-150,w,h+150}, {100, 40, 40, 255}, false, false, false, true, false, false, true, -1, false, false, false, 0, 0, {doorTag} };
}
inline Entity MakePedestal(Vector2 pos) {
    return { "Pedestal", pos, 0, 80.0f, {0,0}, 0, {0,1}, {-40,-30,80,60}, {-40,-30,80,60}, GRAY, false, false, false, true, false, true, true, -1, false, false, false, 0, 0, {} };
}
inline Entity MakeProp(std::string name, Vector2 pos, float zHeight, Color col, std::vector<Tag> tags) {
    return { name, pos, 0, zHeight, {0,0}, 0, {0,1}, {-15,-15,30,30}, {-50,-80,100,120}, col, true, true, false, false, false, true, false, -1, false, false, false, 0, 0, tags };
}
// New helper specifically for heavy, solid items like the sandbag
inline Entity MakeSolidProp(std::string name, Vector2 pos, float zHeight, Color col, std::vector<Tag> tags) {
    return { name, pos, 0, zHeight, {0,0}, 0, {0,1}, {-30,-20,60,40}, {-50,-80,100,120}, col, true, true, false, true, false, true, false, -1, false, false, false, 0, 0, tags };
}
inline Entity MakeArtifact(std::string name, Vector2 pos, float zHeight, Color col, std::vector<Tag> tags) {
    return { name, pos, 80.0f, zHeight, {0,0}, 0, {0,1}, {-20,-15,40,30}, {-50,-80,100,120}, col, false, false, false, true, false, true, false, -1, false, false, false, 0, 0, tags };
}

inline void InitLevel(std::vector<Entity>& e) {
    e.push_back({ "Player", {800, 600}, 0, 0, {0,0}, 0, {0,1}, {-25,-15,50,30}, {-25,-90,50,90}, BLUE, false, false, false, true, false, true, false, -1, false, false, false, 0, 0, {} });

    // --- 2D MAP ARCHITECTURE ---
    e.push_back(MakeWallBox(0, -50, 600, 100));     e.push_back(MakeDoorBox(600, -50, 400, 100, TAG_DOOR_1));   e.push_back(MakeWallBox(1000, -50, 600, 100)); 
    e.push_back(MakeWallBox(0, 1150, 1600, 100)); // Room 1 bottom is totally solid
    
    e.push_back(MakeWallBox(-50, 0, 100, 450));     e.push_back(MakeDoorBox(-50, 450, 100, 300, TAG_DOOR_3));   e.push_back(MakeWallBox(-50, 750, 100, 450)); 
    e.push_back(MakeWallBox(1550, 0, 100, 300));    /* GAP FOR ROOM 2 */                                        e.push_back(MakeWallBox(1550, 900, 100, 300)); 
    e.push_back(MakeWallBox(3150, 0, 100, 450));    e.push_back(MakeDoorBox(3150, 450, 100, 300, TAG_DOOR_4));  e.push_back(MakeWallBox(3150, 750, 100, 450)); 

    // Room 2 Bottom Wall & UTILITY CLOSET DOOR
    e.push_back(MakeWallBox(1600, 1150, 600, 100)); e.push_back(MakeDoorBox(2200, 1150, 400, 100, TAG_DOOR_5)); e.push_back(MakeWallBox(2600, 1150, 600, 100));
    
    // Utility Closet Outer Bounds
    e.push_back(MakeWallBox(1900, 1250, 100, 650)); // Left Wall
    e.push_back(MakeWallBox(3000, 1250, 100, 650)); // Right Wall
    e.push_back(MakeWallBox(1900, 1800, 1200, 100)); // Bottom Wall

    // Map Outer Caps
    e.push_back(MakeWallBox(0, -1250, 1600, 100)); e.push_back(MakeWallBox(-1650, -50, 1600, 100)); e.push_back(MakeWallBox(1600, -50, 1600, 100)); e.push_back(MakeWallBox(3200, -50, 1600, 100));
    e.push_back(MakeWallBox(-50, -1250, 100, 1200)); e.push_back(MakeWallBox(-1650, -50, 100, 1200)); e.push_back(MakeWallBox(-50, 1150, 100, 1200)); e.push_back(MakeWallBox(4750, -50, 100, 1200)); 

    // --- PROPS & ARTIFACTS ---
    e.push_back(MakePedestal({500, 450})); e.push_back(MakePedestal({800, 450})); e.push_back(MakePedestal({1100, 450}));
    e.push_back(MakeArtifact("Fragile Vase", {500, 450}, 30, ORANGE, {TAG_FRAGILE}));
    e.push_back(MakeArtifact("Medusa Head", {800, 450}, 30, GREEN, {TAG_MEDUSA}));
    e.push_back(MakeProp("Duct Tape", {600, 700}, 10, LIGHTGRAY, {TAG_TAPE}));
    e.push_back(MakeProp("Sunglasses", {1100, 450}, 10, BLACK, {TAG_FRAGILE, TAG_EYEWEAR}));

    e.push_back(MakePedestal({2400, 500}));
    e.push_back(MakeArtifact("Holy Cup", {2400, 500}, 30, GOLD, {TAG_WATER_SOURCE}));
    e.push_back(MakeProp("Giant Cork", {2600, 700}, 20, BROWN, {TAG_CORK}));

    // --- UTILITY CLOSET ITEMS ---
    e.push_back(MakeProp("Fire Extinguisher", {2200, 1500}, 40, RED, {TAG_EXTINGUISHER}));
    e.push_back(MakeProp("Rubber Gloves", {2400, 1500}, 10, YELLOW, {TAG_GLOVES}));
    e.push_back(MakeProp("Mop", {2600, 1500}, 60, RAYWHITE, {TAG_MOP}));
    e.push_back(MakeSolidProp("Heavy Sandbag", {2800, 1500}, 30, BEIGE, {TAG_SANDBAG, TAG_HEAVY})); // Note: SolidProp!
}