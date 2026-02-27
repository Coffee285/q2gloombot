/*
 * bot_class_builder_adv.c -- per-class AI overrides for builder_adv
 *
 * Override specific bot_state_t defaults or provide class-unique
 * behaviours (special abilities, preferred weapons, combat tactics).
 * Called from bot_main.c during Bot_Connect() class initialisation.
 */

#include "../bot.h"

/* Canonical implementation in bot_class_engineer.c */
extern void BotClass_Init_engineer(bot_state_t *bs);

void BotClass_Init_builder_adv(bot_state_t *bs)
{
    BotClass_Init_engineer(bs);
}
