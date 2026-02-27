/*
 * bot_debug.h -- debug and diagnostic tools for q2gloombot
 *
 * Provides per-bot and global debug output, state logging,
 * navigation visualization, and combat statistics.
 */

#ifndef BOT_DEBUG_H
#define BOT_DEBUG_H

#include "bot.h"

/* -----------------------------------------------------------------------
   Debug flags — enable via "sv botdebug <flag>"
   ----------------------------------------------------------------------- */
#define BOT_DEBUG_NONE       0x00
#define BOT_DEBUG_STATE      0x01  /* log AI state transitions              */
#define BOT_DEBUG_NAV        0x02  /* log navigation / pathfinding          */
#define BOT_DEBUG_COMBAT     0x04  /* log target selection / firing         */
#define BOT_DEBUG_BUILD      0x08  /* log building decisions                */
#define BOT_DEBUG_STRATEGY   0x10  /* log team strategy changes             */
#define BOT_DEBUG_UPGRADE    0x20  /* log class upgrades / evolution        */
#define BOT_DEBUG_ALL        0x3F  /* all flags combined                    */

/* Global debug mask (defined in bot_debug.c) */
extern int bot_debug_flags;

/* Global pause flag (defined in bot_debug.c) */
extern qboolean bot_paused;

/* -----------------------------------------------------------------------
   Public API
   ----------------------------------------------------------------------- */

/* Initialise debug subsystem. */
void BotDebug_Init(void);

/* Per-frame update — flush buffered debug output. */
void BotDebug_FrameUpdate(void);

/* Log a debug message if the given flag is active. */
void BotDebug_Log(int flag, const char *fmt, ...);

/* Print a full status report for a specific bot. */
void BotDebug_PrintBotStatus(bot_state_t *bs);

/* Print a summary of all active bots. */
void BotDebug_PrintAllBots(void);

/* Print current team strategy info. */
void BotDebug_PrintStrategy(int team);

/* Return a human-readable name for an AI state. */
const char *BotDebug_StateName(bot_ai_state_t state);

/* Return a human-readable name for a strategy. */
const char *BotDebug_StrategyName(int strategy);

/* Return a human-readable name for a role. */
const char *BotDebug_RoleName(int role);

#endif /* BOT_DEBUG_H */
