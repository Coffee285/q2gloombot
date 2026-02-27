/*
 * gloom_classes.c -- per-class metadata tables for q2gloombot
 *
 * Provides the gloom_class_info[] array referenced by the inline helpers
 * in gloom_classes.h.  One entry per class; order must match gloom_class_t.
 */

#include "gloom_classes.h"

/*
 * gloom_class_info[GLOOM_CLASS_MAX]
 *
 * Fields per entry:
 *   name, team, preferred_range, credit_cost, evo_cost,
 *   max_health, can_wall_walk, can_fly, can_build, is_stealth, next_evo
 */
const gloom_class_info_t gloom_class_info[GLOOM_CLASS_MAX] = {
    /* [0] GRUNT — basic infantry */
    { "Grunt",        1, 400.0f,  0,    0,  80,  0, 0, 0, 0, GLOOM_CLASS_MAX },
    /* [1] ST — Shock Trooper */
    { "ST",           1, 250.0f,  1,    0, 120,  0, 0, 0, 0, GLOOM_CLASS_MAX },
    /* [2] BIOTECH — medic */
    { "Biotech",      1, 300.0f,  1,    0,  90,  0, 0, 0, 0, GLOOM_CLASS_MAX },
    /* [3] HT — Heavy Trooper */
    { "HT",           1, 600.0f,  2,    0, 110,  0, 0, 0, 0, GLOOM_CLASS_MAX },
    /* [4] COMMANDO — fast assault */
    { "Commando",     1, 350.0f,  2,    0,  90,  0, 0, 0, 0, GLOOM_CLASS_MAX },
    /* [5] EXTERMINATOR — heavy assault */
    { "Exterminator", 1, 500.0f,  3,    0, 150,  0, 0, 0, 0, GLOOM_CLASS_MAX },
    /* [6] ENGINEER — builder */
    { "Engineer",     1, 200.0f,  0,    0,  80,  0, 0, 1, 0, GLOOM_CLASS_MAX },
    /* [7] MECH — power armour */
    { "Mech",         1, 500.0f,  4,    0, 300,  0, 0, 0, 0, GLOOM_CLASS_MAX },

    /* [8] HATCHLING — fast, weak alien */
    { "Hatchling",    2, 100.0f,  0,    0,  50,  1, 0, 0, 0, GLOOM_CLASS_DRONE },
    /* [9] DRONE — melee fighter */
    { "Drone",        2, 120.0f,  0,    1,  90,  1, 0, 0, 0, GLOOM_CLASS_STINGER },
    /* [10] WRAITH — flying, ranged */
    { "Wraith",       2, 400.0f,  0,    2, 100,  0, 1, 0, 0, GLOOM_CLASS_MAX },
    /* [11] KAMIKAZE — suicide bomber */
    { "Kamikaze",     2,  50.0f,  0,    2, 100,  1, 0, 0, 0, GLOOM_CLASS_MAX },
    /* [12] STINGER — hybrid melee/ranged */
    { "Stinger",      2, 300.0f,  0,    2, 130,  1, 0, 0, 0, GLOOM_CLASS_GUARDIAN },
    /* [13] GUARDIAN — stealth tank */
    { "Guardian",     2, 150.0f,  0,    3, 250,  1, 0, 0, 1, GLOOM_CLASS_MAX },
    /* [14] BREEDER — alien builder */
    { "Breeder",      2, 200.0f,  0,    0, 120,  1, 0, 1, 0, GLOOM_CLASS_MAX },
    /* [15] STALKER — ultimate melee */
    { "Stalker",      2, 100.0f,  0,    4, 400,  1, 0, 0, 0, GLOOM_CLASS_MAX },
};
