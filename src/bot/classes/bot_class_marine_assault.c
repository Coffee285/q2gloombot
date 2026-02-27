/*
 * bot_class_marine_assault.c -- per-class AI overrides for marine_assault
 *
 * Override specific bot_state_t defaults or provide class-unique
 * behaviours (special abilities, preferred weapons, combat tactics).
 * Called from bot_main.c during Bot_Connect() class initialisation.
 */

#include "../bot.h"

/* Canonical implementation in bot_class_st.c */
extern void BotClass_Init_st(bot_state_t *bs);

void BotClass_Init_marine_assault(bot_state_t *bs)
{
    BotClass_Init_st(bs);
}
