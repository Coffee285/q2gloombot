/*
 * bot_commands.c -- server console command handlers for q2gloombot
 *
 * All commands are prefixed with "sv" and dispatched from
 * G_ServerCommand() in g_main.c.
 *
 * Commands:
 *   sv addbot [alien|human] [skill]   — add a bot to the specified team
 *   sv removebot <name|all>           — remove a named bot or all bots
 *   sv listbots                       — list all active bots
 *   sv botstatus [name]               — detailed status of one or all bots
 *   sv botdebug <flag|all|none>       — toggle debug output flags
 *   sv botpause                       — toggle bot thinking on/off
 *   sv botkick <name>                 — alias for removebot
 *   sv botstrategy [team]             — show current team strategy
 *   sv botversion                     — print GloomBot version string
 *   sv navgen                         — auto-generate navigation nodes for current map
 */

#include "bot.h"
#include "bot_cvars.h"
#include "bot_debug.h"
#include "bot_nav.h"
#include "bot_strategy.h"

/* -----------------------------------------------------------------------
   SV_BotVersion_f  —  "sv botversion"
   ----------------------------------------------------------------------- */
void SV_BotVersion_f(void)
{
    gi.dprintf("GloomBot v%s\n", GLOOMBOT_VERSION);
}

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
   SV_AddBot_f  —  "sv addbot [alien|human] [skill]"
   ----------------------------------------------------------------------- */
static void SV_AddBot_f(void)
{
    int         team  = TEAM_HUMAN;
    float       skill = 0.5f;
    int         argc  = gi.argc();
    const char *arg;
    edict_t    *ent;
    int         i;

    if (argc >= 2) {
        arg = gi.argv(1);
        if (Q_stricmp(arg, "alien") == 0)
            team = TEAM_ALIEN;
        else
            team = TEAM_HUMAN;
    }

    if (argc >= 3) {
        skill = (float)atof(gi.argv(2));
        if (skill < 0.0f) skill = 0.0f;
        if (skill > 1.0f) skill = 1.0f;
    }

    /* Find a free edict slot beyond the reserved client slots */
    ent = NULL;
    for (i = (maxclients ? (int)maxclients->value + 1 : 1);
         i < globals.max_edicts; i++) {
        if (!g_edicts[i].inuse) {
            ent = &g_edicts[i];
            break;
        }
    }

    if (!ent) {
        gi.dprintf("addbot: no free edict slots\n");
        return;
    }

    G_InitEdict(ent);
    ent->client = (gclient_t *)gi.TagMalloc(sizeof(gclient_t), TAG_GAME);
    if (!ent->client) {
        gi.dprintf("addbot: out of memory\n");
        return;
    }
    memset(ent->client, 0, sizeof(gclient_t));

    Bot_Connect(ent, team, skill);
}

/* -----------------------------------------------------------------------
   SV_RemoveBot_f  —  "sv removebot <name|all>"
   ----------------------------------------------------------------------- */
static void SV_RemoveBot_f(void)
{
    const char *name;
    int i;

    if (gi.argc() < 2) {
        gi.dprintf("Usage: sv removebot <name|all>\n");
        return;
    }
    name = gi.argv(1);

    if (Q_stricmp(name, "all") == 0) {
        for (i = 0; i < MAX_BOTS; i++) {
            if (g_bots[i].in_use)
                Bot_Disconnect(g_bots[i].ent);
        }
        gi.dprintf("All bots removed.\n");
        return;
    }

    for (i = 0; i < MAX_BOTS; i++) {
        if (g_bots[i].in_use &&
            Q_stricmp(g_bots[i].name, name) == 0) {
            gi.dprintf("Removing bot '%s'\n", name);
            Bot_Disconnect(g_bots[i].ent);
            return;
        }
    }
    gi.dprintf("removebot: '%s' not found\n", name);
}

/* -----------------------------------------------------------------------
   SV_ListBots_f  —  "sv listbots"
   ----------------------------------------------------------------------- */
static void SV_ListBots_f(void)
{
    int i, count = 0;

    gi.dprintf("Active bots (%d / %d):\n", num_bots, MAX_BOTS);
    for (i = 0; i < MAX_BOTS; i++) {
        if (g_bots[i].in_use) {
            gi.dprintf("  [%2d] %-20s team=%-6s class=%-14s skill=%.2f"
                       " credits=%d evos=%d state=%s\n",
                       i,
                       g_bots[i].name,
                       g_bots[i].team == TEAM_HUMAN ? "Human" : "Alien",
                       Gloom_ClassName(g_bots[i].gloom_class),
                       g_bots[i].skill,
                       g_bots[i].credits,
                       g_bots[i].evos,
                       BotDebug_StateName(g_bots[i].ai_state));
            count++;
        }
    }
    if (count == 0)
        gi.dprintf("  (none)\n");
}

/* -----------------------------------------------------------------------
   SV_NavGen_f  —  "sv navgen"
   Auto-generate navigation nodes for the current map.
   ----------------------------------------------------------------------- */
static void SV_NavGen_f(void)
{
    if (!bot_nav_autogen || (int)bot_nav_autogen->value == 0) {
        gi.dprintf("navgen: bot_nav_autogen is disabled. "
                   "Set 'bot_nav_autogen 1' first.\n");
        return;
    }
    gi.dprintf("navgen: generating navigation nodes for '%s'...\n",
               level.mapname);
    BotNav_LoadMap(level.mapname);
    gi.dprintf("navgen: done.\n");
}

/* -----------------------------------------------------------------------
   Bot_ServerCommand  —  dispatch "sv <cmd>" to the appropriate handler
   Returns true if the command was handled.
   ----------------------------------------------------------------------- */
qboolean Bot_ServerCommand(void)
{
    const char *cmd = gi.argv(0);

    if (Q_stricmp(cmd, "addbot") == 0) {
        SV_AddBot_f();
        return true;
    }
    if (Q_stricmp(cmd, "removebot") == 0) {
        SV_RemoveBot_f();
        return true;
    }
    if (Q_stricmp(cmd, "listbots") == 0) {
        SV_ListBots_f();
        return true;
    }
    if (Q_stricmp(cmd, "navgen") == 0) {
        SV_NavGen_f();
        return true;
    }
    if (Q_stricmp(cmd, "botversion") == 0) {
        SV_BotVersion_f();
        return true;
    }
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
