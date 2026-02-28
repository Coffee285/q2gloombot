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

/* q_shared.h -- included first by ALL program modules */

#ifndef Q_SHARED_H
#define Q_SHARED_H

#ifdef _WIN32
#pragma warning(disable : 4244)
#pragma warning(disable : 4136)
#pragma warning(disable : 4051)
#pragma warning(disable : 4018)
#pragma warning(disable : 4305)
#endif

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

typedef unsigned char  byte;

/* qboolean — portable boolean type.
 * MinGW-w64 UCRT headers (pulled in by <string.h> etc.) define false and true
 * as macros that expand to the integer constants 0 and 1.  Attempting to use
 * those tokens as enum member names then produces a compile error:
 *   "expected identifier before numeric constant"
 * Guard against this by falling back to a plain int typedef when the macros
 * are already present (stdbool.h compatible compilers / environments). */
#ifdef false
typedef int qboolean;
#else
typedef enum { false, true } qboolean;
#endif

#ifndef NULL
#define NULL ((void *)0)
#endif

/* angle indexes */
#define PITCH   0
#define YAW     1
#define ROLL    2

#define MAX_STRING_CHARS    1024
#define MAX_STRING_TOKENS   80
#define MAX_TOKEN_CHARS     128
#define MAX_QPATH           64
#define MAX_OSPATH          128

/* per-level limits */
#define MAX_CLIENTS     256
#define MAX_EDICTS      1024
#define MAX_LIGHTSTYLES 256
#define MAX_MODELS      256
#define MAX_SOUNDS      256
#define MAX_IMAGES      256
#define MAX_ITEMS       256
#define MAX_GENERAL     (MAX_CLIENTS * 2)

/* game print flags */
#define PRINT_LOW    0
#define PRINT_MEDIUM 1
#define PRINT_HIGH   2
#define PRINT_CHAT   3

#define ERR_FATAL      0
#define ERR_DROP       1
#define ERR_DISCONNECT 2

#define PRINT_ALL       0
#define PRINT_DEVELOPER 1
#define PRINT_ALERT     2

typedef enum {
    MULTICAST_ALL,
    MULTICAST_PHS,
    MULTICAST_PVS,
    MULTICAST_ALL_R,
    MULTICAST_PHS_R,
    MULTICAST_PVS_R
} multicast_t;

/* ============================================================
   MATHLIB
   ============================================================ */

typedef float  vec_t;
typedef vec_t  vec3_t[3];
typedef vec_t  vec5_t[5];

typedef int fixed4_t;
typedef int fixed8_t;
typedef int fixed16_t;

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

struct cplane_s;

extern vec3_t vec3_origin;

#define nanmask (255 << 23)
#define IS_NAN(x) (((*(int *)&x) & nanmask) == nanmask)

#define Q_ftol(f) ((long)(f))

#define DotProduct(x, y)        (x[0]*y[0]+x[1]*y[1]+x[2]*y[2])
#define VectorSubtract(a, b, c) (c[0]=a[0]-b[0],c[1]=a[1]-b[1],c[2]=a[2]-b[2])
#define VectorAdd(a, b, c)      (c[0]=a[0]+b[0],c[1]=a[1]+b[1],c[2]=a[2]+b[2])
#define VectorCopy(a, b)        (b[0]=a[0],b[1]=a[1],b[2]=a[2])
#define VectorClear(a)          (a[0]=a[1]=a[2]=0)
#define VectorNegate(a, b)      (b[0]=-a[0],b[1]=-a[1],b[2]=-a[2])
#define VectorSet(v, x, y, z)   (v[0]=(x),v[1]=(y),v[2]=(z))

void VectorMA(vec3_t veca, float scale, vec3_t vecb, vec3_t vecc);
vec_t  _DotProduct(vec3_t v1, vec3_t v2);
void   _VectorSubtract(vec3_t veca, vec3_t vecb, vec3_t out);
void   _VectorAdd(vec3_t veca, vec3_t vecb, vec3_t out);
void   _VectorCopy(vec3_t in, vec3_t out);

void   ClearBounds(vec3_t mins, vec3_t maxs);
void   AddPointToBounds(vec3_t v, vec3_t mins, vec3_t maxs);
int    VectorCompare(vec3_t v1, vec3_t v2);
vec_t  VectorLength(vec3_t v);
void   CrossProduct(vec3_t v1, vec3_t v2, vec3_t cross);
vec_t  VectorNormalize(vec3_t v);
vec_t  VectorNormalize2(vec3_t v, vec3_t out);
void   VectorInverse(vec3_t v);
void   VectorScale(vec3_t in, vec_t scale, vec3_t out);
int    Q_log2(int val);

void   AngleVectors(vec3_t angles, vec3_t forward, vec3_t right, vec3_t up);
int    BoxOnPlaneSide(vec3_t emins, vec3_t emaxs, struct cplane_s *plane);
float  anglemod(float a);
float  LerpAngle(float a1, float a2, float frac);

