/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              -	Function prototypes
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */

#ifndef __LUNES_H
#define __LUNES_H


#include "utils.h"
#include "entity_definition.h"


void lunes_real_forward(hash_node_t *, Msg *, unsigned short, cache_element, int, unsigned int, unsigned int);
void lunes_forward_to_neighbors(hash_node_t *, Msg *, unsigned short, cache_element, int, unsigned int, unsigned int);


// LUNES handlers
void lunes_user_request_event_handler(hash_node_t *, int, Msg *);
void lunes_user_item_event_handler(hash_node_t *, int, Msg *);
void lunes_user_tree_event_handler(hash_node_t *, int, Msg *);
void lunes_user_broken_tree_event_handler(hash_node_t *, int, Msg *);
void lunes_user_register_event_handler(hash_node_t *);
void lunes_user_control_handler(hash_node_t *);

// Support functions
int cache_choose_oldest(hash_node_t*);

#endif /* __LUNES_H */
