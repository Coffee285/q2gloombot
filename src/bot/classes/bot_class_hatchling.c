/*
 * bot_class_hatchling.c -- Hatchling AI (starter alien class)
 *
 * The Hatchling is fast and expendable.  Its goal is to accumulate
 * frags to evolve into a better class while exploiting wall-climbing
 * to approach from unexpected angles.
 *
 * Behavior:
 *  - Rush humans using speed and wall-climbing
 *  - Approach from walls/ceilings to drop onto humans from above
 *  - Attack in groups; avoid fights against Mechs or groups
 *  - Target isolated humans, especially Engineers
 *  - Upgrade to Drone at 1 frag
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define HATCHLING_RUSH_RANGE      600.0f  /* start rushing within this dist  */
#define HATCHLING_FLEE_HP_PCT     0.25f   /* flee when very low health       */
#define HATCHLING_GROUP_THREAT    3       /* retreat if this many enemies     */
#define HATCHLING_UPGRADE_FRAGS   1       /* frags needed to evolve          */

/* -----------------------------------------------------------------------
   BotClass_Init_hatchling
   ----------------------------------------------------------------------- */
void BotClass_Init_hatchling(bot_state_t *bs)
{
    bs->combat.engagement_range = gloom_class_info[GLOOM_CLASS_HATCHLING].preferred_range;
    bs->combat.prefer_cover     = true;
    bs->combat.max_range_engage = false;
    bs->nav.wall_walking        = true;

    /* Hatchlings start in patrol mode, rushing toward humans on sight */
    bs->ai_state = BOTSTATE_PATROL;
}

/* -----------------------------------------------------------------------
   BotClass_Think_hatchling
   ----------------------------------------------------------------------- */
void BotClass_Think_hatchling(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Check for upgrade opportunity */
    if (bs->frag_count >= HATCHLING_UPGRADE_FRAGS &&
        bs->evos >= Gloom_NextEvoCost(bs->gloom_class) &&
        bs->ai_state != BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_EVOLVE;
        return;
    }

    /* Flee when critically wounded */
    if (hp_pct < HATCHLING_FLEE_HP_PCT) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* On enemy sight: switch to aggressive rush */
    if (bs->combat.target_visible && bs->ai_state != BOTSTATE_COMBAT) {
        /*
         * Prefer wall/ceiling approach routes.  Navigation subsystem uses
         * nav.wall_walking flag to select wall-climb edges when available.
         */
        bs->nav.wall_walking = true;
        bs->ai_state         = BOTSTATE_COMBAT;
    }

    /* If in combat and target is a high-threat class (Mech), retreat */
    if (bs->ai_state == BOTSTATE_COMBAT && bs->combat.target != NULL) {
        bot_state_t *tgt_state = Bot_GetState(bs->combat.target);
        if (tgt_state &&
            (tgt_state->gloom_class == GLOOM_CLASS_MECH ||
             tgt_state->gloom_class == GLOOM_CLASS_HT)) {
            bs->ai_state = BOTSTATE_FLEE;
        }
    }
}
