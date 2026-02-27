/*
 * bot_humanize.c -- human-like movement and behaviour simulation
 *
 * Implements input smoothing, aim overshoot, movement drift, random
 * speed reductions, idle look-arounds, random jumps, and hesitation
 * delays so bot behaviour looks less robotic to human players.
 */

#include "bot_humanize.h"
#include <math.h>
#include <string.h>

/* -----------------------------------------------------------------------
   Internal constants
   ----------------------------------------------------------------------- */
#define HUMANIZE_AIM_SMOOTH_RATE   0.35f  /* fraction per frame (lerp factor)  */
#define HUMANIZE_OVERSHOOT_CHANCE  0.25f  /* probability per aim snap          */
#define HUMANIZE_OVERSHOOT_MAX     8.0f   /* degrees                           */
#define HUMANIZE_SPEED_REDUCE_PROB 0.05f  /* 5% chance per think of slowing    */
#define HUMANIZE_SPEED_REDUCE_DUR  0.4f   /* seconds of slowed movement        */
#define HUMANIZE_LOOK_INTERVAL_MIN 2.0f   /* minimum seconds between looks     */
#define HUMANIZE_LOOK_INTERVAL_MAX 5.0f   /* maximum seconds between looks     */
#define HUMANIZE_JUMP_INTERVAL_MIN 8.0f   /* minimum seconds between jumps     */
#define HUMANIZE_JUMP_INTERVAL_MAX 20.0f  /* maximum seconds between jumps     */
#define HUMANIZE_HESITATE_MIN      0.1f   /* seconds (high-skill bot)          */
#define HUMANIZE_HESITATE_MAX      0.3f   /* seconds (low-skill bot)           */
#define HUMANIZE_MISTAKE_RATE_MAX  8      /* mistake every N thinks at skill 0 */
#define HUMANIZE_MISTAKE_RATE_MIN  64     /* mistake every N thinks at skill 1 */

/* -----------------------------------------------------------------------
   Internal helpers
   ----------------------------------------------------------------------- */

/*
 * pseudo_rand_f
 * Cheap deterministic float in [0, 1) derived from a changing seed.
 * Avoids calling rand() which shares global state across subsystems.
 */
static float pseudo_rand_f(unsigned int *seed)
{
    *seed = (*seed * 1664525u) + 1013904223u;
    return (float)(*seed >> 8) / (float)(1u << 24);
}

/*
 * lerp_angle
 * Linearly interpolates between two angles accounting for wrap-around.
 * t is in [0, 1].
 */
static float lerp_angle(float from, float to, float t)
{
    float delta = to - from;
    /* Normalise to [-180, 180] */
    while (delta >  180.0f) delta -= 360.0f;
    while (delta < -180.0f) delta += 360.0f;
    return from + delta * t;
}

/* -----------------------------------------------------------------------
   Bot_Humanize_Init
   ----------------------------------------------------------------------- */
void Bot_Humanize_Init(bot_state_t *bs)
{
    unsigned int seed;
    float r1, r2, r3, r4;

    if (!bs) return;

    memset(&bs->humanize, 0, sizeof(bs->humanize));

    /* Seed from bot index + name for deterministic variation */
    seed = (unsigned int)(bs->bot_index * 31337u + 12345u);
    {
        /* Mix in a few characters of the bot name */
        const char *n = bs->name;
        while (*n) { seed ^= (unsigned int)(unsigned char)*n++ << ((seed & 3) * 8); }
    }

    r1 = pseudo_rand_f(&seed);
    r2 = pseudo_rand_f(&seed);
    r3 = pseudo_rand_f(&seed);
    r4 = pseudo_rand_f(&seed);

    /* Drift parameters: amplitude [5, 15] degrees, period [1, 3] seconds */
    bs->humanize.drift_amplitude = 5.0f + r1 * 10.0f;
    bs->humanize.drift_period    = 1.0f + r2 * 2.0f;
    bs->humanize.drift_phase     = r3 * 6.283185f; /* random start phase */

    /* Schedule first idle look and first random jump */
    bs->humanize.next_look_time = level.time +
        HUMANIZE_LOOK_INTERVAL_MIN +
        r4 * (HUMANIZE_LOOK_INTERVAL_MAX - HUMANIZE_LOOK_INTERVAL_MIN);

    {
        float r5 = pseudo_rand_f(&seed);
        bs->humanize.next_jump_time = level.time +
            HUMANIZE_JUMP_INTERVAL_MIN +
            r5 * (HUMANIZE_JUMP_INTERVAL_MAX - HUMANIZE_JUMP_INTERVAL_MIN);
    }
}

/* -----------------------------------------------------------------------
   Bot_Humanize_SetAimTarget
   ----------------------------------------------------------------------- */
void Bot_Humanize_SetAimTarget(bot_state_t *bs,
                                float ideal_yaw, float ideal_pitch)
{
    unsigned int seed;
    float r;

    if (!bs) return;

    bs->humanize.target_yaw   = ideal_yaw;
    bs->humanize.target_pitch = ideal_pitch;

    /* Occasionally add an overshoot */
    seed = (unsigned int)(level.time * 1000.0f) ^ (unsigned int)bs->bot_index;
    r    = pseudo_rand_f(&seed);
    if (r < HUMANIZE_OVERSHOOT_CHANCE) {
        float sign  = (pseudo_rand_f(&seed) > 0.5f) ? 1.0f : -1.0f;
        float mag   = pseudo_rand_f(&seed) * HUMANIZE_OVERSHOOT_MAX;
        bs->humanize.overshoot_yaw   = sign * mag;
        bs->humanize.overshoot_decay = mag / 3.0f; /* corrects over ~3 frames */
    }
}

/* -----------------------------------------------------------------------
   Bot_Humanize_ShouldHesitate
   ----------------------------------------------------------------------- */
