/*
 * g_main.c -- Quake 2 game DLL entry point for q2gloombot
 *
 * Implements GetGameAPI(), the mandatory DLL export that the Quake 2
 * server calls on load.  Also provides the game export function table
 * (game_export_t) wired to minimal stub implementations so the DLL
 * satisfies the server contract while the bot subsystem does its work.
 */

#include "g_local.h"
#include "bot.h"

/* -----------------------------------------------------------------------
   Globals shared between game modules
   ----------------------------------------------------------------------- */
game_import_t gi;          /* engine functions imported at startup           */
game_export_t globals;     /* function table exported to the engine          */

edict_t  *g_edicts  = NULL;
gclient_t *g_clients = NULL;

level_locals_t level;

cvar_t *maxentities = NULL;
cvar_t *deathmatch  = NULL;
cvar_t *maxclients  = NULL;
cvar_t *sv_gravity  = NULL;

/* -----------------------------------------------------------------------
   Edict management helpers
   ----------------------------------------------------------------------- */
void G_InitEdict(edict_t *e)
{
    e->inuse     = true;
    e->classname = "noclass";
    e->gravity   = 1.0f;
    e->s.number  = (int)(e - g_edicts);
}

edict_t *G_Spawn(void)
{
    int     i;
    edict_t *e;

    /* Try to reuse an existing free edict beyond the clients */
    e = &g_edicts[maxclients ? (int)maxclients->value + 1 : 1];
    for (i = (maxclients ? (int)maxclients->value + 1 : 1);
         i < globals.num_edicts; i++, e++) {
        if (!e->inuse) {
            G_InitEdict(e);
            return e;
        }
    }

    if (globals.num_edicts == globals.max_edicts)
        gi.error("G_Spawn: no free edicts");

    e = &g_edicts[globals.num_edicts];
    globals.num_edicts++;
    G_InitEdict(e);
    return e;
}

void G_FreeEdict(edict_t *e)
{
    gi.unlinkentity(e);
    memset(e, 0, globals.edict_size);
    e->classname = "freed";
    e->freetime  = level.time;
    e->inuse     = false;
}

/* -----------------------------------------------------------------------
   Game export implementations
   ----------------------------------------------------------------------- */

/*
 * G_Init — called once when a game session begins (not each map load).
 */
static void G_Init(void)
{
    gi.dprintf("q2gloombot Game DLL Init\n");

    /* Cache commonly used cvars */
    maxclients  = gi.cvar("maxclients",  "8",   CVAR_SERVERINFO | CVAR_LATCH);
    maxentities = gi.cvar("maxentities", "1024", CVAR_LATCH);
    deathmatch  = gi.cvar("deathmatch",  "0",   CVAR_SERVERINFO | CVAR_LATCH);
    sv_gravity  = gi.cvar("sv_gravity",  "800", CVAR_SERVERINFO);

    /* Allocate edict array */
    globals.edict_size = sizeof(edict_t);
    globals.max_edicts = (int)maxentities->value;
    globals.num_edicts = (int)maxclients->value + 1;
    globals.edicts     = gi.TagMalloc(globals.max_edicts * globals.edict_size, TAG_GAME);
    g_edicts           = globals.edicts;
    memset(g_edicts, 0, globals.max_edicts * globals.edict_size);

    /* Allocate client array */
    g_clients = gi.TagMalloc((int)maxclients->value * sizeof(gclient_t), TAG_GAME);
    memset(g_clients, 0, (int)maxclients->value * sizeof(gclient_t));

    /* Initialise bot subsystem */
    Bot_Init();
}

/*
 * G_Shutdown — called when the game DLL is unloaded.
 */
static void G_Shutdown(void)
{
    gi.dprintf("q2gloombot Game DLL Shutdown\n");
    Bot_Shutdown();
    gi.FreeTags(TAG_LEVEL);
    gi.FreeTags(TAG_GAME);
}

static void G_SpawnEntities(char *mapname, char *entstring, char *spawnpoint)
{
    int i;

    /* Reset level state */
    memset(&level, 0, sizeof(level));
    level.time = 0.0f;

    gi.dprintf("G_SpawnEntities: map '%s'\n", mapname);

    /* Free level-scoped memory and re-init client edicts */
    gi.FreeTags(TAG_LEVEL);

    for (i = 0; i < (int)maxclients->value; i++) {
        g_edicts[i + 1].client = &g_clients[i];
        G_InitEdict(&g_edicts[i + 1]);
    }
    globals.num_edicts = (int)maxclients->value + 1;
}

