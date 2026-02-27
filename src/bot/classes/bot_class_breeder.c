/*
 * bot_class_breeder.c -- Breeder AI (alien builder)
 *
 * The Breeder is the most strategically important alien class.
 * It builds and maintains the alien spawn network (Eggs) and
 * supporting structures (Spikers, Cocoons, Obstacles).
 *
 * Behavior priority order:
 *  1. Place first Egg at a safe location near team spawn
 *  2. Build additional Eggs to spread the spawn network
 *  3. Build defensive structures near Eggs (Spikers, Cocoons, Obstacles)
 *  4. Repair damaged structures
 *  5. Flee from combat — Breeder is too valuable to fight
 *  6. Read game flow to choose forward vs. fall-back egg placement
 *  7. Never cluster all eggs in one location
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Engagement / health constants
   ----------------------------------------------------------------------- */
#define BREEDER_FLEE_HP_PCT        0.60f  /* flee above this damage level  */
#define BREEDER_MIN_EGG_COUNT      3      /* target number of active eggs  */
#define BREEDER_MIN_SPIKER_COUNT   2      /* Spikers near eggs             */
#define BREEDER_REPAIR_HP_THRESH   0.50f  /* repair structs below 50% HP   */

/* -----------------------------------------------------------------------
   BotClass_Init_breeder
   Set Breeder-specific combat and navigation defaults.
   ----------------------------------------------------------------------- */
void BotClass_Init_breeder(bot_state_t *bs)
{
    bs->combat.engagement_range = gloom_class_info[GLOOM_CLASS_BREEDER].preferred_range;
    bs->combat.prefer_cover     = true;
    bs->combat.max_range_engage = false;
    bs->nav.wall_walking        = true; /* Breeders use wall paths to reach hidden spots */

    /* Start in build state — placing the first egg is the top priority */
    bs->ai_state             = BOTSTATE_BUILD;
    bs->build.priority       = BUILD_PRIORITY_SPAWNS;
    bs->build.what_to_build  = STRUCT_EGG;
}

/* -----------------------------------------------------------------------
   BotClass_Think_breeder
   Per-frame Breeder-specific behavior overrides.
   Called from the main AI dispatch after the generic state machine.
   ----------------------------------------------------------------------- */
void BotClass_Think_breeder(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Priority 1: Self-preservation — flee if wounded or if enemy is near */
    if (hp_pct < BREEDER_FLEE_HP_PCT || bs->combat.target_visible) {
        if (bs->ai_state != BOTSTATE_FLEE) {
            bs->build.building = false;
            bs->ai_state       = BOTSTATE_FLEE;
        }
        return;
    }

    /* Priority 2: Build / maintain egg network */
    if (bs->ai_state == BOTSTATE_IDLE || bs->ai_state == BOTSTATE_PATROL) {
        /* Decide what the most urgent build need is */
        if (bs->build.spawn_count < BREEDER_MIN_EGG_COUNT) {
            bs->build.priority      = BUILD_PRIORITY_SPAWNS;
            bs->build.what_to_build = STRUCT_EGG;
            bs->ai_state            = BOTSTATE_BUILD;
        } else {
            /* Spawn network is adequate — build defenses around eggs */
            bs->build.priority      = BUILD_PRIORITY_DEFENSE;
            bs->build.what_to_build = STRUCT_SPIKER;
            bs->ai_state            = BOTSTATE_BUILD;
        }
    }

    /* Priority 3: Periodic repair check — leave build state only briefly */
    if (bs->ai_state == BOTSTATE_BUILD && bs->build.priority == BUILD_PRIORITY_NONE) {
        /* Nothing urgent to build; check for damaged structures */
        bs->build.priority      = BUILD_PRIORITY_REPAIR;
        bs->build.what_to_build = STRUCT_NONE;
    }

    /* Strategic egg placement: if aliens are pushing (more spawn points
     * than minimum), place next egg closer to front line */
    if (bs->build.what_to_build == STRUCT_EGG &&
        bs->build.spawn_count >= BREEDER_MIN_EGG_COUNT) {
        bs->build.priority = BUILD_PRIORITY_EXPAND;
    }
}