void ProjectPointOnPlane(vec3_t dst, const vec3_t p, const vec3_t normal);
void PerpendicularVector(vec3_t dst, const vec3_t src);
void RotatePointAroundVector(vec3_t dst, const vec3_t dir, const vec3_t point, float degrees);

char *COM_SkipPath(char *pathname);
void  COM_StripExtension(char *in, char *out);
void  COM_FileBase(char *in, char *out);
void  COM_FilePath(char *in, char *out);
void  COM_DefaultExtension(char *path, char *extension);
char *COM_Parse(char **data_p);
void  Com_sprintf(char *dest, int size, char *fmt, ...);
void  Com_PageInMemory(byte *buffer, int size);

int   Q_stricmp(const char *s1, const char *s2);
int   Q_strcasecmp(const char *s1, const char *s2);
int   Q_strncasecmp(const char *s1, const char *s2, int n);

short BigShort(short l);
short LittleShort(short l);
int   BigLong(int l);
int   LittleLong(int l);
float BigFloat(float l);
float LittleFloat(float l);

void  Swap_Init(void);
char *va(char *format, ...);

#define MAX_INFO_KEY    64
#define MAX_INFO_VALUE  64
#define MAX_INFO_STRING 512

char    *Info_ValueForKey(char *s, char *key);
void     Info_RemoveKey(char *s, char *key);
void     Info_SetValueForKey(char *s, char *key, char *value);
qboolean Info_Validate(char *s);

extern int curtime;
int    Sys_Milliseconds(void);
void   Sys_Mkdir(char *path);

void  *Hunk_Begin(int maxsize);
void  *Hunk_Alloc(int size);
void   Hunk_Free(void *buf);
int    Hunk_End(void);

#define SFF_ARCH   0x01
#define SFF_HIDDEN 0x02
#define SFF_RDONLY 0x04
#define SFF_SUBDIR 0x08
#define SFF_SYSTEM 0x10

char *Sys_FindFirst(char *path, unsigned musthave, unsigned canthave);
char *Sys_FindNext(unsigned musthave, unsigned canthave);
void  Sys_FindClose(void);

void Sys_Error(char *error, ...);
void Com_Printf(char *msg, ...);

/* ============================================================
   CVARS
   ============================================================ */

#ifndef CVAR
#define CVAR

#define CVAR_ARCHIVE  1
#define CVAR_USERINFO 2
#define CVAR_SERVERINFO 4
#define CVAR_NOSET    8
#define CVAR_LATCH    16

typedef struct cvar_s {
    char          *name;
    char          *string;
    char          *latched_string;
    int            flags;
    qboolean       modified;
    float          value;
    struct cvar_s *next;
} cvar_t;

#endif /* CVAR */

/* ============================================================
   COLLISION DETECTION
   ============================================================ */

#define CONTENTS_SOLID       1
#define CONTENTS_WINDOW      2
#define CONTENTS_AUX         4
#define CONTENTS_LAVA        8
#define CONTENTS_SLIME       16
#define CONTENTS_WATER       32
#define CONTENTS_MIST        64
#define LAST_VISIBLE_CONTENTS 64

#define CONTENTS_AREAPORTAL  0x8000
#define CONTENTS_PLAYERCLIP  0x10000
#define CONTENTS_MONSTERCLIP 0x20000
#define CONTENTS_CURRENT_0   0x40000
#define CONTENTS_CURRENT_90  0x80000
#define CONTENTS_CURRENT_180 0x100000
#define CONTENTS_CURRENT_270 0x200000
#define CONTENTS_CURRENT_UP  0x400000
#define CONTENTS_CURRENT_DOWN 0x800000
#define CONTENTS_ORIGIN      0x1000000
#define CONTENTS_MONSTER     0x2000000
#define CONTENTS_DEADMONSTER 0x4000000
#define CONTENTS_DETAIL      0x8000000
#define CONTENTS_TRANSLUCENT 0x10000000
#define CONTENTS_LADDER      0x20000000

#define SURF_LIGHT   0x1
#define SURF_SLICK   0x2
#define SURF_SKY     0x4
#define SURF_WARP    0x8
#define SURF_TRANS33 0x10
#define SURF_TRANS66 0x20
#define SURF_FLOWING 0x40
#define SURF_NODRAW  0x80

#define MASK_ALL          (-1)
#define MASK_SOLID        (CONTENTS_SOLID|CONTENTS_WINDOW)
#define MASK_PLAYERSOLID  (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define MASK_DEADSOLID    (CONTENTS_SOLID|CONTENTS_PLAYERCLIP|CONTENTS_WINDOW)
#define MASK_MONSTERSOLID (CONTENTS_SOLID|CONTENTS_MONSTERCLIP|CONTENTS_WINDOW|CONTENTS_MONSTER)
#define MASK_WATER        (CONTENTS_WATER|CONTENTS_LAVA|CONTENTS_SLIME)
#define MASK_OPAQUE       (CONTENTS_SOLID|CONTENTS_SLIME|CONTENTS_LAVA)
#define MASK_SHOT         (CONTENTS_SOLID|CONTENTS_MONSTER|CONTENTS_WINDOW|CONTENTS_DEADMONSTER)
#define MASK_CURRENT      (CONTENTS_CURRENT_0|CONTENTS_CURRENT_90|CONTENTS_CURRENT_180|CONTENTS_CURRENT_270|CONTENTS_CURRENT_UP|CONTENTS_CURRENT_DOWN)

