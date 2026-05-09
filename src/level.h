#pragma once
#include "Entities.h"

const int GAME_WIDTH = 1600;
const int GAME_HEIGHT = 1200;

// --- Helpers ---
inline Entity MakeWallBox(float x, float y, float w, float h) { return { "Wall", {x, y}, 0, 200.0f, {0,0}, 0, {0,1}, {0,0,w,h}, {0,-150,w,h+150}, {40, 40, 50, 255}, false, false, false, true, false, false, true, -1, false, false, false, false, 0.0f, 0.0f, {} }; }
inline Entity MakeDoorBox(float x, float y, float w, float h, Tag doorTag) { return { "Door", {x, y}, 0, 200.0f, {0,0}, 0, {0,1}, {0,0,w,h}, {0,-150,w,h+150}, {100, 40, 40, 255}, false, false, false, true, false, false, true, -1, false, false, false, false, 0.0f, 0.0f, {doorTag} }; }
inline Entity MakePedestal(Vector2 pos) { return { "Pedestal", pos, 0, 80.0f, {0,0}, 0, {0,1}, {-40,-30,80,60}, {-40,-30,80,60}, GRAY, false, false, false, true, false, true, true, -1, false, false, false, false, 0.0f, 0.0f, {} }; }
inline Entity MakeProp(std::string name, Vector2 pos, float zHeight, Color col, std::vector<Tag> tags) { return { name, pos, 0, zHeight, {0,0}, 0, {0,1}, {-15,-15,30,30}, {-30,-50,60,80}, col, true, true, false, false, false, true, false, -1, false, false, false, false, 0.0f, 0.0f, tags }; }
inline Entity MakeSolidProp(std::string name, Vector2 pos, float zHeight, Color col, std::vector<Tag> tags) { return { name, pos, 0, zHeight, {0,0}, 0, {0,1}, {-30,-20,60,40}, {-30,-50,60,80}, col, true, true, false, true, false, true, false, -1, false, false, false, false, 0.0f, 0.0f, tags }; }
inline Entity MakeArtifact(std::string name, Vector2 pos, float zHeight, Color col, std::vector<Tag> tags) { return { name, pos, 80.0f, zHeight, {0,0}, 0, {0,1}, {-20,-15,40,30}, {-30,-50,60,80}, col, false, false, false, true, false, true, false, -1, false, false, false, false, 0.0f, 0.0f, tags }; }
inline Entity MakeZeus(Vector2 pos) { return { "Zeus Statue", pos, 0, 140.0f, {0,0}, 0, {0,1}, {-50,-40,100,80}, {-50,-40,100,80}, WHITE, false, false, false, true, false, true, true, -1, false, false, false, false, 0.0f, 0.0f, {TAG_ZEUS} }; }
inline Entity MakeFuseBox(Vector2 pos) { return { "Fuse Box", pos, 80.0f, 40.0f, {0,0}, 0, {0,1}, {-30,-10,60,20}, {-40,-60,80,100}, DARKGRAY, false, false, false, true, false, true, true, -1, false, false, false, false, 0.0f, 0.0f /* 0 = closed, 1 = open */, {TAG_FUSEBOX} }; }
inline Entity MakeLightSwitch(Vector2 pos) { return { "Light Switch", pos, 0, 40.0f, {0,0}, 0, {0,1}, {-10,-10,20,20}, {-30,-30,60,60}, LIGHTGRAY, false, false, false, true, false, true, true, -1, false, false, false, false, 0.0f, 1.0f /* 1 = ON */, {TAG_LIGHTSWITCH} }; }

