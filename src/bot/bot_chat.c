/*
 * bot_chat.c -- bot chat and taunt system for q2gloombot
 *
 * Bots occasionally send chat messages to make them feel more human.
 * Messages are sent via gi.bprintf(PRINT_CHAT, ...) which delivers them
 * to all players in the same way a real player message would appear.
 *
 * Event rates (per occurrence):
 *   Kill taunt     : 30%
 *   Death message  : 20%
 *   Round win      : 50%
 *   Spawn/class    : 100% (once per spawn)
 */

#include "bot_chat.h"
#include <string.h>

/* -----------------------------------------------------------------------
   Message tables
   ----------------------------------------------------------------------- */

static const char *kill_taunts[] = {
    "get rekt",
    "too easy",
    "nice try",
    "is that all you got?",
    "you're no match for me",
    "step it up",
    "come back when you're ready",
    "one down",
    "ez",
    "stay down",
};
#define NUM_KILL_TAUNTS ((int)(sizeof(kill_taunts) / sizeof(kill_taunts[0])))

static const char *death_messages[] = {
    "lucky shot",
    "lag",
    "gg",
    "whatever",
    "rigged",
    "hacks",
    "that shouldn't have happened",
    "i'll be back",
};
#define NUM_DEATH_MESSAGES ((int)(sizeof(death_messages) / sizeof(death_messages[0])))

static const char *win_messages[] = {
    "gg well played",
    "that's how it's done",
    "victory!",
    "we win!",
    "gg",
    "nice team effort",
    "couldn't have done it without you all",
};
#define NUM_WIN_MESSAGES ((int)(sizeof(win_messages) / sizeof(win_messages[0])))

/* -----------------------------------------------------------------------
   Internal helpers
   ----------------------------------------------------------------------- */

/*
 * chat_rand
 * Quick deterministic pseudo-random float [0,1) from bot state + event seed.
 */
static float chat_rand(const bot_state_t *bs, unsigned int event_seed)
{
    unsigned int s = (unsigned int)(bs->bot_index * 1664525u) ^
                     (unsigned int)(level.time * 1000.0f) ^
                     event_seed;
    s = (s * 1664525u) + 1013904223u;
    return (float)(s >> 8) / (float)(1u << 24);
}

static int chat_rand_index(const bot_state_t *bs, unsigned int seed, int count)
{
    float r = chat_rand(bs, seed);
    int idx  = (int)(r * (float)count);
    if (idx < 0)     idx = 0;
    if (idx >= count) idx = count - 1;
    return idx;
}

/* -----------------------------------------------------------------------
   Public API
   ----------------------------------------------------------------------- */

void Bot_Chat_OnKill(bot_state_t *bs)
{
    if (!bs || !bs->ent) return;
    if (chat_rand(bs, 0xDEAD1234u) > 0.30f) return;  /* 30% chance */
    gi.bprintf(PRINT_CHAT, "%s: %s\n",
               bs->name,
               kill_taunts[chat_rand_index(bs, 0xDEAD1234u, NUM_KILL_TAUNTS)]);
}

void Bot_Chat_OnDeath(bot_state_t *bs)
{
    if (!bs || !bs->ent) return;
    if (chat_rand(bs, 0xDEADBEEFu) > 0.20f) return;  /* 20% chance */
    gi.bprintf(PRINT_CHAT, "%s: %s\n",
               bs->name,
               death_messages[chat_rand_index(bs, 0xDEADBEEFu, NUM_DEATH_MESSAGES)]);
}

void Bot_Chat_OnTeamWin(bot_state_t *bs)
{
    if (!bs || !bs->ent) return;
    if (chat_rand(bs, 0xC0FFEE01u) > 0.50f) return;  /* 50% chance */
    gi.bprintf(PRINT_CHAT, "%s: %s\n",
               bs->name,
               win_messages[chat_rand_index(bs, 0xC0FFEE01u, NUM_WIN_MESSAGES)]);
}

void Bot_Chat_OnSpawn(bot_state_t *bs)
{
    const char *class_name;

    if (!bs || !bs->ent) return;
    class_name = Gloom_ClassName(bs->gloom_class);
    /* Always announce the new class on spawn */
    gi.bprintf(PRINT_CHAT, "%s: going %s\n", bs->name, class_name);
}
