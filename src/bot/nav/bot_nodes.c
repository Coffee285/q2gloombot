/*
 * bot_nodes.c -- navigation node (waypoint) system for q2gloombot
 *
 * Implements the static node graph used by A* pathfinding.
 * Nodes are stored in a flat array; the node's array index IS its ID.
 * Removed nodes are marked with id == BOT_INVALID_NODE so that the
 * slot can be reused by a future Node_Add call.
 *
 * File I/O uses standard C fopen/fwrite/fread so that nav files can be
 * saved and loaded without depending on the engine's limited game import
 * filesystem API.
 */

#include "bot_nodes.h"
#include <float.h>
#include <math.h>

/* -----------------------------------------------------------------------
   Module globals
   ----------------------------------------------------------------------- */
nav_node_t nav_nodes[MAX_NAV_NODES];
int        nav_node_count = 0;   /* highest used slot + 1 */

/* Binary file format magic and version */
#define NAV_FILE_MAGIC   0x3156414E  /* "NAV1" little-endian */
#define NAV_FILE_VERSION 1

/* -----------------------------------------------------------------------
   Node_Clear
   Reset the entire node graph (e.g. on map change).
   ----------------------------------------------------------------------- */
void Node_Clear(void)
{
    int i;

    for (i = 0; i < MAX_NAV_NODES; i++) {
        nav_nodes[i].id           = BOT_INVALID_NODE;
        nav_nodes[i].num_neighbors = 0;
    }
    nav_node_count = 0;
}

/* -----------------------------------------------------------------------
   Node_Add
   Insert a new node into the first available slot.
   Returns the node's ID on success, or BOT_INVALID_NODE if the graph
   is full.
   ----------------------------------------------------------------------- */
int Node_Add(vec3_t origin, unsigned int flags)
{
    int i;

    /* Search for a free slot (previously removed node or unused tail). */
    for (i = 0; i < MAX_NAV_NODES; i++) {
        if (nav_nodes[i].id == BOT_INVALID_NODE)
            break;
    }

    if (i >= MAX_NAV_NODES) {
        gi.dprintf("Node_Add: node graph full (%d nodes)\n", MAX_NAV_NODES);
        return BOT_INVALID_NODE;
    }

    nav_nodes[i].id            = i;
    VectorCopy(origin, nav_nodes[i].origin);
    nav_nodes[i].flags         = flags;
    nav_nodes[i].num_neighbors = 0;
    nav_nodes[i].team_access   = NAV_TEAM_ALL;

    /* Zero neighbor arrays */
    {
        int j;
        for (j = 0; j < MAX_NODE_NEIGHBORS; j++) {
            nav_nodes[i].neighbors[j]         = BOT_INVALID_NODE;
            nav_nodes[i].neighbor_costs[j]    = 0.0f;
            nav_nodes[i].movement_required[j] = NAV_MOVE_WALK;
        }
    }

    if (i >= nav_node_count)
        nav_node_count = i + 1;

    return i;
}

/* -----------------------------------------------------------------------
   Node_Remove
   Mark the node as free and remove all neighbor references to it.
   ----------------------------------------------------------------------- */
void Node_Remove(int id)
{
    int i, j, k;

    if (id < 0 || id >= nav_node_count)
        return;
    if (nav_nodes[id].id == BOT_INVALID_NODE)
        return;   /* already removed */

    /* Scrub all references to this node from other nodes' neighbor lists. */
    for (i = 0; i < nav_node_count; i++) {
        nav_node_t *n = &nav_nodes[i];

        if (n->id == BOT_INVALID_NODE)
            continue;

        for (j = 0; j < n->num_neighbors; j++) {
            if (n->neighbors[j] != id)
                continue;

            /* Compact the neighbor arrays by shifting entries left. */
            for (k = j; k < n->num_neighbors - 1; k++) {
                n->neighbors[k]         = n->neighbors[k + 1];
                n->neighbor_costs[k]    = n->neighbor_costs[k + 1];
                n->movement_required[k] = n->movement_required[k + 1];
            }
            n->num_neighbors--;

            /* Sentinel the vacated tail slot. */
            n->neighbors[n->num_neighbors]         = BOT_INVALID_NODE;
            n->neighbor_costs[n->num_neighbors]    = 0.0f;
            n->movement_required[n->num_neighbors] = NAV_MOVE_WALK;

            /* Restart scan for this node in case of duplicates. */
            j--;
        }
    }

    /* Mark the slot as free. */
    nav_nodes[id].id           = BOT_INVALID_NODE;
    nav_nodes[id].num_neighbors = 0;
}