#define AREA_SOLID    1
#define AREA_TRIGGERS 2

typedef struct cplane_s {
    vec3_t normal;
    float  dist;
    byte   type;
    byte   signbits;
    byte   pad[2];
} cplane_t;

typedef struct cmodel_s {
    vec3_t mins, maxs;
    vec3_t origin;
    int    headnode;
} cmodel_t;

typedef struct csurface_s {
    char name[16];
    int  flags;
    int  value;
} csurface_t;

typedef struct mapsurface_s {
    csurface_t c;
    char       rname[32];
} mapsurface_t;

typedef struct {
    qboolean    allsolid;
    qboolean    startsolid;
    float       fraction;
    vec3_t      endpos;
    cplane_t    plane;
    csurface_t *surface;
    int         contents;
    struct edict_s *ent;
} trace_t;

typedef enum {
    PM_NORMAL,
    PM_SPECTATOR,
    PM_DEAD,
    PM_GIB,
    PM_FREEZE
} pmtype_t;

#define PMF_DUCKED         1
#define PMF_JUMP_HELD      2
#define PMF_ON_GROUND      4
#define PMF_TIME_WATERJUMP 8
#define PMF_TIME_LAND      16
#define PMF_TIME_TELEPORT  32
#define PMF_NO_PREDICTION  64

typedef struct {
    pmtype_t pm_type;
    short    origin[3];
    short    velocity[3];
    byte     pm_flags;
    byte     pm_time;
    short    gravity;
    short    delta_angles[3];
} pmove_state_t;

#define BUTTON_ATTACK 1
#define BUTTON_USE    2
#define BUTTON_ANY    128

typedef struct usercmd_s {
    byte  msec;
    byte  buttons;
    short angles[3];
    short forwardmove, sidemove, upmove;
    byte  impulse;
    byte  lightlevel;
} usercmd_t;

#define MAXTOUCH 32

typedef struct {
    pmove_state_t  s;
    usercmd_t      cmd;
    qboolean       snapinitial;
    int            numtouch;
    struct edict_s *touchents[MAXTOUCH];
    vec3_t         viewangles;
    float          viewheight;
    vec3_t         mins, maxs;
    struct edict_s *groundentity;
    int            watertype;
    int            waterlevel;
    trace_t        (*trace)(vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end);
    int            (*pointcontents)(vec3_t point);
} pmove_t;

/* entity_state_t->effects */
#define EF_ROTATE       0x00000001
#define EF_GIB          0x00000002
#define EF_BLASTER      0x00000008
#define EF_ROCKET       0x00000010
#define EF_GRENADE      0x00000020
#define EF_HYPERBLASTER 0x00000040
#define EF_BFG          0x00000080
#define EF_COLOR_SHELL  0x00000100
#define EF_POWERSCREEN  0x00000200
#define EF_ANIM01       0x00000400
#define EF_ANIM23       0x00000800
#define EF_ANIM_ALL     0x00001000
#define EF_ANIM_ALLFAST 0x00002000
#define EF_FLIES        0x00004000
#define EF_QUAD         0x00008000
#define EF_PENT         0x00010000
#define EF_TELEPORTER   0x00020000
#define EF_FLAG1        0x00040000
#define EF_FLAG2        0x00080000

/* entity_state_t->renderfx flags */
#define RF_MINLIGHT    1
#define RF_VIEWERMODEL 2
#define RF_WEAPONMODEL 4
#define RF_FULLBRIGHT  8
#define RF_DEPTHHACK   16
#define RF_TRANSLUCENT 32
#define RF_FRAMELERP   64
#define RF_BEAM        128
#define RF_CUSTOMSKIN  256
#define RF_GLOW        512
#define RF_SHELL_RED   1024
#define RF_SHELL_GREEN 2048
#define RF_SHELL_BLUE  4096

/* player_state_t->refdef flags */
#define RDF_UNDERWATER   1
#define RDF_NOWORLDMODEL 2

/* entity state */
typedef struct entity_state_s {
    int    number;
    vec3_t origin;
    vec3_t angles;
    vec3_t old_origin;
    int    modelindex;
    int    modelindex2, modelindex3, modelindex4;
    int    frame;
    int    skinnum;
    unsigned int effects;
    int    renderfx;
    int    solid;
    int    sound;
    int    event;
} entity_state_t;

/* player state */
typedef struct {
    pmove_state_t pmove;
    vec3_t        viewangles;
    vec3_t        viewoffset;
    vec3_t        kick_angles;
    vec3_t        gunangles;
    vec3_t        gunoffset;
    int           gunindex;
    int           gunframe;
    float         blend[4];
    float         fov;
    int           rdflags;
    short         stats[32];
} player_state_t;

#endif /* Q_SHARED_H */
