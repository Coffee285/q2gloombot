/*
 * bot_class_grunt.c -- Grunt AI (basic infantry, starter class)
 *
 * Behavior:
 *  - Medium-range engagement with machinegun (300-600 units)
 *  - Stay in groups; don't wander alone
 *  - Backpedal while firing against melee aliens
 *  - Target Hatchlings and Drones first (easier kills)
 *  - Retreat to Engineer defenses when overwhelmed
 *  - Upgrade to ST or Commando at 1-2 frags
 *  - Fire in controlled bursts for accuracy at longer ranges
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define GRUNT_ENGAGE_RANGE_MIN  300.0f  /* minimum effective range          */
#define GRUNT_ENGAGE_RANGE_MAX  600.0f  /* maximum effective range          */
#define GRUNT_FLEE_HP_PCT       0.30f   /* retreat threshold                */
#define GRUNT_UPGRADE_FRAGS     2       /* frags to seek upgrade            */

/* -----------------------------------------------------------------------
   BotClass_Init_grunt
   ----------------------------------------------------------------------- */
void BotClass_Init_grunt(bot_state_t *bs)
{
    bs->combat.engagement_range = (GRUNT_ENGAGE_RANGE_MIN +
                                   GRUNT_ENGAGE_RANGE_MAX) * 0.5f;
    bs->combat.prefer_cover     = false;
    bs->combat.max_range_engage = true;  /* engage at comfortable range     */
    bs->nav.wall_walking        = false;

    bs->ai_state = BOTSTATE_PATROL;
}

/* -----------------------------------------------------------------------
   BotClass_Think_grunt
   ----------------------------------------------------------------------- */
void BotClass_Think_grunt(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Retreat toward base when low HP */
    if (hp_pct < GRUNT_FLEE_HP_PCT) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Upgrade evaluation */
    if (bs->frag_count >= GRUNT_UPGRADE_FRAGS &&
        bs->credits >= gloom_class_info[GLOOM_CLASS_ST].credit_cost &&
        bs->ai_state != BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_UPGRADE;
        return;
    }

    /* Maintain medium engagement range — backpedal if alien closes in */
    if (bs->ai_state == BOTSTATE_COMBAT) {
        bs->combat.engagement_range = GRUNT_ENGAGE_RANGE_MIN;
        if (bs->combat.target_dist < GRUNT_ENGAGE_RANGE_MIN) {
            /* Alien is too close — back off while firing */
            bs->combat.engagement_range = GRUNT_ENGAGE_RANGE_MAX;
        }
    }

    /* Engage targets on sight */
    if (bs->combat.target_visible && bs->ai_state != BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_COMBAT;
    }
}
