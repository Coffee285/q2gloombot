/*
 * bot_nav.h -- navigation / pathfinding public API
 */

#ifndef BOT_NAV_H
#define BOT_NAV_H

#include "bot.h"

void BotNav_Init(void);
void BotNav_FindPath(bot_state_t *bs, vec3_t goal);
void BotNav_MoveTowardGoal(bot_state_t *bs);
int  BotNav_NearestNode(vec3_t origin);

#endif /* BOT_NAV_H */
