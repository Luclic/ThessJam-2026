#pragma once
#include "Entities.h"
#include <vector>
#include <string>

struct Review {
    std::string artifactName;
    std::string reviewText;
};

struct ShiftReport {
    bool isFired;
    std::string finalVerdict;
    std::vector<Review> reviews;
};

inline ShiftReport EvaluateMuseum(const std::vector<Entity>& entities, int currentNight) {
    ShiftReport report;
    report.isFired = false;
    
    int brokenCount = 0;

    // Evaluate every entity against the rules for the current night
    for (const auto& e : entities) {
        
        // --- NIGHT 1 & BEYOND ---
        if (currentNight >= 1) {
            if (e.HasTag(TAG_MEDUSA) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Medusa", "Left active! Guests were turned to stone."});
            }
            if (e.HasTag(TAG_WATER_SOURCE) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Water Source", "Flooded the Greek exhibit."});
            }
        }

        // --- NIGHT 2 & BEYOND ---
        if (currentNight >= 2) {
            if (e.HasTag(TAG_SANDALS) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Hermes Sandals", "Flew away. Irresponsible."});
            }
            if (e.HasTag(TAG_WIND_BAG) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Aiolus Bag", "Blown open. Hurricane in the hallway."});
            }
            if (e.HasTag(TAG_BOULDER) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Sisyphus Boulder", "Rolled loose and caused property damage."});
            }
            // Check if the tall vase is broken!
            if (e.HasTag(TAG_FRAGILE) && e.HasTag(TAG_BROKEN)) { 
                report.isFired = true; 
                report.reviews.push_back({"Ancient Vase", "Shattered into pieces. Unacceptable."});
            }
        }

        // --- NIGHT 3 & BEYOND ---
        if (currentNight >= 3) {
            if (e.HasTag(TAG_ZEUS) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Zeus Statue", "Electrocuted the staff."});
            }
            if (e.HasTag(TAG_SUN_DISK) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Ra Sun Disk", "Scorched the ceiling."});
            }
            if (e.HasTag(TAG_MUMMY) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Mummy", "Wandered out of its sarcophagus."});
            }
        }

        // --- NIGHT 4 & BEYOND (Nordic Room) ---
        if (currentNight >= 4) {
            if (e.HasTag(TAG_MJOLNIR) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Mjolnir", "Left discarded on the floor."});
            }
            if (e.HasTag(TAG_GLEIPNIR) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Gleipnir", "Failed to secure the binding."});
            }
            if (e.HasTag(TAG_BANSHEE_STONE) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Banshee Stone", "Wreaked havoc on the timeline."});
            }
        }
        
        // --- NIGHT 5 (Everything must be perfect) ---
        if (currentNight >= 5) {
            if (e.HasTag(TAG_PANDORA) && e.isGlitching) {
                report.isFired = true;
                report.reviews.push_back({"Pandora's Box", "You let the box open. The world is doomed."});
            }
        }
    }

    // Set Final Verdicts based on the results
    if (report.isFired) {
        report.finalVerdict = "Multiple critical failures detected. Pack your things.";
        if (report.reviews.empty()) {
            report.reviews.push_back({"Employee File", "You died on the job."});
        }
    } else {
        report.finalVerdict = "Excellent work. The exhibits are safe.";
        report.reviews.push_back({"Management", "No major incidents reported tonight."});
    }

    return report;
}