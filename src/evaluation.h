#pragma once
#include "Entities.h"
#include "raymath.h"
#include <vector>
#include <string>

// --- NEW FIX: Forward declare InitLevel so we can spawn a "Ghost Museum" for coordinate checking ---
extern void InitLevel(std::vector<Entity>& entities);

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
    
    // The player is always entity 0
    const Entity& player = entities[0];
    
    // Create our pristine baseline museum to compare coordinates against
    std::vector<Entity> baseline;
    InitLevel(baseline);
    
    // =========================================================
    // 1. DEATH EVALUATION (AFTERLIFE MESSAGES)
    // =========================================================
    if (player.isDead || player.isStone) {
        report.isFired = true;
        report.finalVerdict = "TERMINATION OF MORTAL CONTRACT";
        
        if (player.isStone) {
            report.reviews.push_back({"The Ferryman", "Another soul heavy as stone. You should have avoided her gaze, mortal."});
        } else {
            // Determine other causes based on active hazards in the room
            bool electrocuted = false;
            bool burned = false;
            bool choked = false;
            
            for (const auto& e : entities) {
                if (e.HasTag(TAG_ZEUS) && e.isGlitching) electrocuted = true;
                if (e.HasTag(TAG_SUN_DISK) && e.isGlitching) burned = true;
                if (e.HasTag(TAG_GLEIPNIR) && e.isUsing) choked = true; 
            }
            
            if (choked) {
                report.reviews.push_back({"Hades", "The magical ribbon squeezed the life from you. A tragic, breathless end."});
            } else if (burned) {
                report.reviews.push_back({"The Underworld", "Your spirit arrived scorched. The Sun Disk shows no mercy to mortals."});
            } else if (electrocuted) {
                report.reviews.push_back({"Charon", "A shocking arrival. Zeus strikes down another unworthy janitor."});
            } else {
                report.reviews.push_back({"The Afterlife", "Your shift has ended permanently. Welcome to the underworld."});
            }
        }
        return report; // End evaluation immediately if dead
    }

    // =========================================================
    // 2. FIRED EVALUATION (NEWS HEADLINES)
    // =========================================================
    int messyItems = 0;

    for (size_t i = 1; i < entities.size(); ++i) {
        const auto& e = entities[i];
        
        // --- NIGHT 1 & BEYOND ---
        if (currentNight >= 1) {
            if (e.HasTag(TAG_MEDUSA) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"The Daily Myth", "BREAKING: LOCAL MUSEUM GUESTS PETRIFIED! JANITOR TO BLAME?"});
            }
            if (e.HasTag(TAG_WATER_SOURCE) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"City News", "HEADLINE: MASSIVE FLOOD AT MYTHIC MUSEUM DESTROYS PRICELESS EXHIBITS!"});
            }
        }

        // --- NIGHT 2 & BEYOND ---
        if (currentNight >= 2) {
            if (e.HasTag(TAG_SANDALS) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Sky Watch", "BREAKING: ANCIENT WINGED SANDALS SIGHTED FLYING OVER DOWNTOWN!"});
            }
            if (e.HasTag(TAG_WIND_BAG) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Weather Channel", "HEADLINE: INDOOR HURRICANE RIPS THROUGH MUSEUM LOBBY!"});
            }
            if (e.HasTag(TAG_BOULDER) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Action News", "BREAKING: ROGUE BOULDER CRUSHES MUSEUM LOBBY, MILLIONS IN DAMAGES!"});
            }
            if (e.HasTag(TAG_FRAGILE) && e.HasTag(TAG_BROKEN)) { 
                report.isFired = true; 
                report.reviews.push_back({"Art Daily", "HEADLINE: PRICELESS ANCIENT VASE SHATTERED! STAFF NEGLIGENCE CITED."});
            }
        }

        // --- NIGHT 3 & BEYOND ---
        if (currentNight >= 3) {
            if (e.HasTag(TAG_ZEUS) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Local Tribune", "BREAKING: MUSEUM PATRONS ELECTROCUTED BY ANGRY ZEUS STATUE!"});
            }
            if (e.HasTag(TAG_SUN_DISK) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"City News", "HEADLINE: ROOF OF MYTHIC MUSEUM SCORCHED OFF BY MINIATURE SUN!"});
            }
            if (e.HasTag(TAG_MUMMY) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"The Daily Myth", "BREAKING: UNDEAD MUMMY SPOTTED WANDERING CITY STREETS!"});
            }
        }

        // --- NIGHT 4 & BEYOND ---
        if (currentNight >= 4) {
            if (e.HasTag(TAG_MJOLNIR) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"Global News", "HEADLINE: NORSE GOD'S HAMMER ABANDONED ON MUSEUM FLOOR!"});
            }
            if (e.HasTag(TAG_GLEIPNIR) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"City News", "BREAKING: MAGICAL RIBBON CAUSES MASS PANIC AT MUSEUM!"});
            }
            if (e.HasTag(TAG_BANSHEE_STONE) && e.isGlitching) { 
                report.isFired = true; 
                report.reviews.push_back({"The Daily Pulse", "HEADLINE: UNEXPLAINED SHOCKWAVES SHATTER WINDOWS CITYWIDE!"});
            }
        }
        
        // --- MINOR SOCIAL MEDIA REVIEWS (Messy Check) ---
        if (i < baseline.size()) {
            float distMoved = Vector3Distance(e.position, baseline[i].position);
            
            // If a tool or artifact was moved >500 units AND was just left on the floor (not attached to a pedestal/statue)
            if (e.canGrab && distMoved > 500.0f && e.attachedTo == -1) {
                // Ignore intended disposable/utility items
                if (!e.HasTag(TAG_HOLE) && !e.HasTag(TAG_MAT) && !e.HasTag(TAG_LIGHTNING)) {
                    messyItems++;
                }
            }
        }
        
        // Check if wet floor signs are knocked over
        if (e.HasTag(TAG_WET_SIGN) && e.position.y < 5.0f) {
            messyItems++;
        }
    }

    // =========================================================
    // 3. FINAL VERDICT COMPUTATION
    // =========================================================
    
    // Apply Social Media Reviews if the museum is a mess (but only if they didn't get fired for something worse)
    if (messyItems > 3 && !report.isFired) {
        report.reviews.push_back({"@ArtFan99", "The museum was a total mess today! Stuff left all over the floor. 1/5 stars."});
        report.reviews.push_back({"@HistoryNerd", "Disappointed. Artifacts and janitor tools were literally tripping hazards. So unprofessional."});
    } else if (messyItems > 0 && !report.isFired) {
        report.reviews.push_back({"@LocalGuide", "Beautiful exhibits, but the floors were pretty cluttered today. 3/5 stars."});
    }

    if (report.isFired) {
        report.finalVerdict = "YOU ARE FIRED.";
    } else {
        report.finalVerdict = "SHIFT COMPLETE. EXCELLENT WORK.";
        if (messyItems == 0) {
            report.reviews.push_back({"@MuseumCurator", "The exhibits are pristine. The maintenance staff deserves a raise!"});
        }
    }

    return report;
}