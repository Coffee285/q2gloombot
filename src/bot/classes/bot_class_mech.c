/*
 * bot_class_mech.c -- Mech AI (power armor — the ultimate human unit)
 *
 * Behavior:
 *  - SLOW AND POWERFUL: advance methodically, never rush
 *  - Dual lasers: continuous fire on any visible alien
 *  - Prioritize Stalkers and Guardians (only things that threaten a Mech)
 *  - Act as team anchor: advance at front of group, absorb alien attacks
 *  - Draw alien attention: fire at everything
 *  - Protect team rear: occasionally turn to check for flanking aliens
 *  - Destroy structures: laser alien Eggs and Spikers systematically
 *  - Avoid water and narrow corridors — Mech mobility is very limited
 *  - Fall back to Biotech for healing if below 20% HP
 *  - Stay near Biotechs for healing support
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define MECH_ENGAGE_RANGE_MIN  300.0f  /* lasers effective minimum range   */
#define MECH_ENGAGE_RANGE_MAX  700.0f  /* lasers effective maximum range   */
#define MECH_RETREAT_HP_PCT    0.20f   /* retreat to Biotech threshold     */
#define MECH_REAR_CHECK_INTERVAL 5.0f  /* check flanks every N seconds     */

/* -----------------------------------------------------------------------
   BotClass_Init_mech
   ----------------------------------------------------------------------- */
void BotClass_Init_mech(bot_state_t *bs)
{
    bs->combat.engagement_range = MECH_ENGAGE_RANGE_MIN;
    bs->combat.prefer_cover     = false; /* Mech tanks in the open         */
    bs->combat.max_range_engage = true;
    bs->nav.wall_walking        = false; /* Mech cannot wall-walk           */

    /* Mech leads the advance */
    bs->ai_state = BOTSTATE_PATROL;
}

/* -----------------------------------------------------------------------
   BotClass_Think_mech
   ----------------------------------------------------------------------- */
void BotClass_Think_mech(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Critical HP: fall back to Biotech rather than dying */
    if (hp_pct < MECH_RETREAT_HP_PCT) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Active engagement */
    if (bs->ai_state == BOTSTATE_COMBAT) {
        bs->combat.engagement_range = MECH_ENGAGE_RANGE_MIN;
        bs->combat.max_range_engage = true;

        /* Priority override: target Stalkers and Guardians first */
        if (bs->combat.target != NULL) {
            bot_state_t *tgt = Bot_GetState(bs->combat.target);
            if (tgt &&
                tgt->gloom_class != GLOOM_CLASS_STALKER &&
                tgt->gloom_class != GLOOM_CLASS_GUARDIAN) {
                /* Check if a higher-priority target exists in memory */
                int i;
                for (i = 0; i < bs->enemy_memory_count; i++) {
                    bot_state_t *mem_tgt = Bot_GetState(bs->enemy_memory[i].ent);
                    if (mem_tgt &&
                        (mem_tgt->gloom_class == GLOOM_CLASS_STALKER ||
                         mem_tgt->gloom_class == GLOOM_CLASS_GUARDIAN)) {
                        bs->combat.target       = bs->enemy_memory[i].ent;
                        break;
                    }
                }
            }
        }
    }

    /* Engage on sight */
    if (bs->combat.target_visible && bs->ai_state != BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_COMBAT;
    }
}
