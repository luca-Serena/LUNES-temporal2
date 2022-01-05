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

typedef struct neighbor{
    unsigned int id;
    unsigned int type;     //'T' = tree neighbor, 'O' = overlay neighbor
    unsigned int cardinality;
    unsigned int timestamp;
} neighbor;


/*! \brief SE state definition */
typedef struct hash_data_t {
    int           key;                  			// SE identifier
    int           lp;                   			// Logical Process ID (that is the SE container)
    int	   status;		  			// 0 off 1 active 2 applicant 3 holder 
    unsigned int  num_neighbors;        			// Number of SE's neighbors (dynamically updated)
    int	   buffer [NUM_CHUNKS];	
    cache_element cache [CACHE_SIZE];
    int	   treeChildren [NUM_TREE_CHILDREN];		// id of the children the node is sending messages to
    int 	   firstChunk;		  			//number of the first chunk received by the node
    int 	   lastChunkTime;	 			//number of the last chunk received by the node
    neighbor       neighbors [MAX_NEIGHBORS];
} hash_data_t;

#endif /* __ENTITY_DEFINITION_H */
