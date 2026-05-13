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
#include "Credits.h"
#include "Review.h"
#include "News.h"
#include <unordered_map>
#include <string>
#include <cstdio>

const int idxIdle = 4;
const int idxInteract = 10;
const int idxRoll = 15;
const int idxRun = 24;
const int idxWalk = 30;

enum GameState { STATE_MENU, STATE_OPTIONS, STATE_TUTORIAL, STATE_PLAYING, STATE_REVIEW, STATE_GAMEOVER_FIRED, STATE_GAMEOVER_DEATH, STATE_CREDITS };

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
    if (currentNight > 1) doorsOpen[0] = true;
    if (currentNight >= 2) doorsOpen[1] = true;
    if (currentNight >= 3) doorsOpen[2] = true;
    if (currentNight >= 4) doorsOpen[4] = true;
    if (currentNight >= 5) doorsOpen[3] = true;

    if (currentNight > 1) LoadGame(currentNight, entities); 
    SetupNightHazards(currentNight, entities); 

    if (isMainMusicLoaded) UnloadMusicStream(mainMusic);    
    int trackNum = std::min(currentNight, 5); 
    mainMusic = LoadMusicStream(TextFormat("resources/music/main_theme%d.ogg", trackNum));
    isMainMusicLoaded = true;

    mainMusic.looping = false; 
    SetMusicVolume(mainMusic, mainMusicVolume);
}
bool triggerNextNight = false;
bool triggerRetryNight = false;
int main(void) {
    SetConfigFlags(FLAG_WINDOW_RESIZABLE);
    SetTraceLogLevel(LOG_WARNING);
    InitWindow(1280, 960, "Mythic Maintenance");

    Font fontMuseum = LoadFontEx("resources/fonts/Playfair_Display/static/PlayfairDisplay-Bold.ttf", 250, 0, 0);
    Font fontEmployee = LoadFontEx("resources/fonts/Courier_Prime/CourierPrime-Bold.ttf", 250, 0, 0);

    SetTextureFilter(fontMuseum.texture, TEXTURE_FILTER_BILINEAR);
    SetTextureFilter(fontEmployee.texture, TEXTURE_FILTER_BILINEAR);
    
    BeginDrawing();
    ClearBackground({15, 15, 20, 255}); // Match your sleek UI background
    const char* loadMsg = "LOADING RESOURCES...";
    Vector2 loadVec = MeasureTextEx(fontMuseum, loadMsg, 40, 1);
    DrawTextEx(fontMuseum, loadMsg, { GetScreenWidth()/2.0f - loadVec.x/2.0f, GetScreenHeight()/2.0f - loadVec.y/2.0f }, 40, 1, {218, 165, 32, 255});
    EndDrawing();
    // ----------------------------------------------

    currentNight = 1; 
    std::ifstream saveFile("museum_save.dat", std::ios::binary);
    if (saveFile.is_open()) {
        saveFile.read((char*)&currentNight, sizeof(currentNight));
        saveFile.close();
    }

    InitAudioDevice();
    const char* sfxNames[] = {
        "breaking-vase", "ducktape", "electric_shock", "fire-extenguisher",
        "handsaw", "light-switch-click-on-and-off", "mjolnir-clang", "petrification",
        "plastictap-wetfloorsign", "popping-bubble-wrap", "poseidon-water",
        "sandbags-put-on-floor", "scream", "shulffing-paper", "sisphus-rolling",
        "sound-of-wind", "wing-flap"
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
        "zeus", "medusa", "lightning", "pandora", "mummy", "Info Button", "Magazine", "Open Book", "Sticker", "Cork", "Snake", "Coffin", "sarcophagus", "Viking Boat", "Shield Round", "Fuse Box"
    
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
    // --- NEW FIX: GENERATE YELLOW TEXTURE ---
    Image yellowImg = GenImageColor(2, 2, {255, 220, 0, 255}); // A nice electric yellow
    Texture2D yellowTex = LoadTextureFromImage(yellowImg);
    UnloadImage(yellowImg);
    
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
    if (models.count("lightning")) {
        for (int i = 0; i < models["lightning"].materialCount; i++) models["lightning"].materials[i].maps[MATERIAL_MAP_DIFFUSE].texture = yellowTex;
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
    InitOverlayAssets();

    RenderTexture2D renderTarget = LoadRenderTexture(GAME_WIDTH, GAME_HEIGHT);
    SetTextureFilter(renderTarget.texture, TEXTURE_FILTER_BILINEAR);

    InitLevel(entities);
    if (!LoadGame(currentNight, entities)) currentNight = 1; 
    camera.zoom = 1.0f;

    GoToMenu();

    float curVolMain = 0.0f, curVolTut = 0.0f, curVolDeath = 0.0f, curVolNews = 0.0f, curVolRev = 0.0f;
    bool mainMusicPaused = false;

    while (!WindowShouldClose()) {
        float dt = GetFrameTime();

        // --- GLOBAL AUDIO FADE SYSTEM ---
        float targetMain = 0.0f, targetTut = 0.0f, targetDeath = 0.0f, targetNews = 0.0f, targetRev = 0.0f;
        
        if (currentState == STATE_MENU || currentState == STATE_OPTIONS || currentState == STATE_TUTORIAL || currentState == STATE_CREDITS) {
            targetTut = mainMusicVolume;
        } else if (currentState == STATE_REVIEW) {
            targetRev = mainMusicVolume;
        } else if (currentState == STATE_GAMEOVER_FIRED) {
            targetNews = mainMusicVolume;
        } else if (currentState == STATE_GAMEOVER_DEATH) {
            targetDeath = mainMusicVolume;
        } else if (currentState == STATE_PLAYING) {
            if (isOverlayActive) targetTut = mainMusicVolume; // Chill music during handbook/overlays!
            else targetMain = mainMusicVolume;                // Spooky music during active gameplay!
        }

        auto HandleFade = [&](Music& m, float& curVol, float targetVol) {
            if (targetVol > 0.0f && !IsMusicStreamPlaying(m)) PlayMusicStream(m);
            curVol += (targetVol - curVol) * dt * 2.0f; // Smooth 2.0x crossfade
            if (curVol < 0.01f) curVol = 0.0f;
            SetMusicVolume(m, curVol);
        };

        if (isMainMusicLoaded) {
            HandleFade(mainMusic, curVolMain, targetMain);
            // Physically pause the level music if the volume hits zero so it doesn't advance
            if (targetMain == 0.0f && curVolMain < 0.05f && !mainMusicPaused) { PauseMusicStream(mainMusic); mainMusicPaused = true; }
            else if (targetMain > 0.0f && mainMusicPaused) { ResumeMusicStream(mainMusic); mainMusicPaused = false; }
        }
        
        HandleFade(tutorialMusic, curVolTut, targetTut);
        HandleFade(deathMusic, curVolDeath, targetDeath);
        HandleFade(newsMusic, curVolNews, targetNews);
        HandleFade(reviewsMusic, curVolRev, targetRev);
        // --------------------------------

        if (isMainMusicLoaded) UpdateMusicStream(mainMusic);
        UpdateMusicStream(tutorialMusic);
        UpdateMusicStream(deathMusic);
        UpdateMusicStream(newsMusic);
        UpdateMusicStream(reviewsMusic);
        
        // --- NEW FIX: The Social Media Review Hook ---
        if (currentState == STATE_REVIEW) {
            UpdateAndRenderReview((int&)currentState, lastReport, triggerNextNight, fontMuseum, fontEmployee);
            
            // If the user hit ENTER inside the review screen
            if (triggerNextNight) {
                SaveGame(currentNight, entities); 
                ResetNight(); 
                currentState = STATE_PLAYING; 
                PlayMusicStream(mainMusic); 
                triggerNextNight = false; // Reset the flag
            }
            continue; // Skip the rest of the 3D game loop
        }

        // --- NEW FIX: The Newspaper Headline Hook ---
        if (currentState == STATE_GAMEOVER_FIRED) {
            UpdateAndRenderNews((int&)currentState, lastReport, triggerRetryNight, fontMuseum, fontEmployee);
            
            // If the user hit ENTER inside the news screen
            if (triggerRetryNight) {
                StopMusicStream(newsMusic); 
                ResetNight(); 
                currentState = STATE_PLAYING; 
                PlayMusicStream(mainMusic); 
                triggerRetryNight = false; // Reset the flag
            }
            continue; // Skip the rest of the 3D game loop
        }

        // --- NEW FIX: The Cinematic Death Screen ---
        if (currentState == STATE_GAMEOVER_DEATH) {
            // A local static timer to handle the fade-in effect
            static float deathFade = 0.0f;
            deathFade += dt * 0.5f; // Takes 2 seconds to fully fade in
            if (deathFade > 1.0f) deathFade = 1.0f;

            BeginDrawing(); 
            ClearBackground(BLACK); // Fade to pitch black
            
            float screenW = (float)GetScreenWidth();
            float screenH = (float)GetScreenHeight();
            float scale = std::min(screenW / 1280.0f, screenH / 960.0f);

            // --- "YOU DIED" TEXT ---
            const char* title = "YOU DIED";
            float titleSize = 120.0f * scale;
            Vector2 titleDims = MeasureTextEx(fontMuseum, title, titleSize, 2);
            // Fade into a dark, blood red
            DrawTextEx(fontMuseum, title, {screenW/2.0f - titleDims.x/2.0f, screenH * 0.25f}, titleSize, 2, Fade(MAROON, deathFade));

            // --- FINAL VERDICT ---
            float verdictSize = 30.0f * scale;
            Vector2 verdictDims = MeasureTextEx(fontEmployee, lastReport.finalVerdict.c_str(), verdictSize, 1);
            DrawTextEx(fontEmployee, lastReport.finalVerdict.c_str(), {screenW/2.0f - verdictDims.x/2.0f, screenH * 0.45f}, verdictSize, 1, Fade(GRAY, deathFade));

            // --- AFTERLIFE MESSAGE (Cause of Death) ---
            if (!lastReport.reviews.empty()) {
                std::string afterlifeMsg = "\"" + lastReport.reviews[0].reviewText + "\" \n\n- " + lastReport.reviews[0].artifactName;
                float msgSize = 25.0f * scale;
                Vector2 msgDims = MeasureTextEx(fontEmployee, afterlifeMsg.c_str(), msgSize, 1);
                DrawTextEx(fontEmployee, afterlifeMsg.c_str(), {screenW/2.0f - msgDims.x/2.0f, screenH * 0.55f}, msgSize, 1, Fade(LIGHTGRAY, deathFade));
            }

            // --- FOOTER PROMPT ---
            const char* enterText = "PRESS ENTER TO ACCEPT YOUR FATE";
            float enterSize = 25.0f * scale;
            Vector2 enterDims = MeasureTextEx(fontEmployee, enterText, enterSize, 1);
            
            // Start blinking only after the fade is mostly complete
            if (deathFade > 0.8f && (int)(GetTime() * 2) % 2 == 0) {
                DrawTextEx(fontEmployee, enterText, {screenW / 2.0f - enterDims.x / 2.0f, screenH * 0.85f}, enterSize, 1, DARKGRAY);
            }

            if (IsKeyPressed(KEY_ENTER)) { 
                StopMusicStream(deathMusic); 
                ResetNight(); 
                currentState = STATE_PLAYING; 
                PlayMusicStream(mainMusic); 
                deathFade = 0.0f; // Reset the fade for the next time you die!
            }
            
            EndDrawing(); 
            continue; // Skip the 3D drawing pipeline
        }

        Entity& player = entities[0];
        bool playerJumpedThisFrame = false; 
        bool triggerShiftStart = false;
        bool triggerNewGame = false;
        bool triggerNextNight = false;
        HazardVisuals hazVis;

        if (!isOverlayActive) {
            
            if (currentState == STATE_PLAYING) {
                shiftTimer += dt;
                if (shiftTimer >= SHIFT_DURATION) {
                    lastReport = EvaluateMuseum(entities, currentNight);
                    if (lastReport.isFired) { currentState = STATE_GAMEOVER_FIRED; PlayMusicStream(newsMusic); } 
                    else { currentNight++; currentState = STATE_REVIEW; PlayMusicStream(reviewsMusic); }
                }
            }

            bool isSprinting = IsKeyDown(KEY_LEFT_SHIFT);
            bool executeAction = false;
            int actionTargetIdx = -1;

            if (currentState == STATE_PLAYING || currentState == STATE_TUTORIAL) {

                if (IsKeyPressed(KEY_Q) && currentNight >= 5 && grabbedEntityIndex == -1 && !isOverlayActive) {
                    for (auto& e : entities) {
                        if (e.HasTag(TAG_PANDORA)) {
                            // Only check X/Z distance so pedestal height doesn't mess it up!
                            float distXZ = Vector2Distance({player.position.x, player.position.z}, {e.position.x, e.position.z});
                            if (distXZ < 120.0f) {
                                currentState = STATE_CREDITS;
                            }
                            break;
                        }
                    }
                }

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
                    
                    // --- DYNAMIC SCALING FOR MOUSE CLICKS ---
                    float screenW = (float)GetScreenWidth();
                    float screenH = (float)GetScreenHeight();
                    float scale = std::min(screenW / 1280.0f, screenH / 960.0f);
                    
                    float menuW = 400.0f * scale; 
                    float itemH = 60.0f * scale; 
                    float menuH = (80.0f * scale) + (interactTargets.size() * itemH);
                    float menuX = screenW / 2.0f - menuW / 2.0f; 
                    float menuY = screenH / 2.0f - menuH / 2.0f;

                    for (size_t i = 0; i < interactTargets.size(); ++i) {
                        Rectangle itemRec = { menuX + (20.0f * scale), menuY + (80.0f * scale) + i * itemH, menuW - (40.0f * scale), itemH - (10.0f * scale) };
                        if (CheckCollisionPointRec(mousePos, itemRec)) {
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
                    
                    if (held.HasTag(TAG_EXTINGUISHER)) {
                        // Play the sound if the player is holding 'F' and no menu is open
                        if (held.isUsing && !isOverlayActive) {
                            if (!IsSoundPlaying(sounds["fire-extenguisher"])) {
                                PlaySound(sounds["fire-extenguisher"]);
                            }
                        } 
                        // Stop the sound the moment they let go of 'F' or an overlay opens
                        else {
                            if (IsSoundPlaying(sounds["fire-extenguisher"])) {
                                StopSound(sounds["fire-extenguisher"]);
                            }
                        }
                    }
                } else {
                    // Failsafe: Stop the sound instantly if the player drops/throws the extinguisher
                    if (IsSoundPlaying(sounds["fire-extenguisher"])) {
                        StopSound(sounds["fire-extenguisher"]);
                    }
                }

                if (IsKeyPressed(KEY_F)) {
                    bool environmentF = false;

                    if (grabbedEntityIndex != -1) {
                        std::string hName = entities[grabbedEntityIndex].name;
                        if (hName.find("Handbook") != std::string::npos || hName.find("Brochure") != std::string::npos || 
                            hName.find("Open Book") != std::string::npos || hName.find("Magazine") != std::string::npos) {
                            isOverlayActive = true;
                            environmentF = true;
                            PlaySound(sounds["shulffing-paper"]);
                        }
                    }

                    if (!environmentF) {
                        std::vector<BoundingBox> pIntBoxList = player.GetWorldInteractBounds(); 
                        for (auto& b : pIntBoxList) {
                            float reach = 100.0f; b.min.x += player.facingDir.x * reach; b.max.x += player.facingDir.x * reach;
                            b.min.z += player.facingDir.z * reach; b.max.z += player.facingDir.z * reach;
                            b.min.x -= 50; b.max.x += 50; b.min.z -= 50; b.max.z += 50; b.min.y -= 20; b.max.y += 500; 
                        }
                        
                        for (auto& e : entities) {
                            if ((e.HasTag(TAG_FUSEBOX) || e.HasTag(TAG_LIGHTSWITCH)) && CheckCollisionLists(pIntBoxList, e.GetWorldInteractBounds())) {
                                e.stateValue = (e.stateValue > 0.5f) ? 0.0f : 1.0f; environmentF = true; PlaySound(sounds["light-switch-click-on-and-off"]); break;
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
                    if (held.HasTag(TAG_TAPE) && held.stateTimer > 0) {
                        for (auto& target : entities) {
                            if (target.HasTag(TAG_WIND_BAG) && Vector3Distance(player.position, target.position) < 80.0f) { PlaySound(sounds["ducktape"]); target.AddTag(TAG_BROKEN); target.isGlitching = false; held.stateTimer -= 1.0f; }
                            if (target.HasTag(TAG_MUMMY) && target.isGlitching && Vector3Distance(player.position, target.position) < 80.0f) { PlaySound(sounds["ducktape"]); target.AddTag(TAG_BROKEN); target.isGlitching = false; target.velocity = {0, 0, 0}; target.color = LIGHTGRAY; held.stateTimer -= 1.0f; }
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
                            b.max.y += dropPos.y+100.0f; 
                            b.min.z += dropPos.z - 15.0f; 
                            b.max.z += dropPos.z + 15.0f; 
                        }
                        
                        for(size_t i = 0; i < entities.size(); ++i) {
                            if(i != grabbedEntityIndex && CheckCollisionLists(dropColList, entities[i].GetWorldInteractBounds())) {
                                bool isStand = entities[i].name.find("stand") != std::string::npos;
                                bool isGlitchingSandal = item.HasTag(TAG_SANDALS) && item.isGlitching;

                                // --- NEW FIX: Check if the stand already has an item attached ---
                                bool standOccupied = false;
                                if (isStand) {
                                    for (size_t j = 0; j < entities.size(); ++j) {
                                        if (entities[j].attachedTo == (int)i) { 
                                            standOccupied = true; 
                                            break; 
                                        }
                                    }
                                }

                                // --- NEW FIX: Add !standOccupied to the if statement ---
                                if ((isStand && !isGlitchingSandal && !standOccupied) || entities[i].HasTag(TAG_ZEUS) || (entities[i].HasTag(TAG_FUSEBOX) && entities[i].stateValue > 0.5f && item.HasTag(TAG_ELECTRIC))) { 
                                    interactTargets.push_back(i); 
                                }                                
                                else if (CanProcessChemistry(grabbedEntityIndex, i, entities) != CHEM_NONE) { 
                                    interactTargets.push_back(i); 
                                }
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
                            if (item.HasTag(TAG_SANDBAG)) PlaySound(sounds["sandbags-put-on-floor"]);
                            item.isGrabbed = false; item.isUsing = false; item.velocity = {0,0,0}; grabbedEntityIndex = -1; item.isSolid = true;
                        } else if (interactTargets.size() == 1) { executeAction = true; actionTargetIdx = interactTargets[0]; isDropMenu = true; } 
                        else { showInteractMenu = true; isDropMenu = true; interactSelectedIndex = 0; }
                    } else {
                        std::vector<BoundingBox> pIntBoxList = player.GetWorldInteractBounds(); 
                        for (auto& b : pIntBoxList) { float reach = 100.0f; b.min.x += player.facingDir.x * reach; b.max.x += player.facingDir.x * reach; b.min.z += player.facingDir.z * reach; b.max.z += player.facingDir.z * reach; b.min.x -= 50; b.max.x += 50; b.min.z -= 50; b.max.z += 50; b.min.y -= 20; b.max.y += 500; }
                        for (size_t i = 1; i < entities.size(); ++i) {
                            if (i == equippedEyewear || i == equippedGloves) continue; 
                            // <--- Change this back to only look for canGrab items!
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
            
            bool isPlaying = (currentState == STATE_PLAYING);
            hazVis = UpdatePhysicsAndHazards(entities, dt, grabbedEntityIndex, equippedEyewear, equippedGloves, currentNight, shiftTimer, isPlaying);
            
            if (currentState == STATE_PLAYING || currentState == STATE_TUTORIAL) {
                if (player.isStone || player.isDead) {
                    lastReport = EvaluateMuseum(entities, currentNight); 
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
            UpdateAndRenderMenu((int&)currentState, mainMusicVolume, triggerShiftStart, triggerNewGame, fontMuseum, fontEmployee, currentNight); 
            SetMusicVolume(mainMusic, mainMusicVolume); SetMusicVolume(tutorialMusic, mainMusicVolume);
        } else if (currentState == STATE_CREDITS) {
            // --- NEW FIX: Draw the Credits! ---
            UpdateAndRenderCredits((int&)currentState, fontMuseum, fontEmployee);
        } else {
            RenderHUD(renderTarget, shiftTimer, SECONDS_PER_HOUR, currentNight, player, showInteractMenu, isDropMenu, interactTargets, interactSelectedIndex, entities, hazVis, fontMuseum, fontEmployee);            
            
            // --- NEW FIX: Upgraded cinematic prompt over the HUD! ---
            if (currentState == STATE_PLAYING && currentNight >= 5 && !isOverlayActive && grabbedEntityIndex == -1) {
                for (auto& e : entities) {
                    if (e.HasTag(TAG_PANDORA)) {
                        float distXZ = Vector2Distance({player.position.x, player.position.z}, {e.position.x, e.position.z});
                        if (distXZ < 120.0f) {
                            const char* promptMsg = "[Q] Stare at the box for the rest of the night";
                            float fontSize = 35.0f * scale; // Made it slightly bigger
                            Vector2 pVec = MeasureTextEx(fontEmployee, promptMsg, fontSize, 1);
                            
                            // Calculate centered coordinates
                            float drawX = (float)GetScreenWidth() / 2.0f - pVec.x / 2.0f;
                            float drawY = (float)GetScreenHeight() - (200.0f * scale); // Moved it higher
                            
                            // Draw a dark background box for maximum contrast
                            DrawRectangle((int)drawX - 15, (int)drawY - 10, (int)pVec.x + 30, (int)pVec.y + 20, {0, 0, 0, 180});
                            
                            // Draw the gold text
                            DrawTextEx(fontEmployee, promptMsg, { drawX, drawY }, fontSize, 1, {218, 165, 32, 255});
                            break;
                        }
                    }
                }
            }
            
            if (currentState == STATE_TUTORIAL && !isOverlayActive) {
                bool tutorialFinished = false;
                UpdateAndRenderTutorial((int&)currentState, entities, grabbedEntityIndex, equippedEyewear, shiftTimer, tutorialFinished, tutorialStep, playerJumpedThisFrame, fontMuseum, fontEmployee);            
                if (tutorialFinished) { 
                    doorsOpen[0] = true; 
                }
            }

            if (isOverlayActive && grabbedEntityIndex != -1) {
                // We now pass both the name AND the stateValue (which holds the ID)
                UpdateAndRenderOverlay(isOverlayActive, tutorialMusic, newsMusic, tutorialStep, entities[grabbedEntityIndex].name, entities[grabbedEntityIndex].stateTimer);        }
            }
        
        EndDrawing();

        if (triggerNewGame) {
            remove("museum_save.dat"); // Deletes the physical file
            currentNight = 1;          // Reset progress
            ResetNight();              // Rebuild the level
            currentState = STATE_TUTORIAL; // Force tutorial
            continue;
        }
        
        if (triggerShiftStart) {
            ResetNight(); 
            if (currentNight == 1) { 
                currentState = STATE_TUTORIAL; 
            } else { 
                currentState = STATE_PLAYING; 
            }
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

    UnloadOverlayAssets();
    
    for (auto& pair : models) { UnloadModel(pair.second); }

    if (isMainMusicLoaded) UnloadMusicStream(mainMusic);
    UnloadMusicStream(tutorialMusic); UnloadMusicStream(deathMusic); UnloadMusicStream(newsMusic); UnloadMusicStream(reviewsMusic); for (auto& pair : sounds) UnloadSound(pair.second); CloseAudioDevice();
    UnloadRenderTexture(renderTarget); 
    UnloadFont(fontMuseum); UnloadFont(fontEmployee);
    CloseWindow(); 
    return 0;
}