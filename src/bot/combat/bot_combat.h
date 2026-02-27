/*
 * bot_combat.h -- combat AI public API
 *
 * Target selection follows Gloom's asymmetric priorities:
 *
 * ALIEN BOTS (in order):
 *   1. Reactor              — destroying it shuts down all human defenses
 *   2. Human Builders       — prevent new defenses being built
 *   3. Automated turrets    — clear the way for the swarm
 *   4. Combat marines       — general threats
 *
 * HUMAN BOTS (in order):
 *   1. Overmind             — destroying it stops alien building & evolution
 *   2. Alien Grangers       — prevent new alien structures
 *   3. Alien structures     — Acid Tubes, Traps, Barricades
 *   4. Alien players        — general threats
 *
 * Human bots prefer to engage at maximum effective range.
 * Alien bots prefer to approach using walls / ceilings to bypass turrets.
 */

#ifndef BOT_COMBAT_H
#define BOT_COMBAT_H

#include "bot.h"

void     BotCombat_Think(bot_state_t *bs);
edict_t *BotCombat_PickTarget(bot_state_t *bs);
void     BotCombat_AimAtTarget(bot_state_t *bs);
void     BotCombat_Fire(bot_state_t *bs);
float    BotCombat_AimError(float skill);

#endif /* BOT_COMBAT_H */
