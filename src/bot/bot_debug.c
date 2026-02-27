/*
 * bot_debug.c -- debug and diagnostic tools for q2gloombot
 */

#include "bot_debug.h"
#include "bot_strategy.h"

/* -----------------------------------------------------------------------
   Module globals
   ----------------------------------------------------------------------- */
int      bot_debug_flags = BOT_DEBUG_NONE;
qboolean bot_paused      = false;

/* -----------------------------------------------------------------------
   State name tables
   ----------------------------------------------------------------------- */
static const char *state_names[BOTSTATE_MAX] = {
    "IDLE",    "PATROL",  "HUNT",   "COMBAT",  "FLEE",
    "DEFEND",  "ESCORT",  "BUILD",  "EVOLVE",  "UPGRADE"
};

static const char *strategy_names[STRATEGY_MAX] = {
    "DEFEND", "PUSH", "HARASS", "ALLIN", "REBUILD"
};

static const char *role_names[ROLE_MAX] = {
    "BUILDER", "ATTACKER", "DEFENDER", "SCOUT", "ESCORT", "HEALER"
};

/* -----------------------------------------------------------------------
   BotDebug_Init
   ----------------------------------------------------------------------- */
void BotDebug_Init(void)
{
    bot_debug_flags = BOT_DEBUG_NONE;
    bot_paused      = false;
}

/* -----------------------------------------------------------------------
   BotDebug_FrameUpdate
   ----------------------------------------------------------------------- */
void BotDebug_FrameUpdate(void)
{
    /* Reserved for future frame-by-frame debug displays */
}

/* -----------------------------------------------------------------------
   BotDebug_Log
   ----------------------------------------------------------------------- */
void BotDebug_Log(int flag, const char *fmt, ...)
{
    va_list ap;
    char    buf[512];

    if (!(bot_debug_flags & flag))
        return;

    va_start(ap, fmt);
    vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    gi.dprintf("[BOT_DBG] %s", buf);
}

/* -----------------------------------------------------------------------
   BotDebug_StateName
   ----------------------------------------------------------------------- */
const char *BotDebug_StateName(bot_ai_state_t state)
{
    if (state < 0 || state >= BOTSTATE_MAX)
        return "UNKNOWN";
    return state_names[state];
}

/* -----------------------------------------------------------------------
   BotDebug_StrategyName
   ----------------------------------------------------------------------- */
const char *BotDebug_StrategyName(int strategy)
{
    if (strategy < 0 || strategy >= STRATEGY_MAX)
        return "UNKNOWN";
    return strategy_names[strategy];
}

/* -----------------------------------------------------------------------
   BotDebug_RoleName
   ----------------------------------------------------------------------- */
const char *BotDebug_RoleName(int role)
{
    if (role < 0 || role >= ROLE_MAX)
        return "UNKNOWN";
    return role_names[role];
}

/* -----------------------------------------------------------------------
   BotDebug_PrintBotStatus
   ----------------------------------------------------------------------- */
void BotDebug_PrintBotStatus(bot_state_t *bs)
{
    if (!bs || !bs->in_use) {
        gi.dprintf("Bot: (invalid or not in use)\n");
        return;
    }

    gi.dprintf("=== Bot Status: %s ===\n", bs->name);
    gi.dprintf("  Team:      %s\n",
               bs->team == TEAM_HUMAN ? "Human" : "Alien");
    gi.dprintf("  Class:     %s\n", Gloom_ClassName(bs->gloom_class));
    gi.dprintf("  Skill:     %.2f\n", bs->skill);
    gi.dprintf("  State:     %s (prev: %s)\n",
               BotDebug_StateName(bs->ai_state),
               BotDebug_StateName(bs->prev_ai_state));
    gi.dprintf("  Health:    %d / %d\n",
               bs->ent ? bs->ent->health : 0,
               bs->ent ? bs->ent->max_health : 0);
    gi.dprintf("  Credits:   %d  Evos: %d\n", bs->credits, bs->evos);
    gi.dprintf("  Frags:     %d  Upgrades: %d\n",
               bs->frag_count, bs->class_upgrades);
    gi.dprintf("  Nav:       node=%d goal=%d path_valid=%d wall_walk=%d\n",
               bs->nav.current_node, bs->nav.goal_node,
               (int)bs->nav.path_valid, (int)bs->nav.wall_walking);
    gi.dprintf("  Combat:    target=%s visible=%d range=%.0f\n",
               bs->combat.target ? "YES" : "no",
               (int)bs->combat.target_visible,
               bs->combat.engagement_range);
    gi.dprintf("  Build:     priority=%d building=%d\n",
               (int)bs->build.priority, (int)bs->build.building);
    gi.dprintf("  Enemies remembered: %d\n", bs->enemy_memory_count);
}

/* -----------------------------------------------------------------------
   BotDebug_PrintAllBots
   ----------------------------------------------------------------------- */
void BotDebug_PrintAllBots(void)
{
    int i, count = 0;

    gi.dprintf("Active bots (%d / %d):\n", num_bots, MAX_BOTS);
    for (i = 0; i < MAX_BOTS; i++) {
        if (g_bots[i].in_use) {
            gi.dprintf("  [%2d] %-20s team=%-6s class=%-14s skill=%.2f"
                       " state=%-8s hp=%d\n",
                       i,
                       g_bots[i].name,
                       g_bots[i].team == TEAM_HUMAN ? "Human" : "Alien",
                       Gloom_ClassName(g_bots[i].gloom_class),
                       g_bots[i].skill,
                       BotDebug_StateName(g_bots[i].ai_state),
                       g_bots[i].ent ? g_bots[i].ent->health : 0);
            count++;
        }
    }
    if (count == 0)
        gi.dprintf("  (none)\n");
}

/* -----------------------------------------------------------------------
   BotDebug_PrintStrategy
   ----------------------------------------------------------------------- */
void BotDebug_PrintStrategy(int team)
{
    bot_strategy_t strat = BotStrategy_GetStrategy(team);
    bot_game_phase_t phase = BotStrategy_GetPhase(team);
    const char *phase_names[] = { "EARLY", "MID", "LATE", "DESPERATE" };

    gi.dprintf("=== Strategy: %s team ===\n",
               team == TEAM_HUMAN ? "Human" : "Alien");
    gi.dprintf("  Strategy:  %s\n", BotDebug_StrategyName((int)strat));
    gi.dprintf("  Phase:     %s\n",
               phase >= 0 && phase <= 3 ? phase_names[phase] : "UNKNOWN");
}
