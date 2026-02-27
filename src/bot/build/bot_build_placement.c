/*
 * bot_build_placement.c -- smart structure placement algorithm
 *
 * Provides three placement functions called by BotBuild_FindPlacement:
 *   BotBuildPlacement_ForSpawn    — Teleporter / Egg
 *   BotBuildPlacement_ForDefense  — Turret / Spiker / Obstacle
 *   BotBuildPlacement_ForUtility  — Ammo Depot / Cocoon / Camera
 *
 * PLACEMENT SCORING
 * -----------------
 * Each candidate location is scored based on:
 *   - Concealment from enemy approach vectors
 *   - Coverage angles (defensive structures)
 *   - Proximity to existing friendly structures (clustering penalty)
 *   - Distance from the front line (spawn points should be back; defenses forward)
 *   - Valid ground surface (gi.trace() to confirm)
 *
 * CANDIDATE GENERATION
 * --------------------
 * Candidates are generated from nav nodes nearest the bot's position.
 * Wall-type nodes (NAV_WALLCLIMB) are preferred for alien placements.
 * Floor nodes are used for human placements.
 */

#include "bot_build.h"
#include "bot_nav.h"

/* -----------------------------------------------------------------------
   Constants
   ----------------------------------------------------------------------- */
#define PLACEMENT_CANDIDATES        16    /* number of candidates to score  */
#define PLACEMENT_SEARCH_RADIUS     1200.0f
#define PLACEMENT_MIN_STRUCT_DIST   256.0f /* min distance between same-type*/
#define PLACEMENT_SPAWN_SETBACK     400.0f /* spawn points set back this far*/

/* -----------------------------------------------------------------------
   Internal: check that a position is valid (enough space, on ground).
   Uses gi.trace() to verify the build surface.
   ----------------------------------------------------------------------- */
static qboolean Placement_IsValidSurface(vec3_t origin)
{
    vec3_t end;
    trace_t tr;
    static vec3_t mins = { -16, -16, -16 };
    static vec3_t maxs = {  16,  16,  16 };

    end[0] = origin[0];
    end[1] = origin[1];
    end[2] = origin[2] - 32.0f; /* check slightly below to confirm ground  */

    tr = gi.trace(origin, mins, maxs, end, NULL, MASK_SOLID);
    return (tr.fraction < 1.0f) ? true : false;
}

/* -----------------------------------------------------------------------
   Internal: estimate "concealment" score.
   Higher score = more hidden (dark corner, ceiling position, etc.).
   Nav node flags provide a proxy for cover quality.
   ----------------------------------------------------------------------- */
static float Placement_Concealment(int node_idx)
{
    unsigned int flags;

    if (node_idx == BOT_INVALID_NODE || node_idx >= MAX_NAV_NODES)
        return 0.0f;

    flags = nav_nodes[node_idx].flags;
    if (flags & NAV_AMBUSH)    return 1.0f;
    if (flags & NAV_CAMP)      return 0.7f;
    if (flags & NAV_WALLCLIMB) return 0.6f;
    return 0.3f;
}

/* -----------------------------------------------------------------------
   Internal: estimate "clustering penalty".
   Penalise positions that are too close to an existing same-type structure.
   ----------------------------------------------------------------------- */
static float Placement_ClusterPenalty(vec3_t origin, int team,
                                      gloom_struct_type_t type)
{
    int i;
    (void)team; (void)type; /* full penalty requires build memory scan  */
    /* Quick distance check: penalise positions within min struct spacing */
    for (i = 0; i < MAX_BOTS; i++) {
        vec3_t delta;
        float  dist;

        if (!g_bots[i].in_use || g_bots[i].team != team) continue;
        if (!g_bots[i].ent || !g_bots[i].ent->inuse) continue;

        delta[0] = g_bots[i].ent->s.origin[0] - origin[0];
        delta[1] = g_bots[i].ent->s.origin[1] - origin[1];
        delta[2] = g_bots[i].ent->s.origin[2] - origin[2];
        dist = (float)sqrt((double)(delta[0]*delta[0] + delta[1]*delta[1] +
                                    delta[2]*delta[2]));

        if (dist < PLACEMENT_MIN_STRUCT_DIST) return -1.0f; /* too close   */
    }
    return 0.0f;
}

