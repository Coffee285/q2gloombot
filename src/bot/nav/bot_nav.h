/*
 * bot_nav.h -- navigation and pathfinding public API
 *
 * ALIEN NAV (3-D / wall-walk aware)
 * -----------------------------------
 * Most alien classes can walk on walls and ceilings.  The navmesh must
 * therefore be three-dimensional: nodes exist on floors, walls, AND
 * ceilings, and edges between them account for gravity changes.
 *
 * The bot sets nav.wall_walking = true when traversing a non-floor
 * surface, and nav.wall_normal is updated to the surface normal.
 * Alien bots use wall-routes to bypass turret fire and ambush humans
 * from unexpected angles.
 *
 * HUMAN NAV (floor-based with choke-point awareness)
 * ----------------------------------------------------
 * Human builder bots use heuristic mapping to identify narrow corridors
 * and doorway thresholds for optimal turret / Tesla placement.  The nav
 * system exposes BotNav_IsChokePoint() for this purpose.
 *
 * WAYPOINT FILE FORMAT
 * --------------------
 * Navigation data is loaded from maps/<mapname>.nav at map load.
 * (Format TBD — binary node/edge graph with surface-normal annotations.)
 */

#ifndef BOT_NAV_H
#define BOT_NAV_H

#include "bot.h"

void     BotNav_Init(void);
void     BotNav_LoadMap(const char *mapname);
void     BotNav_FindPath(bot_state_t *bs, vec3_t goal);
void     BotNav_MoveTowardGoal(bot_state_t *bs);
int      BotNav_NearestNode(vec3_t origin, qboolean allow_wall_nodes);
qboolean BotNav_IsChokePoint(int node_index);
void     BotNav_UpdateWallWalk(bot_state_t *bs);

#endif /* BOT_NAV_H */
