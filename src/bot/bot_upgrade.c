/*
 * bot_upgrade.c -- class upgrade decision engine and game state manager
 *
 * Implements Bot_ChooseClass() and the supporting game-state tracker.
 * See bot_upgrade.h for a detailed design overview.
 */

#include "bot_upgrade.h"
#include "bot_strategy.h"
#include "../nav/bot_nodes.h"

/* -----------------------------------------------------------------------
   Composition caps (enforced by the decision logic below)
   ----------------------------------------------------------------------- */
#define MAX_STALKERS        2
#define MAX_GUARDIANS       2
#define MAX_EXTERMINATORS   2
#define MAX_MECHS           1
#define MAX_ANY_CLASS       3   /* no single non-builder class exceeds this */

/* -----------------------------------------------------------------------
   Game-state update interval
   ----------------------------------------------------------------------- */
#define UPGRADE_STATE_INTERVAL  5.0f   /* seconds between refreshes          */

/* -----------------------------------------------------------------------
   Global game-state snapshot
   ----------------------------------------------------------------------- */
bot_game_state_t g_game_state;

/* =======================================================================
   Internal helpers — team / enemy composition queries
   ======================================================================= */

/*
 * Count active bots on `team` that are currently `cls`.
 */
static int CountTeamClass(int team, gloom_class_t cls)
{
    int i, n = 0;
    for (i = 0; i < MAX_BOTS; i++) {
        if (!g_bots[i].in_use) continue;
        if (g_bots[i].team != team) continue;
        if (g_bots[i].gloom_class == cls) n++;
    }
    return n;
}

/*
 * Count all active bots on `team`.
 */
static int CountTeamSize(int team)
{
    int i, n = 0;
    for (i = 0; i < MAX_BOTS; i++) {
        if (!g_bots[i].in_use) continue;
        if (g_bots[i].team == team) n++;
    }
    return n;
}

/*
 * Count how many bots of the enemy team are class `cls`.
 * Uses direct knowledge of all bot states (server-side omniscience).
 */
static int CountEnemyClass(int my_team, gloom_class_t cls)
{
    int enemy_team = (my_team == TEAM_HUMAN) ? TEAM_ALIEN : TEAM_HUMAN;
    return CountTeamClass(enemy_team, cls);
}

/*
 * Count total enemy bots (any class).
 */
static int CountEnemySize(int my_team)
{
    int enemy_team = (my_team == TEAM_HUMAN) ? TEAM_ALIEN : TEAM_HUMAN;
    return CountTeamSize(enemy_team);
}

/*
 * Return true if the enemy team appears to be rushing with lightweight
 * classes (many Hatchlings / Grunts relative to their total).
 */
static qboolean EnemyIsRushing(int my_team)
{
    int enemy_team  = (my_team == TEAM_HUMAN) ? TEAM_ALIEN : TEAM_HUMAN;
    int total       = CountTeamSize(enemy_team);
    int light_count;

    if (total == 0) return false;

    if (my_team == TEAM_HUMAN) {
        /* Many Hatchlings = alien rush */
        light_count = CountTeamClass(TEAM_ALIEN, GLOOM_CLASS_HATCHLING);
    } else {
        /* Many Grunts = human rush */
        light_count = CountTeamClass(TEAM_HUMAN, GLOOM_CLASS_GRUNT);
    }

    /* Rush threshold: more than half the enemy team is the cheapest class */
    return (light_count * 2 > total) ? true : false;
}

/*
 * Heuristic: is the enemy team in a well-fortified defensive posture?
 * Proxy: enemy has hit mid/late phase AND has multiple defensive-oriented
 * bots (Engineers with structures, HTs, or Guardians).
 */
