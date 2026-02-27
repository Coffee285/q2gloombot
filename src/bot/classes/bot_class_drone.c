/*
 * bot_class_drone.c -- Drone AI (upgraded melee alien)
 *
 * The Drone is an improved melee fighter with better damage output
 * and wall-climbing.  It flanks humans using walls and ceilings.
 *
 * Behavior:
 *  - Aggressive flanking via wall/ceiling paths
 *  - Double-claw attack pattern at close range
 *  - Target mid-tier humans (Grunts, STs, Commandos)
 *  - Jump-attack from elevated positions
 *  - Retreat to Cocoon below 40% HP
 *  - Hunt isolated humans, avoid large groups
 *  - Upgrade to Stinger or Guardian when frags permit
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define DRONE_MELEE_RANGE       150.0f  /* ideal attack distance            */
#define DRONE_RETREAT_HP_PCT    0.40f   /* retreat to Cocoon threshold      */
#define DRONE_JUMP_ATTACK_DIST  300.0f  /* max distance for leap attack     */
#define DRONE_UPGRADE_FRAGS     3       /* frags to consider Stinger/Guard  */

/* -----------------------------------------------------------------------
   BotClass_Init_drone
   ----------------------------------------------------------------------- */
void BotClass_Init_drone(bot_state_t *bs)
{
    bs->combat.engagement_range = DRONE_MELEE_RANGE;
    bs->combat.prefer_cover     = true;
    bs->combat.max_range_engage = false;
    bs->nav.wall_walking        = true;

    bs->ai_state = BOTSTATE_PATROL;
}

/* -----------------------------------------------------------------------
   BotClass_Think_drone
   ----------------------------------------------------------------------- */
void BotClass_Think_drone(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Retreat to Cocoon when sufficiently damaged */
    if (hp_pct < DRONE_RETREAT_HP_PCT && bs->ai_state == BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Upgrade evaluation */
    if (bs->frag_count >= DRONE_UPGRADE_FRAGS &&
        bs->evos >= Gloom_NextEvoCost(bs->gloom_class) &&
        bs->ai_state != BOTSTATE_COMBAT) {
        bs->ai_state = BOTSTATE_EVOLVE;
        return;
    }

    /* Prefer wall-climb approach when hunting targets */
    if (bs->ai_state == BOTSTATE_HUNT || bs->ai_state == BOTSTATE_PATROL) {
        bs->nav.wall_walking = true;
    }

    /* Use close-range engagement distance */
    bs->combat.engagement_range = DRONE_MELEE_RANGE;

    /* Activate if enemy spotted */
    if (bs->combat.target_visible && bs->ai_state != BOTSTATE_COMBAT) {
        bs->nav.wall_walking = true;
        bs->ai_state         = BOTSTATE_COMBAT;
    }

    /* Deprioritize Mechs and HTs — find softer targets */
    if (bs->ai_state == BOTSTATE_COMBAT && bs->combat.target != NULL) {
        bot_state_t *tgt = Bot_GetState(bs->combat.target);
        if (tgt &&
            (tgt->gloom_class == GLOOM_CLASS_MECH ||
             tgt->gloom_class == GLOOM_CLASS_HT)) {
            /* Release this target; bot_combat.c will re-select a softer one */
            bs->combat.target         = NULL;
            bs->combat.target_visible = false;
            bs->ai_state              = BOTSTATE_HUNT;
        }
    }
}
