/*
 * bot_team.c -- team coordination and strategy
 *
 * Provides shared situational awareness for all bots on a team:
 *  - Tracks whether Reactor / Overmind is alive.
 *  - Counts active spawn points (Telenodes / Eggs).
 *  - Assigns roles (e.g. ensures at least one Builder is active).
 *
 * This information is consumed by bot_main.c state handlers and
 * bot_build.c to drive the prioritised build state machine.
 */

#include "bot_team.h"

void BotTeam_Init(void)
{
    gi.dprintf("BotTeam_Init: team coordination ready (stub)\n");
}

/*
 * BotTeam_Frame
 * Called once per server frame; updates cached team state for all bots.
 */
void BotTeam_Frame(void)
{
    qboolean reactor  = BotTeam_ReactorAlive();
    qboolean overmind = BotTeam_OvmindAlive();
    int      i;

    for (i = 0; i < MAX_BOTS; i++) {
        if (!g_bots[i].in_use) continue;
        if (g_bots[i].team == TEAM_HUMAN)
            g_bots[i].build.reactor_exists  = reactor;
        else
            g_bots[i].build.overmind_exists = overmind;
        g_bots[i].build.spawn_count =
            BotTeam_CountSpawnPoints(g_bots[i].team);
    }
}

/*
 * BotTeam_ReactorAlive
 * Returns true if at least one STRUCT_REACTOR entity is alive.
 * (Entity scan implemented when structure entities are spawned.)
 */
qboolean BotTeam_ReactorAlive(void)
{
    /* TODO: scan g_edicts for live Reactor entity */
    return true;
}

/*
 * BotTeam_OvmindAlive
 * Returns true if at least one STRUCT_OVERMIND entity is alive.
 */
qboolean BotTeam_OvmindAlive(void)
{
    /* TODO: scan g_edicts for live Overmind entity */
    return true;
}

int BotTeam_CountSpawnPoints(int team)
{
    /* TODO: count live Telenode / Egg entities */
    (void)team;
    return 1;
}

/*
 * BotTeam_FindBuilder
 * Returns the edict of an active human Builder or alien Granger on the
 * given team.  Used by combat bots to form escort assignments.
 */
edict_t *BotTeam_FindBuilder(int team)
{
    int i;

    for (i = 0; i < MAX_BOTS; i++) {
        if (!g_bots[i].in_use) continue;
        if (g_bots[i].team != team) continue;
        if (Gloom_ClassCanBuild(g_bots[i].gloom_class))
            return g_bots[i].ent;
    }
    return NULL;
}

/*
 * BotTeam_AssignRoles
 * Ensures the team has at least one Builder bot.  If none exists,
 * reassigns the lowest-skill combat bot to the Builder class.
 */
void BotTeam_AssignRoles(int team)
{
    edict_t *builder = BotTeam_FindBuilder(team);
    int      i;
    int      lowest_skill_idx = -1;
    float    lowest_skill     = 2.0f;

    if (builder) return;  /* already have a builder */

    for (i = 0; i < MAX_BOTS; i++) {
        if (!g_bots[i].in_use) continue;
        if (g_bots[i].team != team) continue;
        if (g_bots[i].skill < lowest_skill) {
            lowest_skill     = g_bots[i].skill;
            lowest_skill_idx = i;
        }
    }

    if (lowest_skill_idx < 0) return;

    g_bots[lowest_skill_idx].gloom_class =
        (team == TEAM_HUMAN) ? GLOOM_CLASS_BUILDER : GLOOM_CLASS_GRANGER;

    gi.dprintf("BotTeam_AssignRoles: '%s' reassigned to builder role\n",
               g_bots[lowest_skill_idx].name);
}
