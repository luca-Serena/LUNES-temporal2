/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              -	In this file is defined the state of the simulated entitiesFORKI
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */

#ifndef __ENTITY_DEFINITION_H
#define __ENTITY_DEFINITION_H
//#define HIERARCHY

#include "lunes_constants.h"


/*---- E N T I T I E S    D E F I N I T I O N ---------------------------------*/


/*! \brief Records composing the local state (dynamic part) of each SE
 *         NOTE: no duplicated keys are allowed
 */
struct state_element {
    unsigned int  key;      // Key
    unsigned int value; // Value
};

typedef struct cache_element{
    unsigned int id;
    unsigned int timestamp;
} cache_element;

/*! \brief SE state definition */
typedef struct hash_data_t {
    int           key;                  	// SE identifier
    int           lp;                   	// Logical Process ID (that is the SE container)
    int           internal_timer;       	// Used to track mining activity
    int	   status;		  	// 0 off 1 active 2 applicant 3 holder 
    GHashTable *  state;               	// Local state as an hash table (glib) (dynamic part)
    int	   received;	  		// 0 not received anything, !=0 received the message, -1 received the message back in the fluff phase
    unsigned int  num_neighbors;        	// Number of SE's neighbors (dynamically updated)
    int	   buffer [NUM_CHUNKS];	
    cache_element cache [CACHE_SIZE];
    int	   treeChildren [5];	  	// id of the children the node is sending messages to
    int 	   firstChunk;		  	//number of the first chunk received by the node
    int 	   lastChunkTime;	 	//number of the last chunk received by the node
} hash_data_t;

#endif /* __ENTITY_DEFINITION_H */
