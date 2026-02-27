/*
 * bot_config.c -- configuration file loading for q2gloombot
 *
 * Loads gloombot.cfg (if present) on initialization and provides
 * bot name roster management for both teams.
 */

#include "bot.h"
#include "bot_cvars.h"
#include "bot_safety.h"

/* -----------------------------------------------------------------------
   Bot name rosters
   ----------------------------------------------------------------------- */
#define BOT_MAX_NAMES 32

static char bot_names_alien[BOT_MAX_NAMES][32];
static int  bot_names_alien_count = 0;
static int  bot_names_alien_next  = 0;

static char bot_names_human[BOT_MAX_NAMES][32];
static int  bot_names_human_count = 0;
static int  bot_names_human_next  = 0;

/* -----------------------------------------------------------------------
   Default names (used when config files are not found)
   ----------------------------------------------------------------------- */
static const char *default_alien_names[] = {
    "Xenomorph", "FaceHugger", "ChestBurster", "AcidBlood",
    "Lurker", "Crawler", "Spitter", "Ravager",
    "Hivemind", "Carapace", "Tendril", "Slasher",
    "Parasite", "Swarmling", "Mantis", "Venom",
    "Broodmother", "Impaler", "Skitter", "Chitin",
    "Mandible", "Toxin", "Stalker_X", "Feeder",
    "Gnasher", "Borer", "Molt", "Frenzy",
    "Scythe", "Devour", "Morphling", "Predator_X"
};

static const char *default_human_names[] = {
    "Sgt.Rock", "Pvt.Hudson", "Cpl.Hicks", "Lt.Ripley",
    "Cpt.Dallas", "Sgt.Apone", "Pvt.Drake", "Cpl.Ferro",
    "Lt.Gorman", "Sgt.Johnson", "Pvt.Vasquez", "Cpt.Miller",
    "Sgt.Barnes", "Pvt.Taylor", "Cpl.Dunn", "Lt.Price",
    "Cpt.MacTavish", "Sgt.Foley", "Pvt.Allen", "Cpl.Jackson",
    "Lt.Turner", "Sgt.Davis", "Pvt.Jones", "Cpl.Walker",
    "Cpt.Mitchell", "Sgt.Adams", "Pvt.Wilson", "Lt.Baker",
    "Sgt.Clark", "Pvt.Lewis", "Cpl.Harris", "Cpt.Young"
};

/* -----------------------------------------------------------------------
   BotConfig_Init -- load config files at startup
   ----------------------------------------------------------------------- */
void BotConfig_Init(void)
{
    int i;

    gi.dprintf("BotConfig_Init: loading configuration\n");

    /* Exec the main config file if it exists */
    gi.AddCommandString("exec gloombot.cfg\n");

    /* Load default name rosters (file loading is engine-dependent;
     * for now use the compiled-in defaults) */
    bot_names_alien_count = BOT_MAX_NAMES;
    for (i = 0; i < BOT_MAX_NAMES; i++)
        Q_strncpyz(bot_names_alien[i], default_alien_names[i],
                    sizeof(bot_names_alien[i]));

    bot_names_human_count = BOT_MAX_NAMES;
    for (i = 0; i < BOT_MAX_NAMES; i++)
        Q_strncpyz(bot_names_human[i], default_human_names[i],
                    sizeof(bot_names_human[i]));

    bot_names_alien_next = 0;
    bot_names_human_next = 0;

    gi.dprintf("BotConfig_Init: %d alien names, %d human names loaded\n",
               bot_names_alien_count, bot_names_human_count);
}

/* -----------------------------------------------------------------------
   BotConfig_NextName -- get the next name for a team
   Returns a pointer to a static name buffer; caller should copy.
   ----------------------------------------------------------------------- */
const char *BotConfig_NextName(int team)
{
    const char *name;

    if (team == TEAM_ALIEN) {
        if (bot_names_alien_count <= 0) return "AlienBot";
        name = bot_names_alien[bot_names_alien_next];
        bot_names_alien_next = (bot_names_alien_next + 1) % bot_names_alien_count;
        return name;
    } else {
        if (bot_names_human_count <= 0) return "HumanBot";
        name = bot_names_human[bot_names_human_next];
        bot_names_human_next = (bot_names_human_next + 1) % bot_names_human_count;
        return name;
    }
}
