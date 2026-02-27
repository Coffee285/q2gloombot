/*
 * bot_cvars.c -- cvar registration for q2gloombot
 *
 * Registers all bot-related cvars with the Quake 2 engine so server
 * operators can tune behaviour at runtime via the console.
 */

#include "bot_cvars.h"

/* -----------------------------------------------------------------------
   Cvar storage
   ----------------------------------------------------------------------- */

/* General */
cvar_t *bot_enable         = NULL;
cvar_t *bot_skill          = NULL;
cvar_t *bot_skill_min      = NULL;
cvar_t *bot_skill_max      = NULL;
cvar_t *bot_skill_random   = NULL;
cvar_t *bot_count          = NULL;
cvar_t *bot_count_alien    = NULL;
cvar_t *bot_count_human    = NULL;
cvar_t *bot_think_interval = NULL;
cvar_t *bot_chat_enable    = NULL;

/* Difficulty scaling */
cvar_t *bot_aim_skill_scale = NULL;
cvar_t *bot_reaction_scale  = NULL;
cvar_t *bot_awareness_range = NULL;
cvar_t *bot_hearing_range   = NULL;
cvar_t *bot_fov             = NULL;
cvar_t *bot_strafe_skill    = NULL;

/* Behaviour */
cvar_t *bot_aggression      = NULL;
cvar_t *bot_teamwork        = NULL;
cvar_t *bot_build_priority  = NULL;
cvar_t *bot_upgrade_enabled = NULL;
cvar_t *bot_upgrade_delay   = NULL;
cvar_t *bot_taunt           = NULL;

/* Navigation */
cvar_t *bot_nav_autogen = NULL;
cvar_t *bot_nav_show    = NULL;
cvar_t *bot_nav_density = NULL;

/* Debug */
cvar_t *bot_debug_cvar   = NULL;
cvar_t *bot_debug_target = NULL;
cvar_t *bot_perf         = NULL;

/* -----------------------------------------------------------------------
   BotCvars_Init
   ----------------------------------------------------------------------- */
void BotCvars_Init(void)
{
    /* General */
    bot_enable         = gi.cvar("bot_enable",         "1",   CVAR_ARCHIVE);
    bot_skill          = gi.cvar("bot_skill",          "0.5", CVAR_ARCHIVE);
    bot_skill_min      = gi.cvar("bot_skill_min",      "0.2", CVAR_ARCHIVE);
    bot_skill_max      = gi.cvar("bot_skill_max",      "0.8", CVAR_ARCHIVE);
    bot_skill_random   = gi.cvar("bot_skill_random",   "1",   CVAR_ARCHIVE);
    bot_count          = gi.cvar("bot_count",          "0",   CVAR_ARCHIVE);
    bot_count_alien    = gi.cvar("bot_count_alien",    "0",   CVAR_ARCHIVE);
    bot_count_human    = gi.cvar("bot_count_human",    "0",   CVAR_ARCHIVE);
    bot_think_interval = gi.cvar("bot_think_interval", "100", CVAR_ARCHIVE);
    bot_chat_enable    = gi.cvar("bot_chat",           "1",   CVAR_ARCHIVE);

    /* Difficulty scaling */
    bot_aim_skill_scale = gi.cvar("bot_aim_skill_scale", "1.0",  CVAR_ARCHIVE);
    bot_reaction_scale  = gi.cvar("bot_reaction_scale",  "1.0",  CVAR_ARCHIVE);
    bot_awareness_range = gi.cvar("bot_awareness_range", "1000", CVAR_ARCHIVE);
    bot_hearing_range   = gi.cvar("bot_hearing_range",   "800",  CVAR_ARCHIVE);
    bot_fov             = gi.cvar("bot_fov",             "120",  CVAR_ARCHIVE);
    bot_strafe_skill    = gi.cvar("bot_strafe_skill",    "0.5",  CVAR_ARCHIVE);

    /* Behaviour */
    bot_aggression      = gi.cvar("bot_aggression",      "0.5", CVAR_ARCHIVE);
    bot_teamwork        = gi.cvar("bot_teamwork",        "0.7", CVAR_ARCHIVE);
    bot_build_priority  = gi.cvar("bot_build_priority",  "0.8", CVAR_ARCHIVE);
    bot_upgrade_enabled = gi.cvar("bot_upgrade_enabled", "1",   CVAR_ARCHIVE);
    bot_upgrade_delay   = gi.cvar("bot_upgrade_delay",   "5",   CVAR_ARCHIVE);
    bot_taunt           = gi.cvar("bot_taunt",           "1",   CVAR_ARCHIVE);

    /* Navigation */
    bot_nav_autogen = gi.cvar("bot_nav_autogen", "1",   CVAR_ARCHIVE);
    bot_nav_show    = gi.cvar("bot_nav_show",    "0",   0);
    bot_nav_density = gi.cvar("bot_nav_density", "128", CVAR_ARCHIVE);

    /* Debug */
    bot_debug_cvar   = gi.cvar("bot_debug",        "0", 0);
    bot_debug_target = gi.cvar("bot_debug_target", "",  0);
    bot_perf         = gi.cvar("bot_perf",         "0", 0);

    gi.dprintf("BotCvars_Init: %d cvars registered\n", 27);
}
