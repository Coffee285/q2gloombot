/*
 * bot_class_marauder.c -- per-class AI overrides for marauder
 *
 * Override specific bot_state_t defaults or provide class-unique
 * behaviours (special abilities, preferred weapons, combat tactics).
 * Called from bot_main.c during Bot_Connect() class initialisation.
 */

#include "../bot.h"

/* Canonical implementation in bot_class_drone.c */
extern void BotClass_Init_drone(bot_state_t *bs);

void BotClass_Init_marauder(bot_state_t *bs)
{
    BotClass_Init_drone(bs);
}
