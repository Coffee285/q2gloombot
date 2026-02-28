/*
 * bot_strategy.c -- team strategy system implementation for q2gloombot
 *
 * Evaluates game state every BOT_STRATEGY_INTERVAL seconds and selects
 * the appropriate team strategy.  Assigns roles to individual bots.
 */

#include "bot_strategy.h"
#include "bot_team.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define BOT_STRATEGY_INTERVAL   3.0f   /* reassess strategy every 3 seconds */
#define PHASE_EARLY_TIME        120.0f /* first 2 minutes = early phase     */
#define PHASE_DESPERATE_SPAWNS  1      /* <= this many spawns = desperate   */

/* -----------------------------------------------------------------------
   Module state
   ----------------------------------------------------------------------- */
static bot_strategy_t s_strategy[3]; /* index by TEAM_HUMAN / TEAM_ALIEN  */
static bot_role_t     s_roles[MAX_BOTS];
static float          s_next_assess = 0.0f;
static bot_game_phase_t s_phase[3];

/* -----------------------------------------------------------------------
   BotStrategy_Init
   ----------------------------------------------------------------------- */
void BotStrategy_Init(void)
{
    int i;
    s_next_assess = 0.0f;
    for (i = 0; i < 3; i++) {
        s_strategy[i] = STRATEGY_DEFEND;
        s_phase[i]    = PHASE_EARLY;
    }
    for (i = 0; i < MAX_BOTS; i++)
        s_roles[i] = ROLE_ATTACKER;
}

/* -----------------------------------------------------------------------
   BuildTeamSnapshot
   Collect per-team statistics for the current frame.
   ----------------------------------------------------------------------- */
static team_snapshot_t BuildTeamSnapshot(int team)
{
    team_snapshot_t snap;
    int   i, count = 0;
    float hp_sum = 0.0f;
    int   tier_sum = 0;

    memset(&snap, 0, sizeof(snap));
    snap.team = team;

    for (i = 0; i < MAX_BOTS; i++) {
        bot_state_t *bs = &g_bots[i];
        if (!bs->in_use || bs->team != team) continue;
        if (!bs->ent || !bs->ent->inuse) continue;

        count++;
        if (bs->ent->max_health > 0)
            hp_sum += (float)bs->ent->health / (float)bs->ent->max_health;

        /* Class tier: builders/basic=1, mid-tier=2, advanced=3 */
        switch (bs->gloom_class) {
        case GLOOM_CLASS_MECH:
        case GLOOM_CLASS_EXTERMINATOR:
        case GLOOM_CLASS_STALKER:
        case GLOOM_CLASS_GUARDIAN:
            tier_sum += 3;  break;
        case GLOOM_CLASS_HT:
        case GLOOM_CLASS_COMMANDO:
        case GLOOM_CLASS_ST:
        case GLOOM_CLASS_STINGER:
        case GLOOM_CLASS_DRONE:
        case GLOOM_CLASS_WRAITH:
        case GLOOM_CLASS_KAMIKAZE:
            tier_sum += 2;  break;
        case GLOOM_CLASS_GRUNT:
        case GLOOM_CLASS_BIOTECH:
        case GLOOM_CLASS_ENGINEER:
        case GLOOM_CLASS_HATCHLING:
        case GLOOM_CLASS_BREEDER:
        default:
            tier_sum += 1;  break;
        }

        snap.struct_count += bs->build.spawn_count;
    }

    snap.alive_count    = count;
    snap.avg_hp_pct     = (count > 0) ? hp_sum / (float)count : 0.0f;
    snap.avg_class_tier = (count > 0) ? tier_sum / count : 1;
    /* spawn_count is queried from the team-wide system, not accumulated
     * from individual bot fields (which only track what each builder built) */
    snap.spawn_count    = BotTeam_CountSpawnPoints(team);

    /* Power score: alive bots × tier × HP */
    snap.power_score = (int)((float)(snap.alive_count * snap.avg_class_tier) *
                              snap.avg_hp_pct * 100.0f);

    /* Game phase */
    if (level.time < PHASE_EARLY_TIME) {
        snap.phase = PHASE_EARLY;
    } else if (snap.spawn_count <= PHASE_DESPERATE_SPAWNS) {
        snap.phase = PHASE_DESPERATE;
    } else if (snap.avg_class_tier >= 2 && snap.spawn_count >= 2) {
        snap.phase = PHASE_MID;
    } else {
        snap.phase = PHASE_LATE;
    }

    s_phase[team] = snap.phase;
    return snap;
}

/* -----------------------------------------------------------------------
   SelectStrategy
   Choose the best strategy based on two team snapshots.
   ----------------------------------------------------------------------- */
