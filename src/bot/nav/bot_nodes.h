/*
 * bot_nodes.h -- navigation node (waypoint) system for q2gloombot
 *
 * Nodes are waypoints placed throughout each Gloom map.  They form a
 * weighted directed graph that A* traverses to plan bot movement.
 *
 * NODE TYPE FLAGS
 * ---------------
 * Flags are bitmasks; a single node may carry several types.  For
 * example, a ledge at the edge of a water pool might be both
 * NAV_GROUND and NAV_WATER.
 *
 * MOVEMENT TYPES
 * ---------------
 * Each edge (neighbor link) records which movement capability a bot
 * must have to traverse it (NAV_MOVE_*).  BotNav_FindPath filters
 * edges that require capabilities the bot lacks (e.g. wall-climb
 * edges are skipped for non-wall-walking classes).
 *
 * FILE FORMAT (maps/<mapname>.nav)
 * ---------------------------------
 * Binary, little-endian:
 *   4 bytes  magic   "NAV1"
 *   4 bytes  version 1
 *   4 bytes  node count
 *   Per node:
 *     4 bytes  id
 *     12 bytes origin (3 × float)
 *     4 bytes  flags
 *     4 bytes  team_access
 *     4 bytes  num_neighbors
 *     MAX_NODE_NEIGHBORS × (4+4+4) bytes  (neighbor_id, cost, move_type)
 */

#ifndef BOT_NODES_H
#define BOT_NODES_H

#include "bot.h"

/* -----------------------------------------------------------------------
   Node type flags (bitmask)
   ----------------------------------------------------------------------- */
#define NAV_GROUND      0x0001  /* standard walkable floor position          */
#define NAV_JUMP        0x0002  /* requires a jump to traverse               */
#define NAV_WALLCLIMB   0x0004  /* alien wall-climbing surface (wall/ceiling) */
#define NAV_FLY         0x0008  /* aerial node for Wraith class              */
#define NAV_WATER       0x0010  /* underwater/swim node                      */
#define NAV_LADDER      0x0020  /* ladder-climbing node                      */
#define NAV_TELEPORTER  0x0040  /* Gloom human teleporter spawn location     */
#define NAV_EGG         0x0080  /* Gloom alien egg spawn location            */
#define NAV_CAMP        0x0100  /* good defensive/camping position           */
#define NAV_SNIPE       0x0200  /* good ranged-attack position (human)       */
#define NAV_AMBUSH      0x0400  /* good ambush position (alien)              */
#define NAV_ITEM        0x0800  /* item/health/ammo pickup location          */

/* -----------------------------------------------------------------------
   Movement type constants (used in movement_required[])
   ----------------------------------------------------------------------- */
#define NAV_MOVE_WALK   0   /* standard ground movement                      */
#define NAV_MOVE_JUMP   1   /* must jump to reach the neighbor               */
#define NAV_MOVE_CLIMB  2   /* wall/ceiling climb                            */
#define NAV_MOVE_FLY    3   /* flight required (Wraith class)                */
#define NAV_MOVE_SWIM   4   /* swimming                                      */
#define NAV_MOVE_LADDER 5   /* ladder traverse                               */

/* -----------------------------------------------------------------------
   Team/capability access flags (team_access bitmask)
   ----------------------------------------------------------------------- */
#define NAV_TEAM_HUMAN  0x01
#define NAV_TEAM_ALIEN  0x02
#define NAV_TEAM_ALL    (NAV_TEAM_HUMAN | NAV_TEAM_ALIEN)

/* -----------------------------------------------------------------------
   Limits
   ----------------------------------------------------------------------- */
#define MAX_NODE_NEIGHBORS  8
#define MAX_NAV_NODES       1024

/* -----------------------------------------------------------------------
   Navigation node structure
   ----------------------------------------------------------------------- */
typedef struct {
    int          id;                                    /* unique node ID; BOT_INVALID_NODE when slot is free */
    vec3_t       origin;                                /* 3D world position                                  */
    unsigned int flags;                                 /* NAV_* type bitmask                                 */
    int          neighbors[MAX_NODE_NEIGHBORS];         /* neighbor node IDs                                  */
    float        neighbor_costs[MAX_NODE_NEIGHBORS];    /* traversal cost to each neighbor                    */
    int          num_neighbors;                         /* number of active neighbor links                    */
    unsigned int team_access;                           /* NAV_TEAM_* bitmask                                 */
    int          movement_required[MAX_NODE_NEIGHBORS]; /* NAV_MOVE_* for each neighbor edge                  */
} nav_node_t;

/* -----------------------------------------------------------------------
   Global node graph (defined in bot_nodes.c)
   ----------------------------------------------------------------------- */
extern nav_node_t nav_nodes[MAX_NAV_NODES];
extern int        nav_node_count;   /* highest allocated slot index + 1 */

/* -----------------------------------------------------------------------
   Node operations
   ----------------------------------------------------------------------- */

/* Add a new node; returns its ID, or BOT_INVALID_NODE if the graph is full. */
int      Node_Add(vec3_t origin, unsigned int flags);

/* Remove a node and scrub all references to it from neighbor lists. */
void     Node_Remove(int id);

/*
 * Find the nearest valid node that has ALL required_flags set and lies
 * within max_range units.  Pass required_flags=0 to accept any node.
 * Returns the node ID, or BOT_INVALID_NODE if none qualifies.
 */
int      Node_FindNearest(vec3_t origin, unsigned int required_flags,
                          float max_range);

/*
 * Create a bidirectional link between id1 and id2 with the given traversal
 * cost and movement type (NAV_MOVE_*).
 */
void     Node_Connect(int id1, int id2, float cost, int move_type);

/* Serialize the node graph to maps/<mapname>.nav; returns true on success. */
qboolean Node_Save(const char *mapname);

/* Deserialize the node graph from maps/<mapname>.nav; returns true on success. */
qboolean Node_Load(const char *mapname);

/* Reset the node graph (called on map change). */
void     Node_Clear(void);

#endif /* BOT_NODES_H */
