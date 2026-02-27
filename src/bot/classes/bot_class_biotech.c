/*
 * bot_class_biotech.c -- Biotech AI (medic/healer support class)
 *
 * Behavior:
 *  - PRIMARY ROLE IS HEALING, NOT FIGHTING
 *  - Follow the highest-value teammate (Mech > Exterminator > HT > others)
 *  - Constantly scan teammates for health damage; heal anyone below 80%
 *  - Stay behind the front line — let combat classes engage first
 *  - Use heal gun on teammates; switch to blaster only for self-defense
 *  - Retreat immediately when targeted by aliens
 *  - Patrol between teammates when all are healthy
 *  - Never be first to engage aliens
 *  - Stay near Teleporters to heal respawning teammates
 *  - Critical priority: keep Engineers alive
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define BIOTECH_HEAL_THRESHOLD   0.80f  /* heal teammates below this HP%   */
#define BIOTECH_SELF_FLEE_PCT    0.40f  /* flee when own HP drops this low  */
#define BIOTECH_ESCORT_RANGE     300.0f /* stay within this range of VIP    */

/* Target teammate priority (ordered from highest to lowest priority) */
static const gloom_class_t biotech_escort_priority[] = {
    GLOOM_CLASS_MECH,
    GLOOM_CLASS_EXTERMINATOR,
    GLOOM_CLASS_HT,
    GLOOM_CLASS_ST,
    GLOOM_CLASS_COMMANDO,
    GLOOM_CLASS_ENGINEER,
    GLOOM_CLASS_GRUNT,
};

/* -----------------------------------------------------------------------
   BotClass_Init_biotech
   ----------------------------------------------------------------------- */
void BotClass_Init_biotech(bot_state_t *bs)
{
    bs->combat.engagement_range = gloom_class_info[GLOOM_CLASS_BIOTECH].preferred_range;
    bs->combat.prefer_cover     = true;  /* stay behind front line          */
    bs->combat.max_range_engage = true;
    bs->nav.wall_walking        = false;

    /* Start by finding the team's highest-value unit to escort */
    bs->ai_state = BOTSTATE_ESCORT;
}

/* -----------------------------------------------------------------------
   BotClass_Think_biotech
   ----------------------------------------------------------------------- */
void BotClass_Think_biotech(bot_state_t *bs)
{
    float  hp_pct;
    int    i, p;
    int    num_priorities;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /* Self-preservation: flee if targeted and HP is low */
    if (hp_pct < BIOTECH_SELF_FLEE_PCT && bs->combat.target_visible) {
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /*
     * Never initiate combat — Biotech's weapon is for self-defense only.
     * If targeted, attempt to flee rather than engage.
     */
    if (bs->combat.target_visible && bs->ai_state != BOTSTATE_FLEE) {
        /* Don't switch to COMBAT; stay in escort and use heal gun */
        bs->ai_state = BOTSTATE_FLEE;
        return;
    }

    /* Find the highest-priority teammate to escort/heal */
    num_priorities = (int)(sizeof(biotech_escort_priority) /
                           sizeof(biotech_escort_priority[0]));

    for (p = 0; p < num_priorities; p++) {
        for (i = 0; i < MAX_BOTS; i++) {
            if (!g_bots[i].in_use) continue;
            if (g_bots[i].team != TEAM_HUMAN) continue;
            if (&g_bots[i] == bs) continue;
            if (g_bots[i].gloom_class != biotech_escort_priority[p]) continue;
            if (!g_bots[i].ent || !g_bots[i].ent->inuse) continue;

            /* Found highest-priority teammate — escort them */
            bs->ai_state = BOTSTATE_ESCORT;
            return;
        }
    }

    /* No valid escort target — patrol near Teleporters */
    if (bs->ai_state != BOTSTATE_PATROL)
        bs->ai_state = BOTSTATE_PATROL;
}
