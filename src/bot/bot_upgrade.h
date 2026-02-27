/*
 * bot_upgrade.h -- class upgrade decision engine and game state manager
 *
 * Provides the public API for deciding which class a bot should become
 * when it has enough resources (evos for aliens, credits for humans) and
 * a class-change opportunity arises (respawn, Overmind/Armory visit).
 *
 * GAME STATE
 * ----------
 * BotUpgrade_UpdateGameState() is called periodically to refresh a shared
 * game-state snapshot:
 *   - win_state  : whether our team is winning, losing, or roughly even
 *   - map_type   : open (favours ranged), tight (favours melee), or mixed
 *
 * CLASS DECISION
 * --------------
 * int Bot_ChooseClass(bot_state_t *bs)
 *   Evaluates frags/evos/credits, team composition, enemy composition,
 *   game phase, win state, and map type, then selects the best class,
 *   applies it to bs->gloom_class, and returns the class ID.
 *
 * ALIEN LOGIC SUMMARY
 *   - Always keep 1 Breeder; become Breeder if none present.
 *   - Early (0-1 evo):  Hatchling; Drone if >= 1 evo.
 *   - Mid   (2-4 evos): Wraith > Stinger > Kamikaze (desperation/losing).
 *   - Late  (5+ evos):  Guardian (ambush/defense) or Stalker (push/tank).
 *   - Caps: max 2 Stalkers, max 2 Guardians, max 3 of any single class.
 *
 * HUMAN LOGIC SUMMARY
 *   - Always keep 1 Engineer; become Engineer if none present.
 *   - Keep 1 Biotech when team has 4+ players.
 *   - Early (0-1 credit): Grunt; ST if aliens are aggressive.
 *   - Mid   (2-3 credits): Commando > HT > Biotech.
 *   - Late  (5+ credits):  Exterminator; Mech if 6+ players and no Mech.
 *   - Caps: max 1 Mech, max 2 Exterminators, max 3 of any single class.
 */

#ifndef BOT_UPGRADE_H
#define BOT_UPGRADE_H

#include "bot.h"

/* -----------------------------------------------------------------------
   Win-state identifiers
   ----------------------------------------------------------------------- */
typedef enum {
    WIN_STATE_EVEN    = 0,  /* teams roughly equal                          */
    WIN_STATE_WINNING = 1,  /* our team clearly ahead                       */
    WIN_STATE_LOSING  = 2   /* our team clearly behind                      */
} bot_win_state_t;

/* -----------------------------------------------------------------------
   Map-type identifiers
   Open maps favour long-range classes; tight maps favour melee classes.
   ----------------------------------------------------------------------- */
typedef enum {
    MAP_TYPE_MIXED = 0,  /* balanced; use general composition               */
    MAP_TYPE_OPEN  = 1,  /* many sightlines; prefer ranged classes          */
    MAP_TYPE_TIGHT = 2   /* narrow corridors; prefer melee/close classes    */
} bot_map_type_t;

/* -----------------------------------------------------------------------
   Shared game-state snapshot (refreshed by BotUpgrade_UpdateGameState)
   ----------------------------------------------------------------------- */
typedef struct {
    bot_win_state_t win_state[3];  /* indexed by TEAM_HUMAN(1) / TEAM_ALIEN(2); [0] unused */
    bot_map_type_t  map_type;
    int             human_frags;
    int             alien_frags;
    float           last_update;
} bot_game_state_t;

/* Global snapshot (defined in bot_upgrade.c) */
extern bot_game_state_t g_game_state;

/* -----------------------------------------------------------------------
   Public API
   ----------------------------------------------------------------------- */

/* Initialise the upgrade subsystem; call once from Bot_Init(). */
void BotUpgrade_Init(void);

/*
 * Refresh g_game_state from current frag counts and nav topology.
 * Call every few seconds (e.g. alongside BotStrategy_Frame).
 */
void BotUpgrade_UpdateGameState(void);

/* Return the current win state for the given team. */
bot_win_state_t BotUpgrade_GetWinState(int team);

/* Return the current map type. */
bot_map_type_t BotUpgrade_GetMapType(void);

/*
 * Bot_ChooseClass  —  main upgrade decision entry point.
 *
 * Selects the best class for bot `bs` using team composition, enemy
 * composition, game phase, win state, and map type.  Applies the
 * decision by updating bs->gloom_class and returns the class ID (int).
 *
 * Called when a bot respawns or reaches the Overmind/Armory.
 */
int Bot_ChooseClass(bot_state_t *bs);

#endif /* BOT_UPGRADE_H */
