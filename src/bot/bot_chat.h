/*
 * bot_chat.h -- bot chat and taunt system for q2gloombot
 *
 * Bots occasionally emit chat messages in response to game events.
 * All functions are safe to call on any bot_state_t; they apply
 * probability gates internally so callers need no extra checks.
 */

#ifndef BOT_CHAT_H
#define BOT_CHAT_H

#include "bot.h"

/* Called when this bot scores a kill. 30% chance of a taunt. */
void Bot_Chat_OnKill(bot_state_t *bs);

/* Called when this bot dies. 20% chance of a frustration message. */
void Bot_Chat_OnDeath(bot_state_t *bs);

/* Called when this bot's team wins the round. 50% chance of celebration. */
void Bot_Chat_OnTeamWin(bot_state_t *bs);

/* Called when this bot spawns with a specific class. Always announces class. */
void Bot_Chat_OnSpawn(bot_state_t *bs);

#endif /* BOT_CHAT_H */
