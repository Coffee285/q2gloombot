/*
 * bot_nav.c -- navigation and pathfinding stubs
 *
 * Full A* / waypoint-graph pathfinding implementation goes here.
 * Waypoint data is loaded from maps/<mapname>.nav files at map load.
 */

#include "bot_nav.h"

void BotNav_Init(void)
{
    gi.dprintf("BotNav_Init: navigation subsystem ready (stub)\n");
}

void BotNav_FindPath(bot_state_t *bs, vec3_t goal)
{
    /* Stub: copy goal directly; full implementation uses A* on nav graph */
    VectorCopy(goal, bs->nav.goal_origin);
    bs->nav.goal_node  = BOT_INVALID_NODE;
    bs->nav.path_valid = false;
}

void BotNav_MoveTowardGoal(bot_state_t *bs)
{
    /* Stub: movement inputs will be applied here via usercmd injection */
    (void)bs;
}

int BotNav_NearestNode(vec3_t origin)
{
    (void)origin;
    return BOT_INVALID_NODE;
}
