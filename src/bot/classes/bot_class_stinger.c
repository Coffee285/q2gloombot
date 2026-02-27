/*
 * bot_class_stinger.c -- Stinger AI (versatile hybrid alien)
 *
 * The Stinger combines ranged stinger shots with a powerful tail-whip
 * melee attack.  It kites weaker humans and duels most classes 1v1.
 *
 * Behavior:
 *  - Hybrid engagement: open with ranged stinger at 400-600 units,
 *    then close for melee
 *  - Kite Engineer/Biotech with melee; strafe HT/Exterminator with ranged
 *  - Lead projectiles against moving targets
 *  - Strong duelist — fight most humans 1v1
 *  - Escort Breeders: stay near builder, intercept threats
 *  - Upgrade to Guardian or Stalker when frags permit
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define STINGER_RANGED_MIN      400.0f  /* preferred ranged engagement min  */
#define STINGER_RANGED_MAX      600.0f  /* preferred ranged engagement max  */
#define STINGER_MELEE_RANGE     180.0f  /* switch to melee within this dist */
#define STINGER_RETREAT_HP_PCT  0.30f   /* retreat threshold                */
#define STINGER_UPGRADE_FRAGS   4       /* frags to consider upgrade        */

/* -----------------------------------------------------------------------
   BotClass_Init_stinger
   ----------------------------------------------------------------------- */
void BotClass_Init_stinger(bot_state_t *bs)
{
    bs->combat.engagement_range = STINGER_RANGED_MIN;
    bs->combat.prefer_cover     = true;
    bs->combat.max_range_engage = false;
    bs->nav.wall_walking        = true;

    bs->ai_state = BOTSTATE_PATROL;
}

/* -----------------------------------------------------------------------
   BotClass_Think_stinger
   ----------------------------------------------------------------------- */
void BotClass_Think_stinger(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Retreat when low on health */
    if (hp_pct < STINGER_RETREAT_HP_PCT) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Upgrade evaluation */
    if (bs->frag_count >= STINGER_UPGRADE_FRAGS &&
        bs->evos >= Gloom_NextEvoCost(bs->gloom_class) &&
        bs->ai_state != BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_EVOLVE;
        return;
    }

    /* Dynamic engagement range based on target class */
    if (bs->ai_state == BOTSTATE_COMBAT && bs->combat.target != NULL) {
        bot_state_t *tgt = Bot_GetState(bs->combat.target);

        if (tgt) {
            /* Melee-weak humans: close and tail-whip */
            if (tgt->gloom_class == GLOOM_CLASS_ENGINEER ||
                tgt->gloom_class == GLOOM_CLASS_BIOTECH ||
                tgt->gloom_class == GLOOM_CLASS_GRUNT) {
                bs->combat.engagement_range = STINGER_MELEE_RANGE;
            } else {
                /* Ranged or armored threats: stay at stinger range */
                bs->combat.engagement_range = STINGER_RANGED_MIN;
                bs->combat.max_range_engage = true;
            }
        }
    }

    /* Breeder escort: if nearby Breeder, stay in defend mode when no target */
    if (bs->ai_state == BOTSTATE_IDLE || bs->ai_state == BOTSTATE_PATROL) {
        int i;
        for (i = 0; i < MAX_BOTS; i++) {
            if (g_bots[i].in_use &&
                g_bots[i].team == TEAM_ALIEN &&
                g_bots[i].gloom_class == GLOOM_CLASS_BREEDER) {
                bs->ai_state = BOTSTATE_ESCORT;
                break;
            }
        }
    }

    /* Engage on sight */
    if (bs->combat.target_visible && bs->ai_state != BOTSTATE_COMBAT) {
        bs->nav.wall_walking = true;
        bs->ai_state         = BOTSTATE_COMBAT;
    }
}
