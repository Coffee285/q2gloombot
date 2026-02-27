/*
 * bot_autofill.c -- automatic bot population management for q2gloombot
 *
 * Monitors player count and adds/removes bots to maintain the target
 * total set by the bot_count cvar.  Bots are staggered (one per second)
 * to avoid lag spikes.
 */

#include "bot.h"
#include "bot_cvars.h"

/* -----------------------------------------------------------------------
   Internal state
   ----------------------------------------------------------------------- */
static float autofill_next_check = 0.0f;
static float autofill_next_add   = 0.0f;

#define AUTOFILL_CHECK_INTERVAL  5.0f  /* seconds between population checks */
#define AUTOFILL_ADD_INTERVAL    1.0f  /* seconds between individual bot adds */

/* -----------------------------------------------------------------------
   Helpers
   ----------------------------------------------------------------------- */

/* Count human (non-bot) players currently connected */
static int BotAutofill_CountHumanPlayers(void)
{
    int i, count = 0;

    if (!g_edicts || !maxclients) return 0;

    for (i = 0; i < (int)maxclients->value; i++) {
        edict_t *ent = &g_edicts[i + 1];
        if (ent->inuse && ent->client && !ent->client->is_bot)
            count++;
    }
    return count;
}

/* Count active bots on a specific team */
static int BotAutofill_CountBotsOnTeam(int team)
{
    int i, count = 0;
    for (i = 0; i < MAX_BOTS; i++) {
        if (g_bots[i].in_use && g_bots[i].team == team)
            count++;
    }
    return count;
}

/* -----------------------------------------------------------------------
   BotAutofill_Init
   ----------------------------------------------------------------------- */
void BotAutofill_Init(void)
{
    autofill_next_check = 0.0f;
    autofill_next_add   = 0.0f;
}

/* -----------------------------------------------------------------------
   BotAutofill_Frame -- called every server frame from Bot_Frame()
   ----------------------------------------------------------------------- */
void BotAutofill_Frame(void)
{
    int target, humans, current_bots;
    int alien_bots, human_bots;
    int need;

    /* Skip if cvars not registered yet */
    if (!bot_count || !bot_enable) return;

    /* Skip if bots are disabled */
    if ((int)bot_enable->value == 0) return;

    /* Only check periodically */
    if (level.time < autofill_next_check) return;
    autofill_next_check = level.time + AUTOFILL_CHECK_INTERVAL;

    target = (int)bot_count->value;
    if (target <= 0) return;

    humans       = BotAutofill_CountHumanPlayers();
    current_bots = num_bots;
    need         = target - humans - current_bots;

    /* Remove excess bots */
    if (need < 0) {
        int to_remove = -need;
        int i;
        for (i = MAX_BOTS - 1; i >= 0 && to_remove > 0; i--) {
            if (g_bots[i].in_use) {
                Bot_Disconnect(g_bots[i].ent);
                to_remove--;
            }
        }
        return;
    }

    /* Add bots one at a time (staggered) */
    if (need > 0 && level.time >= autofill_next_add) {
        int team;
        float skill;
        edict_t *ent;
        int i;

        /* Balance teams */
        alien_bots = BotAutofill_CountBotsOnTeam(TEAM_ALIEN);
        human_bots = BotAutofill_CountBotsOnTeam(TEAM_HUMAN);

        /* Use specific counts if set */
        if ((int)bot_count_alien->value > 0 || (int)bot_count_human->value > 0) {
            if (alien_bots < (int)bot_count_alien->value)
                team = TEAM_ALIEN;
            else if (human_bots < (int)bot_count_human->value)
                team = TEAM_HUMAN;
            else
                return; /* both teams at target */
        } else {
            /* Auto-balance: add to smaller team */
            team = (alien_bots <= human_bots) ? TEAM_ALIEN : TEAM_HUMAN;
        }

        /* Determine skill */
        if (bot_skill_random && (int)bot_skill_random->value) {
            float lo = bot_skill_min ? bot_skill_min->value : 0.2f;
            float hi = bot_skill_max ? bot_skill_max->value : 0.8f;
            skill = lo + (float)(rand() % 100) / 100.0f * (hi - lo);
        } else {
            skill = bot_skill ? bot_skill->value : 0.5f;
        }

        /* Find a free edict slot */
        ent = NULL;
        for (i = 0; i < globals.max_edicts; i++) {
            if (!g_edicts[i].inuse) {
                ent = &g_edicts[i];
                break;
            }
        }

        if (ent) {
            G_InitEdict(ent);
            ent->client = (gclient_t *)gi.TagMalloc(sizeof(gclient_t), TAG_GAME);
            if (ent->client) {
                memset(ent->client, 0, sizeof(gclient_t));
                Bot_Connect(ent, team, skill);
            }
        }

        autofill_next_add = level.time + AUTOFILL_ADD_INTERVAL;
    }
}
