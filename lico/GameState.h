// GameState.h
#pragma once
#include <vector>
#include "Entities.h"
#include "Evaluation.h"

enum GameState { STATE_MENU, STATE_PLAYING, STATE_REVIEW, STATE_GAMEOVER_FIRED, STATE_GAMEOVER_DEATH };
const float SECONDS_PER_HOUR = 30.0f;

// Added 'inline' to prevent Multiple Definition Linker Errors!
// Fixed AppState -> GameState
inline GameState currentState = GameState::STATE_MENU; 

inline std::vector<Entity> entities;
inline int grabbedEntityIndex = -1;
inline int equippedEyewear = -1; 
inline int equippedGloves = -1; 

inline int currentNight = 1;
inline float shiftTimer = 0.0f;
inline ShiftReport lastReport;

// UI State
inline bool showInteractMenu = false;
inline bool isDropMenu = false;
inline std::vector<int> interactTargets;
inline int interactSelectedIndex = 0;

inline bool doorsOpen[5] = {false, false, false, false, false};