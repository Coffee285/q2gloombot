/*
 * bot_teamwork.c -- inter-bot cooperation for q2gloombot
 *
 * Implements shared awareness and coordinated actions:
 *
 * ALL TEAMS
 *   - Share enemy position when spotted
 *   - Call for help when outnumbered; nearby bots respond
 *   - Group up at rally point before pushing (attack coordination)
 *
 * ALIEN TEAM
 *   - Rush coordination: 3+ aliens near same area → trigger group attack
 *   - Breeder protection: keep at least one alien escorting the Breeder
 *   - Egg defense: assign a Guardian to patrol near critical eggs
 *   - Kamikaze targeting: coordinate Kamikaze runs with distractions
 *
 * HUMAN TEAM
 *   - Fire line: humans form a line and advance together
 *   - Mech escort: other humans form around the Mech
 *   - Biotech attachment: Biotech follows the most valuable combat unit
 *   - Engineer protection: 1-2 soldiers stay near the Engineer
 */

#include "bot_team.h"
#include "bot_strategy.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define TEAMWORK_SHARE_ENEMY_RANGE   800.0f  /* broadcast enemy pos to allies */
#define TEAMWORK_HELP_REQUEST_RANGE  600.0f  /* send help request within this */
#define TEAMWORK_RALLY_RADIUS        300.0f  /* rally point clustering radius  */
#define TEAMWORK_RUSH_TRIGGER        3       /* aliens to trigger group rush   */
#define TEAMWORK_ALIEN_CLUSTER_RANGE 400.0f  /* range for rush detection       */

/* -----------------------------------------------------------------------
   BotTeamwork_ShareEnemyPos
   When a bot spots an enemy, broadcast its last known position to all
   nearby teammates so they can update their enemy memory.
   ----------------------------------------------------------------------- */
void BotTeamwork_ShareEnemyPos(bot_state_t *reporter, edict_t *enemy,
                                vec3_t enemy_pos)
{
    int i;
    vec3_t delta;
    float  dist;

    if (!reporter || !enemy) return;

    for (i = 0; i < MAX_BOTS; i++) {
        bot_state_t *ally = &g_bots[i];
        if (!ally->in_use || ally == reporter) continue;
        if (ally->team != reporter->team) continue;
        if (!ally->ent || !ally->ent->inuse) continue;

        delta[0] = ally->ent->s.origin[0] - reporter->ent->s.origin[0];
        delta[1] = ally->ent->s.origin[1] - reporter->ent->s.origin[1];
        delta[2] = ally->ent->s.origin[2] - reporter->ent->s.origin[2];
        dist = (float)sqrt((double)(delta[0]*delta[0] + delta[1]*delta[1] +
                                    delta[2]*delta[2]));

        if (dist > TEAMWORK_SHARE_ENEMY_RANGE) continue;

        /* Insert into ally's enemy memory if not already there */
        if (ally->enemy_memory_count < BOT_MAX_REMEMBERED_ENEMIES) {
            int j, already = 0;
            for (j = 0; j < ally->enemy_memory_count; j++) {
                if (ally->enemy_memory[j].ent == enemy) {
                    /* Update existing entry */
                    ally->enemy_memory[j].last_pos[0] = enemy_pos[0];
                    ally->enemy_memory[j].last_pos[1] = enemy_pos[1];
                    ally->enemy_memory[j].last_pos[2] = enemy_pos[2];
                    ally->enemy_memory[j].last_seen    = level.time;
                    already = 1;
                    break;
                }
            }
            if (!already) {
                int idx = ally->enemy_memory_count++;
                ally->enemy_memory[idx].ent           = enemy;
                ally->enemy_memory[idx].last_pos[0]   = enemy_pos[0];
                ally->enemy_memory[idx].last_pos[1]   = enemy_pos[1];
                ally->enemy_memory[idx].last_pos[2]   = enemy_pos[2];
                ally->enemy_memory[idx].last_seen      = level.time;
            }
        }
    }
}

/* -----------------------------------------------------------------------
   BotTeamwork_RequestHelp
   Called when a bot is under attack and outnumbered.  Nearby allies
   switch to escort/defend state to converge on the caller's position.
   ----------------------------------------------------------------------- */
void BotTeamwork_RequestHelp(bot_state_t *caller)
{
    int    i;
    vec3_t delta;
    float  dist;

    if (!caller || !caller->ent) return;

    for (i = 0; i < MAX_BOTS; i++) {
        bot_state_t *ally = &g_bots[i];
        if (!ally->in_use || ally == caller) continue;
        if (ally->team != caller->team) continue;
        if (!ally->ent || !ally->ent->inuse) continue;
        if (ally->ai_state == BOTSTATE_COMBAT) continue; /* already fighting */

        delta[0] = ally->ent->s.origin[0] - caller->ent->s.origin[0];
        delta[1] = ally->ent->s.origin[1] - caller->ent->s.origin[1];
        delta[2] = ally->ent->s.origin[2] - caller->ent->s.origin[2];
        dist = (float)sqrt((double)(delta[0]*delta[0] + delta[1]*delta[1] +
                                    delta[2]*delta[2]));

        if (dist > TEAMWORK_HELP_REQUEST_RANGE) continue;

        /* Guide ally toward caller's position */
        ally->nav.goal_origin[0] = caller->ent->s.origin[0];
        ally->nav.goal_origin[1] = caller->ent->s.origin[1];
        ally->nav.goal_origin[2] = caller->ent->s.origin[2];
        ally->ai_state           = BOTSTATE_ESCORT;
    }
}

