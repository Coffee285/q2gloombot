/*
 * bot_class_kamikaze.c -- per-class AI overrides for kamikaze
 *
 * Override specific bot_state_t defaults or provide class-unique
 * behaviours (special abilities, preferred weapons, combat tactics).
 * Called from bot_main.c during Bot_Connect() class initialisation.
 */

#include "../bot.h"

void BotClass_Init_kamikaze(bot_state_t *bs)
{
    (void)bs; /* TODO: class-specific init */
}
