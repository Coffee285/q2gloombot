/*
 * bot_class_kamikaze.c -- Kamikaze AI (suicide bomber alien)
 *
 * The Kamikaze ignores most enemies and charges toward a high-value
 * detonation target.  Its only purpose is to reach the target and
 * detonate within blast radius.
 *
 * Behavior:
 *  - Identify high-value targets: Teleporter with humans, turret cluster,
 *    group of 3+ humans, Engineer building, lone Mech
 *  - Sprint directly toward target using the shortest path
 *  - Use wall-climbing to avoid being killed before reaching target
 *  - Detonate when within ~200 units (KAMIKAZE_BLAST_RADIUS)
 *  - Do NOT detonate on a single low-value target (one Grunt alone)
 *  - If critically wounded near ANY enemy, detonate immediately
 *  - Zigzag approach if under sustained fire
 */

#include "../bot.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define KAMIKAZE_BLAST_RADIUS     200.0f   /* detonate within this range    */
#define KAMIKAZE_CRITICAL_HP_PCT  0.20f    /* immediate detonate threshold  */
#define KAMIKAZE_LOW_VALUE_DIST_MULT 3.0f  /* low-value target must be this many blast radii away to skip */

/* -----------------------------------------------------------------------
   BotClass_Init_kamikaze
   ----------------------------------------------------------------------- */
void BotClass_Init_kamikaze(bot_state_t *bs)
{
    bs->combat.engagement_range = KAMIKAZE_BLAST_RADIUS;
    bs->combat.prefer_cover     = false; /* sprint straight to target       */
    bs->combat.max_range_engage = false;
    bs->nav.wall_walking        = true;  /* use walls to bypass fire        */

    bs->ai_state = BOTSTATE_HUNT;
}

/* -----------------------------------------------------------------------
   BotClass_Think_kamikaze
   ----------------------------------------------------------------------- */
void BotClass_Think_kamikaze(bot_state_t *bs)
{
    float hp_pct;

    if (!bs->ent || !bs->ent->inuse) return;

    hp_pct = (bs->ent->max_health > 0)
             ? (float)bs->ent->health / (float)bs->ent->max_health
             : 1.0f;

    /*
     * If critically wounded AND near any enemy, detonate immediately.
     * The actual explosion is triggered by the game engine; here we ensure
     * the bot charges into melee range so the detonation is effective.
     */
    if (hp_pct < KAMIKAZE_CRITICAL_HP_PCT && bs->combat.target_visible) {
        bs->combat.engagement_range = 0.0f; /* close to point-blank         */
        bs->ai_state                = BOTSTATE_COMBAT;
        return;
    }

    /*
     * Acquire a detonation target.  Kamikaze ignores other enemies in
     * favor of the highest-value target in memory.
     */
    if (bs->combat.target_visible) {
        bot_state_t *tgt = Bot_GetState(bs->combat.target);

        /* Low-value lone Grunt far away: not worth detonating — look for better */
        if (tgt && tgt->gloom_class == GLOOM_CLASS_GRUNT &&
            bs->combat.target_dist > KAMIKAZE_BLAST_RADIUS * KAMIKAZE_LOW_VALUE_DIST_MULT) {
            bs->combat.target         = NULL;
            bs->combat.target_visible = false;
            bs->ai_state              = BOTSTATE_PATROL;
            return;
        }

        /* Good target in sight — charge */
        bs->combat.engagement_range = KAMIKAZE_BLAST_RADIUS * 0.8f;
        bs->ai_state                = BOTSTATE_COMBAT;
    } else if (bs->combat.target == NULL) {
        /* No target yet — patrol to find a suitable detonation opportunity */
        bs->ai_state = BOTSTATE_PATROL;
    }
}
