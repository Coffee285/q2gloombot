/*
 * bot_class_marine_light.c -- per-class AI overrides for marine_light
 *
 * Override specific bot_state_t defaults or provide class-unique
 * behaviours (special abilities, preferred weapons, combat tactics).
 * Called from bot_main.c during Bot_Connect() class initialisation.
 */

#include "../bot.h"

/* Canonical implementation in bot_class_grunt.c */
extern void BotClass_Init_grunt(bot_state_t *bs);

void BotClass_Init_marine_light(bot_state_t *bs)
{
    BotClass_Init_grunt(bs);
}
