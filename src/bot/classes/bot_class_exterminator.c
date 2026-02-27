/*
 * bot_class_exterminator.c -- Exterminator AI (heavy assault with pulse rifle)
 *
 * Behavior:
 *  - Primary damage dealer at medium-long range (300-700 units)
 *  - Pulse rifle bursts: controlled fire at center mass
 *  - Grenades to flush aliens from wall-climb positions
 *  - Push into alien territory as part of team assault
 *  - Can solo most alien classes except Stalker
 *  - Cover teammates: suppressive fire during team pushes
 *  - Destroy alien structures from safe range
 *  - Tank role when Mech is unavailable — absorb damage while dealing damage
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define EXTERMINATOR_ENGAGE_RANGE_MIN  300.0f  /* minimum pulse rifle range */
#define EXTERMINATOR_ENGAGE_RANGE_MAX  700.0f  /* maximum useful range      */
#define EXTERMINATOR_RETREAT_HP_PCT    0.20f   /* retreat threshold         */

/* -----------------------------------------------------------------------
   BotClass_Init_exterminator
   ----------------------------------------------------------------------- */
void BotClass_Init_exterminator(bot_state_t *bs)
{
    bs->combat.engagement_range = EXTERMINATOR_ENGAGE_RANGE_MIN;
    bs->combat.prefer_cover     = false; /* aggressive attacker             */
    bs->combat.max_range_engage = true;
    bs->nav.wall_walking        = false;

    bs->ai_state = BOTSTATE_PATROL;
}

/* -----------------------------------------------------------------------
   BotClass_Think_exterminator
   ----------------------------------------------------------------------- */
void BotClass_Think_exterminator(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Retreat only when critically low — Exterminator is very durable */
    if (hp_pct < EXTERMINATOR_RETREAT_HP_PCT) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Active engagement */
    if (bs->ai_state == BOTSTATE_COMBAT) {
        bs->combat.engagement_range = EXTERMINATOR_ENGAGE_RANGE_MIN;
        bs->combat.max_range_engage = true;

        /* Treat Stalker specially — it can threaten even an Exterminator */
        if (bs->combat.target != NULL) {
            bot_state_t *tgt = Bot_GetState(bs->combat.target);
            if (tgt && tgt->gloom_class == GLOOM_CLASS_STALKER) {
                /* Back off slightly from Stalkers */
                bs->combat.engagement_range = EXTERMINATOR_ENGAGE_RANGE_MIN * 1.5f;
            }
        }
    }

    /* Engage on sight */
    if (bs->combat.target_visible && bs->ai_state != BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_COMBAT;
    }
}