/* -----------------------------------------------------------------------
   Node_FindNearest
   Return the ID of the nearest valid node that:
     - has ALL bits in required_flags set (pass 0 to accept any node), and
     - lies within max_range world units (pass 0.0f for unlimited range).
   Returns BOT_INVALID_NODE if no qualifying node is found.
   ----------------------------------------------------------------------- */
int Node_FindNearest(vec3_t origin, unsigned int required_flags,
                     float max_range)
{
    int   best_id   = BOT_INVALID_NODE;
    float best_dist = (max_range > 0.0f) ? (max_range * max_range)
                                          : FLT_MAX;
    int   i;

    for (i = 0; i < nav_node_count; i++) {
        vec3_t delta;
        float  dist_sq;
        const nav_node_t *n = &nav_nodes[i];

        if (n->id == BOT_INVALID_NODE)
            continue;

        /* Must satisfy all required flags. */
        if (required_flags != 0 &&
            (n->flags & required_flags) != required_flags)
            continue;

        VectorSubtract(origin, n->origin, delta);
        dist_sq = DotProduct(delta, delta);

        if (dist_sq < best_dist) {
            best_dist = dist_sq;
            best_id   = n->id;
        }
    }

    return best_id;
}

/* -----------------------------------------------------------------------
   Internal helper: add a one-way neighbor link.
   Returns true on success, false if the neighbor list is full.
   ----------------------------------------------------------------------- */
static qboolean Node_AddLink(int from_id, int to_id, float cost, int move_type)
{
    nav_node_t *n = &nav_nodes[from_id];
    int         j;

    if (n->num_neighbors >= MAX_NODE_NEIGHBORS) {
        gi.dprintf("Node_Connect: node %d neighbor list full\n", from_id);
        return false;
    }

    /* Avoid duplicate links. */
    for (j = 0; j < n->num_neighbors; j++) {
        if (n->neighbors[j] == to_id)
            return true;   /* already linked */
    }

    j = n->num_neighbors;
    n->neighbors[j]         = to_id;
    n->neighbor_costs[j]    = cost;
    n->movement_required[j] = move_type;
    n->num_neighbors++;
    return true;
}

/* -----------------------------------------------------------------------
   Node_Connect
   Create a bidirectional link between id1 and id2.
   ----------------------------------------------------------------------- */
void Node_Connect(int id1, int id2, float cost, int move_type)
{
    if (id1 < 0 || id1 >= nav_node_count || nav_nodes[id1].id == BOT_INVALID_NODE) {
        gi.dprintf("Node_Connect: invalid node id1=%d\n", id1);
        return;
    }
    if (id2 < 0 || id2 >= nav_node_count || nav_nodes[id2].id == BOT_INVALID_NODE) {
        gi.dprintf("Node_Connect: invalid node id2=%d\n", id2);
        return;
    }
    if (id1 == id2)
        return;

    Node_AddLink(id1, id2, cost, move_type);
    Node_AddLink(id2, id1, cost, move_type);
}

/* -----------------------------------------------------------------------
   Node_Save
   Serialize the node graph to  maps/<mapname>.nav  in binary form.
   Returns true on success.
   ----------------------------------------------------------------------- */
qboolean Node_Save(const char *mapname)
{
    char     path[MAX_QPATH + 16];
    FILE    *f;
    int      i, valid_count, version, magic;

    if (!mapname || !mapname[0]) {
        gi.dprintf("Node_Save: empty mapname\n");
        return false;
    }

    Com_sprintf(path, sizeof(path), "maps/%s.nav", mapname);

    f = fopen(path, "wb");
    if (!f) {
        gi.dprintf("Node_Save: cannot open '%s' for writing\n", path);
        return false;
    }

    /* Count valid nodes. */
    valid_count = 0;
    for (i = 0; i < nav_node_count; i++) {
        if (nav_nodes[i].id != BOT_INVALID_NODE)
            valid_count++;
    }

    /* Header */
    magic   = NAV_FILE_MAGIC;
    version = NAV_FILE_VERSION;
    fwrite(&magic,       sizeof(int), 1, f);
    fwrite(&version,     sizeof(int), 1, f);
    fwrite(&valid_count, sizeof(int), 1, f);

    /* Node records */
    for (i = 0; i < nav_node_count; i++) {
        const nav_node_t *n = &nav_nodes[i];
        int               j;
        int               neighbor_id;
        float             neighbor_cost;
        int               move_req;

        if (n->id == BOT_INVALID_NODE)
            continue;

        fwrite(&n->id,           sizeof(int),   1, f);
        fwrite(n->origin,        sizeof(float),  3, f);
        fwrite(&n->flags,        sizeof(unsigned int), 1, f);
        fwrite(&n->team_access,  sizeof(unsigned int), 1, f);
        fwrite(&n->num_neighbors,sizeof(int),   1, f);

        for (j = 0; j < MAX_NODE_NEIGHBORS; j++) {
            neighbor_id   = n->neighbors[j];
            neighbor_cost = n->neighbor_costs[j];
            move_req      = n->movement_required[j];
            fwrite(&neighbor_id,   sizeof(int),   1, f);
            fwrite(&neighbor_cost, sizeof(float), 1, f);
            fwrite(&move_req,      sizeof(int),   1, f);
        }
    }

    fclose(f);
    gi.dprintf("Node_Save: saved %d nodes to '%s'\n", valid_count, path);
    return true;
}

