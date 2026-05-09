#pragma once
#include "Entities.h"
#include <vector>
#include <string>
#include <iostream>

enum ScoreLevel { SCORE_PERFECT, SCORE_ADEQUATE, SCORE_GAMEOVER };

struct ArtifactReview {
    std::string artifactName;
    ScoreLevel score;
    std::string reviewText;
};

struct ShiftReport {
    bool isFired; 
    std::string finalVerdict;
    std::vector<ArtifactReview> reviews;
};

inline const Entity* FindArtifactByTag(const std::vector<Entity>& entities, Tag targetTag) {
    for (const auto& e : entities) if (e.HasTag(targetTag)) return &e;
    return nullptr;
}

inline ShiftReport EvaluateMuseum(const std::vector<Entity>& entities) {
    ShiftReport report;
    report.isFired = false;
    
    // 1. Check Player Life Status
    const Entity* player = nullptr;
    for (const auto& e : entities) if (e.name == "Player") player = &e;
    
    if (player && player->isStone) {
        report.isFired = true;
        report.finalVerdict = "FATAL ERROR: You were petrified by Medusa. We are using you as a coat rack in the lobby.";
        return report; // Instant fail
    }
    if (player && player->isDead) {
        report.isFired = true;
        report.finalVerdict = "FATAL ERROR: You were electrocuted. Please remember to wear your rubber gloves.";
        return report; // Instant fail
    }

    // 2. Evaluate Medusa
    const Entity* medusa = FindArtifactByTag(entities, TAG_MEDUSA);
    if (medusa) {
        if (medusa->HasTag(TAG_BROKEN)) { // NEW: BOULDER CRUSH CHECK
            report.reviews.push_back({"Medusa's Head", SCORE_GAMEOVER, "The artifact was completely crushed by a giant boulder!"});
            report.isFired = true;
            if (report.finalVerdict.empty()) report.finalVerdict = "FIRED: Irreplaceable historical artifact pulverized!";
        } else {
            bool isBlocked = false;
            for (const auto& other : entities) {
                if (other.attachedTo != -1 && &entities[other.attachedTo] == medusa && other.HasTag(TAG_EYEWEAR) && !other.HasTag(TAG_BROKEN)) {
                    isBlocked = true; break;
                }
            }
            if (isBlocked) report.reviews.push_back({"Medusa's Head", SCORE_PERFECT, "Sunglasses blocked the gaze. Stylish and effective."});
            else if (medusa->isGlitching) {
                report.reviews.push_back({"Medusa's Head", SCORE_GAMEOVER, "Left unblocked! The morning tour group was petrified."});
                report.isFired = true;
                if (report.finalVerdict.empty()) report.finalVerdict = "FIRED: Medusa petrified the morning guests!";
            } else {
                report.reviews.push_back({"Medusa's Head", SCORE_PERFECT, "Gaze was deactivated manually. Good job."});
            }
        }
    }

    // 3. Evaluate Holy Cup
    const Entity* cup = FindArtifactByTag(entities, TAG_WATER_SOURCE);
    if (cup) {
        bool hasCork = false;
        for (const auto& other : entities) if (other.attachedTo != -1 && &entities[other.attachedTo] == cup && other.HasTag(TAG_CORK)) hasCork = true;

        if (hasCork) {
            report.reviews.push_back({"Holy Cup", SCORE_PERFECT, "Properly sealed with the giant cork. No water damage."});
        } else if (!cup->isGlitching) {
            report.reviews.push_back({"Holy Cup", SCORE_ADEQUATE, "Duct tape on a holy relic? Management is unhappy, but it held."});
        } else {
            if (cup->stateValue > 200.0f) {
                // NEW: WET FLOOR SIGN OSHA COMPLIANCE CHECK
                bool hasSign = false;
                for (const auto& sign : entities) {
                    if (sign.HasTag(TAG_WET_SIGN) && Vector2Distance(sign.position, cup->position) < cup->stateValue) hasSign = true;
                }
                if (hasSign) {
                    report.reviews.push_back({"Holy Cup", SCORE_ADEQUATE, "Egyptian wing is flooded, but you placed a Wet Floor sign. OSHA compliance saved your job."});
                } else {
                    report.reviews.push_back({"Holy Cup", SCORE_GAMEOVER, "Left leaking without a warning sign! Massive lawsuit incoming."});
                    report.isFired = true;
                    if (report.finalVerdict.empty()) report.finalVerdict = "FIRED: Gross negligence led to visitor slip-and-fall injuries!";
                }
            } else {
                report.reviews.push_back({"Holy Cup", SCORE_ADEQUATE, "Sponge absorbed the worst of it, but it's still an active leak."});
            }
        }
    }

    // NEW 3.5: Evaluate Fragile Vase
    const Entity* vase = nullptr;
    for (const auto& e : entities) if (e.name == "Fragile Vase") vase = &e;
    if (vase) {
        if (vase->HasTag(TAG_BROKEN)) {
            report.reviews.push_back({"Fragile Vase", SCORE_GAMEOVER, "Shattered into a million pieces. The curator is furious."});
            report.isFired = true;
            if (report.finalVerdict.empty()) report.finalVerdict = "FIRED: You let a priceless Ming Vase shatter!";
        } else if (vase->color.r == YELLOW.r) { 
            report.reviews.push_back({"Fragile Vase", SCORE_ADEQUATE, "Held together by duct tape. Appalling, but technically whole."});
        } else {
            report.reviews.push_back({"Fragile Vase", SCORE_PERFECT, "Pristine and untouched."});
        }
    }

    // 4. Evaluate Hermes Sandals
    const Entity* sandals = FindArtifactByTag(entities, TAG_SANDALS);
    if (sandals) {
        if (sandals->isGlitching) {
            report.reviews.push_back({"Hermes Sandals", SCORE_GAMEOVER, "Left flying around. They escaped out an open window. Irreplaceable loss!"});
            report.isFired = true;
            if (report.finalVerdict.empty()) report.finalVerdict = "FIRED: A priceless artifact flew out the window!";
        } else if (sandals->color.r == DARKBLUE.r && sandals->color.g == DARKBLUE.g && sandals->color.b == DARKBLUE.b) {
            report.reviews.push_back({"Hermes Sandals", SCORE_ADEQUATE, "You soaked the divine feathers in water to ground them. Effective, but the curator is crying."});
        } else if (sandals->attachedTo != -1) {
             report.reviews.push_back({"Hermes Sandals", SCORE_PERFECT, "Securely stored and immobilized."});
        } else {
            report.reviews.push_back({"Hermes Sandals", SCORE_ADEQUATE, "You duct-taped the sandals to the floor. Barbaric, but they didn't escape."});
        }
    }

    // 5. Evaluate Zeus & Lightning
    const Entity* lightning = FindArtifactByTag(entities, TAG_ELECTRIC);
    if (lightning) {
        if (lightning->attachedTo != -1 && entities[lightning->attachedTo].HasTag(TAG_ZEUS)) {
            report.reviews.push_back({"Lightning Bolt", SCORE_PERFECT, "Returned safely to the All-Father's hand."});
        } else if (lightning->attachedTo != -1 && entities[lightning->attachedTo].HasTag(TAG_FUSEBOX)) {
            report.reviews.push_back({"Lightning Bolt", SCORE_ADEQUATE, "Using divine lightning to power the museum's fusebox? Brilliant cost-saving!"});
        } else if (lightning->isStone) {
             report.reviews.push_back({"Lightning Bolt", SCORE_ADEQUATE, "You petrified pure energy. Science team is confused, but it's safe."});
        } else {
            report.reviews.push_back({"Lightning Bolt", SCORE_GAMEOVER, "Uncontained divine electricity burned a hole through the floorboards!"});
            report.isFired = true;
            if (report.finalVerdict.empty()) report.finalVerdict = "FIRED: Loose lightning burned the museum down!";
        }
    }

    // 6. Evaluate Aeolus Bag
    const Entity* windBag = FindArtifactByTag(entities, TAG_WIND_BAG);
    if (windBag) {
        if (windBag->isGlitching) {
            report.reviews.push_back({"Aeolus Bag", SCORE_GAMEOVER, "Hurricane-force winds destroyed the exhibits. Disaster!"});
            report.isFired = true;
            if (report.finalVerdict.empty()) report.finalVerdict = "FIRED: Hurricane force winds shattered the exhibits!";
        } else {
            report.reviews.push_back({"Aeolus Bag", SCORE_PERFECT, "The winds are calm and securely tied."});
        }
    }

    // 6. Evaluate Mjolnir
    const Entity* mjolnir = FindArtifactByTag(entities, TAG_MJOLNIR);
    if (mjolnir) {
        if (mjolnir->attachedTo != -1) {
            report.reviews.push_back({"Mjolnir", SCORE_PERFECT, "Returned to its pedestal. Thor is pleased."});
        } else if (mjolnir->isGlitching) {
            report.reviews.push_back({"Mjolnir", SCORE_GAMEOVER, "Left blocking a major hallway. The museum had to shut down the wing."});
            report.isFired = true;
            if (report.finalVerdict.empty()) report.finalVerdict = "FIRED: You blocked fire exits with a divine hammer!";
        } else {
            report.reviews.push_back({"Mjolnir", SCORE_ADEQUATE, "You pushed it into a corner. Not great, but nobody tripped."});
        }
    }

    // 7. Evaluate Gleipnir
    const Entity* gleipnir = FindArtifactByTag(entities, TAG_GLEIPNIR);
    if (gleipnir) {
        if (gleipnir->isGlitching) {
            report.reviews.push_back({"Gleipnir's Ribbon", SCORE_GAMEOVER, "The magic ribbon escaped and strangled a mannequin."});
            report.isFired = true;
            if (report.finalVerdict.empty()) report.finalVerdict = "FIRED: Gleipnir strangled a security guard!";
        } else if (gleipnir->attachedTo != -1 && entities[gleipnir->attachedTo].HasTag(TAG_SANDBAG)) {
            report.reviews.push_back({"Gleipnir's Ribbon", SCORE_PERFECT, "Safely distracted by a heavy sandbag."});
        } else if (gleipnir->attachedTo != -1 && entities[gleipnir->attachedTo].HasTag(TAG_MUMMY)) {
            report.reviews.push_back({"Gleipnir's Ribbon", SCORE_ADEQUATE, "You used it to tie up a mummy. Effective, but the exhibits are all mixed up."});
        } else if (gleipnir->isStone) {
            report.reviews.push_back({"Gleipnir's Ribbon", SCORE_ADEQUATE, "Petrified into a solid squiggle. The historians are crying."});
        }
    }

    // 8. Evaluate Banshee Stone
    const Entity* banshee = FindArtifactByTag(entities, TAG_BANSHEE_STONE);
    if (banshee) {
        if (banshee->isGlitching) {
            report.reviews.push_back({"Banshee Stone", SCORE_GAMEOVER, "The screaming shattered the glass cases and deafened staff!"});
            report.isFired = true;
            if (report.finalVerdict.empty()) report.finalVerdict = "FIRED: Banshee wails shattered eardrums and exhibits!";
        } else {
            report.reviews.push_back({"Banshee Stone", SCORE_PERFECT, "Successfully muffled. Blissful silence."});
        }
    }
    // 9. Evaluate Floor Holes (OSHA Hazard)
    for (const auto& hole : entities) {
        if (hole.HasTag(TAG_HOLE)) {
            bool isSafe = (hole.stateValue > 0.5f); // 1.0f means it has a tape web
            for (const auto& other : entities) if (other.attachedTo != -1 && &entities[other.attachedTo] == &hole && other.HasTag(TAG_WET_SIGN)) isSafe = true;

            if (!isSafe) {
                report.reviews.push_back({"Floor Hole", SCORE_GAMEOVER, "You left an unmarked hole in the floor. A child fell in."});
                report.isFired = true;
                if (report.finalVerdict.empty()) report.finalVerdict = "FIRED: Severe OSHA violation! An open pit was left unmarked.";
            } else {
                report.reviews.push_back({"Floor Hole", SCORE_ADEQUATE, "You sawed through the floor, but at least you warned people."});
            }
        }
    }
    
    // Generate Final Verdict if somehow missed
    if (!report.isFired) {
        report.finalVerdict = "NIGHT SHIFT COMPLETE: The museum opens on time. See you tomorrow, Janitor.";
    } else if (report.finalVerdict.empty()) { 
        report.finalVerdict = "FIRED: Catastrophic damage to the museum. Turn in your mop bucket.";
    }

    return report;
}