static qboolean EnemyHasFortifiedBase(int my_team)
{
    int enemy_team = (my_team == TEAM_HUMAN) ? TEAM_ALIEN : TEAM_HUMAN;
    bot_game_phase_t enemy_phase = BotStrategy_GetPhase(enemy_team);

    if (enemy_phase < PHASE_MID) return false;

    if (my_team == TEAM_HUMAN) {
        /* Alien "fortification": Breeder built structures + Guardian defense */
        return (CountTeamClass(TEAM_ALIEN, GLOOM_CLASS_BREEDER) > 0 &&
                CountTeamClass(TEAM_ALIEN, GLOOM_CLASS_GUARDIAN) > 0)
               ? true : false;
    } else {
        /* Human "fortification": Engineer built structures + HT defense */
        return (CountTeamClass(TEAM_HUMAN, GLOOM_CLASS_ENGINEER) > 0 &&
                CountTeamClass(TEAM_HUMAN, GLOOM_CLASS_HT) > 0)
               ? true : false;
    }
}

/* =======================================================================
   Game-state tracker
   ======================================================================= */

void BotUpgrade_Init(void)
{
    memset(&g_game_state, 0, sizeof(g_game_state));
    g_game_state.win_state[TEAM_HUMAN] = WIN_STATE_EVEN;
    g_game_state.win_state[TEAM_ALIEN] = WIN_STATE_EVEN;
    g_game_state.map_type              = MAP_TYPE_MIXED;
    g_game_state.last_update           = 0.0f;
}

/*
 * Determine map openness from nav node distribution:
 *   - High ratio of NAV_SNIPE nodes  → open sightlines → MAP_TYPE_OPEN
 *   - High ratio of NAV_WALLCLIMB nodes relative to NAV_GROUND → tight corridors
 *     that force close-quarters → MAP_TYPE_TIGHT
 *   - Otherwise → MAP_TYPE_MIXED
 */
static bot_map_type_t DeriveMapType(void)
{
    int i, snipe = 0, wallclimb = 0, ground = 0, total = 0;

    for (i = 0; i < nav_node_count; i++) {
        if (nav_nodes[i].id == BOT_INVALID_NODE) continue;
        total++;
        if (nav_nodes[i].flags & NAV_SNIPE)     snipe++;
        if (nav_nodes[i].flags & NAV_WALLCLIMB) wallclimb++;
        if (nav_nodes[i].flags & NAV_GROUND)    ground++;
    }

    if (total == 0) return MAP_TYPE_MIXED;

    /* More than 20% of nodes are sniper perches → open */
    if (snipe * 5 > total) return MAP_TYPE_OPEN;

    /* Wall-climb nodes outnumber ground nodes 2:1 → tight corridor map */
    if (ground > 0 && wallclimb > ground * 2) return MAP_TYPE_TIGHT;

    return MAP_TYPE_MIXED;
}

void BotUpgrade_UpdateGameState(void)
{
    int   i;
    int   h_frags = 0, a_frags = 0;
    float threshold = 1.25f;

    if (level.time < g_game_state.last_update + UPGRADE_STATE_INTERVAL)
        return;
    g_game_state.last_update = level.time;

    /* Tally total frags per team from gclient_t */
    for (i = 0; i < MAX_BOTS; i++) {
        bot_state_t *bs = &g_bots[i];
        if (!bs->in_use || !bs->ent || !bs->ent->client) continue;
        if (bs->team == TEAM_HUMAN)
            h_frags += bs->ent->client->frags;
        else
            a_frags += bs->ent->client->frags;
    }

    g_game_state.human_frags = h_frags;
    g_game_state.alien_frags = a_frags;

    /* Determine win state for each team */
    if (h_frags > (int)(a_frags * threshold)) {
        g_game_state.win_state[TEAM_HUMAN] = WIN_STATE_WINNING;
        g_game_state.win_state[TEAM_ALIEN] = WIN_STATE_LOSING;
    } else if (a_frags > (int)(h_frags * threshold)) {
        g_game_state.win_state[TEAM_ALIEN] = WIN_STATE_WINNING;
        g_game_state.win_state[TEAM_HUMAN] = WIN_STATE_LOSING;
    } else {
        g_game_state.win_state[TEAM_HUMAN] = WIN_STATE_EVEN;
        g_game_state.win_state[TEAM_ALIEN] = WIN_STATE_EVEN;
    }

    g_game_state.map_type = DeriveMapType();
}

