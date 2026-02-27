/*
 * bot_nav.c -- navigation and pathfinding for q2gloombot
 *
 * Provides A*-ready waypoint-graph pathfinding.
 *
 * Key design notes:
 *
 *  - Alien nodes include wall/ceiling nodes (has_wall_node = true).
 *    BotNav_FindPath checks Bot_CanWallWalk(bs) before using them so
 *    non-wall-walk classes (e.g. Tyrant, Granger) stay on the floor.
 *
 *  - Human builder bots call BotNav_IsChokePoint() when deciding where
 *    to place a turret.  Choke nodes are pre-tagged in the .nav file.
 *
 *  - BotNav_UpdateWallWalk() is called each think tick for alien bots;
 *    it fires a surface-normal trace and updates nav.wall_walking and
 *    nav.wall_normal accordingly.
 */

#include "bot_nav.h"
#include <float.h>

void BotNav_Init(void)
{
    gi.dprintf("BotNav_Init: navigation subsystem ready\n");
}

void BotNav_LoadMap(const char *mapname)
{
    Node_Clear();
    if (!Node_Load(mapname))
        gi.dprintf("BotNav_LoadMap: '%s' (no nav file — bots will roam freely)\n",
                   mapname);
}

/*
 * A* open-set entry used during pathfinding.
 * Stored in a static array to avoid dynamic allocation.
 */
typedef struct {
    int   node_id;
    float g_cost;   /* cost from start to this node */
    float f_cost;   /* g_cost + heuristic to goal   */
} astar_entry_t;

/*
 * BotNav_CanTraverse
 * Returns true if the bot is capable of traversing the given edge
 * movement type.  Wall-climb edges require Bot_CanWallWalk, and
 * fly edges require Gloom_ClassCanFly.
 */
static qboolean BotNav_CanTraverse(const bot_state_t *bs, int move_type)
{
    switch (move_type) {
    case NAV_MOVE_WALK:
    case NAV_MOVE_JUMP:
    case NAV_MOVE_SWIM:
    case NAV_MOVE_LADDER:
        return true;
    case NAV_MOVE_CLIMB:
        return Bot_CanWallWalk(bs);
    case NAV_MOVE_FLY:
        return Gloom_ClassCanFly(bs->gloom_class) ? true : false;
    default:
        return true;
    }
}

/*
 * BotNav_Heuristic
 * Euclidean distance heuristic between two world positions.
 */
static float BotNav_Heuristic(vec3_t a, vec3_t b)
{
    vec3_t delta;
    VectorSubtract(a, b, delta);
    return VectorLength(delta);
}