static bot_strategy_t SelectStrategy(const team_snapshot_t *us,
                                      const team_snapshot_t *them)
{
    /* Desperate: no spawn points — rush or rebuild */
    if (us->phase == PHASE_DESPERATE || us->spawn_count == 0) {
        if (us->alive_count >= 3) return STRATEGY_ALLIN;
        return STRATEGY_REBUILD;
    }

    /* Early phase: focus on building up */
    if (us->phase == PHASE_EARLY) {
        return STRATEGY_DEFEND;
    }

    /* Compare power scores */
    if (us->power_score > them->power_score * 1.25f) {
        return STRATEGY_PUSH;
    } else if (them->power_score > us->power_score * 1.25f) {
        return STRATEGY_DEFEND;
    } else {
        /* Roughly even: send harassers */
        return STRATEGY_HARASS;
    }
}

/* -----------------------------------------------------------------------
   AssignRoles
   Give each bot a role consistent with the current strategy.
   ----------------------------------------------------------------------- */
static void AssignRoles(int team, bot_strategy_t strat)
{
    int i, builders = 0, healers = 0, total = 0;

    /* First pass: count team */
    for (i = 0; i < MAX_BOTS; i++) {
        if (!g_bots[i].in_use || g_bots[i].team != team) continue;
        if (!g_bots[i].ent || !g_bots[i].ent->inuse) continue;
        total++;
    }

    /* Second pass: assign roles */
    for (i = 0; i < MAX_BOTS; i++) {
        bot_state_t *bs = &g_bots[i];
        if (!bs->in_use || bs->team != team) continue;
        if (!bs->ent || !bs->ent->inuse) continue;

        /* Builders always have ROLE_BUILDER */
        if (Gloom_ClassCanBuild(bs->gloom_class)) {
            s_roles[bs->bot_index] = ROLE_BUILDER;
            builders++;
            continue;
        }

        /* Biotechs always heal */
        if (bs->gloom_class == GLOOM_CLASS_BIOTECH) {
            s_roles[bs->bot_index] = ROLE_HEALER;
            healers++;
            continue;
        }

        /* Assign combat roles based on strategy */
        switch (strat) {
        case STRATEGY_PUSH:
            s_roles[bs->bot_index] = ROLE_ATTACKER;
            break;
        case STRATEGY_DEFEND:
            s_roles[bs->bot_index] = ROLE_DEFENDER;
            break;
        case STRATEGY_HARASS:
            /* Half attack, half defend */
            s_roles[bs->bot_index] = (i % 2 == 0) ? ROLE_ATTACKER : ROLE_DEFENDER;
            break;
        case STRATEGY_ALLIN:
            s_roles[bs->bot_index] = ROLE_ATTACKER;
            break;
        case STRATEGY_REBUILD:
            /* Most defenders; one scout */
            s_roles[bs->bot_index] = (total > 2 && i == 0) ? ROLE_SCOUT : ROLE_DEFENDER;
            break;
        default:
            s_roles[bs->bot_index] = ROLE_ATTACKER;
            break;
        }
    }

    (void)builders; (void)healers; /* suppress unused warnings */
}

/* -----------------------------------------------------------------------
   BotStrategy_Frame
   ----------------------------------------------------------------------- */
void BotStrategy_Frame(void)
{
    team_snapshot_t human_snap, alien_snap;

    if (level.time < s_next_assess) return;
    s_next_assess = level.time + BOT_STRATEGY_INTERVAL;

    human_snap = BuildTeamSnapshot(TEAM_HUMAN);
    alien_snap = BuildTeamSnapshot(TEAM_ALIEN);

    s_strategy[TEAM_HUMAN] = SelectStrategy(&human_snap, &alien_snap);
    s_strategy[TEAM_ALIEN] = SelectStrategy(&alien_snap, &human_snap);

    AssignRoles(TEAM_HUMAN, s_strategy[TEAM_HUMAN]);
    AssignRoles(TEAM_ALIEN, s_strategy[TEAM_ALIEN]);
}

/* -----------------------------------------------------------------------
   Public accessors
   ----------------------------------------------------------------------- */
bot_strategy_t BotStrategy_GetStrategy(int team)
{
    if (team < 1 || team > 2) return STRATEGY_DEFEND;
    return s_strategy[team];
}

bot_role_t BotStrategy_GetRole(bot_state_t *bs)
{
    if (!bs || bs->bot_index < 0 || bs->bot_index >= MAX_BOTS)
        return ROLE_ATTACKER;
    return s_roles[bs->bot_index];
}

void BotStrategy_SetStrategy(int team, bot_strategy_t strat)
{
    if (team < 1 || team > 2) return;
    if (strat < 0 || strat >= STRATEGY_MAX) return;
    s_strategy[team] = strat;
}

bot_game_phase_t BotStrategy_GetPhase(int team)
{
    if (team < 1 || team > 2) return PHASE_EARLY;
    return s_phase[team];
}
