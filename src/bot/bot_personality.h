/*
 * bot_personality.h -- per-bot personality traits for q2gloombot
 *
 * Each bot is assigned a randomly generated personality on creation that
 * modifies how it weighs decisions throughout the AI.  The same named bot
 * always receives the same personality (seeded from the name hash) so
 * behaviour is deterministic across sessions.
 *
 * Trait summary
 * -------------
 *  aggression  (0-1): eagerness to seek combat vs. play defensively
 *  caution     (0-1): speed of retreat when damaged
 *  teamwork    (0-1): tendency to stick with teammates vs. solo roam
 *  patience    (0-1): willingness to wait in ambush vs. actively hunt
 *  build_focus (0-1): builder bias toward construction vs. combat
 *
 * Application rules (see bot_personality.c)
 * -----------------------------------------
 *  flee_health  = base * (1 + caution - aggression)
 *  ambush_wait  = patience * BASE_AMBUSH_WAIT
 *  build_check  = build_focus modifies how often builders scan for
 *                 build opportunities instead of scanning for enemies
 */

#ifndef BOT_PERSONALITY_H
#define BOT_PERSONALITY_H

#include "bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
/* Centre and spread of the normal distribution used for trait sampling */
#define PERSONALITY_MEAN   0.5f
#define PERSONALITY_STDDEV 0.2f

/* Base flee-health fraction (fraction of max_health).
 * Modified by aggression / caution at runtime. */
#define PERSONALITY_BASE_FLEE_HEALTH  0.2f

/* Base ambush wait time in seconds; scaled by patience. */
#define PERSONALITY_BASE_AMBUSH_WAIT  10.0f

/* -----------------------------------------------------------------------
   Public API
   ----------------------------------------------------------------------- */

/* Called once from Bot_Connect(); seeds traits from the bot's name hash. */
void  Bot_GeneratePersonality(bot_state_t *bs);

/* Returns the flee-health threshold (fraction of max_health) adjusted for
 * the bot's aggression/caution levels. */
float Bot_FleeHealthThreshold(const bot_state_t *bs);

/* Returns the ambush wait time (seconds) for this bot. */
float Bot_AmbushWaitTime(const bot_state_t *bs);

/* Returns true when a builder bot should check for build opportunities
 * this think-tick (probability influenced by build_focus). */
qboolean Bot_ShouldCheckBuild(const bot_state_t *bs);

#endif /* BOT_PERSONALITY_H */
