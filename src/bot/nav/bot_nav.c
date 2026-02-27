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

void BotNav_Init(void)
{
    gi.dprintf("BotNav_Init: navigation subsystem ready (stub)\n");
}

void BotNav_LoadMap(const char *mapname)
{
    /* TODO: load maps/<mapname>.nav waypoint file */
    gi.dprintf("BotNav_LoadMap: '%s' (no nav file — bots will roam freely)\n",
               mapname);
}

void BotNav_FindPath(bot_state_t *bs, vec3_t goal)
{
    VectorCopy(goal, bs->nav.goal_origin);
    bs->nav.goal_node  = BOT_INVALID_NODE;
    bs->nav.path_valid = false;
    /* TODO: A* search on loaded node graph; respect wall-walk capability */
}

void BotNav_MoveTowardGoal(bot_state_t *bs)
{
    /* TODO: synthesise usercmd movement inputs toward next path node */
    (void)bs;
}

int BotNav_NearestNode(vec3_t origin, qboolean allow_wall_nodes)
{
    /* TODO: spatial search in node graph */
    (void)origin;
    (void)allow_wall_nodes;
    return BOT_INVALID_NODE;
}

/*
 * BotNav_IsChokePoint
 * Returns true if the given node is tagged as a map choke point.
 * Used by human builder bots to place turrets optimally.
 */
qboolean BotNav_IsChokePoint(int node_index)
{
    /* TODO: check choke flag in loaded nav data */
    (void)node_index;
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
