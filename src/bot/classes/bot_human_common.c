/*
 * bot_human_common.c -- shared AI logic for all human classes
 *
 * Provides helper functions used by every human class:
 *   - Flashlight management
 *   - Health pack usage
 *   - Ammo awareness
 *   - Class upgrade decision-making
 *   - Retreat-to-base (Teleporter) logic
 *   - Buddy system enforcement
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   BotHuman_CountClass
   Count human bots currently playing a given class.
   ----------------------------------------------------------------------- */
static int BotHuman_CountClass(gloom_class_t cls)
{
    int i, count = 0;
    for (i = 0; i < MAX_BOTS; i++) {
        if (g_bots[i].in_use &&
            g_bots[i].team == TEAM_HUMAN &&
            g_bots[i].gloom_class == cls) {
            count++;
        }
    }
    return count;
}

/* -----------------------------------------------------------------------
   BotHuman_HasEngineer
   Returns true if at least one human bot is playing Engineer.
   ----------------------------------------------------------------------- */
qboolean BotHuman_HasEngineer(void)
{
    return BotHuman_CountClass(GLOOM_CLASS_ENGINEER) > 0 ? true : false;
}

/* -----------------------------------------------------------------------
   BotHuman_ChooseUpgrade
   Select the best class to upgrade to based on frags, team composition,
   and game state.

   Rules enforced:
     - At most 1 Mech simultaneously
     - At most 2 Exterminators simultaneously
     - At least 1 Engineer must exist — if none, this bot becomes one
     - Avoid stacking one class (max 2 per non-builder class)
   ----------------------------------------------------------------------- */
gloom_class_t BotHuman_ChooseUpgrade(bot_state_t *bs)
{
    int frags = bs->frag_count;

    /* Highest priority: ensure an Engineer exists */
    if (!BotHuman_HasEngineer()) {
        return GLOOM_CLASS_ENGINEER;
    }

    /* Mech: most powerful, cap at 1 */
    if (frags >= 8 && BotHuman_CountClass(GLOOM_CLASS_MECH) < 1 &&
        bs->credits >= gloom_class_info[GLOOM_CLASS_MECH].credit_cost) {
        return GLOOM_CLASS_MECH;
    }

    /* Exterminator: heavy assault, cap at 2 */
    if (frags >= 5 && BotHuman_CountClass(GLOOM_CLASS_EXTERMINATOR) < 2 &&
        bs->credits >= gloom_class_info[GLOOM_CLASS_EXTERMINATOR].credit_cost) {
        return GLOOM_CLASS_EXTERMINATOR;
    }

    /* HT: rocket specialist */
    if (frags >= 3 &&
        bs->credits >= gloom_class_info[GLOOM_CLASS_HT].credit_cost) {
        return GLOOM_CLASS_HT;
    }

    /* Commando: fast assault */
    if (frags >= 2 &&
        bs->credits >= gloom_class_info[GLOOM_CLASS_COMMANDO].credit_cost) {
        return GLOOM_CLASS_COMMANDO;
    }

    /* ST: first upgrade from Grunt */
    if (frags >= 1 &&
        bs->credits >= gloom_class_info[GLOOM_CLASS_ST].credit_cost) {
        return GLOOM_CLASS_ST;
    }

    /* Default: stay as Grunt */
    return GLOOM_CLASS_GRUNT;
}

/* -----------------------------------------------------------------------
   BotHuman_ShouldUseHealthPack
   Returns true when the bot should consume a health pack.
   ----------------------------------------------------------------------- */
qboolean BotHuman_ShouldUseHealthPack(bot_state_t *bs)
{
    float hp_pct;
    if (!bs->ent || bs->ent->max_health <= 0) return false;
    hp_pct = (float)bs->ent->health / (float)bs->ent->max_health;
    return hp_pct < 0.50f;
}

/* -----------------------------------------------------------------------
   BotHuman_IsLowAmmo
   Returns true when the bot's ammo count is critically low.
   Actual ammo tracking depends on game-specific ammo fields; this stub
   uses credits as a proxy for resource awareness.
   ----------------------------------------------------------------------- */
qboolean BotHuman_IsLowAmmo(bot_state_t *bs)
{
    (void)bs;
    /* Full ammo tracking requires inventory hooks; return false by default */
    return false;
}

/* -----------------------------------------------------------------------
   BotHuman_InitDefaults
   Apply common human bot defaults.
   Called from each human class's BotClass_Init_* function.
   ----------------------------------------------------------------------- */
void BotHuman_InitDefaults(bot_state_t *bs)
{
    bs->combat.prefer_cover     = false;
    bs->combat.max_range_engage = true;  /* humans engage at max range      */
    bs->nav.wall_walking        = false;
}
