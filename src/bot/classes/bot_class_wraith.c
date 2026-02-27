/*
 * bot_class_wraith.c -- Wraith AI (flying alien with acid spit)
 *
 * The Wraith maintains altitude advantage and harasses humans with
 * ranged acid projectiles.  It uses flight to reach positions humans
 * cannot easily target.
 *
 * Behavior:
 *  - Maintain altitude advantage; fly above human sight lines
 *  - Strafe in air while spitting acid — never hover stationary
 *  - Circle-strafe at 300-500 unit range
 *  - Target distracted humans (those fighting other aliens)
 *  - Retreat by flying upward when damaged
 *  - Harass human builders; fly over base, spit, fly away
 *  - Avoid tight corridors; prefer open rooms and outdoor areas
 *  - Scout and expose human positions
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define WRAITH_PREFERRED_RANGE_MIN  300.0f /* minimum spit range           */
#define WRAITH_PREFERRED_RANGE_MAX  500.0f /* maximum spit range           */
#define WRAITH_RETREAT_HP_PCT       0.35f  /* retreat upward on low HP     */
#define WRAITH_HARASS_INTERVAL      5.0f   /* seconds between harass runs  */
#define WRAITH_CORRIDOR_WIDTH_MIN   200.0f /* avoid corridors narrower than this */

/* -----------------------------------------------------------------------
   BotClass_Init_wraith
   ----------------------------------------------------------------------- */
void BotClass_Init_wraith(bot_state_t *bs)
{
    bs->combat.engagement_range = (WRAITH_PREFERRED_RANGE_MIN +
                                   WRAITH_PREFERRED_RANGE_MAX) * 0.5f;
    bs->combat.prefer_cover     = false; /* flies in open, uses altitude   */
    bs->combat.max_range_engage = true;  /* engage at long range           */
    bs->nav.wall_walking        = false; /* flight, not wall-climbing      */

    /* Wraith starts patrolling at altitude */
    bs->ai_state = BOTSTATE_PATROL;
}

/* -----------------------------------------------------------------------
   BotClass_Think_wraith
   ----------------------------------------------------------------------- */
void BotClass_Think_wraith(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Retreat by flying upward when damaged */
    if (hp_pct < WRAITH_RETREAT_HP_PCT) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Maintain preferred range: strafe while spitting */
    if (bs->ai_state == BOTSTATE_COMBAT) {
        /*
         * Keep engagement range in the spit range band.
         * combat subsystem handles actual movement, but we set the hint here.
         */
        bs->combat.engagement_range = WRAITH_PREFERRED_RANGE_MIN +
            (WRAITH_PREFERRED_RANGE_MAX - WRAITH_PREFERRED_RANGE_MIN) * 0.5f;
        bs->combat.max_range_engage = true;

        /* Prefer targets that are already fighting other aliens */
        if (bs->combat.target != NULL) {
            bot_state_t *tgt = Bot_GetState(bs->combat.target);
            (void)tgt; /* target selection refined in BotCombat_PickTarget */
        }
    }

    /* Harass: if no direct combat target, fly toward human base */
    if ((bs->ai_state == BOTSTATE_IDLE || bs->ai_state == BOTSTATE_PATROL) &&
        level.time - bs->state_enter_time > WRAITH_HARASS_INTERVAL) {
        /* Scout: transition to patrol to seek human structures */
        bs->ai_state = BOTSTATE_PATROL;
    }

    /* Engage targets on sight */
    if (bs->combat.target_visible && bs->ai_state != BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_COMBAT;
    }
}