inline void InitLevel(std::vector<Entity>& e) {
    // Player spawns in the Entrance
    e.push_back({ "Player", {-800, 600}, 0, 0, {0,0}, 0, {0,1}, {-25,-15,50,30}, {-25,-90,50,90}, BLUE, false, false, false, true, false, true, false, -1, false, false, false, false, 0.0f, 0.0f, {} });

    // ==========================================
    // MAP ARCHITECTURE (3x3 Grid Layout)
    // ==========================================
    
    // --- Horizontal Walls ---
    // Top Row (Egyptian Top)
    e.push_back(MakeWallBox(1600, -1250, 1600, 100)); 
    
    // Upper Middle Row
    e.push_back(MakeWallBox(-1600, -50, 1600, 100)); // Entrance Top
    e.push_back(MakeWallBox(0, -50, 1600, 100));     // Main Top 
    e.push_back(MakeWallBox(1600, -50, 600, 100)); e.push_back(MakeDoorBox(2200, -50, 400, 100, TAG_DOOR_4)); e.push_back(MakeWallBox(2600, -50, 600, 100)); // Egyptian Bottom / Medusa Top
    e.push_back(MakeWallBox(3200, -50, 1600, 100));  // Empty Right Top
    
    // Lower Middle Row
    e.push_back(MakeWallBox(-1600, 1150, 1600, 100)); // Entrance Bottom
    e.push_back(MakeWallBox(0, 1150, 600, 100)); e.push_back(MakeDoorBox(600, 1150, 400, 100, TAG_DOOR_5)); e.push_back(MakeWallBox(1000, 1150, 600, 100)); // Main Bottom / Closet Top
    e.push_back(MakeWallBox(1600, 1150, 600, 100)); e.push_back(MakeDoorBox(2200, 1150, 400, 100, TAG_DOOR_3)); e.push_back(MakeWallBox(2600, 1150, 600, 100)); // Medusa Bottom / Nordic Top
    e.push_back(MakeWallBox(3200, 1150, 1600, 100)); // Empty Right Bottom
    
    // Bottom Row
    e.push_back(MakeWallBox(0, 2350, 1600, 100));    // Closet Bottom
    e.push_back(MakeWallBox(1600, 2350, 1600, 100)); // Nordic Bottom

    // --- Vertical Walls ---
    // Leftmost Edge
    e.push_back(MakeWallBox(-1650, 0, 100, 1200));   // Entrance Left
    
    // Middle Left Edge (Separates Entrance/Main & Blocks Closet Left)
    e.push_back(MakeWallBox(-50, 0, 100, 400)); e.push_back(MakeDoorBox(-50, 400, 100, 400, TAG_DOOR_1)); e.push_back(MakeWallBox(-50, 800, 100, 400)); // Entrance -> Main
    e.push_back(MakeWallBox(-50, 1200, 100, 1200));  // Closet Left
    
    // Middle Right Edge (Separates Main/Medusa, Egyptian Left, Nordic Left)
    e.push_back(MakeWallBox(1550, -1200, 100, 1200));// Egyptian Left
    e.push_back(MakeWallBox(1550, 0, 100, 400)); e.push_back(MakeDoorBox(1550, 400, 100, 400, TAG_DOOR_2)); e.push_back(MakeWallBox(1550, 800, 100, 400)); // Main -> Medusa
    e.push_back(MakeWallBox(1550, 1200, 100, 1200)); // Closet Right / Nordic Left
    
    // Far Right Edge (Separates Medusa/Empty, Egyptian Right, Nordic Right)
    e.push_back(MakeWallBox(3150, -1200, 100, 1200));// Egyptian Right
    e.push_back(MakeWallBox(3150, 0, 100, 400)); /* OPEN DOORWAY */ e.push_back(MakeWallBox(3150, 800, 100, 400)); // Medusa -> Empty Right
    e.push_back(MakeWallBox(3150, 1200, 100, 1200)); // Nordic Right
    
    // Absolute Right Edge
    e.push_back(MakeWallBox(4750, 0, 100, 1200));    // Empty Right Right Boundary


    // ==========================================
    // EXHIBITS & PROPS 
    // ==========================================

    // --- ENTRANCE (X: -1600 to 0, Y: 0 to 1200) ---
    e.push_back(MakeLightSwitch({-1400, 100}));
    e.push_back(MakeSolidProp("Ticket Counter", {-1000, 400}, 60, BROWN, {TAG_CONTAINER}));

    // --- MAIN ROOM (X: 0 to 1600, Y: 0 to 1200) ---
    e.push_back(MakeLightSwitch({200, 100}));
    e.push_back(MakeFuseBox({800, 0}));
    
    // The Boulder
    Entity boulder = MakeSolidProp("Sisyphus Boulder", {200, 450}, 100, DARKGRAY, {TAG_BOULDER, TAG_HEAVY});
    boulder.canGrab = false; boulder.isGlitching = false; boulder.movementBox = {-60, -30, 120, 60}; 
    e.push_back(boulder);

    // The NPC Display Vase
    e.push_back(MakePedestal({700, 450}));
    Entity displayVase = MakeArtifact("Display Vase", {700, 450}, 30, ORANGE, {TAG_FRAGILE});
    displayVase.canGrab = true; displayVase.canThrow = true; displayVase.isSolid = false;
    e.push_back(displayVase);

    // Zeus
    e.push_back(MakeZeus({1100, 450})); 
    Entity lightning = MakeArtifact("Lightning", {1100, 450}, 40, YELLOW, {TAG_ELECTRIC});
    lightning.attachedTo = e.size() - 1; lightning.canGrab = true; lightning.canThrow = true; lightning.isSolid = false;
    e.push_back(lightning);


    // --- MEDUSA ROOM (X: 1600 to 3200, Y: 0 to 1200) ---
    e.push_back(MakeLightSwitch({1800, 100}));
    
    e.push_back(MakePedestal({1900, 450}));
    Entity fragileVase = MakeArtifact("Fragile Vase", {1900, 450}, 30, ORANGE, {TAG_FRAGILE});
    fragileVase.canGrab = true; fragileVase.canThrow = true; fragileVase.isSolid = false;
    e.push_back(fragileVase);

    e.push_back(MakePedestal({2300, 450}));
    e.push_back(MakeArtifact("Medusa Head", {2300, 450}, 30, GREEN, {TAG_MEDUSA}));
    
    e.push_back(MakePedestal({2700, 450}));
    Entity sandals = MakeArtifact("Hermes Sandals", {2700, 450}, 20, SKYBLUE, {TAG_SANDALS});
    sandals.canGrab = true; sandals.canThrow = true; sandals.isSolid = false;
    e.push_back(sandals);

    e.push_back(MakePedestal({2300, 800}));
    e.push_back(MakeArtifact("Holy Cup", {2300, 800}, 30, GOLD, {TAG_WATER_SOURCE}));
    
    e.push_back(MakePedestal({2700, 800}));
    Entity windBag = MakeArtifact("Bag of Winds", {2700, 800}, 30, LIGHTGRAY, {TAG_WIND_BAG});
    windBag.canGrab = true; windBag.canThrow = true; windBag.isSolid = false;
    e.push_back(windBag);

    // Scattered Room Props
    e.push_back(MakeProp("Duct Tape", {2000, 600}, 10, LIGHTGRAY, {TAG_TAPE}));
    e.push_back(MakeProp("Sunglasses", {2100, 600}, 10, BLACK, {TAG_FRAGILE, TAG_EYEWEAR}));
    e.push_back(MakeProp("Giant Cork", {2400, 900}, 20, BROWN, {TAG_CORK}));


    // --- EGYPTIAN WING (X: 1600 to 3200, Y: -1200 to 0) ---
    e.push_back(MakeLightSwitch({1800, -1100}));
    
    e.push_back(MakeSolidProp("Sarcophagus", {2000, -800}, 40, DARKPURPLE, {TAG_SARCOPHAGUS}));
    Entity mummy = MakeArtifact("Restless Mummy", {2000, -700}, 80, BEIGE, {TAG_MUMMY});
    mummy.isGlitching = true; mummy.canGrab = true; mummy.canThrow = true; mummy.isSolid = true;
    e.push_back(mummy);

    e.push_back(MakeSolidProp("The Sphinx", {2500, -800}, 100, GOLD, {TAG_SPHINX}));
    e.push_back(MakeProp("Sphinx Nose", {2500, -500}, 10, GOLD, {TAG_SPHINX_NOSE}));

    e.push_back(MakePedestal({2900, -800}));
    Entity sunDisk = MakeArtifact("Ra's Sun Disk", {2900, -800}, 40, ORANGE, {TAG_SUN_DISK});
    sunDisk.isGlitching = true; sunDisk.canGrab = false; // Too hot initially!
    e.push_back(sunDisk);


    // --- JANITOR'S CLOSET (X: 0 to 1600, Y: 1200 to 2400) ---
    e.push_back(MakeLightSwitch({200, 1300}));
    e.push_back(MakeProp("Fire Extinguisher", {400, 1500}, 40, RED, {TAG_EXTINGUISHER}));
    e.push_back(MakeProp("Rubber Gloves", {600, 1500}, 10, YELLOW, {TAG_GLOVES}));
    e.push_back(MakeProp("Mop", {800, 1500}, 60, RAYWHITE, {TAG_MOP}));
    e.push_back(MakeSolidProp("Heavy Sandbag", {1000, 1500}, 30, BEIGE, {TAG_SANDBAG, TAG_HEAVY})); 
    e.push_back(MakeProp("Magic Sponge", {1200, 1500}, 20, PINK, {TAG_SPONGE})); 
    e.push_back(MakeProp("Wet Floor Sign", {1400, 1500}, 40, YELLOW, {TAG_WET_SIGN}));

    // Multi-Use Tools (stateValue = 3.0f means 3 uses!)
    Entity tape = MakeProp("Duct Tape", {400, 1800}, 10, LIGHTGRAY, {TAG_TAPE});
    tape.stateValue = 3.0f; e.push_back(tape);
    
    Entity bubble = MakeProp("Bubble Wrap", {600, 1800}, 10, SKYBLUE, {TAG_BUBBLE_WRAP});
    bubble.stateValue = 3.0f; e.push_back(bubble);

    // New Heavy Tools
    e.push_back(MakeProp("Concrete Saw", {800, 1800}, 20, DARKGRAY, {TAG_HOLE_SAW, TAG_HEAVY}));
    e.push_back(MakeProp("Flashlight", {1000, 1800}, 10, BLACK, {TAG_FLASHLIGHT}));


    // --- NORDIC ROOM (X: 1600 to 3200, Y: 1200 to 2400) ---
    e.push_back(MakeLightSwitch({1800, 1300}));
    

    // --- EMPTY RIGHT ROOM (X: 3200 to 4800, Y: 0 to 1200) ---
    e.push_back(MakeLightSwitch({3400, 100}));

    // --- NORDIC ROOM (X: 1600 to 3200, Y: 1200 to 2400) ---
    e.push_back(MakePedestal({2000, 1800}));
    
    // Mjolnir has "fallen" off its pedestal. Too heavy to grab natively!
    Entity mjolnir = MakeArtifact("Mjolnir", {2000, 1900}, 30, DARKGRAY, {TAG_MJOLNIR, TAG_HEAVY});
    mjolnir.canGrab = false; mjolnir.isGlitching = true; // True means it's stuck on the floor
    e.push_back(mjolnir);

    e.push_back(MakePedestal({2600, 1800}));
    Entity gleipnir = MakeArtifact("Gleipnir's Ribbon", {2600, 1800}, 10, MAGENTA, {TAG_GLEIPNIR});
    gleipnir.canGrab = false; gleipnir.isGlitching = true; // Actively slithering
    e.push_back(gleipnir);

    // --- CELTIC WING (X: 3200 to 4800, Y: 0 to 1200) ---
    e.push_back(MakePedestal({4000, 600}));
    Entity banshee = MakeArtifact("Banshee Stone", {4000, 600}, 40, PURPLE, {TAG_BANSHEE_STONE});
    banshee.isGlitching = true; // Actively screaming
    e.push_back(banshee);

    // Add Bubble Wrap to Utility Closet!
    e.push_back(MakeProp("Bubble Wrap", {1000, 1800}, 10, SKYBLUE, {TAG_BUBBLE_WRAP}));
}