bot_win_state_t BotUpgrade_GetWinState(int team)
{
    if (team < 1 || team > 2) return WIN_STATE_EVEN;
    return g_game_state.win_state[team];
}

bot_map_type_t BotUpgrade_GetMapType(void)
{
    return g_game_state.map_type;
}

/* =======================================================================
   Alien upgrade decision
   ======================================================================= */

static gloom_class_t ChooseAlienClass(bot_state_t *bs)
{
    int              evos       = bs->evos;
    int              team       = TEAM_ALIEN;
    int              team_size  = CountTeamSize(team);
    bot_game_phase_t phase      = BotStrategy_GetPhase(team);
    bot_win_state_t  win_state  = BotUpgrade_GetWinState(team);
    bot_map_type_t   map_type   = BotUpgrade_GetMapType();

    /* ---- Rule: always keep at least 1 Breeder ---- */
    if (CountTeamClass(team, GLOOM_CLASS_BREEDER) == 0 &&
        gloom_class_info[GLOOM_CLASS_BREEDER].evo_cost <= evos)
        return GLOOM_CLASS_BREEDER;

    /* ---- Early game (0-1 evo or PHASE_EARLY) ---- */
    if (phase == PHASE_EARLY || evos <= 1) {
        if (evos >= gloom_class_info[GLOOM_CLASS_DRONE].evo_cost)
            return GLOOM_CLASS_DRONE;   /* Drone preferred if >= 1 evo */
        return GLOOM_CLASS_HATCHLING;
    }

    /* ---- Desperate: losing badly → Kamikaze to break fortified base ---- */
    if (win_state == WIN_STATE_LOSING &&
        evos >= gloom_class_info[GLOOM_CLASS_KAMIKAZE].evo_cost &&
        CountTeamClass(team, GLOOM_CLASS_KAMIKAZE) < MAX_ANY_CLASS)
        return GLOOM_CLASS_KAMIKAZE;

    /* ---- Wraith: no team Wraith and enemy has fortified base ---- */
    if (CountTeamClass(team, GLOOM_CLASS_WRAITH) == 0 &&
        EnemyHasFortifiedBase(team) &&
        evos >= gloom_class_info[GLOOM_CLASS_WRAITH].evo_cost)
        return GLOOM_CLASS_WRAITH;

    /* ---- Mid game (2-4 evos) ---- */
    if (evos <= 4) {
        /* Wraith for ranged harass: always on open maps; fill gap when none exists */
        if (evos >= gloom_class_info[GLOOM_CLASS_WRAITH].evo_cost &&
            CountTeamClass(team, GLOOM_CLASS_WRAITH) < MAX_ANY_CLASS &&
            (map_type == MAP_TYPE_OPEN ||
             CountTeamClass(team, GLOOM_CLASS_WRAITH) == 0))
            return GLOOM_CLASS_WRAITH;

        /* Kamikaze to break fortified base */
        if (EnemyHasFortifiedBase(team) &&
            evos >= gloom_class_info[GLOOM_CLASS_KAMIKAZE].evo_cost &&
            CountTeamClass(team, GLOOM_CLASS_KAMIKAZE) < MAX_ANY_CLASS)
            return GLOOM_CLASS_KAMIKAZE;

        /* Stinger: versatile mid-tier fighter */
        if (evos >= gloom_class_info[GLOOM_CLASS_STINGER].evo_cost &&
            CountTeamClass(team, GLOOM_CLASS_STINGER) < MAX_ANY_CLASS)
            return GLOOM_CLASS_STINGER;

        /* Fallback to Drone if nothing else fits */
        if (evos >= gloom_class_info[GLOOM_CLASS_DRONE].evo_cost)
            return GLOOM_CLASS_DRONE;

        return GLOOM_CLASS_HATCHLING;
    }

    /* ---- Late game (5+ evos) ---- */

    /* Guardian: ambush/defense, stealth tank */
    if (evos >= gloom_class_info[GLOOM_CLASS_GUARDIAN].evo_cost &&
        CountTeamClass(team, GLOOM_CLASS_GUARDIAN) < MAX_GUARDIANS)
        return GLOOM_CLASS_GUARDIAN;

    /* Stalker: push/assault tank — use when team is pushing */
    if (evos >= gloom_class_info[GLOOM_CLASS_STALKER].evo_cost &&
        CountTeamClass(team, GLOOM_CLASS_STALKER) < MAX_STALKERS &&
        BotStrategy_GetStrategy(team) == STRATEGY_PUSH)
        return GLOOM_CLASS_STALKER;

    /* Cap check: if Guardian cap reached, take Stalker */
    if (evos >= gloom_class_info[GLOOM_CLASS_STALKER].evo_cost &&
        CountTeamClass(team, GLOOM_CLASS_STALKER) < MAX_STALKERS)
        return GLOOM_CLASS_STALKER;

    /* Fallback late-game class */
    if (CountTeamClass(team, GLOOM_CLASS_STINGER) < MAX_ANY_CLASS &&
        evos >= gloom_class_info[GLOOM_CLASS_STINGER].evo_cost)
        return GLOOM_CLASS_STINGER;

    (void)team_size; /* reserved for future composition heuristics */

    return GLOOM_CLASS_HATCHLING;
}

