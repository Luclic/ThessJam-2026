#pragma once
#include <vector>
#include <fstream>
#include "Entities.h"

inline void SaveGame(int currentNight, const std::vector<Entity>& entities) {
    std::ofstream out("museum_save.dat", std::ios::binary);
    out.write((char*)&currentNight, sizeof(currentNight));
    size_t count = entities.size();
    out.write((char*)&count, sizeof(count));
    
    for (const auto& e : entities) {
        out.write((char*)&e.position, sizeof(Vector2));
        out.write((char*)&e.z, sizeof(float));
        out.write((char*)&e.attachedTo, sizeof(int));
        out.write((char*)&e.isGlitching, sizeof(bool));
        out.write((char*)&e.isStone, sizeof(bool));
        out.write((char*)&e.stateValue, sizeof(float));
        out.write((char*)&e.color, sizeof(Color)); 
    }
}

inline bool LoadGame(int& currentNight, std::vector<Entity>& entities) {
    std::ifstream in("museum_save.dat", std::ios::binary);
    if (!in.is_open()) return false;
    
    in.read((char*)&currentNight, sizeof(currentNight));
    size_t count;
    in.read((char*)&count, sizeof(count));
    
    if (count != entities.size()) return false; // Version mismatch, abort load
    
    for (auto& e : entities) {
        in.read((char*)&e.position, sizeof(Vector2));
        in.read((char*)&e.z, sizeof(float));
        in.read((char*)&e.attachedTo, sizeof(int));
        in.read((char*)&e.isGlitching, sizeof(bool));
        in.read((char*)&e.isStone, sizeof(bool));
        in.read((char*)&e.stateValue, sizeof(float));
        in.read((char*)&e.color, sizeof(Color));
    }
    return true;
}