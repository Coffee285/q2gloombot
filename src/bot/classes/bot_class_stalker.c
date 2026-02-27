/*
 * bot_class_stalker.c -- Stalker AI (ultimate alien melee tank)
 *
 * The Stalker is the most powerful alien — massive health and devastating
 * melee.  It leads front-line assaults and destroys human infrastructure.
 *
 * Behavior:
 *  - Front-line assault: push directly into human positions
 *  - Target priority: Teleporters > Turrets > humans
 *  - Tank for the team: absorb damage while smaller aliens flank
 *  - Chase fleeing humans relentlessly
 *  - Ignore minor threats (focus on objectives)
 *  - Retreat to Cocoon below 30% HP rather than dying
 *  - Coordinate pushes with teammates before charging
 *  - Destroy teleporters and turrets when no humans visible
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define STALKER_MELEE_RANGE       150.0f  /* preferred melee distance       */
#define STALKER_RETREAT_HP_PCT    0.30f   /* retreat to Cocoon threshold    */
#define STALKER_PUSH_WAIT         3.0f    /* seconds to wait for allies     */
#define STALKER_CHASE_DIST_MAX    800.0f  /* don't chase targets beyond this*/

/* -----------------------------------------------------------------------
   BotClass_Init_stalker
   ----------------------------------------------------------------------- */
void BotClass_Init_stalker(bot_state_t *bs)
{
    bs->combat.engagement_range = STALKER_MELEE_RANGE;
    bs->combat.prefer_cover     = false; /* Stalker tanks hits, doesn't hide*/
    bs->combat.max_range_engage = false;
    bs->nav.wall_walking        = true;

    /* Begin by assessing the team situation before charging */
    bs->ai_state = BOTSTATE_IDLE;
}

/* -----------------------------------------------------------------------
   BotClass_Think_stalker
   ----------------------------------------------------------------------- */
void BotClass_Think_stalker(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Retreat to Cocoon when low HP rather than dying pointlessly */
    if (hp_pct < STALKER_RETREAT_HP_PCT && bs->ai_state == BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Aggressive engagement: close to melee range immediately */
    if (bs->combat.target_visible && bs->ai_state != BOTSTATE_COMBAT) {
        bs->combat.engagement_range = STALKER_MELEE_RANGE;
        bs->ai_state                = BOTSTATE_COMBAT;
        return;
    }

    /* Objective hunting: when no player target, seek and destroy structures */
    if ((bs->ai_state == BOTSTATE_IDLE || bs->ai_state == BOTSTATE_PATROL) &&
        bs->combat.target == NULL) {
        /*
         * Structure targeting (Teleporters first, then Turrets) is handled
         * by BotCombat_PickTarget with TARGET_PRIORITY_SPAWN and
         * TARGET_PRIORITY_DEFENSE flags.  We just ensure patrol mode.
         */
        bs->ai_state = BOTSTATE_PATROL;
    }

    /* Chase fleeing humans within range */
    if (bs->ai_state == BOTSTATE_HUNT &&
        bs->combat.target_dist > STALKER_CHASE_DIST_MAX) {
        /* Target escaped too far — release and find next objective */
        bs->combat.target         = NULL;
        bs->combat.target_visible = false;
        bs->ai_state              = BOTSTATE_PATROL;
    }

    /* Don't waste time chasing Hatchlings or Drones (they're allies) */
    /* Stalker should never target its own team — handled by game logic */
}
