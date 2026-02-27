/*
 * bot_class_tyrant.c -- per-class AI overrides for tyrant
 *
 * Override specific bot_state_t defaults or provide class-unique
 * behaviours (special abilities, preferred weapons, combat tactics).
 * Called from bot_main.c during Bot_Connect() class initialisation.
 */

#include "../bot.h"

/* Canonical implementation in bot_class_stalker.c */
extern void BotClass_Init_stalker(bot_state_t *bs);

void BotClass_Init_tyrant(bot_state_t *bs)
{
    BotClass_Init_stalker(bs);
}
