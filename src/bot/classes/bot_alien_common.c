/*
 * bot_alien_common.c -- shared AI logic for all alien classes
 *
 * Provides helper functions used by every alien class:
 *   - Class upgrade decision-making
 *   - Retreat-to-Cocoon pathfinding
 *   - Egg awareness (known spawn point tracking)
 *   - Team composition checks (avoid over-stacking)
 */

#include "../bot.h"
#include "../nav/bot_nav.h"

/* -----------------------------------------------------------------------
   BotAlien_CountClass
   Count how many bots on the alien team are currently playing a class.
   ----------------------------------------------------------------------- */
static int BotAlien_CountClass(gloom_class_t cls)
{
    int i, count = 0;
    for (i = 0; i < MAX_BOTS; i++) {
        if (g_bots[i].in_use &&
            g_bots[i].team == TEAM_ALIEN &&
            g_bots[i].gloom_class == cls) {
            count++;
        }
    }
    return count;
}

/* -----------------------------------------------------------------------
   BotAlien_HasBreeder
   Returns true if at least one alien bot is playing Breeder.
   ----------------------------------------------------------------------- */
qboolean BotAlien_HasBreeder(void)
{
    return BotAlien_CountClass(GLOOM_CLASS_BREEDER) > 0 ? true : false;
}

/* -----------------------------------------------------------------------
   BotAlien_ChooseUpgrade
   Evaluate frag count, team composition, and game state to select the
   best class to evolve/spawn as.

   Rules enforced:
     - At most 2 Stalkers simultaneously
     - At most 2 Guardians simultaneously
     - Always ensure at least 1 Breeder; if none exists, this bot becomes one
     - Prefer classes the team is short on
   ----------------------------------------------------------------------- */
gloom_class_t BotAlien_ChooseUpgrade(bot_state_t *bs)
{
    int frags = bs->frag_count;

    /* Highest priority: ensure a Breeder exists */
    if (!BotAlien_HasBreeder() && bs->evos >= gloom_class_info[GLOOM_CLASS_BREEDER].evo_cost) {
        return GLOOM_CLASS_BREEDER;
    }

    /* Stalker: powerful but cap at 2 */
    if (frags >= 6 && BotAlien_CountClass(GLOOM_CLASS_STALKER) < 2 &&
        bs->evos >= gloom_class_info[GLOOM_CLASS_STALKER].evo_cost) {
        return GLOOM_CLASS_STALKER;
    }

    /* Guardian: stealth tank, cap at 2 */
    if (frags >= 4 && BotAlien_CountClass(GLOOM_CLASS_GUARDIAN) < 2 &&
        bs->evos >= gloom_class_info[GLOOM_CLASS_GUARDIAN].evo_cost) {
        return GLOOM_CLASS_GUARDIAN;
    }

    /* Stinger: versatile mid-tier */
    if (frags >= 2 && bs->evos >= gloom_class_info[GLOOM_CLASS_STINGER].evo_cost) {
        return GLOOM_CLASS_STINGER;
    }

    /* Drone: first upgrade from Hatchling */
    if (frags >= 1 && bs->evos >= gloom_class_info[GLOOM_CLASS_DRONE].evo_cost) {
        return GLOOM_CLASS_DRONE;
    }

    /* Default: stay as Hatchling */
    return GLOOM_CLASS_HATCHLING;
}

/* -----------------------------------------------------------------------
   BotAlien_ShouldRetreatToCocoon
   Returns true when the bot is damaged enough to warrant retreating to
   a Cocoon structure for healing.  Threshold varies by class tier.
   ----------------------------------------------------------------------- */
qboolean BotAlien_ShouldRetreatToCocoon(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || bs->ent->max_health <= 0) return false;
    hp_pct = (float)bs->ent->health / (float)bs->ent->max_health;

    switch (bs->gloom_class) {
    case GLOOM_CLASS_STALKER:
    case GLOOM_CLASS_GUARDIAN:
        return hp_pct < 0.30f; /* tankier classes retreat later */
    case GLOOM_CLASS_BREEDER:
        return hp_pct < 0.60f; /* Breeder is fragile and valuable */
    default:
        return hp_pct < 0.40f;
    }
}

/* -----------------------------------------------------------------------
   BotAlien_GetNearestEgg
   Return the index of the nearest known alien Egg spawn point, or
   BOT_INVALID_NODE if none is known.  Used for retreat and respawn logic.
   ----------------------------------------------------------------------- */
int BotAlien_GetNearestEgg(bot_state_t *bs)
{
    /* Navigation to eggs is handled via nav nodes flagged NAV_EGG. */
    if (!bs || !bs->ent) return BOT_INVALID_NODE;
    return BotNav_NearestNode(bs->ent->s.origin, NAV_EGG);
}

/* -----------------------------------------------------------------------
   BotAlien_InitDefaults
   Apply common alien bot defaults (wall-walk awareness, prefer cover).
   Called from each alien class's BotClass_Init_* function.
   ----------------------------------------------------------------------- */
void BotAlien_InitDefaults(bot_state_t *bs)
{
    bs->combat.prefer_cover     = true;
    bs->combat.max_range_engage = false;
    bs->nav.wall_walking        = Gloom_ClassCanWallWalk(bs->gloom_class) ? true : false;
}