/* -----------------------------------------------------------------------
   Node_Load
   Deserialize the node graph from  maps/<mapname>.nav .
   Clears the existing graph before loading.
   Returns true on success.
   ----------------------------------------------------------------------- */
qboolean Node_Load(const char *mapname)
{
    char  path[MAX_QPATH + 16];
    FILE *f;
    int   magic, version, count, i;

    if (!mapname || !mapname[0]) {
        gi.dprintf("Node_Load: empty mapname\n");
        return false;
    }

    Com_sprintf(path, sizeof(path), "maps/%s.nav", mapname);

    f = fopen(path, "rb");
    if (!f) {
        gi.dprintf("Node_Load: no nav file '%s'\n", path);
        return false;
    }

    /* Validate header */
    if (fread(&magic,   sizeof(int), 1, f) != 1 ||
        fread(&version, sizeof(int), 1, f) != 1 ||
        fread(&count,   sizeof(int), 1, f) != 1) {
        gi.dprintf("Node_Load: '%s' truncated header\n", path);
        fclose(f);
        return false;
    }

    if (magic != NAV_FILE_MAGIC) {
        gi.dprintf("Node_Load: '%s' bad magic\n", path);
        fclose(f);
        return false;
    }

    if (version != NAV_FILE_VERSION) {
        gi.dprintf("Node_Load: '%s' unsupported version %d\n", path, version);
        fclose(f);
        return false;
    }

    if (count < 0 || count > MAX_NAV_NODES) {
        gi.dprintf("Node_Load: '%s' invalid node count %d\n", path, count);
        fclose(f);
        return false;
    }

    Node_Clear();

    for (i = 0; i < count; i++) {
        nav_node_t  n;
        int         j;
        int         id;

        memset(&n, 0, sizeof(n));

        if (fread(&id,            sizeof(int),          1, f) != 1 ||
            fread(n.origin,       sizeof(float),         3, f) != 3 ||
            fread(&n.flags,       sizeof(unsigned int),  1, f) != 1 ||
            fread(&n.team_access, sizeof(unsigned int),  1, f) != 1 ||
            fread(&n.num_neighbors, sizeof(int),         1, f) != 1) {
            gi.dprintf("Node_Load: '%s' truncated at node %d\n", path, i);
            fclose(f);
            return false;
        }

        for (j = 0; j < MAX_NODE_NEIGHBORS; j++) {
            if (fread(&n.neighbors[j],         sizeof(int),   1, f) != 1 ||
                fread(&n.neighbor_costs[j],    sizeof(float), 1, f) != 1 ||
                fread(&n.movement_required[j], sizeof(int),   1, f) != 1) {
                gi.dprintf("Node_Load: '%s' truncated at node %d neighbor %d\n",
                           path, i, j);
                fclose(f);
                return false;
            }
        }

        if (id < 0 || id >= MAX_NAV_NODES) {
            gi.dprintf("Node_Load: '%s' node index %d out of range\n", path, id);
            fclose(f);
            return false;
        }

        if (n.num_neighbors < 0 || n.num_neighbors > MAX_NODE_NEIGHBORS) {
            gi.dprintf("Node_Load: '%s' node %d invalid neighbor count %d\n",
                       path, id, n.num_neighbors);
            fclose(f);
            return false;
        }

        n.id = id;
        nav_nodes[id] = n;

        if (id >= nav_node_count)
            nav_node_count = id + 1;
    }

    fclose(f);
    gi.dprintf("Node_Load: loaded %d nodes from '%s'\n", count, path);
    return true;
}