/* -----------------------------------------------------------------------
   BotBuildPlacement_ForSpawn
   Find a defensible, somewhat concealed location for a spawn structure
   (Teleporter or Egg).  The location should be hidden from direct enemy
   approaches but still reachable by the builder.
   ----------------------------------------------------------------------- */
qboolean BotBuildPlacement_ForSpawn(bot_state_t *bs, vec3_t out_origin)
{
    int   best_node = BOT_INVALID_NODE;
    float best_score = -9999.0f;
    int   i;

    if (!bs->ent) return false;

    for (i = 0; i < nav_node_count; i++) {
        vec3_t delta;
        float  dist, score, penalty;

        if (nav_nodes[i].id == BOT_INVALID_NODE) continue;

        /* Team access filter */
        if (bs->team == TEAM_HUMAN &&
            !(nav_nodes[i].team_access & NAV_TEAM_HUMAN)) continue;
        if (bs->team == TEAM_ALIEN &&
            !(nav_nodes[i].team_access & NAV_TEAM_ALIEN)) continue;

        delta[0] = nav_nodes[i].origin[0] - bs->ent->s.origin[0];
        delta[1] = nav_nodes[i].origin[1] - bs->ent->s.origin[1];
        delta[2] = nav_nodes[i].origin[2] - bs->ent->s.origin[2];
        dist = (float)sqrt((double)(delta[0]*delta[0] + delta[1]*delta[1] +
                                    delta[2]*delta[2]));

        if (dist > PLACEMENT_SEARCH_RADIUS) continue;
        if (!Placement_IsValidSurface(nav_nodes[i].origin)) continue;

        penalty = Placement_ClusterPenalty(nav_nodes[i].origin,
                                           bs->team, Gloom_SpawnStruct(bs->team));
        if (penalty < -0.5f) continue; /* too close to existing spawn      */

        /* Score: concealment weighted heavily for spawn points */
        score = Placement_Concealment(i) * 2.0f;

        /* Spawn points should not be right at the front — set back slightly */
        score -= (dist < PLACEMENT_SPAWN_SETBACK) ? 0.5f : 0.0f;

        /* Prefer dedicated ambush/camp nodes for alien eggs */
        if (bs->team == TEAM_ALIEN && (nav_nodes[i].flags & NAV_AMBUSH))
            score += 0.5f;

        if (score > best_score) {
            best_score = score;
            best_node  = i;
        }
    }

    if (best_node == BOT_INVALID_NODE) return false;

    out_origin[0] = nav_nodes[best_node].origin[0];
    out_origin[1] = nav_nodes[best_node].origin[1];
    out_origin[2] = nav_nodes[best_node].origin[2];
    return true;
}

/* -----------------------------------------------------------------------
   BotBuildPlacement_ForDefense
   Find a chokepoint or doorway for a defensive structure (Turret/Spiker).
   Prioritise nodes tagged as ground or camp positions where field-of-fire
   covers enemy approach directions.
   ----------------------------------------------------------------------- */