qboolean Bot_Humanize_ShouldHesitate(bot_state_t *bs)
{
    unsigned int seed;
    float r, delay;
    int threshold;

    if (!bs) return false;

    /* If already inside a hesitation window, keep delaying */
    if (bs->humanize.hesitate_end > level.time)
        return true;

    /* Decide whether to trigger a new hesitation this tick.
     * mistake_counter counts thinks; threshold scales inversely with skill. */
    bs->humanize.mistake_counter++;
    threshold = (int)(HUMANIZE_MISTAKE_RATE_MIN * bs->skill +
                      HUMANIZE_MISTAKE_RATE_MAX * (1.0f - bs->skill));
    if (threshold < 1) threshold = 1;

    if (bs->humanize.mistake_counter < threshold)
        return false;

    /* Roll for a new hesitation */
    seed = (unsigned int)(level.time * 7919.0f) ^ (unsigned int)(bs->bot_index * 31337u);
    r    = pseudo_rand_f(&seed);
    if (r > (1.0f - bs->skill) * 0.4f) {
        bs->humanize.mistake_counter = 0;
        return false; /* No hesitation this time */
    }

    /* Apply a hesitation delay */
    delay = HUMANIZE_HESITATE_MIN +
            (1.0f - bs->skill) * (HUMANIZE_HESITATE_MAX - HUMANIZE_HESITATE_MIN);
    bs->humanize.hesitate_end    = level.time + delay;
    bs->humanize.mistake_counter = 0;
    return true;
}

/* -----------------------------------------------------------------------
   Bot_Humanize_ApplyDrift
   ----------------------------------------------------------------------- */
float Bot_Humanize_ApplyDrift(bot_state_t *bs, float base_yaw)
{
    float offset;

    if (!bs) return base_yaw;

    offset = bs->humanize.drift_amplitude *
             (float)sin((double)bs->humanize.drift_phase);
    return base_yaw + offset;
}

/* -----------------------------------------------------------------------
   Bot_Humanize_Think
   Called every bot think frame.
   ----------------------------------------------------------------------- */
void Bot_Humanize_Think(bot_state_t *bs)
{
    unsigned int seed;
    float r;

    if (!bs) return;

    /* 1. Advance sinusoidal drift phase */
    {
        float period = bs->humanize.drift_period;
        if (period < 0.01f) period = 0.01f;
        bs->humanize.drift_phase += (2.0f * 3.14159265f * BOT_THINK_RATE) / period;
        if (bs->humanize.drift_phase > 2.0f * 3.14159265f)
            bs->humanize.drift_phase -= 2.0f * 3.14159265f;
    }

    /* 2. Smooth aim toward target, applying / decaying overshoot */
    {
        float effective_target_yaw = bs->humanize.target_yaw +
                                     bs->humanize.overshoot_yaw;

        bs->humanize.view_yaw = lerp_angle(bs->humanize.view_yaw,
                                            effective_target_yaw,
                                            HUMANIZE_AIM_SMOOTH_RATE);
        bs->humanize.view_pitch = lerp_angle(bs->humanize.view_pitch,
                                              bs->humanize.target_pitch,
                                              HUMANIZE_AIM_SMOOTH_RATE);

        /* Decay overshoot */
        if (bs->humanize.overshoot_yaw > 0.0f) {
            bs->humanize.overshoot_yaw -= bs->humanize.overshoot_decay;
            if (bs->humanize.overshoot_yaw < 0.0f)
                bs->humanize.overshoot_yaw = 0.0f;
        } else if (bs->humanize.overshoot_yaw < 0.0f) {
            bs->humanize.overshoot_yaw += bs->humanize.overshoot_decay;
            if (bs->humanize.overshoot_yaw > 0.0f)
                bs->humanize.overshoot_yaw = 0.0f;
        }
    }

    /* 3. Random speed reduction: occasionally slow down to simulate
     *    a bot checking a corner or looking around */
    seed = (unsigned int)(level.time * 3571.0f) ^ (unsigned int)(bs->bot_index * 6271u);
    r    = pseudo_rand_f(&seed);
    if (bs->humanize.speed_reduce_end < level.time &&
        r < HUMANIZE_SPEED_REDUCE_PROB * (1.0f - bs->skill * 0.7f)) {
        bs->humanize.speed_reduce_end = level.time + HUMANIZE_SPEED_REDUCE_DUR;
    }

    /* 4. Idle look-around: rotate view when not in combat */
    if (level.time >= bs->humanize.next_look_time &&
        bs->ai_state == BOTSTATE_IDLE) {
        float look_delta;
        r          = pseudo_rand_f(&seed);
        look_delta = (r - 0.5f) * 60.0f; /* ±30 degrees */
        bs->humanize.target_yaw += look_delta;

        r = pseudo_rand_f(&seed);
        bs->humanize.next_look_time = level.time +
            HUMANIZE_LOOK_INTERVAL_MIN +
            r * (HUMANIZE_LOOK_INTERVAL_MAX - HUMANIZE_LOOK_INTERVAL_MIN);
    }

    /* 5. Random jump: schedule a jump for no tactical reason */
    if (level.time >= bs->humanize.next_jump_time &&
        bs->ai_state != BOTSTATE_FLEE &&
        bs->ai_state != BOTSTATE_BUILD) {
        /* The jump is signalled via the humanize state; bot_main.c checks
         * this when building the usercmd.  We just reschedule here. */
        r = pseudo_rand_f(&seed);
        bs->humanize.next_jump_time = level.time +
            HUMANIZE_JUMP_INTERVAL_MIN +
            r * (HUMANIZE_JUMP_INTERVAL_MAX - HUMANIZE_JUMP_INTERVAL_MIN);
    }
}
