/*
 * bot_team.h -- team coordination and strategy public API
 */

#ifndef BOT_TEAM_H
#define BOT_TEAM_H

#include "bot.h"

void     BotTeam_Init(void);
void     BotTeam_Frame(void);
void     BotTeam_AssignRoles(int team);
edict_t *BotTeam_FindBuilder(int team);
qboolean BotTeam_ReactorAlive(void);
qboolean BotTeam_OvmindAlive(void);
int      BotTeam_CountSpawnPoints(int team);

#endif /* BOT_TEAM_H */
