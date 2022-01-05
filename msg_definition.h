/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              -	In this file are defined the message types and their structures
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */

#ifndef __MESSAGE_DEFINITION_H
#define __MESSAGE_DEFINITION_H

#include "entity_definition.h"

/*---- M E S S A G E S    D E F I N I T I O N ---------------------------------*/

// Model messages definition
typedef struct _request_msg    RequestMsg; // Requesting chunks
typedef struct _item_msg	ItemMsg;    // Sending chunks
typedef struct _tree_msg	TreeMsg;    // Sending Neighbors contacts
typedef struct _ping_msg       PingMsg;    // Network constructions
typedef struct _pong_msg       PongMsg;  // Network constructions
typedef struct _migr_msg       MigrMsg;    // Migration message
typedef union   msg            Msg;

// General note:
//	each type of message is composed of a static and a dynamic part
//	-	the static part contains a pre-defined set of variables, and the size
//		of the dynamic part (as number of records)
//	-	a dynamic part that is composed of a sequence of records


struct _request_static_part {
    char           type;                    // Message type
    cache_element  cache;                   // info needed to cache the message
    unsigned short ttl;                     // Time-To-Live
    int            chunkId;                 // chunk number
    unsigned int   creator;                 // ID of the original sender of the message
    unsigned int   num_neighbors;           // Number of neighbors of forwarder
};
//

struct _request_msg {
    struct  _request_static_part request_static;
};


struct _tree_static_part {
    char           type;                    // Message type                 
    int            childId;                   // chunk number
   // #ifdef DEGREE_DEPENDENT_GOSSIP_SUPPORT
    unsigned int   num_neighbors;           // Number of neighbors of forwarder
    //#endif
};
//

struct _tree_msg {
    struct  _tree_static_part tree_static;
};


struct _item_static_part {
    char           type;                    // Message type
    cache_element  cache;                   // info needed to cache the message
    unsigned short ttl;                     // Time-To-Live
    int            chunkId;                 // Message Identifier
    unsigned int   creator;                 // ID of the original sender of the message
    unsigned int   num_neighbors;           // Number of neighbors of forwarder
};
//

struct _item_msg {
    struct  _item_static_part item_static;
};


struct _ping_static_part {
    char           type;                    // Message type
    unsigned int   creator;                 // ID of the original sender of the message
    int	    is_first_ping;	     // 1 if it is the first ping, then add such a neighbor if possible
};
//

struct _ping_msg {
    struct  _ping_static_part ping_static;
};


struct _pong_static_part {
    char           type;                    // Message type
    int            neighbor_type;          // 1 neighbor tree  ----  0 overlay neighbor
    unsigned int   from;                    // ID of the original sender of the message
    unsigned int   num_neighbors;           // Number of neighbors of forwarder
};
//

struct _pong_msg {
    struct  _pong_static_part pong_static;
};



// **********************************************
// MIGRATION MESSAGES
// **********************************************
//
/*! \brief Static part of migration messages */
struct _migration_static_part {
    char          type;        // Message type
    unsigned int  dyn_records; // Number of records in the dynamic part of the message
};
//
/*! \brief Dynamic part of migration messages */
struct _migration_dynamic_part {
    struct state_element records[MAX_MIGRATION_DYNAMIC_RECORDS]; // It is an array of records
};
//
/*! \brief Migration message */
struct _migr_msg {
    struct  _migration_static_part  migration_static;  // Static part
    struct  _migration_dynamic_part migration_dynamic; // Dynamic part
};



/*! \brief Union structure for all types of messages */
union msg {
    char        type;
    PingMsg     ping;
    RequestMsg  request;
    MigrMsg     migr;
    ItemMsg     item;
    TreeMsg     tree;
    PongMsg     pong;
};
/*---------------------------------------------------------------------------*/

#endif /* __MESSAGE_DEFINITION_H */
