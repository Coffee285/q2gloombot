/*
 * bot_class_engineer.c -- Engineer AI (human builder)
 *
 * The Engineer is the most strategically critical human class.
 * It builds and maintains the human base infrastructure.
 *
 * Behavior priority order:
 *  1. Build initial Teleporter at a defensible location
 *  2. Build Turrets to defend the Teleporter
 *  3. Build Ammo Depot near Teleporter
 *  4. Build Camera in forward positions
 *  5. Expand with forward Teleporters as team pushes
 *  6. Repair damaged structures (Teleporters > Turrets > others)
 *  7. Flee from combat toward turrets and teammates
 *  8. If all structures lost, immediately rebuild a Teleporter
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define ENGINEER_FLEE_HP_PCT   0.50f  /* flee from combat above this damage */
#define ENGINEER_MIN_TURRETS   2      /* minimum turrets per Teleporter     */
#define ENGINEER_REPAIR_THRESH 0.50f  /* repair structures below 50% HP     */

/* -----------------------------------------------------------------------
   BotClass_Init_engineer
   ----------------------------------------------------------------------- */
void BotClass_Init_engineer(bot_state_t *bs)
{
    bs->combat.engagement_range = gloom_class_info[GLOOM_CLASS_ENGINEER].preferred_range;
    bs->combat.prefer_cover     = true;  /* stay behind defenses            */
    bs->combat.max_range_engage = true;
    bs->nav.wall_walking        = false;

    /* Engineers start by building — immediately assess build needs */
    bs->ai_state             = BOTSTATE_BUILD;
    bs->build.priority       = BUILD_PRIORITY_SPAWNS;
    bs->build.what_to_build  = STRUCT_TELEPORTER;
}

/* -----------------------------------------------------------------------
   BotClass_Think_engineer
   ----------------------------------------------------------------------- */
void BotClass_Think_engineer(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Flee immediately when attacked — Engineer is too valuable to fight */
    if (hp_pct < ENGINEER_FLEE_HP_PCT && bs->combat.target_visible) {
        bs->build.building = false;
        bs->ai_state       = BOTSTATE_FLEE;
        return;
    }

    /* If all structures are gone, critical rebuild takes precedence */
    if (bs->build.spawn_count == 0 && !bs->build.reactor_exists) {
        bs->build.priority      = BUILD_PRIORITY_CRITICAL;
        bs->build.what_to_build = STRUCT_TELEPORTER;
        bs->ai_state            = BOTSTATE_BUILD;
        return;
    }

    /* Standard build priority progression */
    if (bs->ai_state == BOTSTATE_IDLE || bs->ai_state == BOTSTATE_PATROL) {
        if (bs->build.spawn_count == 0) {
            bs->build.priority      = BUILD_PRIORITY_SPAWNS;
            bs->build.what_to_build = STRUCT_TELEPORTER;
        } else {
            /* Teleporter exists — build turrets to defend it */
            bs->build.priority      = BUILD_PRIORITY_DEFENSE;
            bs->build.what_to_build = STRUCT_TURRET_MG;
        }
        bs->ai_state = BOTSTATE_BUILD;
    }

    /* Repair damaged structures periodically */
    if (bs->ai_state == BOTSTATE_BUILD &&
        bs->build.priority == BUILD_PRIORITY_NONE) {
        bs->build.priority      = BUILD_PRIORITY_REPAIR;
        bs->build.what_to_build = STRUCT_NONE;
    }
}