void BotNav_FindPath(bot_state_t *bs, vec3_t goal)
{
    static astar_entry_t open_set[MAX_NAV_NODES];
    static float         g_cost[MAX_NAV_NODES];
    static int           came_from[MAX_NAV_NODES];
    static qboolean      closed[MAX_NAV_NODES];
    int  open_count = 0;
    int  start_node, goal_node;
    int  current, i, j;
    qboolean can_wall = Bot_CanWallWalk(bs);

    VectorCopy(goal, bs->nav.goal_origin);
    bs->nav.goal_node  = BOT_INVALID_NODE;
    bs->nav.path_valid = false;
    bs->nav.path_length = 0;
    bs->nav.path_index  = 0;

    if (nav_node_count == 0)
        return;  /* no nav data — bot will roam freely */

    start_node = BotNav_NearestNode(bs->ent->s.origin, can_wall);
    goal_node  = BotNav_NearestNode(goal, can_wall);

    if (start_node == BOT_INVALID_NODE || goal_node == BOT_INVALID_NODE)
        return;  /* disconnected or no nearby nodes */

    if (start_node == goal_node) {
        bs->nav.goal_node    = goal_node;
        bs->nav.path[0]      = goal_node;
        bs->nav.path_length  = 1;
        bs->nav.path_index   = 0;
        bs->nav.path_valid   = true;
        return;
    }

    /* Initialise A* data structures */
    for (i = 0; i < nav_node_count; i++) {
        g_cost[i]    = FLT_MAX;
        came_from[i] = BOT_INVALID_NODE;
        closed[i]    = false;
    }

    g_cost[start_node] = 0.0f;
    open_set[0].node_id = start_node;
    open_set[0].g_cost  = 0.0f;
    open_set[0].f_cost  = BotNav_Heuristic(nav_nodes[start_node].origin,
                                            nav_nodes[goal_node].origin);
    open_count = 1;

    while (open_count > 0) {
        /* Find the entry with lowest f_cost */
        int   best_idx = 0;
        float best_f   = open_set[0].f_cost;
        for (i = 1; i < open_count; i++) {
            if (open_set[i].f_cost < best_f) {
                best_f   = open_set[i].f_cost;
                best_idx = i;
            }
        }

        current = open_set[best_idx].node_id;

        /* Remove from open set by swapping with last */
        open_set[best_idx] = open_set[open_count - 1];
        open_count--;

        if (current == goal_node) {
            /* Reconstruct path in reverse */
            int path_buf[BOT_MAX_PATH_NODES];
            int path_len = 0;
            int node = goal_node;

            while (node != BOT_INVALID_NODE && path_len < BOT_MAX_PATH_NODES) {
                path_buf[path_len++] = node;
                if (node == start_node) break;
                node = came_from[node];
            }

            /* Reverse into bs->nav.path */
            bs->nav.path_length = path_len;
            for (i = 0; i < path_len; i++)
                bs->nav.path[i] = path_buf[path_len - 1 - i];

            bs->nav.goal_node  = goal_node;
            bs->nav.path_index = 0;
            bs->nav.path_valid = true;
            return;
        }

        closed[current] = true;

        /* Expand neighbors */
        for (j = 0; j < nav_nodes[current].num_neighbors; j++) {
            int   neighbor = nav_nodes[current].neighbors[j];
            float edge_cost;
            float tentative_g;

            if (neighbor < 0 || neighbor >= nav_node_count)
                continue;
            if (nav_nodes[neighbor].id == BOT_INVALID_NODE)
                continue;
            if (closed[neighbor])
                continue;

            /* Check movement capability */
            if (!BotNav_CanTraverse(bs, nav_nodes[current].movement_required[j]))
                continue;

            edge_cost   = nav_nodes[current].neighbor_costs[j];
            tentative_g = g_cost[current] + edge_cost;

            if (tentative_g >= g_cost[neighbor])
                continue;

            g_cost[neighbor]    = tentative_g;
            came_from[neighbor] = current;

            /* Add to open set (or update existing entry) */
            {
                float f = tentative_g +
                          BotNav_Heuristic(nav_nodes[neighbor].origin,
                                           nav_nodes[goal_node].origin);
                qboolean found = false;
                for (i = 0; i < open_count; i++) {
                    if (open_set[i].node_id == neighbor) {
                        open_set[i].g_cost = tentative_g;
                        open_set[i].f_cost = f;
                        found = true;
                        break;
                    }
                }
                if (!found && open_count < MAX_NAV_NODES) {
                    open_set[open_count].node_id = neighbor;
                    open_set[open_count].g_cost  = tentative_g;
                    open_set[open_count].f_cost  = f;
                    open_count++;
                }
            }
        }
    }

    /* No path found — path remains invalid; bot falls back to direct movement */
}