/* =======================================================================
   Human upgrade decision
   ======================================================================= */

static gloom_class_t ChooseHumanClass(bot_state_t *bs)
{
    int              credits    = bs->credits;
    int              team       = TEAM_HUMAN;
    int              team_size  = CountTeamSize(team);
    bot_game_phase_t phase      = BotStrategy_GetPhase(team);
    bot_win_state_t  win_state  = BotUpgrade_GetWinState(team);
    bot_map_type_t   map_type   = BotUpgrade_GetMapType();

    /* ---- Rule: always keep at least 1 Engineer ---- */
    if (CountTeamClass(team, GLOOM_CLASS_ENGINEER) == 0 &&
        gloom_class_info[GLOOM_CLASS_ENGINEER].credit_cost <= credits)
        return GLOOM_CLASS_ENGINEER;

    /* ---- Rule: always keep at least 1 Biotech when team has 4+ players ---- */
    if (team_size >= 4 &&
        CountTeamClass(team, GLOOM_CLASS_BIOTECH) == 0 &&
        gloom_class_info[GLOOM_CLASS_BIOTECH].credit_cost <= credits)
        return GLOOM_CLASS_BIOTECH;

    /* ---- Counter-picks ---- */

    /* Aliens have Stalker(s): need HT or Exterminator to deal with it */
    if (CountEnemyClass(team, GLOOM_CLASS_STALKER) > 0) {
        if (credits >= gloom_class_info[GLOOM_CLASS_EXTERMINATOR].credit_cost &&
            CountTeamClass(team, GLOOM_CLASS_EXTERMINATOR) < MAX_EXTERMINATORS)
            return GLOOM_CLASS_EXTERMINATOR;
        if (credits >= gloom_class_info[GLOOM_CLASS_HT].credit_cost &&
            CountTeamClass(team, GLOOM_CLASS_HT) < MAX_ANY_CLASS)
            return GLOOM_CLASS_HT;
    }

    /* Aliens are rushing with Hatchlings: ST with shotgun is most effective */
    if (EnemyIsRushing(team) &&
        credits >= gloom_class_info[GLOOM_CLASS_ST].credit_cost &&
        CountTeamClass(team, GLOOM_CLASS_ST) < MAX_ANY_CLASS)
        return GLOOM_CLASS_ST;

    /* ---- Early game (0-1 credit or PHASE_EARLY) ---- */
    if (phase == PHASE_EARLY || credits <= 1) {
        /* ST preferred if aliens are aggressive */
        if (CountEnemySize(team) > 0 && EnemyIsRushing(team) &&
            credits >= gloom_class_info[GLOOM_CLASS_ST].credit_cost)
            return GLOOM_CLASS_ST;
        return GLOOM_CLASS_GRUNT;
    }

    /* ---- Mid game (2-3 credits) ---- */
    if (credits <= 3) {
        /* Biotech if team needs a healer */
        if (team_size >= 4 &&
            CountTeamClass(team, GLOOM_CLASS_BIOTECH) == 0 &&
            credits >= gloom_class_info[GLOOM_CLASS_BIOTECH].credit_cost)
            return GLOOM_CLASS_BIOTECH;

        /* HT for anti-structure / anti-Stalker */
        if (credits >= gloom_class_info[GLOOM_CLASS_HT].credit_cost &&
            CountTeamClass(team, GLOOM_CLASS_HT) < MAX_ANY_CLASS)
            return GLOOM_CLASS_HT;

        /* Commando for scouting / flanking; open maps benefit most */
        if (credits >= gloom_class_info[GLOOM_CLASS_COMMANDO].credit_cost &&
            CountTeamClass(team, GLOOM_CLASS_COMMANDO) < MAX_ANY_CLASS &&
            (map_type != MAP_TYPE_TIGHT))
            return GLOOM_CLASS_COMMANDO;

        /* Fallback mid-tier */
        if (credits >= gloom_class_info[GLOOM_CLASS_ST].credit_cost &&
            CountTeamClass(team, GLOOM_CLASS_ST) < MAX_ANY_CLASS)
            return GLOOM_CLASS_ST;

        return GLOOM_CLASS_GRUNT;
    }

    /* ---- Late game (5+ credits) ---- */

    /* Mech: only if team has 6+ players and no Mech yet */
    if (credits >= gloom_class_info[GLOOM_CLASS_MECH].credit_cost &&
        team_size >= 6 &&
        CountTeamClass(team, GLOOM_CLASS_MECH) < MAX_MECHS)
        return GLOOM_CLASS_MECH;

    /* Exterminator: primary late-game assault */
    if (credits >= gloom_class_info[GLOOM_CLASS_EXTERMINATOR].credit_cost &&
        CountTeamClass(team, GLOOM_CLASS_EXTERMINATOR) < MAX_EXTERMINATORS)
        return GLOOM_CLASS_EXTERMINATOR;

    /* If Exterminator cap reached, take HT */
    if (credits >= gloom_class_info[GLOOM_CLASS_HT].credit_cost &&
        CountTeamClass(team, GLOOM_CLASS_HT) < MAX_ANY_CLASS)
        return GLOOM_CLASS_HT;

    (void)win_state; /* reserved: could de-prioritise heavy classes when losing */

    return GLOOM_CLASS_GRUNT;
}

