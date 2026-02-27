/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

/* game.h -- game DLL information visible to the server */

#ifndef GAME_H
#define GAME_H

#define GAME_API_VERSION 3

/* edict->svflags */
#define SVF_NOCLIENT    0x00000001
#define SVF_DEADMONSTER 0x00000002
#define SVF_MONSTER     0x00000004

typedef enum {
    SOLID_NOT,
    SOLID_TRIGGER,
    SOLID_BBOX,
    SOLID_BSP
} solid_t;

typedef struct link_s {
    struct link_s *prev, *next;
} link_t;

#define MAX_ENT_CLUSTERS 16

typedef struct edict_s  edict_t;
typedef struct gclient_s gclient_t;

#ifndef GAME_INCLUDE

struct gclient_s {
    player_state_t ps;
    int            ping;
};

struct edict_s {
    entity_state_t   s;
    struct gclient_s *client;
    qboolean         inuse;
    int              linkcount;

    link_t area;
    int    num_clusters;
    int    clusternums[MAX_ENT_CLUSTERS];
    int    headnode;
    int    areanum, areanum2;

    int    svflags;
    vec3_t mins, maxs;
    vec3_t absmin, absmax, size;
    solid_t solid;
    int    clipmask;
    edict_t *owner;
};

#endif /* GAME_INCLUDE */

/* ----------------------------------------------------------------
   Functions provided by the main engine
   ---------------------------------------------------------------- */
typedef struct {
    void (*bprintf)(int printlevel, char *fmt, ...);
    void (*dprintf)(char *fmt, ...);
    void (*cprintf)(edict_t *ent, int printlevel, char *fmt, ...);
    void (*centerprintf)(edict_t *ent, char *fmt, ...);
    void (*sound)(edict_t *ent, int channel, int soundindex, float volume,
                  float attenuation, float timeofs);
    void (*positioned_sound)(vec3_t origin, edict_t *ent, int channel,
                             int soundindex, float volume, float attenuation,
                             float timeofs);
    void (*configstring)(int num, char *string);
    void (*error)(char *fmt, ...);

    int  (*modelindex)(char *name);
    int  (*soundindex)(char *name);
    int  (*imageindex)(char *name);
    void (*setmodel)(edict_t *ent, char *name);

    trace_t  (*trace)(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end,
                      edict_t *passent, int contentmask);
    int      (*pointcontents)(vec3_t point);
    qboolean (*inPVS)(vec3_t p1, vec3_t p2);
    qboolean (*inPHS)(vec3_t p1, vec3_t p2);
    void     (*SetAreaPortalState)(int portalnum, qboolean open);
    qboolean (*AreasConnected)(int area1, int area2);

    void (*linkentity)(edict_t *ent);
    void (*unlinkentity)(edict_t *ent);
    int  (*BoxEdicts)(vec3_t mins, vec3_t maxs, edict_t **list, int maxcount,
                      int areatype);
    void (*Pmove)(pmove_t *pmove);

    void (*multicast)(vec3_t origin, multicast_t to);
    void (*unicast)(edict_t *ent, qboolean reliable);
    void (*WriteChar)(int c);
    void (*WriteByte)(int c);
    void (*WriteShort)(int c);
    void (*WriteLong)(int c);
    void (*WriteFloat)(float f);
    void (*WriteString)(char *s);
    void (*WritePosition)(vec3_t pos);
    void (*WriteDir)(vec3_t pos);
    void (*WriteAngle)(float f);

    void  *(*TagMalloc)(int size, int tag);
    void   (*TagFree)(void *block);
    void   (*FreeTags)(int tag);

    cvar_t *(*cvar)(char *var_name, char *value, int flags);
    cvar_t *(*cvar_set)(char *var_name, char *value);
    cvar_t *(*cvar_forceset)(char *var_name, char *value);

    int    (*argc)(void);
    char  *(*argv)(int n);
    char  *(*args)(void);

    void   (*AddCommandString)(char *text);
    void   (*DebugGraph)(float value, int color);
} game_import_t;

/* ----------------------------------------------------------------
   Functions exported by the game subsystem
   ---------------------------------------------------------------- */
typedef struct {
    int apiversion;

    void (*Init)(void);
    void (*Shutdown)(void);

    void (*SpawnEntities)(char *mapname, char *entstring, char *spawnpoint);

    void (*WriteGame)(char *filename, qboolean autosave);
    void (*ReadGame)(char *filename);
    void (*WriteLevel)(char *filename);
    void (*ReadLevel)(char *filename);

    qboolean (*ClientConnect)(edict_t *ent, char *userinfo);
    void     (*ClientBegin)(edict_t *ent);
    void     (*ClientUserinfoChanged)(edict_t *ent, char *userinfo);
    void     (*ClientDisconnect)(edict_t *ent);
    void     (*ClientCommand)(edict_t *ent);
    void     (*ClientThink)(edict_t *ent, usercmd_t *cmd);

    void (*RunFrame)(void);
    void (*ServerCommand)(void);

    struct edict_s *edicts;
    int             edict_size;
    int             num_edicts;
    int             max_edicts;
} game_export_t;

game_export_t *GetGameAPI(game_import_t *import);

#endif /* GAME_H */
