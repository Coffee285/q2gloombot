/*
 * bot_class_marine_laser.c -- per-class AI overrides for marine_laser
 *
 * Override specific bot_state_t defaults or provide class-unique
 * behaviours (special abilities, preferred weapons, combat tactics).
 * Called from bot_main.c during Bot_Connect() class initialisation.
 */

#include "../bot.h"

/* Canonical implementation in bot_class_mech.c */
extern void BotClass_Init_mech(bot_state_t *bs);

void BotClass_Init_marine_laser(bot_state_t *bs)
{
    BotClass_Init_mech(bs);
}