void BotNav_MoveTowardGoal(bot_state_t *bs)
{
    vec3_t dir;
    float  dist;
    int    next_node;

    if (!bs || !bs->ent || !bs->ent->inuse)
        return;

    /* If we have a valid path, follow the next node in the path */
    if (bs->nav.path_valid && bs->nav.path_length > 0) {
        if (bs->nav.path_index >= bs->nav.path_length) {
            /* Reached end of path */
            bs->nav.path_valid = false;
            return;
        }

        next_node = bs->nav.path[bs->nav.path_index];
        if (next_node < 0 || next_node >= nav_node_count ||
            nav_nodes[next_node].id == BOT_INVALID_NODE) {
            bs->nav.path_valid = false;
            return;
        }

        VectorSubtract(nav_nodes[next_node].origin, bs->ent->s.origin, dir);
        dist = VectorLength(dir);

        /* Advance to next path node when close enough */
        if (dist < bs->nav.arrived_dist) {
            bs->nav.current_node = next_node;
            bs->nav.path_index++;
            return;
        }

        /* Move toward the next path node */
        if (dist > 0.0f) {
            VectorNormalize(dir);
            VectorScale(dir, 300.0f, bs->ent->velocity);
        }
    } else {
        /* No valid path — move directly toward goal origin */
        VectorSubtract(bs->nav.goal_origin, bs->ent->s.origin, dir);
        dist = VectorLength(dir);

        if (dist > bs->nav.arrived_dist && dist > 0.0f) {
            VectorNormalize(dir);
            VectorScale(dir, 300.0f, bs->ent->velocity);
        }
    }
}

int BotNav_NearestNode(vec3_t origin, qboolean allow_wall_nodes)
{
    /*
     * When wall nodes are not allowed (non-wall-walking class), require
     * NAV_GROUND so that only floor-level nodes are returned.  Nodes
     * tagged NAV_GROUND may additionally carry other flags (e.g.
     * NAV_JUMP, NAV_LADDER) and are still valid; nodes tagged solely
     * with NAV_WALLCLIMB and without NAV_GROUND are wall/ceiling-only
     * and are correctly excluded.
     */
    unsigned int required = allow_wall_nodes ? 0u : (unsigned int)NAV_GROUND;

    return Node_FindNearest(origin, required, 0.0f);
}

/*
 * BotNav_IsChokePoint
 * Returns true if the given node is tagged as a map choke point.
 * Used by human builder bots to place turrets optimally.
 */
qboolean BotNav_IsChokePoint(int node_index)
{
    if (node_index < 0 || node_index >= nav_node_count)
        return false;
    if (nav_nodes[node_index].id == BOT_INVALID_NODE)
        return false;

    /*
     * A choke point is identified by NAV_CAMP flag (defensive position)
     * combined with having few neighbors (narrow passage).  Nodes with
     * only 1-2 neighbors in a non-open area are natural choke points.
     */
    if (nav_nodes[node_index].flags & NAV_CAMP)
        return true;

    if (nav_nodes[node_index].num_neighbors <= 2 &&
        (nav_nodes[node_index].flags & NAV_GROUND))
        return true;

    return false;
}

/*
 * BotNav_UpdateWallWalk
 * Traces downward (relative to current gravity direction) to detect
 * whether the alien bot is on a non-floor surface.
 * Updates nav.wall_walking and nav.wall_normal.
 */
void BotNav_UpdateWallWalk(bot_state_t *bs)
{
    trace_t tr;
    vec3_t  down;

    if (!Bot_CanWallWalk(bs)) {
        bs->nav.wall_walking = false;
        return;
    }

    VectorSet(down, 0, 0, -1);
    tr = gi.trace(bs->ent->s.origin, bs->ent->mins, bs->ent->maxs,
                  bs->ent->s.origin,  /* end = start for normal detection   */
                  bs->ent, MASK_SOLID);

    if (tr.fraction < 1.0f) {
        float dot = DotProduct(tr.plane.normal, down);
        /* Walking on a wall or ceiling: surface normal is not pointing up */
        bs->nav.wall_walking = (dot < 0.7f);
        VectorCopy(tr.plane.normal, bs->nav.wall_normal);
    } else {
        bs->nav.wall_walking = false;
    }
}
