/*
 * bot_humanize.h -- human-like movement and behaviour simulation for q2gloombot
 *
 * Makes bots look less robotic by:
 *   - Smoothing input changes over 2-3 frames instead of instant snaps.
 *   - Simulating mouse overshoot and correction on aim transitions.
 *   - Introducing skill-scaled imperfect decisions (wrong target, hesitation).
 *   - Adding slight sinusoidal drift to movement direction.
 *   - Randomly slowing down, looking around when idle, and jumping.
 */

#ifndef BOT_HUMANIZE_H
#define BOT_HUMANIZE_H

#include "bot.h"

/* -----------------------------------------------------------------------
   Public API
   ----------------------------------------------------------------------- */

/* Called from Bot_Connect(); initialises the humanise state for a bot. */
void     Bot_Humanize_Init(bot_state_t *bs);

/*
 * Bot_Humanize_Think
 * Called each bot think frame; updates smoothed aim, drift, and imperfect
 * decision flags.  Should be called before Bot_FinalizeMovement().
 */
void     Bot_Humanize_Think(bot_state_t *bs);

/*
 * Bot_Humanize_SetAimTarget
 * Registers a new ideal aim direction.  The humanise system will
 * interpolate toward it with optional overshoot.
 */
void     Bot_Humanize_SetAimTarget(bot_state_t *bs,
                                   float ideal_yaw, float ideal_pitch);

/*
 * Bot_Humanize_ShouldHesitate
 * Returns true if the bot should delay firing this frame (simulates
 * human reaction latency).  Clears the flag once the delay expires.
 */
qboolean Bot_Humanize_ShouldHesitate(bot_state_t *bs);

/*
 * Bot_Humanize_ApplyDrift
 * Returns a modified movement yaw angle with sinusoidal drift applied.
 */
float    Bot_Humanize_ApplyDrift(bot_state_t *bs, float base_yaw);

#endif /* BOT_HUMANIZE_H */
