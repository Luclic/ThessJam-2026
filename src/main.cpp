#include "raylib.h"
#include "raymath.h"
#include <algorithm>
#include <vector>
#include "Entities.h"
#include "Level.h"
#include "Interactions.h"
#include "Evaluation.h"
#include "SaveSystem.h"
#include "GameSystems.h"
#include "RenderSystem.h"
#include "Menu.h"      
#include "Tutorial.h"  
#include "Overlay.h"   
#include <unordered_map>
#include <string>

const int idxIdle = 4;
const int idxInteract = 10;
const int idxRoll = 15;
const int idxRun = 24;
const int idxWalk = 30;

enum GameState { STATE_MENU, STATE_OPTIONS, STATE_TUTORIAL, STATE_PLAYING, STATE_REVIEW, STATE_GAMEOVER_FIRED, STATE_GAMEOVER_DEATH };

const float SHIFT_DURATION = 180.0f; 
const float SECONDS_PER_HOUR = 22.5f;

GameState currentState = STATE_MENU;
Camera2D camera = { 0 };
std::vector<Entity> entities;
std::unordered_map<std::string, Model> models;

// --- LIGHTING SYSTEM ---
struct PointLight {
    Vector3 position;
    Vector3 color;  // 0.0f to 1.0f RGB
    float radius;
};
std::vector<PointLight> pointLights;

int grabbedEntityIndex = -1;
int equippedEyewear = -1; 
int equippedGloves = -1; 

int currentNight = 1;
float shiftTimer = 0.0f;
ShiftReport lastReport;

bool showInteractMenu = false;
bool isDropMenu = false;
std::vector<int> interactTargets;
int interactSelectedIndex = 0;
bool doorsOpen[5] = {false, false, false, false, false};

// --- SYSTEM VARS ---
int tutorialStep = 0;
bool isOverlayActive = false;

// --- GLOBAL AUDIO TRACKS ---
Music tutorialMusic;
Music mainMusic; 
bool isMainMusicLoaded = false;
Music deathMusic;
Music newsMusic;
Music reviewsMusic;
float mainMusicVolume = 0.2f; 

std::unordered_map<std::string, Sound> sounds;
float stepTimer = 0.0f;

void GoToMenu() {
    currentState = STATE_MENU;
    entities.clear();
    InitLevel(entities);
    AssignEntityRules(entities);
    for(int i = 0; i < 5; i++) doorsOpen[i] = false; 
    if (!IsMusicStreamPlaying(tutorialMusic)) PlayMusicStream(tutorialMusic);
}

void ResetNight() {
    entities.clear();
    InitLevel(entities);
    
    AssignEntityRules(entities); 

    grabbedEntityIndex = -1;
    equippedEyewear = -1; 
    equippedGloves = -1; 
    shiftTimer = 0.0f;
    tutorialStep = 0; 
    isOverlayActive = false;
    showInteractMenu = false;
    
    for(int i = 0; i < 5; i++) doorsOpen[i] = false;
    if (currentNight >= 1) doorsOpen[0] = true;
    if (currentNight >= 2) doorsOpen[1] = true;
    if (currentNight >= 3) doorsOpen[2] = true;
    if (currentNight >= 4) doorsOpen[3] = true;
    if (currentNight >= 5) doorsOpen[4] = true;

    if (currentNight > 1) LoadGame(currentNight, entities); 
    SetupNightHazards(currentNight, entities); 

    if (isMainMusicLoaded) UnloadMusicStream(mainMusic);    
    int trackNum = std::min(currentNight, 5); 
    mainMusic = LoadMusicStream(TextFormat("resources/music/main_theme%d.ogg", trackNum));
    isMainMusicLoaded = true;

    mainMusic.looping = false; 
    SetMusicVolume(mainMusic, mainMusicVolume);
}

