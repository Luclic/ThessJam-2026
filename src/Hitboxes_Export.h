#pragma once
#include "raylib.h"
#include <unordered_map>
#include <vector>
#include <string>

inline std::unordered_map<std::string, std::vector<BoundingBox>> GetGlobalHitboxes() {
    return {
        {"Player", {
            {{30.0, 110.0, 30.0}, {0.0, 0.0, 0.0}},
        }},
        {"floor11", {
            {{-100.0, -20.0, -100.0}, {100.0, 0.0, 100.0}},
        }},
        {"wall1", {
            {{100.0, 400.0, -20.0}, {-100.0, 0.0, 10.0}},
        }},
        {"arch1", {
            {{-200.0, 325.0, -20.0}, {-135.0, 0.0, 0.0}},
            {{200.0, 400.0, 20.0}, {130.0, 0.0, 0.0}},
        }},
        {"Light Switch", {
            {{-22.0, 0.0, -22.0}, {0.0, 0.0, 0.0}},
            {{22.0, 45.0, 22.0}, {0.0, 0.0, 0.0}},
        }},
        {"ticketstand", {
            {{50.0, 95.0, 30.0}, {-50.0, 0.0, -25.0}},
        }},
        {"ticketstandseat", {
            {{22.0, 65.0, 22.0}, {0.0, 0.0, 0.0}},
        }},
        {"Fuse Box", {
            {{22.0, 45.0, 22.0}, {0.0, 0.0, 0.0}},
        }},
        {"Sisyphus Boulder", {
            {{60.0, 130.0, 60.0}, {0.0, 0.0, 0.0}},
        }},
        {"stand2", {
            {{60.0, 100.0, 45.0}, {0.0, 0.0, 0.0}},
        }},
        {"Display Vase", {
            {{22.0, 45.0, 22.0}, {0.0, 0.0, 0.0}},
        }},
        {"Zeus Statue", {
            {{60.0, 130.0, 60.0}, {0.0, 0.0, 0.0}},
        }},
        {"Lightning", {
            {{22.0, 45.0, 22.0}, {0.0, 0.0, 0.0}},
        }},
    };
}

struct ModelTweak { float scale; Vector3 offset; float rot; };
inline std::unordered_map<std::string, ModelTweak> GetGlobalTweaks() {
    return {
        {"Player", {100.0, {0.0, 0.0, 0.0}, 0.0}},
        {"floor11", {100.0, {0.0, 0.0, 0.0}, 0.0}},
        {"wall1", {100.0, {0.0, 0.0, 80.0}, 0.0}},
        {"arch1", {100.0, {0.0, 0.0, 0.0}, 0.0}},
        {"Light Switch", {100.0, {0.0, 0.0, 0.0}, 0.0}},
        {"ticketstand", {100.0, {0.0, 0.0, 0.0}, 0.0}},
        {"ticketstandseat", {100.0, {0.0, 0.0, 0.0}, 0.0}},
        {"Fuse Box", {100.0, {0.0, 0.0, 0.0}, 0.0}},
        {"Sisyphus Boulder", {100.0, {0.0, 0.0, 0.0}, 0.0}},
        {"stand2", {100.0, {0.0, 0.0, 0.0}, 0.0}},
        {"Display Vase", {100.0, {0.0, 0.0, 0.0}, 0.0}},
        {"Zeus Statue", {100.0, {0.0, 0.0, 0.0}, 0.0}},
        {"Lightning", {100.0, {0.0, 0.0, 0.0}, 0.0}},
    };
}
