/*
 * bot_class_st.c -- Shock Trooper AI (shotgun-wielding armored soldier)
 *
 * Behavior:
 *  - Close to medium range fighter (150-400 units effective)
 *  - Aggressive: push toward aliens, absorb hits with armor
 *  - Shotgun devastating at close range — rush aliens after they commit
 *  - Excellent anti-Hatchling/Drone class
 *  - Protect Engineers: guard builder, intercept approaching aliens
 *  - Hold chokepoints: blast aliens through doorways
 *  - Upgrade to HT or Exterminator when frags permit
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define ST_ENGAGE_RANGE_MIN    150.0f  /* minimum shotgun range             */
#define ST_ENGAGE_RANGE_MAX    400.0f  /* maximum effective range           */
#define ST_RETREAT_HP_PCT      0.25f   /* retreat threshold                 */
#define ST_UPGRADE_FRAGS       3       /* frags to consider upgrade         */

/* -----------------------------------------------------------------------
   BotClass_Init_st
   ----------------------------------------------------------------------- */
void BotClass_Init_st(bot_state_t *bs)
{
    bs->combat.engagement_range = ST_ENGAGE_RANGE_MIN;
    bs->combat.prefer_cover     = false; /* ST charges in                   */
    bs->combat.max_range_engage = false; /* close-range preferred           */
    bs->nav.wall_walking        = false;

    bs->ai_state = BOTSTATE_PATROL;
}

/* -----------------------------------------------------------------------
   BotClass_Think_st
   ----------------------------------------------------------------------- */
void BotClass_Think_st(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Retreat when armor is spent */
    if (hp_pct < ST_RETREAT_HP_PCT) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Upgrade evaluation */
    if (bs->frag_count >= ST_UPGRADE_FRAGS &&
        bs->credits >= gloom_class_info[GLOOM_CLASS_HT].credit_cost &&
        bs->ai_state != BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_UPGRADE;
        return;
    }

    /* ST is aggressive: push into aliens at close range */
    if (bs->ai_state == BOTSTATE_COMBAT) {
        bs->combat.engagement_range = ST_ENGAGE_RANGE_MIN;
    }

    /* Engineer escort: guard the nearest Engineer if one exists */
    if ((bs->ai_state == BOTSTATE_IDLE || bs->ai_state == BOTSTATE_PATROL) &&
        !bs->combat.target_visible) {
        int i;
        for (i = 0; i < MAX_BOTS; i++) {
            if (g_bots[i].in_use &&
                g_bots[i].team == TEAM_HUMAN &&
                g_bots[i].gloom_class == GLOOM_CLASS_ENGINEER) {
                bs->ai_state = BOTSTATE_ESCORT;
                break;
            }
        }
    }

    /* Engage on sight */
    if (bs->combat.target_visible && bs->ai_state != BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_COMBAT;
    }
}