int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 960, "Museum Tech Support");

    Font fontMuseum = LoadFontEx("resources/fonts/Playfair_Display/static/PlayfairDisplay-Bold.ttf", 250, 0, 0);
    Font fontEmployee = LoadFontEx("resources/fonts/Courier_Prime/CourierPrime-Bold.ttf", 250, 0, 0);

    SetTextureFilter(fontMuseum.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(fontEmployee.texture, TEXTURE_FILTER_BILINEAR);
    
    InitAudioDevice();
    const char* sfxNames[] = {
        "ducktape", "fire-extenguisher", "handsaw", "plastictap-wetfloors", 
        "popping-bubble-wr", "sandbags-put-on-fl" 
    };
    for (const char* sn : sfxNames) {
        sounds[sn] = LoadSound(TextFormat("resources/sound_effects/%s.ogg", sn)); 
    }
    tutorialMusic = LoadMusicStream("resources/music/tutorial.ogg");
    deathMusic = LoadMusicStream("resources/music/death.ogg");
    newsMusic = LoadMusicStream("resources/music/news.ogg"); 
    reviewsMusic = LoadMusicStream("resources/music/reviews.ogg");
    
    tutorialMusic.looping = true;
    deathMusic.looping = false;
    newsMusic.looping = true;
    reviewsMusic.looping = true;

    SetTargetFPS(60);

    // ==============================================================
    // --- SHADER PIPELINE INITIALIZATION ---
    // ==============================================================

    // Phase 1: Clay Multi-Light Shader
    Shader clayShader = LoadShader(0, "resources/shaders/clay.fs");
    
    // A nice, dark, cool-toned ambient base for the night shift
    Vector3 ambientColor = { 0.15f, 0.15f, 0.20f }; 
    SetShaderValue(clayShader, GetShaderLocation(clayShader, "ambientColor"), &ambientColor, SHADER_UNIFORM_VEC3);

    int lightCountLoc = GetShaderLocation(clayShader, "activeLightCount");
    int lightPosLoc = GetShaderLocation(clayShader, "lightPositions");
    int lightColLoc = GetShaderLocation(clayShader, "lightColors");
    int lightRadLoc = GetShaderLocation(clayShader, "lightRadii");

    // Temporarily add a couple of test lights to prove it works before the Editor hooks up!
    pointLights.push_back({ {300.0f, 150.0f, 300.0f}, {1.0f, 0.9f, 0.7f}, 600.0f }); // Warm light
    pointLights.push_back({ {1200.0f, 150.0f, 800.0f}, {0.7f, 0.8f, 1.0f}, 800.0f }); // Cool light

    // Phase 3: Screen Post-Processing Shader
    Shader postShader = LoadShader(0, "resources/shaders/postprocess.fs");
    Vector3 tintColor = { 1.02f, 0.98f, 0.96f }; 
    float vignetteStrength = 0.5f;               
    
    SetShaderValue(postShader, GetShaderLocation(postShader, "tintColor"), &tintColor, SHADER_UNIFORM_VEC3);
    SetShaderValue(postShader, GetShaderLocation(postShader, "vignetteStrength"), &vignetteStrength, SHADER_UNIFORM_FLOAT);
    
    // ==============================================================

    const char* modelNames[] = {
        "artifactsign2", "bench1", "bench2", "bench3", "desk", "fence", "floor11", "floor12", "floor21", "floor22", "floor31", "floor32", "floor41", "floor42", 
        "glasscase", "light1", "light2", "painting1", "painting2", "paintinglight", "pillar1", "pillar2", "rope1", "rope2", "sign1", "sign2", "sign3", 
        "stand1", "stand2", "stand3", "stand4", "ticketstand", "ticketstandseat", "wall1", "wall1corner", "wall2", "wall2corner", "wall3", "wall3corner", "wall4", "wall4corner", 
        "arch1", "arch2", "arch3", "archarch1", "archarch2", "archarch3", "artifactsign1", "Waterfall", "Time Hotel 7.07", "Saw", "Wall Shelf", "Shelves", 
        "Cardboard Boxes", "Time Hotel 5.25 Painters Tape", "Pixel Sunglasses", "Sunglasses", "Glove", "Broom", "Sponge", "Bag", "Fire Extinguisher", "rocks", 
        "Ocean", "Coin", "Sandal", "Greek Temple", "Chalice", "Coin Pouch", "Pyramid", "Anubis Statue", "mjolner", "Rock", "Tall Vase", "Generic",
        "zeus", "medusa", "lightning", "pandora", "mummy", "Info Button", "Magazine", "Open Book", "Sticker", "Cork", "Snake", "Coffin"
    };
    for (const char* mn : modelNames) { 
        const char* filePath = TextFormat("resources/models/%s.glb", mn);
        models[mn] = LoadModel(filePath);
    }
    
    models["Player"] = LoadModel("resources/models/worker.glb"); 
    int animCount = 0;
    ModelAnimation* anims = LoadModelAnimations("resources/models/worker.glb", &animCount);
    float animTimer = 0.0f;
    int currentAnimState = 0; 
    
    Image stoneImg = GenImageColor(2, 2, { 180, 180, 180, 255 }); 
    Texture2D stoneTex = LoadTextureFromImage(stoneImg);
    UnloadImage(stoneImg);

    Texture2D zombieTex = LoadTexture("resources/models/ZombieTexture.png");
    
    // Assign textures safely
    if (models.count("zeus")) {
        for (int i = 0; i < models["zeus"].materialCount; i++) models["zeus"].materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = stoneTex;
    }
    if (models.count("Greek Temple")) {
        for (int i = 0; i < models["Greek Temple"].materialCount; i++) models["Greek Temple"].materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = stoneTex;
    }
    if (models.count("mummy")) {
        for (int i = 0; i < models["mummy"].materialCount; i++) models["mummy"].materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = zombieTex;
    }

    // Apply the custom clay shader to EVERY model loaded in the game
    for (auto& pair : models) {
        for (int i = 0; i < pair.second.materialCount; i++) {
            pair.second.materials[i].shader = clayShader;
        }
    }

    // Load Mummy Animations
    int mummyAnimCount = 0;
    ModelAnimation* mummyAnims = LoadModelAnimations("resources/models/mummy.glb", &mummyAnimCount);
    float mummyAnimTimer = 0.0f;

    RenderTexture2D renderTarget = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_BILINEAR);

    InitLevel(entities);
    if (!LoadGame(currentNight, entities)) currentNight = 1; 
    camera.zoom = 1.0f;

    GoToMenu();

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        if (isMainMusicLoaded) UpdateMusicStream(mainMusic);
        UpdateMusicStream(tutorialMusic);
        UpdateMusicStream(deathMusic);
        UpdateMusicStream(newsMusic);
        UpdateMusicStream(reviewsMusic);

        if (currentState == STATE_REVIEW) {
            if (mainMusicVolume > 0.0f) { mainMusicVolume -= dt * 2.0f; SetMusicVolume(mainMusic, std::max(0.0f, mainMusicVolume)); }
            BeginDrawing(); ClearBackground(RAYWHITE);
            DrawText("SHIFT COMPLETE - 8:00 AM", GetScreenWidth()/2 - 200, 100, 30, BLACK);
            DrawText(lastReport.finalVerdict.c_str(), GetScreenWidth()/2 - MeasureText(lastReport.finalVerdict.c_str(), 20)/2, 160, 20, DARKBLUE);
            int yOffset = 250;
            for (const auto& rev : lastReport.reviews) { DrawText(TextFormat("- %s: %s", rev.artifactName.c_str(), rev.reviewText.c_str()), 100, yOffset, 20, DARKGRAY); yOffset += 40; }
            DrawText("PRESS ENTER TO CONTINUE", GetScreenWidth()/2 - 150, GetScreenHeight() - 100, 20, GREEN);
            if (IsKeyPressed(KEY_ENTER)) { SaveGame(currentNight, entities); StopMusicStream(reviewsMusic); ResetNight(); currentState = STATE_PLAYING; PlayMusicStream(mainMusic); }
            EndDrawing(); continue;
        }

        if (currentState == STATE_GAMEOVER_FIRED) {
            if (mainMusicVolume > 0.0f) { mainMusicVolume -= dt * 2.0f; SetMusicVolume(mainMusic, std::max(0.0f, mainMusicVolume)); }
            BeginDrawing(); ClearBackground(MAROON);
            DrawText("YOU'RE FIRED.", GetScreenWidth()/2 - 150, 200, 50, WHITE);
            DrawText(lastReport.finalVerdict.c_str(), GetScreenWidth()/2 - MeasureText(lastReport.finalVerdict.c_str(), 20)/2, 300, 20, LIGHTGRAY);
            DrawText("PRESS ENTER TO RETRY THE NIGHT", GetScreenWidth()/2 - 200, GetScreenHeight() - 100, 20, YELLOW);
            if (IsKeyPressed(KEY_ENTER)) { StopMusicStream(newsMusic); ResetNight(); currentState = STATE_PLAYING; PlayMusicStream(mainMusic); }
            EndDrawing(); continue;
        }

        if (currentState == STATE_GAMEOVER_DEATH) {
            if (mainMusicVolume > 0.0f) { mainMusicVolume -= dt * 1.0f; SetMusicVolume(mainMusic, std::max(0.0f, mainMusicVolume)); }
            BeginDrawing(); ClearBackground(RED);
            DrawText("YOU DIED.", GetScreenWidth()/2 - 120, 200, 50, BLACK);
            DrawText(lastReport.finalVerdict.c_str(), GetScreenWidth()/2 - MeasureText(lastReport.finalVerdict.c_str(), 20)/2, 300, 20, LIGHTGRAY);
            DrawText("PRESS ENTER TO RETRY THE NIGHT", GetScreenWidth()/2 - 200, GetScreenHeight() - 100, 20, YELLOW);
            if (IsKeyPressed(KEY_ENTER)) { StopMusicStream(deathMusic); ResetNight(); currentState = STATE_PLAYING; PlayMusicStream(mainMusic); }
            EndDrawing(); continue;
        }

        Entity& player = entities[0];
        bool playerJumpedThisFrame = false; 
        bool triggerShiftStart = false;
        HazardVisuals hazVis;

        if (!isOverlayActive) {
            
            if (currentState == STATE_PLAYING) {
                shiftTimer += dt;
                if (shiftTimer >= SHIFT_DURATION) {
                    lastReport = EvaluateMuseum(entities);
                    if (lastReport.isFired) { currentState = STATE_GAMEOVER_FIRED; PlayMusicStream(newsMusic); } 
                    else { currentNight++; currentState = STATE_REVIEW; PlayMusicStream(reviewsMusic); }
                }
            }

            bool isSprinting = IsKeyDown(KEY_LEFT_SHIFT);
            bool executeAction = false;
            int actionTargetIdx = -1;

            if (currentState == STATE_PLAYING || currentState == STATE_TUTORIAL) {
                if (IsKeyPressed(KEY_I)) { for (auto& e : entities) if (e.HasTag(TAG_MEDUSA)) e.isGlitching = !e.isGlitching; }
                if (IsKeyPressed(KEY_O)) { for (auto& e : entities) if (e.HasTag(TAG_WATER_SOURCE)) e.isGlitching = !e.isGlitching; }
                if (IsKeyPressed(KEY_P)) { for (auto& e : entities) if (e.HasTag(TAG_SANDALS)) e.isGlitching = true; }
                if (IsKeyPressed(KEY_K)) { for (auto& e : entities) if (e.HasTag(TAG_WIND_BAG)) e.isGlitching = !e.isGlitching; }
                if (IsKeyPressed(KEY_L)) { for (auto& e : entities) if (e.HasTag(TAG_BOULDER)) e.isGlitching = !e.isGlitching; }
                if (IsKeyPressed(KEY_SIX)) { shiftTimer = SHIFT_DURATION; } 
                
                for(auto& e : entities) {
                    if (e.HasTag(TAG_DOOR_1)) { e.isSolid = !doorsOpen[0]; e.color.a = doorsOpen[0] ? 30 : 255; }
                    if (e.HasTag(TAG_DOOR_2)) { e.isSolid = !doorsOpen[1]; e.color.a = doorsOpen[1] ? 30 : 255; }
                    if (e.HasTag(TAG_DOOR_3)) { e.isSolid = !doorsOpen[2]; e.color.a = doorsOpen[2] ? 30 : 255; }
                    if (e.HasTag(TAG_DOOR_5)) { e.isSolid = !doorsOpen[3]; e.color.a = doorsOpen[3] ? 30 : 255; }
                    if (e.HasTag(TAG_DOOR_4)) { e.isSolid = !doorsOpen[4]; e.color.a = doorsOpen[4] ? 30 : 255; }
                }
                
                if (showInteractMenu) {
                    if (IsKeyPressed(KEY_W) || IsKeyPressed(KEY_UP)) interactSelectedIndex = (interactSelectedIndex - 1 + interactTargets.size()) % interactTargets.size();
                    if (IsKeyPressed(KEY_S) || IsKeyPressed(KEY_DOWN)) interactSelectedIndex = (interactSelectedIndex + 1) % interactTargets.size();
                    bool clicked = false;
                    Vector2 mousePos = GetMousePosition();
                    float menuW = 400; float itemH = 50; float menuH = 60 + interactTargets.size() * itemH;
                    float menuX = GetScreenWidth() / 2.0f - menuW / 2.0f; float menuY = GetScreenHeight() / 2.0f - menuH / 2.0f;

                    for (size_t i = 0; i < interactTargets.size(); ++i) {
                        if (CheckCollisionPointRec(mousePos, { menuX + 20, menuY + 60 + i * itemH, menuW - 40, itemH - 10 })) {
                            if (Vector2Length(GetMouseDelta()) > 0.1f) interactSelectedIndex = i; 
                            if (IsMouseButtonReleased(MOUSE_LEFT_BUTTON)) clicked = true;
                        }
                    }
                    if (IsKeyPressed(KEY_SPACE) || IsKeyPressed(KEY_ENTER) || clicked) {
                        executeAction = true; actionTargetIdx = interactTargets[interactSelectedIndex]; showInteractMenu = false;
                    }
                    if (IsKeyPressed(KEY_E) || IsKeyPressed(KEY_ESCAPE)) showInteractMenu = false;
                } 
            }

            if (!showInteractMenu && !player.isStone && !player.isDead) {
                Vector3 input = { 0.0f, 0.0f, 0.0f };
                float baseAccel = 6000.0f; 
                float maxSpeed = isSprinting ? 1200.0f : 300.0f; 
                float currentAccel = isSprinting ? baseAccel * 1.5f : baseAccel;

                if (grabbedEntityIndex != -1 && entities[grabbedEntityIndex].HasTag(TAG_HEAVY)) { currentAccel = 3000.0f; maxSpeed = 200.0f; }

                if (IsKeyDown(KEY_W)) input.z -= 1.0f; if (IsKeyDown(KEY_S)) input.z += 1.0f;
                if (IsKeyDown(KEY_A)) input.x -= 1.0f; if (IsKeyDown(KEY_D)) input.x += 1.0f;
                
                if (Vector3Length(input) > 0.0f) {
                    input = Vector3Normalize(input); player.facingDir = input;
                    player.velocity.x += input.x * currentAccel * dt; 
                    player.velocity.z += input.z * currentAccel * dt;
                    player.stateValue = atan2(player.facingDir.x, player.facingDir.z) * RAD2DEG;
                }

                Vector2 horizVel = { player.velocity.x, player.velocity.z };
                if (Vector2Length(horizVel) > maxSpeed) {
                    horizVel = Vector2Scale(Vector2Normalize(horizVel), maxSpeed);
                    player.velocity.x = horizVel.x; player.velocity.z = horizVel.y;
                }

                if (IsKeyPressed(KEY_SPACE) && player.velocity.y == 0.0f && grabbedEntityIndex == -1) {
                    player.velocity.y = 800.0f; 
                    playerJumpedThisFrame = true;
                }
                
                if (grabbedEntityIndex != -1) {
                    Entity& held = entities[grabbedEntityIndex];
                    held.isUsing = IsKeyDown(KEY_F);
                    
                    if (held.HasTag(TAG_EXTINGUISHER) && held.isUsing && !isOverlayActive) {
                        if (!IsSoundPlaying(sounds["fire-extenguisher"])) PlaySound(sounds["fire-extenguisher"]);
                    }
                }

                if (IsKeyPressed(KEY_F)) {
                    bool environmentF = false;

                    if (grabbedEntityIndex != -1) {
                        std::string hName = entities[grabbedEntityIndex].name;
                        if (hName.find("Handbook") != std::string::npos || hName.find("Brochure") != std::string::npos || 
                            hName.find("Open Book") != std::string::npos || hName.find("Magazine") != std::string::npos) {
                            isOverlayActive = true;
                            if (currentState == STATE_TUTORIAL) PauseMusicStream(tutorialMusic);
                            PlayMusicStream(newsMusic); 
                            environmentF = true;
                        }
                    }

                    if (!environmentF) {
                        std::vector<BoundingBox> pIntBoxList = player.GetWorldInteractBounds(); 
                        for (auto& b : pIntBoxList) {
                            float reach = 60.0f; b.min.x += player.facingDir.x * reach; b.max.x += player.facingDir.x * reach;
                            b.min.z += player.facingDir.z * reach; b.max.z += player.facingDir.z * reach;
                            b.min.x -= 50; b.max.x += 50; b.min.z -= 50; b.max.z += 50; b.min.y -= 20; b.max.y += 120; 
                        }
                        
                        for (auto& e : entities) {
                            if ((e.HasTag(TAG_FUSEBOX) || e.HasTag(TAG_LIGHTSWITCH)) && CheckCollisionLists(pIntBoxList, e.GetWorldInteractBounds())) {
                                e.stateValue = (e.stateValue > 0.5f) ? 0.0f : 1.0f; environmentF = true; break;
                            }
                        }
                        if (!environmentF && grabbedEntityIndex != -1) {
                            for (size_t i = 1; i < entities.size(); ++i) {
                                if (i != grabbedEntityIndex && CheckCollisionLists(pIntBoxList, entities[i].GetWorldInteractBounds())) {
                                    ChemResult res = ProcessChemistry(grabbedEntityIndex, i, entities);
                                    if (res != CHEM_NONE) {
                                        environmentF = true;
                                        if (res == CHEM_ATTACHED) { entities[grabbedEntityIndex].isGrabbed = false; grabbedEntityIndex = -1; }
                                        break;
                                    }
                                }
                            }
                        }
                        if (!environmentF) {
                            if (grabbedEntityIndex != -1) {
                                if (entities[grabbedEntityIndex].HasTag(TAG_EYEWEAR) && !entities[grabbedEntityIndex].HasTag(TAG_BROKEN)) { equippedEyewear = grabbedEntityIndex; entities[grabbedEntityIndex].isGrabbed = false; grabbedEntityIndex = -1; }
                                else if (entities[grabbedEntityIndex].HasTag(TAG_GLOVES)) { equippedGloves = grabbedEntityIndex; entities[grabbedEntityIndex].isGrabbed = false; grabbedEntityIndex = -1; }
                            } else {
                                if (equippedGloves != -1) { grabbedEntityIndex = equippedGloves; entities[equippedGloves].isGrabbed = true; equippedGloves = -1; }
                                else if (equippedEyewear != -1) { grabbedEntityIndex = equippedEyewear; entities[equippedEyewear].isGrabbed = true; equippedEyewear = -1; }
                            }
                        }
                    }
                }
                
                if (IsKeyPressed(KEY_F) && grabbedEntityIndex != -1 && !isOverlayActive) {
                    Entity& held = entities[grabbedEntityIndex];
                    if (held.HasTag(TAG_HOLE_SAW)) {
                        PlaySound(sounds["handsaw"]);
                        BoundingBox holeBox = {{-30, 0, -30}, {30, 5, 30}};
                        Entity hole = MakeProp("Floor Hole", {player.position.x + player.facingDir.x * 70.0f, 0.0f, player.position.z + player.facingDir.z * 70.0f}, holeBox, BLACK, {TAG_HOLE});
                        hole.isSolid = false; hole.canGrab = false; entities.push_back(hole);
                    }
                    if (held.HasTag(TAG_BUBBLE_WRAP) && held.stateValue > 0) {
                        PlaySound(sounds["popping-bubble-wr"]);
                        held.stateValue -= 1.0f; BoundingBox matBox = {{-50, 0, -50}, {50, 5, 50}};
                        Entity mat = MakeProp("Bubble Mat", player.position, matBox, SKYBLUE, {TAG_MAT});
                        mat.isSolid = false; mat.canGrab = false; entities.push_back(mat);
                    }
                    if (held.HasTag(TAG_TAPE) && held.stateValue > 0) {
                        for (auto& target : entities) {
                            if (target.HasTag(TAG_WIND_BAG) && Vector3Distance(player.position, target.position) < 80.0f) { PlaySound(sounds["ducktape"]); target.AddTag(TAG_BROKEN); target.isGlitching = false; held.stateValue -= 1.0f; }
                            if (target.HasTag(TAG_MUMMY) && target.isGlitching && Vector3Distance(player.position, target.position) < 80.0f) { PlaySound(sounds["ducktape"]); target.AddTag(TAG_BROKEN); target.isGlitching = false; target.velocity = {0, 0, 0}; target.color = LIGHTGRAY; held.stateValue -= 1.0f; }
                        }
                    }
                }
                
                if (IsKeyPressed(KEY_E)) {
                    interactTargets.clear();
                    if (grabbedEntityIndex != -1) {
                        Entity& item = entities[grabbedEntityIndex];
                        
                        Vector3 dropPos = { player.position.x + player.facingDir.x * 50.0f, player.position.y + 100.0f, player.position.z + player.facingDir.z * 50.0f };
                        
                        std::vector<BoundingBox> dropColList = item.boundsList; 
                        for (auto& b : dropColList) { 
                            b.min.x += dropPos.x - 15.0f; 
                            b.max.x += dropPos.x + 15.0f; 
                            b.min.y = -1000.0f; 
                            b.max.y += dropPos.y; 
                            b.min.z += dropPos.z - 15.0f; 
                            b.max.z += dropPos.z + 15.0f; 
                        }
                        
                        for(size_t i = 0; i < entities.size(); ++i) {
                            if(i != grabbedEntityIndex && CheckCollisionLists(dropColList, entities[i].GetWorldInteractBounds())) {
                                bool isStand = entities[i].name.find("stand") != std::string::npos;
                                bool isGlitchingSandal = item.HasTag(TAG_SANDALS) && item.isGlitching;

                                if ((isStand && !isGlitchingSandal) || entities[i].HasTag(TAG_ZEUS) || (entities[i].HasTag(TAG_FUSEBOX) && entities[i].stateValue > 0.5f && item.HasTag(TAG_ELECTRIC))) { 
                                    interactTargets.push_back(i); 
                                }                                
                                else if (CanProcessChemistry(grabbedEntityIndex, i, entities) != CHEM_NONE) { interactTargets.push_back(i); }
                            }
                        }

                        if (interactTargets.empty()) {
                            float targetY = 0.0f;
                            for(size_t i = 0; i < entities.size(); ++i) {
                                if(i != grabbedEntityIndex && entities[i].isSolid && CheckCollisionLists(dropColList, entities[i].GetWorldBounds())) {
                                    for (const auto& b : entities[i].GetWorldBounds()) { 
                                        if (b.max.y > targetY && b.max.y < player.position.y + 150.0f) {
                                            targetY = b.max.y; 
                                        }
                                    }
                                }
                            }
                            
                            float itemLocalMinY = 0.0f;
                            if (!item.boundsList.empty()) itemLocalMinY = item.boundsList[0].min.y;
                            
                            item.position = dropPos; 
                            item.position.y = targetY - itemLocalMinY; 
                            
                            if (item.HasTag(TAG_SANDALS)) item.stateTimer = 4.0f;
                            if (item.name.find("sign") != std::string::npos) PlaySound(sounds["plastictap-wetfloors"]);
                            if (item.HasTag(TAG_SANDBAG)) PlaySound(sounds["sandbags-put-on-fl"]);
                            item.isGrabbed = false; item.isUsing = false; item.velocity = {0,0,0}; grabbedEntityIndex = -1;
                        } else if (interactTargets.size() == 1) { executeAction = true; actionTargetIdx = interactTargets[0]; isDropMenu = true; } 
                        else { showInteractMenu = true; isDropMenu = true; interactSelectedIndex = 0; }
                    } else {
                        std::vector<BoundingBox> pIntBoxList = player.GetWorldInteractBounds(); 
                        for (auto& b : pIntBoxList) { float reach = 60.0f; b.min.x += player.facingDir.x * reach; b.max.x += player.facingDir.x * reach; b.min.z += player.facingDir.z * reach; b.max.z += player.facingDir.z * reach; b.min.x -= 50; b.max.x += 50; b.min.z -= 50; b.max.z += 50; b.min.y -= 20; b.max.y += 120; }
                        for (size_t i = 1; i < entities.size(); ++i) {
                            if (i == equippedEyewear || i == equippedGloves) continue; 
                            if (!entities[i].isGrabbed && entities[i].canGrab && CheckCollisionLists(pIntBoxList, entities[i].GetWorldInteractBounds())) interactTargets.push_back(i);
                        }
                        std::sort(interactTargets.begin(), interactTargets.end(), [&](int a, int b) { return Vector3Distance(player.position, entities[a].position) < Vector3Distance(player.position, entities[b].position); });

                        if (interactTargets.size() == 1) { executeAction = true; actionTargetIdx = interactTargets[0]; isDropMenu = false; } 
                        else if (interactTargets.size() > 1) { showInteractMenu = true; isDropMenu = false; interactSelectedIndex = 0; }
                    }
                }

                if (IsKeyPressed(KEY_SPACE) && grabbedEntityIndex != -1 && entities[grabbedEntityIndex].canThrow) {
                    Entity& held = entities[grabbedEntityIndex];
                    held.isGrabbed = false; held.isUsing = false; 
                    float throwStrength = 2200.0f; float upwardArc = 800.0f;    
                    held.velocity = Vector3Scale(player.facingDir, throwStrength); held.velocity.y = upwardArc; 
                    if (held.HasTag(TAG_SANDALS)) held.isGlitching = true;
                    grabbedEntityIndex = -1;
                }
            }

            if (executeAction) {
                if (!isDropMenu) {
                    grabbedEntityIndex = actionTargetIdx; entities[grabbedEntityIndex].isGrabbed = true; entities[grabbedEntityIndex].attachedTo = -1;
                } else {
                    Entity& item = entities[grabbedEntityIndex];
                    ChemResult res = ProcessChemistry(grabbedEntityIndex, actionTargetIdx, entities);
                    if (res != CHEM_NONE) { if (res == CHEM_ATTACHED) { item.attachedTo = actionTargetIdx; item.isGrabbed = false; item.isUsing = false; item.velocity = {0,0,0}; grabbedEntityIndex = -1; } } 
                    else { item.attachedTo = actionTargetIdx; item.isGrabbed = false; item.isUsing = false; item.velocity = {0,0,0}; grabbedEntityIndex = -1; }
                }
            }

            int targetAnimState = 0; 
            float speed = Vector2Length({player.velocity.x, player.velocity.z});
            if (abs(player.velocity.y) > 5.0f) targetAnimState = 3; 
            else if (speed > 20.0f) targetAnimState = isSprinting ? 2 : 1; 
            else if (IsKeyDown(KEY_F) || IsKeyDown(KEY_E)) targetAnimState = 4; 

            if (currentAnimState != targetAnimState) { currentAnimState = targetAnimState; animTimer = 0.0f; }

            int activeIdx = idxIdle;
            if (currentAnimState == 1) activeIdx = idxWalk;
            else if (currentAnimState == 2) activeIdx = idxRun;
            else if (currentAnimState == 3) activeIdx = idxRoll;
            else if (currentAnimState == 4) activeIdx = idxInteract;

            if (anims != nullptr && activeIdx < animCount) {
                float playbackSpeed = 45.0f; 
                if (currentAnimState == 2) playbackSpeed = 60.0f; 
                if (currentAnimState == 1) playbackSpeed = 50.0f; 
                if (currentAnimState == 3) playbackSpeed = 90.0f; 

                animTimer += dt * playbackSpeed; 
                animTimer = fmod(animTimer, (float)anims[activeIdx].frameCount); 
                UpdateModelAnimation(models["Player"], anims[activeIdx], (int)animTimer);
            }

            if (mummyAnims != nullptr && mummyAnimCount > 0) {
                mummyAnimTimer += dt * 30.0f; 
                mummyAnimTimer = fmod(mummyAnimTimer, (float)mummyAnims[0].frameCount);
                UpdateModelAnimation(models["mummy"], mummyAnims[0], (int)mummyAnimTimer);
            }

            hazVis = UpdatePhysicsAndHazards(entities, dt, grabbedEntityIndex, equippedEyewear, equippedGloves, currentNight, shiftTimer);
            
            if (currentState == STATE_PLAYING || currentState == STATE_TUTORIAL) {
                if (player.isStone || player.isDead) {
                    lastReport = EvaluateMuseum(entities); 
                    currentState = STATE_GAMEOVER_DEATH;
                    PlayMusicStream(deathMusic); 
                }
            }
        } 

        // --- UPDATE LIGHTS TO SHADER ---
        float positions[16 * 3] = {0};
        float colors[16 * 3] = {0};
        float radii[16] = {0};
        int count = std::min((int)pointLights.size(), 16);

        for (int i = 0; i < count; i++) {
            positions[i*3] = pointLights[i].position.x;
            positions[i*3+1] = pointLights[i].position.y;
            positions[i*3+2] = pointLights[i].position.z;
            
            colors[i*3] = pointLights[i].color.x;
            colors[i*3+1] = pointLights[i].color.y;
            colors[i*3+2] = pointLights[i].color.z;
            
            radii[i] = pointLights[i].radius;
        }

        SetShaderValue(clayShader, lightCountLoc, &count, SHADER_UNIFORM_INT);
        SetShaderValueV(clayShader, lightPosLoc, positions, SHADER_UNIFORM_VEC3, count);
        SetShaderValueV(clayShader, lightColLoc, colors, SHADER_UNIFORM_VEC3, count);
        SetShaderValueV(clayShader, lightRadLoc, radii, SHADER_UNIFORM_FLOAT, count);

        RenderWorld(renderTarget, camera, dt, entities, player, grabbedEntityIndex, models, hazVis);
        
        BeginDrawing(); 
        ClearBackground(BLACK);
        float scale = std::min((float)GetScreenWidth() / GAME_WIDTH, (float)GetScreenHeight() / GAME_HEIGHT);
        Rectangle sourceRec = { 0.0f, 0.0f, (float)renderTarget.texture.width, (float)-renderTarget.texture.height };
        Rectangle destRec = { (GetScreenWidth() - ((float)GAME_WIDTH * scale)) * 0.5f, (GetScreenHeight() - ((float)GAME_HEIGHT * scale)) * 0.5f, (float)GAME_WIDTH * scale, (float)GAME_HEIGHT * scale };
        
        // ==============================================================
        // --- DRAW THE 3D WORLD WITH THE POST-PROCESSING SHADER ---
        // ==============================================================
        if (currentState != STATE_MENU && currentState != STATE_OPTIONS) {
            BeginShaderMode(postShader);
        }
        
        DrawTexturePro(renderTarget.texture, sourceRec, destRec, { 0, 0 }, 0.0f, WHITE);
        
        if (currentState != STATE_MENU && currentState != STATE_OPTIONS) {
            EndShaderMode();
        }
        // ==============================================================

        // The HUD and menus are drawn AFTER the shader mode is ended, so they remain unaffected!
        if (currentState == STATE_MENU || currentState == STATE_OPTIONS) {
            UpdateAndRenderMenu((int&)currentState, mainMusicVolume, triggerShiftStart, fontMuseum, fontEmployee); 
            SetMusicVolume(mainMusic, mainMusicVolume); SetMusicVolume(tutorialMusic, mainMusicVolume);
        } else {
            RenderHUD(renderTarget, shiftTimer, SECONDS_PER_HOUR, currentNight, player, showInteractMenu, isDropMenu, interactTargets, interactSelectedIndex, entities, hazVis);
            
            if (currentState == STATE_TUTORIAL && !isOverlayActive) {
                bool tutorialFinished = false;
                UpdateAndRenderTutorial((int&)currentState, entities, grabbedEntityIndex, shiftTimer, tutorialFinished, tutorialStep, playerJumpedThisFrame);
                if (tutorialFinished) { doorsOpen[0] = true; StopMusicStream(tutorialMusic); PlayMusicStream(mainMusic); }
            }

            if (isOverlayActive && grabbedEntityIndex != -1) {
                // We now pass both the name AND the stateValue (which holds the ID)
                UpdateAndRenderOverlay(isOverlayActive, tutorialMusic, newsMusic, tutorialStep, entities[grabbedEntityIndex].name, entities[grabbedEntityIndex].stateValue);            }
        }
        EndDrawing();
        
        if (triggerShiftStart) {
            StopMusicStream(tutorialMusic);
            ResetNight(); 
            if (currentNight == 1) { currentState = STATE_TUTORIAL; PlayMusicStream(tutorialMusic); } 
            else { currentState = STATE_PLAYING; PlayMusicStream(mainMusic); }
            continue; 
        }
    }

    if (anims) UnloadModelAnimations(anims, animCount);
    if (mummyAnims) UnloadModelAnimations(mummyAnims, mummyAnimCount);
    UnloadTexture(stoneTex);
    UnloadTexture(zombieTex);
    
    // Clean up the shader programs from the GPU
    UnloadShader(clayShader);
    UnloadShader(postShader);
    
    for (auto& pair : models) { UnloadModel(pair.second); }

    if (isMainMusicLoaded) UnloadMusicStream(mainMusic);
    UnloadMusicStream(tutorialMusic); UnloadMusicStream(deathMusic); UnloadMusicStream(newsMusic); UnloadMusicStream(reviewsMusic); for (auto& pair : sounds) UnloadSound(pair.second); CloseAudioDevice();
    UnloadRenderTexture(renderTarget); 
    UnloadFont(fontMuseum); UnloadFont(fontEmployee);
    CloseWindow(); 
    return 0;
}