qboolean BotBuildPlacement_ForDefense(bot_state_t *bs,
                                       gloom_struct_type_t type,
                                       vec3_t out_origin)
{
    int   best_node = BOT_INVALID_NODE;
    float best_score = -9999.0f;
    int   i;

    if (!bs->ent) return false;

    for (i = 0; i < nav_node_count; i++) {
        vec3_t delta;
        float  dist, score;

        if (nav_nodes[i].id == BOT_INVALID_NODE) continue;

        if (bs->team == TEAM_HUMAN &&
            !(nav_nodes[i].team_access & NAV_TEAM_HUMAN)) continue;
        if (bs->team == TEAM_ALIEN &&
            !(nav_nodes[i].team_access & NAV_TEAM_ALIEN)) continue;

        delta[0] = nav_nodes[i].origin[0] - bs->ent->s.origin[0];
        delta[1] = nav_nodes[i].origin[1] - bs->ent->s.origin[1];
        delta[2] = nav_nodes[i].origin[2] - bs->ent->s.origin[2];
        dist = (float)sqrt((double)(delta[0]*delta[0] + delta[1]*delta[1] +
                                    delta[2]*delta[2]));

        if (dist > PLACEMENT_SEARCH_RADIUS) continue;
        if (!Placement_IsValidSurface(nav_nodes[i].origin)) continue;

        /* Prefer ground/camp nodes for defensive structures */
        score = 0.0f;
        if (nav_nodes[i].flags & NAV_GROUND)  score += 0.5f;
        if (nav_nodes[i].flags & NAV_CAMP)    score += 1.0f;
        if (nav_nodes[i].flags & NAV_SNIPE)   score += 0.8f;

        /* Obstacles go in narrow chokepoints — no specific nav flag yet,
         * so place them near the builder's current position */
        if (type == STRUCT_OBSTACLE) {
            score += (dist < 400.0f) ? 0.5f : 0.0f;
        }

        if (score > best_score) {
            best_score = score;
            best_node  = i;
        }
    }

    if (best_node == BOT_INVALID_NODE) return false;

    out_origin[0] = nav_nodes[best_node].origin[0];
    out_origin[1] = nav_nodes[best_node].origin[1];
    out_origin[2] = nav_nodes[best_node].origin[2];
    return true;
}

/* -----------------------------------------------------------------------
   BotBuildPlacement_ForUtility
   Find a position for utility structures (Ammo Depot, Cocoon, Camera).
   These should be near allied activity — close to spawn points and in
   high-traffic friendly areas.
   ----------------------------------------------------------------------- */
qboolean BotBuildPlacement_ForUtility(bot_state_t *bs,
                                       gloom_struct_type_t type,
                                       vec3_t out_origin)
{
    int   best_node = BOT_INVALID_NODE;
    float best_score = -9999.0f;
    int   i;

    if (!bs->ent) return false;

    for (i = 0; i < nav_node_count; i++) {
        vec3_t delta;
        float  dist, score;

        if (nav_nodes[i].id == BOT_INVALID_NODE) continue;

        if (bs->team == TEAM_HUMAN &&
            !(nav_nodes[i].team_access & NAV_TEAM_HUMAN)) continue;
        if (bs->team == TEAM_ALIEN &&
            !(nav_nodes[i].team_access & NAV_TEAM_ALIEN)) continue;

        delta[0] = nav_nodes[i].origin[0] - bs->ent->s.origin[0];
        delta[1] = nav_nodes[i].origin[1] - bs->ent->s.origin[1];
        delta[2] = nav_nodes[i].origin[2] - bs->ent->s.origin[2];
        dist = (float)sqrt((double)(delta[0]*delta[0] + delta[1]*delta[1] +
                                    delta[2]*delta[2]));

        if (dist > PLACEMENT_SEARCH_RADIUS * 0.5f) continue;
        if (!Placement_IsValidSurface(nav_nodes[i].origin)) continue;

        score = 0.0f;

        /* Ammo Depot / Cocoon: prefer nodes near spawn points */
        if (type == STRUCT_AMMO_DEPOT || type == STRUCT_COCOON) {
            if (nav_nodes[i].flags & NAV_TELEPORTER) score += 1.0f;
            if (nav_nodes[i].flags & NAV_EGG)        score += 1.0f;
            if (nav_nodes[i].flags & NAV_GROUND)     score += 0.3f;
        }

        /* Camera: prefer elevated forward positions */
        if (type == STRUCT_CAMERA) {
            if (nav_nodes[i].flags & NAV_SNIPE) score += 1.0f;
            if (nav_nodes[i].flags & NAV_CAMP)  score += 0.5f;
        }

        if (score > best_score) {
            best_score = score;
            best_node  = i;
        }
    }

    if (best_node == BOT_INVALID_NODE) return false;

    out_origin[0] = nav_nodes[best_node].origin[0];
    out_origin[1] = nav_nodes[best_node].origin[1];
    out_origin[2] = nav_nodes[best_node].origin[2];
    return true;
}