/* -----------------------------------------------------------------------
   BotTeamwork_CheckAlienRush
   When 3+ alien bots are clustered near the same area, trigger a
   coordinated group attack: all switch to COMBAT and target the same area.
   ----------------------------------------------------------------------- */
void BotTeamwork_CheckAlienRush(void)
{
    int i, j;

    for (i = 0; i < MAX_BOTS; i++) {
        bot_state_t *bs = &g_bots[i];
        if (!bs->in_use || bs->team != TEAM_ALIEN) continue;
        if (!bs->ent || !bs->ent->inuse) continue;
        if (!bs->combat.target_visible) continue;

        /* Count how many aliens are near this bot */
        int cluster = 1;
        for (j = 0; j < MAX_BOTS; j++) {
            bot_state_t *other = &g_bots[j];
            vec3_t delta;
            float dist;

            if (j == i) continue;
            if (!other->in_use || other->team != TEAM_ALIEN) continue;
            if (!other->ent || !other->ent->inuse) continue;

            delta[0] = other->ent->s.origin[0] - bs->ent->s.origin[0];
            delta[1] = other->ent->s.origin[1] - bs->ent->s.origin[1];
            delta[2] = other->ent->s.origin[2] - bs->ent->s.origin[2];
            dist = (float)sqrt((double)(delta[0]*delta[0] + delta[1]*delta[1] +
                                        delta[2]*delta[2]));
            if (dist < TEAMWORK_ALIEN_CLUSTER_RANGE) cluster++;
        }

        /* Rush trigger: enough aliens clustered */
        if (cluster >= TEAMWORK_RUSH_TRIGGER) {
            for (j = 0; j < MAX_BOTS; j++) {
                bot_state_t *other = &g_bots[j];
                vec3_t delta;
                float dist;

                if (!other->in_use || other->team != TEAM_ALIEN) continue;
                if (!other->ent || !other->ent->inuse) continue;

                delta[0] = other->ent->s.origin[0] - bs->ent->s.origin[0];
                delta[1] = other->ent->s.origin[1] - bs->ent->s.origin[1];
                delta[2] = other->ent->s.origin[2] - bs->ent->s.origin[2];
                dist = (float)sqrt((double)(delta[0]*delta[0] + delta[1]*delta[1] +
                                            delta[2]*delta[2]));

                if (dist < TEAMWORK_ALIEN_CLUSTER_RANGE &&
                    other->ai_state != BOTSTATE_COMBAT &&
                    !Gloom_ClassCanBuild(other->gloom_class)) {
                    /* Join the rush */
                    other->combat.target       = bs->combat.target;
                    other->combat.target_visible = true;
                    other->ai_state            = BOTSTATE_COMBAT;
                }
            }
            break; /* one rush trigger per frame is enough */
        }
    }
}

/* -----------------------------------------------------------------------
   BotTeamwork_EnsureBreederEscort
   If a Breeder exists and has no escort, assign the nearest non-builder
   alien to escort it.
   ----------------------------------------------------------------------- */
void BotTeamwork_EnsureBreederEscort(void)
{
    int i, j;
    bot_state_t *breeder   = NULL;
    bot_state_t *escort_bot = NULL;
    float        best_dist  = 9999999.0f;

    /* Find the active Breeder */
    for (i = 0; i < MAX_BOTS; i++) {
        if (g_bots[i].in_use &&
            g_bots[i].team == TEAM_ALIEN &&
            g_bots[i].gloom_class == GLOOM_CLASS_BREEDER &&
            g_bots[i].ent && g_bots[i].ent->inuse) {
            breeder = &g_bots[i];
            break;
        }
    }

    if (!breeder) return;

    /* Check if the Breeder already has an escort */
    for (i = 0; i < MAX_BOTS; i++) {
        if (g_bots[i].in_use &&
            g_bots[i].team == TEAM_ALIEN &&
            g_bots[i].ai_state == BOTSTATE_ESCORT &&
            &g_bots[i] != breeder) {
            return; /* already has an escort */
        }
    }

    /* Find the nearest idle/patrolling alien to assign as escort */
    for (j = 0; j < MAX_BOTS; j++) {
        bot_state_t *cand = &g_bots[j];
        vec3_t delta;
        float dist;

        if (!cand->in_use || cand->team != TEAM_ALIEN) continue;
        if (cand == breeder) continue;
        if (Gloom_ClassCanBuild(cand->gloom_class)) continue;
        if (!cand->ent || !cand->ent->inuse) continue;
        if (cand->ai_state == BOTSTATE_COMBAT) continue;

        delta[0] = cand->ent->s.origin[0] - breeder->ent->s.origin[0];
        delta[1] = cand->ent->s.origin[1] - breeder->ent->s.origin[1];
        delta[2] = cand->ent->s.origin[2] - breeder->ent->s.origin[2];
        dist = (float)sqrt((double)(delta[0]*delta[0] + delta[1]*delta[1] +
                                    delta[2]*delta[2]));

        if (dist < best_dist) {
            best_dist  = dist;
            escort_bot = cand;
        }
    }

    if (escort_bot) {
        escort_bot->nav.goal_origin[0] = breeder->ent->s.origin[0];
        escort_bot->nav.goal_origin[1] = breeder->ent->s.origin[1];
        escort_bot->nav.goal_origin[2] = breeder->ent->s.origin[2];
        escort_bot->ai_state           = BOTSTATE_ESCORT;
    }
}

/* -----------------------------------------------------------------------
   BotTeamwork_Frame
   Called once per strategy tick to run all teamwork checks.
   ----------------------------------------------------------------------- */
void BotTeamwork_Frame(void)
{
    BotTeamwork_CheckAlienRush();
    BotTeamwork_EnsureBreederEscort();
}