static void G_WriteGame(char *filename, qboolean autosave)
{
    (void)filename; (void)autosave;
}

static void G_ReadGame(char *filename)
{
    (void)filename;
}

static void G_WriteLevel(char *filename)
{
    (void)filename;
}

static void G_ReadLevel(char *filename)
{
    (void)filename;
}

static qboolean G_ClientConnect(edict_t *ent, char *userinfo)
{
    (void)ent; (void)userinfo;
    return true;
}

static void G_ClientBegin(edict_t *ent)
{
    (void)ent;
}

static void G_ClientUserinfoChanged(edict_t *ent, char *userinfo)
{
    (void)ent; (void)userinfo;
}

static void G_ClientDisconnect(edict_t *ent)
{
    bot_state_t *bs = Bot_GetState(ent);
    if (bs)
        Bot_Disconnect(ent);
}

static void G_ClientCommand(edict_t *ent)
{
    (void)ent;
}

static void G_ClientThink(edict_t *ent, usercmd_t *cmd)
{
    (void)ent; (void)cmd;
}

/*
 * G_RunFrame — called every server frame.
 * Advances the level timer and updates all bots.
 */
static void G_RunFrame(void)
{
    level.framenum++;
    level.time = level.framenum * FRAMETIME;

    Bot_Frame();
}

/*
 * G_ServerCommand — handles "sv <cmd>" console commands.
 */
static void G_ServerCommand(void)
{
    char *cmd = gi.argv(0);

    if (Q_stricmp(cmd, "addbot") == 0) {
        /* Handled via gi.AddCommandString routed through SV_AddBot_f
         * in bot_main.c.  Nothing to do here except acknowledge. */
        gi.dprintf("Use: sv addbot [team] [class] [skill]\n");
    } else if (Q_stricmp(cmd, "removebot") == 0) {
        gi.dprintf("Use: sv removebot <name>\n");
    } else if (Q_stricmp(cmd, "listbots") == 0) {
        /* List bots directly */
        int i, count = 0;
        gi.dprintf("Active bots (%d / %d):\n", num_bots, MAX_BOTS);
        for (i = 0; i < MAX_BOTS; i++) {
            if (g_bots[i].in_use) {
                gi.dprintf("  [%2d] %-20s team=%d class=%-10s skill=%.2f\n",
                           i, g_bots[i].name, g_bots[i].team,
                           Gloom_ClassName(g_bots[i].gloom_class),
                           g_bots[i].skill);
                count++;
            }
        }
        if (count == 0)
            gi.dprintf("  (none)\n");
    } else {
        gi.dprintf("Unknown server command '%s'\n", cmd);
    }
}

/* -----------------------------------------------------------------------
   GetGameAPI — the single exported entry point
   The Quake 2 server calls this after loading the DLL.
   ----------------------------------------------------------------------- */
#ifdef _WIN32
__declspec(dllexport)
#endif
game_export_t *GetGameAPI(game_import_t *import)
{
    gi = *import;

    globals.apiversion = GAME_API_VERSION;

    globals.Init                  = G_Init;
    globals.Shutdown              = G_Shutdown;
    globals.SpawnEntities         = G_SpawnEntities;
    globals.WriteGame             = G_WriteGame;
    globals.ReadGame              = G_ReadGame;
    globals.WriteLevel            = G_WriteLevel;
    globals.ReadLevel             = G_ReadLevel;
    globals.ClientConnect         = G_ClientConnect;
    globals.ClientBegin           = G_ClientBegin;
    globals.ClientUserinfoChanged = G_ClientUserinfoChanged;
    globals.ClientDisconnect      = G_ClientDisconnect;
    globals.ClientCommand         = G_ClientCommand;
    globals.ClientThink           = G_ClientThink;
    globals.RunFrame              = G_RunFrame;
    globals.ServerCommand         = G_ServerCommand;

    /* edicts are allocated in G_Init once we know maxclients */
    globals.edicts      = NULL;
    globals.edict_size  = sizeof(edict_t);
    globals.num_edicts  = 0;
    globals.max_edicts  = MAX_EDICTS;

    return &globals;
}
