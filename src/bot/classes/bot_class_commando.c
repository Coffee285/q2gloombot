/*
 * bot_class_commando.c -- Commando AI (fast assault with uzi and grenades)
 *
 * Behavior:
 *  - Aggressive flanker: circle around alien positions
 *  - Uzi at medium range (200-500 units), grenades for area denial
 *  - Throw grenades into rooms before entering
 *  - Throw grenades at alien Eggs from around corners
 *  - Fast movement: constant strafing, never stand still
 *  - Hit-and-run: engage, deal damage, retreat before aliens close
 *  - Scout ahead of team, expose alien positions
 *  - Solo harassment of alien builders — find and kill Breeders
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define COMMANDO_ENGAGE_RANGE_MIN  200.0f  /* uzi minimum effective range   */
#define COMMANDO_ENGAGE_RANGE_MAX  500.0f  /* uzi maximum effective range   */
#define COMMANDO_GRENADE_RANGE     400.0f  /* grenade toss range            */
#define COMMANDO_RETREAT_HP_PCT    0.30f   /* retreat threshold             */
#define COMMANDO_STRAFE_INTERVAL   0.5f    /* seconds between direction changes */

/* -----------------------------------------------------------------------
   BotClass_Init_commando
   ----------------------------------------------------------------------- */
void BotClass_Init_commando(bot_state_t *bs)
{
    bs->combat.engagement_range = COMMANDO_ENGAGE_RANGE_MIN;
    bs->combat.prefer_cover     = false; /* constant movement, no camping   */
    bs->combat.max_range_engage = false;
    bs->nav.wall_walking        = false;

    /* Commando scouts ahead */
    bs->ai_state = BOTSTATE_PATROL;
}

/* -----------------------------------------------------------------------
   BotClass_Think_commando
   ----------------------------------------------------------------------- */
void BotClass_Think_commando(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Hit-and-run: retreat after taking damage */
    if (hp_pct < COMMANDO_RETREAT_HP_PCT) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Active engagement */
    if (bs->ai_state == BOTSTATE_COMBAT) {
        /*
         * Keep medium range — close enough for uzi effectiveness,
         * far enough to avoid alien melee.
         */
        bs->combat.engagement_range = COMMANDO_ENGAGE_RANGE_MIN +
            (COMMANDO_ENGAGE_RANGE_MAX - COMMANDO_ENGAGE_RANGE_MIN) * 0.3f;

        /* After a short engagement, break off and re-approach from new angle */
        if (level.time - bs->state_enter_time > 3.0f) {
            bs->ai_state = BOTSTATE_PATROL; /* re-flank before re-engaging  */
        }
    }

    /* Engage on sight */
    if (bs->combat.target_visible && bs->ai_state != BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_COMBAT;
    }
}
