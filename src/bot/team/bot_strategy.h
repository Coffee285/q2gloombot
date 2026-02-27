/*
 * bot_strategy.h -- team strategy system public API for q2gloombot
 *
 * The strategy system evaluates the overall game state every few seconds
 * and assigns a team-wide strategy.  Individual bots consult their
 * assigned role and strategy when choosing what to do during IDLE state.
 *
 * STRATEGIES
 * ----------
 * DEFEND   — team is weaker; protect spawn points, build defenses
 * PUSH     — team is stronger; advance on enemy base, destroy spawns
 * HARASS   — teams are even; send raiders to disrupt enemy builders
 * ALLIN    — desperate; all players rush enemy base last spawns
 * REBUILD  — lost spawn points; builders urgently build while fighters hold
 *
 * ROLES
 * -----
 * Each bot is assigned one role per strategy tick.  The role guides which
 * BOTSTATE_* the bot should be in when not in direct combat.
 */

#ifndef BOT_STRATEGY_H
#define BOT_STRATEGY_H

#include "bot.h"

/* -----------------------------------------------------------------------
   Strategy identifiers
   ----------------------------------------------------------------------- */
typedef enum {
    STRATEGY_DEFEND  = 0,
    STRATEGY_PUSH    = 1,
    STRATEGY_HARASS  = 2,
    STRATEGY_ALLIN   = 3,
    STRATEGY_REBUILD = 4,

    STRATEGY_MAX
} bot_strategy_t;

/* -----------------------------------------------------------------------
   Role identifiers
   ----------------------------------------------------------------------- */
typedef enum {
    ROLE_BUILDER   = 0,  /* construct and maintain structures              */
    ROLE_ATTACKER  = 1,  /* advance on enemy territory                     */
    ROLE_DEFENDER  = 2,  /* hold current positions                         */
    ROLE_SCOUT     = 3,  /* patrol and expose enemy positions               */
    ROLE_ESCORT    = 4,  /* follow and protect a key teammate              */
    ROLE_HEALER    = 5,  /* support teammates with healing                 */

    ROLE_MAX
} bot_role_t;

/* -----------------------------------------------------------------------
   Game phase identifiers
   ----------------------------------------------------------------------- */
typedef enum {
    PHASE_EARLY      = 0,  /* first ~2 minutes; both teams building up     */
    PHASE_MID        = 1,  /* both teams established                       */
    PHASE_LATE       = 2,  /* one team has major advantage                 */
    PHASE_DESPERATE  = 3,  /* losing badly; few or no spawn points         */
} bot_game_phase_t;

/* -----------------------------------------------------------------------
   Team snapshot used for strategy assessment
   ----------------------------------------------------------------------- */
typedef struct {
    int             team;
    int             alive_count;
    int             spawn_count;
    int             struct_count;
    int             avg_class_tier; /* 1=basic, 2=mid, 3=advanced          */
    float           avg_hp_pct;
    bot_game_phase_t phase;
    int             power_score;   /* combined metric                      */
} team_snapshot_t;

/* -----------------------------------------------------------------------
   Public API
   ----------------------------------------------------------------------- */

/* Initialise strategy system (called once from Bot_Init). */
void BotStrategy_Init(void);

/* Per-frame update; re-evaluates strategy every BOT_STRATEGY_INTERVAL seconds. */
void BotStrategy_Frame(void);

/* Get the current strategy for a given team. */
bot_strategy_t BotStrategy_GetStrategy(int team);

/* Get the role assigned to a specific bot. */
bot_role_t BotStrategy_GetRole(bot_state_t *bs);

/* Force a strategy for a given team (e.g. for testing). */
void BotStrategy_SetStrategy(int team, bot_strategy_t strat);

/* Get the current game phase for a team. */
bot_game_phase_t BotStrategy_GetPhase(int team);

#endif /* BOT_STRATEGY_H */
