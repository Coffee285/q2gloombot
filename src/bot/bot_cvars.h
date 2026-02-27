/*
 * bot_cvars.h -- cvar declarations for q2gloombot
 *
 * All bot configuration is exposed as Quake 2 cvars so operators can
 * change behaviour at runtime via the console.
 */

#ifndef BOT_CVARS_H
#define BOT_CVARS_H

#include "g_local.h"

/* -----------------------------------------------------------------------
   Cvar pointers (defined in bot_cvars.c)
   ----------------------------------------------------------------------- */

/* General */
extern cvar_t *bot_enable;
extern cvar_t *bot_skill;
extern cvar_t *bot_skill_min;
extern cvar_t *bot_skill_max;
extern cvar_t *bot_skill_random;
extern cvar_t *bot_count;
extern cvar_t *bot_count_alien;
extern cvar_t *bot_count_human;
extern cvar_t *bot_think_interval;
extern cvar_t *bot_chat_enable;

/* Difficulty scaling */
extern cvar_t *bot_aim_skill_scale;
extern cvar_t *bot_reaction_scale;
extern cvar_t *bot_awareness_range;
extern cvar_t *bot_hearing_range;
extern cvar_t *bot_fov;
extern cvar_t *bot_strafe_skill;

/* Behaviour */
extern cvar_t *bot_aggression;
extern cvar_t *bot_teamwork;
extern cvar_t *bot_build_priority;
extern cvar_t *bot_upgrade_enabled;
extern cvar_t *bot_upgrade_delay;
extern cvar_t *bot_taunt;

/* Navigation */
extern cvar_t *bot_nav_autogen;
extern cvar_t *bot_nav_show;
extern cvar_t *bot_nav_density;

/* Debug */
extern cvar_t *bot_debug_cvar;
extern cvar_t *bot_debug_target;
extern cvar_t *bot_perf;

/* -----------------------------------------------------------------------
   Public API
   ----------------------------------------------------------------------- */

/* Register all bot cvars with the engine. Call once from Bot_Init(). */
void BotCvars_Init(void);

#endif /* BOT_CVARS_H */
