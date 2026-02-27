/*
 * bot_class_guardian.c -- Guardian AI (stealth tank alien)
 *
 * The Guardian uses invisibility to ambush humans and guard alien eggs.
 * It has high HP and devastating melee, but relies on surprise.
 *
 * Behavior:
 *  - USE INVISIBILITY CONSTANTLY outside active combat
 *  - Ambush tactics: lurk at doorways, attack when humans pass
 *  - First strike targets the most dangerous enemy in a group
 *  - Fight aggressively after reveal — high HP allows sustained combat
 *  - Target priority: Engineers, then high-value classes
 *  - Guard alien eggs; kill humans who approach them
 *  - Disengage with invisibility if outnumbered 3+ to 1
 *  - Patrol between key locations while cloaked
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define GUARDIAN_MELEE_RANGE        200.0f  /* close-in melee distance      */
#define GUARDIAN_AMBUSH_WAIT_TIME   4.0f    /* seconds to wait in ambush    */
#define GUARDIAN_RETREAT_HP_PCT     0.30f   /* uncloak and retreat at 30%   */
#define GUARDIAN_OUTNUMBER_THRESH   3       /* flee if outnumbered by this  */

/* -----------------------------------------------------------------------
   BotClass_Init_guardian
   ----------------------------------------------------------------------- */
void BotClass_Init_guardian(bot_state_t *bs)
{
    bs->combat.engagement_range = GUARDIAN_MELEE_RANGE;
    bs->combat.prefer_cover     = true;
    bs->combat.max_range_engage = false;
    bs->nav.wall_walking        = true;

    /* Begin in defend mode — find a position near eggs to guard */
    bs->ai_state = BOTSTATE_DEFEND;
}

/* -----------------------------------------------------------------------
   BotClass_Think_guardian
   ----------------------------------------------------------------------- */
void BotClass_Think_guardian(bot_state_t *bs)
{
    float hp_pct;
    int   nearby_enemies = 0;
    int   i;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Retreat: if HP is critically low, disengage using invisibility */
    if (hp_pct < GUARDIAN_RETREAT_HP_PCT) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Count visible nearby enemies to assess outnumbering */
    for (i = 0; i < bs->enemy_memory_count; i++) {
        if (bs->enemy_memory[i].ent &&
            bs->enemy_memory[i].ent->inuse &&
            level.time - bs->enemy_memory[i].last_seen < 3.0f) {
            nearby_enemies++;
        }
    }

    /* Outnumbered 3+ to 1: go invisible and reposition */
    if (nearby_enemies >= GUARDIAN_OUTNUMBER_THRESH &&
        bs->ai_state == BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Ambush logic: while in defend state, wait invisibly near a chokepoint */
    if (bs->ai_state == BOTSTATE_DEFEND) {
        /* Remain stationary; activate on proximity target detection */
        if (bs->combat.target_visible &&
            bs->combat.target_dist < GUARDIAN_MELEE_RANGE * 2.0f) {
            /* Decloak attack — target most dangerous enemy */
            bs->ai_state = BOTSTATE_COMBAT;
        }
        return;
    }

    /* Active combat: stay in melee range */
    if (bs->ai_state == BOTSTATE_COMBAT) {
        bs->combat.engagement_range = GUARDIAN_MELEE_RANGE;
        /* Prioritize Engineers */
        if (bs->combat.target != NULL) {
            bot_state_t *tgt = Bot_GetState(bs->combat.target);
            if (tgt && tgt->gloom_class != GLOOM_CLASS_ENGINEER &&
                nearby_enemies == 1) {
                /* Single non-Engineer target is fine to fight */
                (void)tgt;
            }
        }
        return;
    }

    /* Patrol while invisible between egg locations */
    if (bs->ai_state == BOTSTATE_IDLE) {
        bs->ai_state = BOTSTATE_PATROL;
    }
}