/* =======================================================================
   Public entry point — Bot_ChooseClass
   ======================================================================= */

/*
 * Bot_ChooseClass
 *
 * Selects the best class for `bs` using team composition, enemy
 * composition, game phase, win state, and map type.
 *
 * Applies the decision to bs->gloom_class (equivalent to the bot
 * issuing "cmd class <ID>" via the Gloom menu), updates the combat
 * engagement range for the new class, and returns the class ID.
 *
 * Called from Bot_StateEvolve / Bot_StateUpgrade when the bot reaches
 * the Overmind (alien) or Armory (human).
 */
int Bot_ChooseClass(bot_state_t *bs)
{
    gloom_class_t chosen;

    if (!bs) return (int)GLOOM_CLASS_GRUNT;

    /* Refresh game state if stale */
    BotUpgrade_UpdateGameState();

    if (bs->team == TEAM_ALIEN)
        chosen = ChooseAlienClass(bs);
    else
        chosen = ChooseHumanClass(bs);

    /* Validate: the chosen class must belong to the bot's team */
    if (Gloom_ClassTeam(chosen) != bs->team) {
        chosen = (bs->team == TEAM_HUMAN)
            ? GLOOM_CLASS_GRUNT : GLOOM_CLASS_HATCHLING;
    }

    /* Apply the class change */
    bs->gloom_class = chosen;
    bs->combat.engagement_range = gloom_class_info[chosen].preferred_range;
    bs->class_upgrades++;

    gi.dprintf("Bot_ChooseClass: '%s' → %s (evos=%d credits=%d)\n",
               bs->name, Gloom_ClassName(chosen), bs->evos, bs->credits);

    return (int)chosen;
}
