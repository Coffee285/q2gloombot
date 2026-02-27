/*
 * bot_commands.c -- server console command handlers for q2gloombot
 *
 * All commands are prefixed with "sv" and dispatched from
 * G_ServerCommand() in g_main.c.
 *
 * Commands:
 *   sv addbot [team] [class] [skill]  — add a bot
 *   sv removebot <name|all>           — remove a bot or all bots
 *   sv listbots                       — list all active bots
 *   sv botstatus [name]               — detailed status of one or all bots
 *   sv botdebug <flag|all|none>       — toggle debug output flags
 *   sv botpause                       — toggle bot thinking on/off
 *   sv botkick <name>                 — alias for removebot
 *   sv botstrategy [team]             — show current team strategy
 */

#include "bot.h"
#include "bot_debug.h"
#include "bot_strategy.h"

/* -----------------------------------------------------------------------
   SV_BotStatus_f  —  "sv botstatus [name]"
   ----------------------------------------------------------------------- */
void SV_BotStatus_f(void)
{
    const char *name;
    int i;

    if (gi.argc() >= 2) {
        name = gi.argv(1);
        for (i = 0; i < MAX_BOTS; i++) {
            if (g_bots[i].in_use &&
                Q_stricmp(g_bots[i].name, name) == 0) {
                BotDebug_PrintBotStatus(&g_bots[i]);
                return;
            }
        }
        gi.dprintf("botstatus: '%s' not found\n", name);
    } else {
        BotDebug_PrintAllBots();
    }
}

/* -----------------------------------------------------------------------
   SV_BotDebug_f  —  "sv botdebug <flag|all|none>"
   ----------------------------------------------------------------------- */
void SV_BotDebug_f(void)
{
    const char *arg;

    if (gi.argc() < 2) {
        gi.dprintf("Current debug flags: 0x%02X\n", bot_debug_flags);
        gi.dprintf("Usage: sv botdebug <state|nav|combat|build|strategy|upgrade|all|none>\n");
        return;
    }

    arg = gi.argv(1);

    if (Q_stricmp(arg, "all") == 0) {
        bot_debug_flags = BOT_DEBUG_ALL;
        gi.dprintf("Bot debug: ALL enabled\n");
    } else if (Q_stricmp(arg, "none") == 0) {
        bot_debug_flags = BOT_DEBUG_NONE;
        gi.dprintf("Bot debug: NONE\n");
    } else if (Q_stricmp(arg, "state") == 0) {
        bot_debug_flags ^= BOT_DEBUG_STATE;
        gi.dprintf("Bot debug STATE: %s\n",
                   (bot_debug_flags & BOT_DEBUG_STATE) ? "ON" : "OFF");
    } else if (Q_stricmp(arg, "nav") == 0) {
        bot_debug_flags ^= BOT_DEBUG_NAV;
        gi.dprintf("Bot debug NAV: %s\n",
                   (bot_debug_flags & BOT_DEBUG_NAV) ? "ON" : "OFF");
    } else if (Q_stricmp(arg, "combat") == 0) {
        bot_debug_flags ^= BOT_DEBUG_COMBAT;
        gi.dprintf("Bot debug COMBAT: %s\n",
                   (bot_debug_flags & BOT_DEBUG_COMBAT) ? "ON" : "OFF");
    } else if (Q_stricmp(arg, "build") == 0) {
        bot_debug_flags ^= BOT_DEBUG_BUILD;
        gi.dprintf("Bot debug BUILD: %s\n",
                   (bot_debug_flags & BOT_DEBUG_BUILD) ? "ON" : "OFF");
    } else if (Q_stricmp(arg, "strategy") == 0) {
        bot_debug_flags ^= BOT_DEBUG_STRATEGY;
        gi.dprintf("Bot debug STRATEGY: %s\n",
                   (bot_debug_flags & BOT_DEBUG_STRATEGY) ? "ON" : "OFF");
    } else if (Q_stricmp(arg, "upgrade") == 0) {
        bot_debug_flags ^= BOT_DEBUG_UPGRADE;
        gi.dprintf("Bot debug UPGRADE: %s\n",
                   (bot_debug_flags & BOT_DEBUG_UPGRADE) ? "ON" : "OFF");
    } else {
        gi.dprintf("Unknown debug flag '%s'\n", arg);
    }
}

/* -----------------------------------------------------------------------
   SV_BotPause_f  —  "sv botpause"
   ----------------------------------------------------------------------- */
void SV_BotPause_f(void)
{
    bot_paused = !bot_paused;
    gi.dprintf("Bot thinking %s\n", bot_paused ? "PAUSED" : "RESUMED");
}

/* -----------------------------------------------------------------------
   SV_BotKick_f  —  "sv botkick <name>"  (alias for removebot)
   ----------------------------------------------------------------------- */
void SV_BotKick_f(void)
{
    const char *name;
    int i;

    if (gi.argc() < 2) {
        gi.dprintf("Usage: sv botkick <name>\n");
        return;
    }
    name = gi.argv(1);

    for (i = 0; i < MAX_BOTS; i++) {
        if (g_bots[i].in_use &&
            Q_stricmp(g_bots[i].name, name) == 0) {
            gi.dprintf("Kicking bot '%s'\n", name);
            Bot_Disconnect(g_bots[i].ent);
            return;
        }
    }
    gi.dprintf("botkick: '%s' not found\n", name);
}

/* -----------------------------------------------------------------------
   SV_BotStrategy_f  —  "sv botstrategy [team]"
   ----------------------------------------------------------------------- */
void SV_BotStrategy_f(void)
{
    if (gi.argc() >= 2) {
        const char *arg = gi.argv(1);
        if (Q_stricmp(arg, "alien") == 0)
            BotDebug_PrintStrategy(TEAM_ALIEN);
        else
            BotDebug_PrintStrategy(TEAM_HUMAN);
    } else {
        BotDebug_PrintStrategy(TEAM_HUMAN);
        BotDebug_PrintStrategy(TEAM_ALIEN);
    }
}

/* -----------------------------------------------------------------------
   Bot_ServerCommand  —  dispatch "sv <cmd>" to the appropriate handler
   Returns true if the command was handled.
   ----------------------------------------------------------------------- */
qboolean Bot_ServerCommand(void)
{
    const char *cmd = gi.argv(0);

    if (Q_stricmp(cmd, "botstatus") == 0) {
        SV_BotStatus_f();
        return true;
    }
    if (Q_stricmp(cmd, "botdebug") == 0) {
        SV_BotDebug_f();
        return true;
    }
    if (Q_stricmp(cmd, "botpause") == 0) {
        SV_BotPause_f();
        return true;
    }
    if (Q_stricmp(cmd, "botkick") == 0) {
        SV_BotKick_f();
        return true;
    }
    if (Q_stricmp(cmd, "botstrategy") == 0) {
        SV_BotStrategy_f();
        return true;
    }

    return false;
}
