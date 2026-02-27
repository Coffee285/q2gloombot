/*
 * bot_personality.c -- per-bot personality generation and application
 *
 * Traits are sampled from a normal distribution (mean 0.5, stddev 0.2)
 * seeded by the bot's name hash so each named bot is repeatable.
 * The Box-Muller transform is used for the normal distribution because
 * it requires only standard math.h functions (no external libraries).
 */

#include "bot_personality.h"
#include <math.h>
#include <string.h>

/* -----------------------------------------------------------------------
   Internal helpers
   ----------------------------------------------------------------------- */

/*
 * name_hash
 * djb2-style hash of the bot name — deterministic across platforms.
 */
static unsigned int name_hash(const char *s)
{
    unsigned int h = 5381u;
    int c;
    while ((c = (unsigned char)*s++) != 0)
        h = ((h << 5) + h) ^ (unsigned int)c;
    return h;
}

/*
 * lcg_next
 * Minimal 32-bit linear congruential generator step.
 * Returns the updated state and a pseudo-random float in [0, 1).
 */
static float lcg_next(unsigned int *state)
{
    *state = (*state * 1664525u) + 1013904223u;
    return (float)(*state >> 8) / (float)(1u << 24);
}

/*
 * box_muller
 * Generates a normal-distributed value (mean, stddev) from two uniform
 * samples u1 and u2 in (0, 1].  Clamps to [0, 1] before returning.
 */
static float box_muller(float mean, float stddev, float u1, float u2)
{
    float z, result;

    /* Guard against log(0) — u1 is already in (0,1) from lcg_next
     * but add a tiny epsilon for safety. */
    if (u1 < 1e-7f) u1 = 1e-7f;

    z = (float)(sqrt(-2.0 * log((double)u1)) * cos(2.0 * 3.14159265358979323846 * (double)u2));
    result = mean + stddev * z;

    if (result < 0.0f) result = 0.0f;
    if (result > 1.0f) result = 1.0f;
    return result;
}

/* -----------------------------------------------------------------------
   Bot_GeneratePersonality
   ----------------------------------------------------------------------- */
void Bot_GeneratePersonality(bot_state_t *bs)
{
    unsigned int seed;
    float u[10]; /* 10 uniform samples → 5 Box-Muller pairs             */
    int i;

    if (!bs) return;

    seed = name_hash(bs->name);

    /* Generate 10 uniform samples */
    for (i = 0; i < 10; i++)
        u[i] = lcg_next(&seed);

    /* Ensure each u value is > 0 so log() is safe */
    for (i = 0; i < 10; i++)
        if (u[i] < 1e-7f) u[i] = 1e-7f;

    bs->personality.aggression  = box_muller(PERSONALITY_MEAN, PERSONALITY_STDDEV, u[0], u[1]);
    bs->personality.caution     = box_muller(PERSONALITY_MEAN, PERSONALITY_STDDEV, u[2], u[3]);
    bs->personality.teamwork    = box_muller(PERSONALITY_MEAN, PERSONALITY_STDDEV, u[4], u[5]);
    bs->personality.patience    = box_muller(PERSONALITY_MEAN, PERSONALITY_STDDEV, u[6], u[7]);
    bs->personality.build_focus = box_muller(PERSONALITY_MEAN, PERSONALITY_STDDEV, u[8], u[9]);

    gi.dprintf("Bot_GeneratePersonality: %s agg=%.2f caut=%.2f team=%.2f "
               "pat=%.2f build=%.2f\n",
               bs->name,
               bs->personality.aggression,
               bs->personality.caution,
               bs->personality.teamwork,
               bs->personality.patience,
               bs->personality.build_focus);
}

/* -----------------------------------------------------------------------
   Bot_FleeHealthThreshold
   Returns the flee-health fraction of max_health for this bot.

   Formula: base * (1 + caution - aggression)
     - High caution → retreats earlier (higher threshold)
     - High aggression → fights longer (lower threshold)
   Result is clamped to [0.05, 0.60] so bots never flee at 5% or less
   and never flee when still above 60% health.
   ----------------------------------------------------------------------- */
float Bot_FleeHealthThreshold(const bot_state_t *bs)
{
    float threshold;

    if (!bs) return PERSONALITY_BASE_FLEE_HEALTH;

    threshold = PERSONALITY_BASE_FLEE_HEALTH *
                (1.0f + bs->personality.caution - bs->personality.aggression);

    if (threshold < 0.05f) threshold = 0.05f;
    if (threshold > 0.60f) threshold = 0.60f;
    return threshold;
}

/* -----------------------------------------------------------------------
   Bot_AmbushWaitTime
   Returns the ambush wait time in seconds for this bot.
   High patience → waits longer in ambush (up to 30+ seconds).
   ----------------------------------------------------------------------- */
float Bot_AmbushWaitTime(const bot_state_t *bs)
{
    if (!bs) return PERSONALITY_BASE_AMBUSH_WAIT;
    return PERSONALITY_BASE_AMBUSH_WAIT * (0.5f + bs->personality.patience);
}

/* -----------------------------------------------------------------------
   Bot_ShouldCheckBuild
   Returns true when a builder bot should scan for build opportunities
   this think-tick rather than scanning for enemies.  The build_focus
   trait increases the probability: a bot with build_focus=1.0 always
   checks, build_focus=0.0 checks only 10% of the time.
   ----------------------------------------------------------------------- */
qboolean Bot_ShouldCheckBuild(const bot_state_t *bs)
{
    float threshold;
    float roll;

    if (!bs) return false;
    if (!Gloom_ClassCanBuild(bs->gloom_class)) return false;

    /* Probability scales linearly from 0.10 (build_focus=0) to
     * 1.00 (build_focus=1). */
    threshold = 0.10f + bs->personality.build_focus * 0.90f;

    /* Use a simple LCG derived from the bot index and level time for
     * a per-tick roll that doesn't need rand() state management. */
    roll = (float)((unsigned int)(bs->bot_index * 1664525u +
                   (unsigned int)(level.time * 100.0f) * 1013904223u) >> 8)
           / (float)(1u << 24);

    return (qboolean)(roll < threshold);
}
