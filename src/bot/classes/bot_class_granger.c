/*
 * bot_class_granger.c -- per-class AI overrides for granger
 *
 * Override specific bot_state_t defaults or provide class-unique
 * behaviours (special abilities, preferred weapons, combat tactics).
 * Called from bot_main.c during Bot_Connect() class initialisation.
 */

#include "../bot.h"

/* Canonical implementation in bot_class_breeder.c */
extern void BotClass_Init_breeder(bot_state_t *bs);

void BotClass_Init_granger(bot_state_t *bs)
{
    BotClass_Init_breeder(bs);
}
