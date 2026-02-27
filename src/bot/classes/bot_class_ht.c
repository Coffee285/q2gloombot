/*
 * bot_class_ht.c -- Heavy Trooper AI (rocket launcher specialist)
 *
 * Behavior:
 *  - Long range engagement (400-900 units ideal)
 *  - Lead targets with rockets — predict enemy movement
 *  - Aim at feet/ground near target for splash damage against fast aliens
 *  - NEVER fire rockets at close range — retreat if aliens close in
 *  - Destroy alien Eggs and Spikers with rockets from safe distance
 *  - Hold elevated positions for better rocket angles
 *  - Suppress alien chokepoints with rocket spam
 *  - Self-damage awareness: maintain 300+ units from impact point
 *  - Upgrade to Exterminator when frags permit
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define HT_ENGAGE_RANGE_MIN    400.0f  /* minimum safe rocket range        */
#define HT_ENGAGE_RANGE_MAX    900.0f  /* maximum useful range             */
#define HT_DANGER_RANGE        300.0f  /* retreat if alien gets this close  */
#define HT_SELF_DAMAGE_RANGE   300.0f  /* don't shoot within this distance */
#define HT_RETREAT_HP_PCT      0.35f   /* retreat threshold                */
#define HT_UPGRADE_FRAGS       4       /* frags to consider upgrade        */

/* -----------------------------------------------------------------------
   BotClass_Init_ht
   ----------------------------------------------------------------------- */
void BotClass_Init_ht(bot_state_t *bs)
{
    bs->combat.engagement_range = HT_ENGAGE_RANGE_MIN;
    bs->combat.prefer_cover     = false; /* needs open sightlines           */
    bs->combat.max_range_engage = true;  /* always engage at maximum range  */
    bs->nav.wall_walking        = false;

    bs->ai_state = BOTSTATE_PATROL;
}

/* -----------------------------------------------------------------------
   BotClass_Think_ht
   ----------------------------------------------------------------------- */
void BotClass_Think_ht(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Retreat when low HP */
    if (hp_pct < HT_RETREAT_HP_PCT) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Upgrade evaluation */
    if (bs->frag_count >= HT_UPGRADE_FRAGS &&
        bs->credits >= gloom_class_info[GLOOM_CLASS_EXTERMINATOR].credit_cost &&
        bs->ai_state != BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_UPGRADE;
        return;
    }

    /* Critical safety rule: if alien is within danger range, retreat */
    if (bs->combat.target_dist < HT_DANGER_RANGE &&
        bs->combat.target_visible) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Maintain long engagement range at all times */
    bs->combat.engagement_range = HT_ENGAGE_RANGE_MIN;
    bs->combat.max_range_engage = true;

    /* Engage targets on sight at maximum range */
    if (bs->combat.target_visible && bs->ai_state != BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_COMBAT;
    }
}
