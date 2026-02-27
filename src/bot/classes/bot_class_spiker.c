/*
 * bot_class_spiker.c -- per-class AI overrides for spiker
 *
 * Override specific bot_state_t defaults or provide class-unique
 * behaviours (special abilities, preferred weapons, combat tactics).
 * Called from bot_main.c during Bot_Connect() class initialisation.
 */

#include "../bot.h"

/* Canonical implementation in bot_class_stinger.c */
extern void BotClass_Init_stinger(bot_state_t *bs);

void BotClass_Init_spiker(bot_state_t *bs)
{
    BotClass_Init_stinger(bs);
}
