/*
 * bot_combat.c -- combat AI for q2gloombot
 *
 * Implements asymmetric target prioritisation and firing logic.
 *
 * ALIEN combat doctrine:
 *   - Target: Reactor → Builders → Turrets → Marines
 *   - Approach using wall-walk routes to avoid turret fire.
 *   - Close to preferred_range before firing (melee/short-range classes).
 *   - Kamikaze subclass ignores personal safety; charges structures.
 *
 * HUMAN combat doctrine:
 *   - Target: Overmind → Grangers → Alien structures → Alien players
 *   - Maintain maximum effective range at all times.
 *   - Prioritise protecting the Builder teammate and the Reactor.
 *   - Fall back toward Reactor / Medistation at low health.
 */

#include "bot_combat.h"

/* Maximum positional aim offset at skill 0.0 (world units) */
#define BOT_MAX_AIM_ERROR 150.0f

/*
 * BotCombat_AimError
 * Returns a positional aim offset based on skill level.
 * Low skill → large random error; high skill → near-zero error.
 */
float BotCombat_AimError(float skill)
{
    return BOT_MAX_AIM_ERROR * (1.0f - skill) * ((rand() & 0xFF) / 255.0f);
}

/*
 * BotCombat_PickTarget
 * Scans the edict list and returns the highest-priority valid target.
 * Full priority logic is team-specific (see header).
 */
edict_t *BotCombat_PickTarget(bot_state_t *bs)
{
    /* Stub: full scan with priority ordering in later iteration */
    (void)bs;
    return NULL;
}

/*
 * BotCombat_AimAtTarget
 * Adjusts the bot's view angles toward the target, adding skill-based error.
 */
void BotCombat_AimAtTarget(bot_state_t *bs)
{
    vec3_t dir;

    if (!bs->combat.target) return;

    VectorSubtract(bs->combat.target->s.origin,
                   bs->ent->s.origin, dir);
    VectorNormalize(dir);

    /* Add aim error proportional to inverse skill */
    dir[0] += BotCombat_AimError(bs->skill);
    dir[1] += BotCombat_AimError(bs->skill);
    /* (Apply to ent->client->ps.viewangles in full impl) */
}

/*
 * BotCombat_Fire
 * Issues the attack input if conditions are met.
 */
void BotCombat_Fire(bot_state_t *bs)
{
    if (!bs->combat.target_visible) return;
    if (level.time < bs->combat.next_attack_time) return;

    bs->combat.weapon_state    = BOT_WEAPON_FIRING;
    bs->combat.next_attack_time = level.time + 0.1f;  /* rate-limited */
}

/*
 * BotCombat_Think
 * Called from Bot_StateCombat each think tick.
 */
void BotCombat_Think(bot_state_t *bs)
{
    if (!bs->combat.target)
        bs->combat.target = BotCombat_PickTarget(bs);

    if (!bs->combat.target) return;

    BotCombat_AimAtTarget(bs);
    BotCombat_Fire(bs);
}
