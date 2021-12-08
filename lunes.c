/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              For a general introduction to LUNES implmentation please see the
 *              file: mig-agents.c
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <unistd.h> 
#include <fcntl.h>
#include <math.h>
#include <ctype.h>
#include <assert.h>
#include <ini.h>
#include <ts.h>
#include <rnd.h>
#include <gaia.h>
#include <rnd.h>
#include <values.h>
#include "utils.h"
#include "user_event_handlers.h"
#include "lunes.h"
#include "lunes_constants.h"
#include "entity_definition.h"


/* ************************************************************************ */
/*       L O C A L	V A R I A B L E S			                            */
/* ************************************************************************ */

FILE *         fp_print_trace;        // File descriptor for simulation trace file
unsigned short env_max_ttl = MAX_TTL; // TTL of newly created messages


/* ************************************************************************ */
/*          E X T E R N A L     V A R I A B L E S                           */
/* ************************************************************************ */

extern hash_t hash_table, *table;                   /* Global hash table of simulated entities */
extern hash_t sim_table, *stable;                   /* Hash table of locally simulated entities */
extern double simclock;                             /* Time management, simulated time */
extern TSeed  Seed, *S;                             /* Seed used for the random generator */
extern char * TESTNAME;                             /* Test name */
extern int    NSIMULATE;                            /* Number of Interacting Agents (Simulated Entities) per LP */
extern int    NLP;                                  /* Number of Logical Processes */
// Simulation control
extern float          env_end_clock;                /* End clock (simulated time) */
extern unsigned short env_dissemination_mode;       /* Dissemination mode */
extern float          env_broadcast_prob_threshold; /* Dissemination: conditional broadcast, probability threshold */
extern float          env_fixed_prob_threshold;     /* Dissemination: fixed probability, probability threshold */
extern unsigned int   env_probability_function;     /* Probability function for Degree Dependent Gossip */
extern double         env_function_coefficient;     /* Coefficient of the probability function */
extern int            env_epoch_steps;              /* Duration of a single epoch*/
extern int            holder;                       /* ID of the holder node*/
extern unsigned short env_max_ttl;                  /* TTL of new messages */
extern int 			  env_perc_active_nodes_;		/* Initial percentage of active node*/
extern long 		  countMessages;
extern int 			  countEpochs;
extern int 			  countDelivers;
extern int            countChunks;
extern double		  countSteps;


int numChunks = 50;
int cacheSize = 40;
int applicantPerc = 7;
int activationPerc = 20;   // on base 10000 

int numChildrenTree = 5;

int tempcountLinks=0;
int tempcountActive=0;
int countRequestMessages=0;
int countTreeMessages=0;
int countItemMessages=0;


//*************************************FUNCTIONS TO MANAGE THE CACHE*****************************************************


void add_into_cache (hash_node_t *node, cache_element elem){
    int newIndex = cache_choose_oldest(node);
    node->data->cache[newIndex] = elem;
}

int cache_choose_oldest (hash_node_t *node){
    int oldest = env_end_clock;
    int res = 0;
    for (int i=0; i< cacheSize; i++){
        if (node->data->cache[i].id == 0){
            res = i;
            break;
        } else if (node->data->cache[i].timestamp < oldest){
            oldest = node->data->cache[i].timestamp;
            res = i;
        }
    }
    return res;
}

int is_in_cache (hash_node_t *node, int elemId){
    int res = 0;
    for (int i = 0; i< cacheSize; i++){
        if (node->data->cache[i].id ==elemId){
            res = 1;
            break;
        }
    }
    return res;
}


//*************************************TREE MANAGEMENT**************************************************************

int addChildInTree (hash_node_t *node, int id){
    int res= -1;
    for (int i = 0; i< numChildrenTree; i++){
        if (node->data->treeChildren[i] == -1){
            node->data->treeChildren[i] = id;
            res = i;
            break;
        }
    }

    return res;
}

//*************************************FUNCTIONS FOR MANAGING DISSEMINATION ALGORITHS**********************************************
 //brief Used to calculate the forwarding probability value for a given node
double lunes_degdependent_prob(unsigned int deg) {
    double prob = 0.0;

    switch (env_probability_function) {
    // Function 2
    case 2:
        prob = 1.0 / log(env_function_coefficient * deg);
        break;

    // Function 1
    case 1:
        prob = 1.0 / pow(deg, env_function_coefficient);
        break;

    default:
        fprintf(stdout, "%12.2f FATAL ERROR, function: %d does NOT exist!\n", simclock, env_probability_function);
        fflush(stdout);
        exit(-1);
        break;
    }

    /*
     * The probability is defined in the range [0, 1], to get full dissemination some functions require that the negative
     * results are treated as "true" (i.e. 1)
     */
    if ((prob < 0) || (prob > 1)) {
        prob = 1;
    }
    return(prob);
}

int is_in_array (gpointer arr[], int length, gpointer elem){
	int i;
	for (i=0; i< length; i++){
		if (arr[i] == NULL){
			return 0;
		}
		else if (arr[i] == elem){
			return 1;
		}
	}
	return 0;
}


//********************************************FUNCTIONS FOR DISSEMINATION************************************************************************
void lunes_real_forward(hash_node_t *node, Msg *msg, unsigned short ttl, cache_element elem, int id, unsigned int creator, unsigned int forwarder) {
    // Iterator to scan the whole state hashtable of neighbors
    GHashTableIter iter;
    gpointer       key, destination;
    float          threshold;         // Tmp, used for probabilistic-based dissemination algorithms
    hash_node_t *  sender, *receiver; // Sender and receiver nodes in the global hashtable


    switch (msg->type) {
    // Request is always disseminated to all neighbors
    case 'I':
        g_hash_table_iter_init(&iter, node->data->state);
        // All neighbors
        while (g_hash_table_iter_next(&iter, &key, &destination)) {
            sender   = hash_lookup(stable, node->data->key);             // This node
            receiver = hash_lookup(table, *(unsigned int *)destination); // The neighbor

            // The original forwarder of this message and its creator are exclueded
            // from this dissemination
            if ((receiver->data->key != forwarder) && (receiver->data->key != creator)) {
                execute_item(simclock + FLIGHT_TIME, sender, receiver,id);
            }
        }
        break;

   /* case 'T':
        g_hash_table_iter_init(&iter, node->data->state);
        // All neighbors
        while (g_hash_table_iter_next(&iter, &key, &destination)) {
            sender   = hash_lookup(stable, node->data->key);             // This node
            receiver = hash_lookup(table, *(unsigned int *)destination); // The neighbor

            // The original forwarder of this message and its creator are exclueded
            // from this dissemination
            if ((receiver->data->key != forwarder) && (receiver->data->key != creator)) {
                execute_tree(simclock + FLIGHT_TIME, sender, receiver, ttl, id, elem, creator);
            }
        }
        break;*/

    case 'R':
        // Dissemination mode for the forwarded messages (dissemination algorithm)
        switch (env_dissemination_mode) {
            case BROADCAST:
                // The message is forwarded to ALL neighbors of this node
                // NOTE: in case of probabilistic broadcast dissemination this function is called
                //		 only if the probabilities evaluation was positive
                g_hash_table_iter_init(&iter, node->data->state);

                // All neighbors
                while (g_hash_table_iter_next(&iter, &key, &destination)) {
                    sender   = hash_lookup(stable, node->data->key);             // This node
                    receiver = hash_lookup(table, *(unsigned int *)destination); // The neighbor

                    // The original forwarder of this message and its creator are exclueded
                    // from this dissemination
                    if ((receiver->data->key != forwarder) && (receiver->data->key != creator)) {
                        execute_request(simclock + FLIGHT_TIME, sender, receiver, ttl, msg->request.request_static.chunkId, elem, creator);
                    }
                }
                break;

            case GOSSIP_FIXED_PROB:
                // In this case, all neighbors will be analyzed but the message will be
                // forwarded only to some of them
                g_hash_table_iter_init(&iter, node->data->state);

                // All neighbors
                while (g_hash_table_iter_next(&iter, &key, &destination)) {
                    // Probabilistic evaluation
                    threshold = RND_Interval(S, (double)0, (double)100);

                    if (threshold <= env_fixed_prob_threshold) {
                        sender   = hash_lookup(stable, node->data->key);             // This node
                        receiver = hash_lookup(table, *(unsigned int *)destination); // The neighbor

                        // The original forwarder of this message and its creator are exclueded
                        // from this dissemination
                        if ((receiver->data->key != forwarder) && (receiver->data->key != creator)) {
                            execute_request(simclock + FLIGHT_TIME, sender, receiver, ttl, msg->request.request_static.chunkId, elem, creator);
                        }
                    }
                }
                break;
            // Degree Dependent dissemination algorithm
            case DEGREE_DEPENDENT_GOSSIP:
                g_hash_table_iter_init(&iter, node->data->state);

                // All neighbors
                while (g_hash_table_iter_next(&iter, &key, &destination)) {
                    sender   = hash_lookup(stable, node->data->key);             // This node
                    receiver = hash_lookup(table, *(unsigned int *)destination); // The neighbor

                    // The original forwarder of this message and its creator are excluded
                    // from this dissemination
                    if ((receiver->data->key != forwarder) && (receiver->data->key != creator)) {
                        // Probabilistic evaluation
                        threshold = (RND_Interval(S, (double)0, (double)100)) / 100;

                        // If the eligible recipient has less than 3 neighbors, its reception probability is 1. However,
                        // if its value of num_neighbors is 0, it means that I don't know the dimension of
                        // that node's neighborhood, so the threshold is set to 1/n, being n
                        // the dimension of my neighborhood
                        if (receiver->data->num_neighbors < 3) {
                            // Note that, the startup phase (when the number of neighbors is not known) falls in
                            // this case (num_neighbors = 0)
                            // -> full dissemination
                            execute_request(simclock + FLIGHT_TIME, sender, receiver, ttl, msg->request.request_static.chunkId, elem, creator);
                        }
                        // Otherwise, the probability is evaluated according to the function defined by the
                        // environment variable env_probability_function
                        else{
                            if (threshold <= lunes_degdependent_prob(receiver->data->num_neighbors)) {
                                execute_request(simclock + FLIGHT_TIME, sender, receiver, ttl, msg->request.request_static.chunkId, elem, creator);
                            }
                        }
                    }
                }
                break;

            case FIXED_FANOUT:
            	g_hash_table_iter_init (&iter, node->data->state);
            	sender   = hash_lookup(stable, node->data->key);             // This node

            	if (node->data->num_neighbors <= 3) {
            		while (g_hash_table_iter_next(&iter, &key, &destination)) {
    	                receiver = hash_lookup(table, *(unsigned int *)destination); // The neighbor

    	                // The original forwarder of this message and its creator are exclueded
    	                // from this dissemination
    	                if ((receiver->data->key != forwarder) && (receiver->data->key != creator)) {
    	                    execute_request(simclock + FLIGHT_TIME, sender, receiver, ttl, msg->request.request_static.chunkId, elem, creator);
    	                }
    	            }
            	} else {
            		int count = 0;

            		threshold = RND_Interval(S, (double)0, (double)100);
            		int number = 3;
            		gpointer arr [number];
            		while (count < number){                  
            			destination = hash_table_random_key(node->data->state); 
                    	receiver = hash_lookup(table, *(unsigned int *)destination);        // The neighbor     
            			if (is_in_array(arr, number, destination)==0){
            				arr [count] = destination;
          				 	count++;
            				if ((receiver->data->key != forwarder) && (receiver->data->key != creator)) {
    		                	execute_request(simclock + FLIGHT_TIME, sender, receiver, ttl, msg->request.request_static.chunkId, elem, creator);
    		            	}
            			}
            		}
            	}
        	   break;
        }
    }
}


void lunes_forward_to_neighbors(hash_node_t *node, Msg *msg, unsigned short ttl, cache_element elem, int id, unsigned int creator, unsigned int forwarder) {
    float threshold; // Tmp, probabilistic evaluation

    // Dissemination mode for the forwarded messages
    switch (env_dissemination_mode) {
    case BROADCAST:
        // Probabilistic evaluation
        threshold = RND_Interval(S, (double)0, (double)100);
        if (threshold <= env_broadcast_prob_threshold) {
            lunes_real_forward(node, msg, ttl, elem, id, creator, forwarder);
        }
        break;

    case GOSSIP_FIXED_PROB:      
    case DEGREE_DEPENDENT_GOSSIP:
    case FIXED_FANOUT:
        lunes_real_forward(node, msg, ttl, elem, id, creator, forwarder);
        break;

    default:
        fprintf(stdout, "%12.2f FATAL ERROR, the dissemination mode [%2d] is NOT implemented in this version of LUNES!!!\n", simclock, env_dissemination_mode);
        fprintf(stdout, "%12.2f NOTE: all the adaptive protocols require compile time support: see the ADAPTIVE_GOSSIP_SUPPORT define in sim-parameters.h\n", simclock);
        fflush(stdout);
        exit(-1);
        break;
    }
}


void lunes_send_item_to_neighbors(hash_node_t *node, int item_id) {
    int i = 0;
    while (i < numChildrenTree) {
        int child = node->data->treeChildren[i];
        if ( child != -1)
            execute_item(simclock + FLIGHT_TIME, hash_lookup(stable, node->data->key), hash_lookup(stable, child), item_id);
    }
}


void lunes_send_request_to_neighbors(hash_node_t *node, int req_id) {
    // Iterator to scan the whole state hashtable of neighbors
    GHashTableIter iter;
    gpointer       key, destination;
    cache_element elem = {.timestamp = (int)simclock, .id = RND_Interval(S, 0, 10000000)};     //info to cache the message. Id used to identify the message

    // All neighbors
    g_hash_table_iter_init(&iter, node->data->state);

    while (g_hash_table_iter_next(&iter, &key, &destination)) {
        execute_request(simclock + FLIGHT_TIME, hash_lookup(stable, node->data->key), hash_lookup(table, *(unsigned int *)destination), env_max_ttl, req_id, elem, node->data->key);
    }
}


void lunes_send_tree_to_neighbor (hash_node_t *node, int child_id){
    int index = RND_Interval(S, 0,numChildrenTree);
    execute_tree(simclock + FLIGHT_TIME, node, hash_lookup(stable,node->data->treeChildren[index]), child_id);
}


//*********************************FUNCTIONS TO MANAGE THE TEMPORARINESS OF THE NETWORK*******************************************

int percentage_to_deactivate(int activate_perc){    //denominator = 10000
	int ratio = (int) env_perc_active_nodes_ / (100 - env_perc_active_nodes_);
	return (int) activate_perc / ratio - 1;
}

int count_neighbors (hash_node_t *node){
	GHashTableIter iter;
    gpointer       key, destination;
	int count =0;
	g_hash_table_iter_init(&iter, node->data->state);
    while (g_hash_table_iter_next(&iter, &key, &destination)) {
    	count++;
    }	
    return count;
}

void print_neighbors (hash_node_t *node){
	GHashTableIter iter;
    gpointer       key, destination;
    hash_node_t *  neigh;
	g_hash_table_iter_init(&iter, node->data->state);
    while (g_hash_table_iter_next(&iter, &key, &destination)) {
    	neigh = hash_lookup(table, *(unsigned int *)destination);
    	fprintf(stdout, "%d,%d,0\n", node->data->key, neigh->data->key);
    }	
}


void attach_node (hash_node_t *node){
	int connections = RND_Interval(S, 5,11); //how many connections to establish  #12-19 to get 12 edges per node, #5-11 to get 6 edges per node, #8-15 to get 9 edges per node
	int count = 0;
	while (count < connections){
		int new_neighbor_id = RND_Interval(S, 0, (NLP*NSIMULATE));        // chose a random node of the graph
		hash_node_t * new_neighbor = hash_lookup(table, new_neighbor_id); // The neighbor
		if (new_neighbor_id != node->data->key && new_neighbor->data->status != 0){			
		    if (add_entity_state_entry(new_neighbor_id, new_neighbor_id, node->data->key, node) != -1) {
		    	execute_link(simclock + FLIGHT_TIME, node, new_neighbor);
		    	count++;
            }
		}
	}
	node->data->num_neighbors = connections;
}  


void detach_node (hash_node_t *node){

    //cancel all the links in both direction
	GHashTableIter iter;
    gpointer       key, destination;
    hash_node_t *  toDel;

	g_hash_table_iter_init(&iter, node->data->state);
    while (g_hash_table_iter_next(&iter, &key, &destination)) {
    	toDel = hash_lookup(table, *(unsigned int *)destination);   // The neighbor
    	execute_unlink(simclock + FLIGHT_TIME, node, toDel);		// To signal that node has deactivated so the link is broken
    	execute_unlink(simclock + FLIGHT_TIME, toDel, node) ;       //link will be actually removed at the next step
    }	
    node->data->num_neighbors = 0;

    //cancel the tree links
    for (int i = 0; i < numChildrenTree; i++){
        int idChild = node->data->treeChildren[i];
        if (idChild != -1){
            execute_broken_tree(simclock + FLIGHT_TIME, node, hash_lookup(table, idChild));
            node->data->treeChildren[i] = -1;
        }       
    }
} 



//*****************************************RATIONALE OF THE NODES**********************************************************
void lunes_user_control_handler(hash_node_t *node) {


	if (simclock == BUILDING_STEP){						//just once: Building graph topology
        for (int i = 0; i < numChildrenTree; i++){
            node->data->treeChildren[i] = -1;
        }
		int rnd = RND_Interval(S, 0, 100);
		if (rnd >= env_perc_active_nodes_){
			node->data->status = 0;
		}
    }	

	
    // just once: initial graph building
    if (simclock == BUILDING_STEP + 1 && node->data->status != 0){
        attach_node(node);
    }


    //just once: counting neighbors
    if (simclock == BUILDING_STEP +2){					
		GHashTableIter iter;
	    gpointer       key, destination;
	    g_hash_table_iter_init(&iter, node->data->state);
	    int count = 0;
	        // All neighbors
	    while (g_hash_table_iter_next(&iter, &key, &destination)) {
	        count = count +1;
	    }
	    node->data->num_neighbors = count;
    }


    //at the beginning of every epoch
    if ((int)simclock % env_epoch_steps == 1 && simclock > BUILDING_STEP && node->data->status != 0 && node->data->status != 3){
        node->data->status = 1;
        if (RND_Interval(S, 0, 100) < applicantPerc){          //some of the active participant are applicants
            node->data->status = 2;
        }
    }

// at each timestep there is the chance for a node to activate or deactivate 
    if ((int)simclock > env_max_ttl) {                          
        int rnd = RND_Interval(S, 0, 10000);
        if (node->data->status == 0){               
            if (rnd < activationPerc){                           //1% for an off node to activate
                node->data->status = 1;
                attach_node(node);
                if (RND_Interval(S, 0, 100) < applicantPerc){          //some of the active participant are applicants
                    node->data->status = 2;
                    lunes_send_request_to_neighbors(node, 0); //temp! 
                }
            }
        } 
        else if (node->data->status == 1 || node->data->status == 2){        
            if (rnd < percentage_to_deactivate (activationPerc)){
                node->data->status = 0;
                detach_node(node);
            }
        }
    }


    //don't let nodes to be without neighbors
    if (node->data->status != 0 && node->data->num_neighbors == 0 && simclock > BUILDING_STEP +2){
    	attach_node (node);
    }


    //at the first timestep of each epoch
    if ((int)simclock % env_epoch_steps == 0 && (node->data->status ==2 || node->data->status == 3)){
        node->data->status = 1;
    }



//********************HOLDER BEHAVIOUR***********************************


    if ((int)simclock % env_epoch_steps == 0 && node->data->key == holder){
        node->data->status = 3;
    }

    if (simclock >= env_epoch_steps && node->data->key == holder && (int)simclock % env_epoch_steps < numChunks ){
        //send items to the sons
       // lunes_send_item_to_neighbors(node, (int)simclock % env_epoch_steps);         //at each epoch the holder sends one of the chunks
        countChunks++;
    }
    
    


// *******************APPLICANT BEHAVIOUR***********************************************************
    
    if (node->data->status == 2 && (int)simclock % env_epoch_steps == 1 && simclock > BUILDING_STEP){  //if the node is applicant ask for the first chunk
        lunes_send_request_to_neighbors(node, 0);
    }

    if ((int)simclock % env_epoch_steps == 0){
        for (int i = 0; i < numChunks; i++){
            node->data->buffer[i] = 0;
        }
        for (int i = 0; i < numChildrenTree; i++){
            node->data->treeChildren[i] = -1;
        }
    }


//***************************TEST*************************************************************
    if ((int)simclock % 1000 == 0 && simclock > BUILDING_STEP){
        tempcountLinks = tempcountLinks + node->data->num_neighbors;                // temp to delete
        tempcountActive = (node->data->status != 0) ? tempcountActive + 1 : tempcountActive;
    }

    if ((int)simclock % 1000 == 1 && simclock > BUILDING_STEP && node->data->key == 100){
        fprintf(stdout, "At %d we have %d nodes and %d links active\n", (int)simclock, tempcountActive, tempcountLinks);
        tempcountActive = 0;
        tempcountLinks = 0;
    }

    if ((int)simclock == env_end_clock - 10 && node->data->key == 4){
        fprintf (stdout, "A total of %d delivers out of %d chunks. A total of %ld", countDelivers, countChunks, countMessages);
    }

    if (((int)simclock == 1034 || (int)simclock == 1800) && node->data->status != 1 && node->data->status != 0 ){
        fprintf (stdout, "\n node %d with status %d --> ", node->data->key, node->data->status );
        for (int i =0; i< numChildrenTree; i++){
            fprintf(stdout, "%d ", node->data->treeChildren[i]);
        }
        //fprintf (stdout, "h %d at %d with status %d\n", node->data->key, (int)simclock, node->data->status);
    }
}

// **************************************MANAGING THE RECEIVED MESSAGES********************************************************************** 
void lunes_user_request_event_handler(hash_node_t *node, int forwarder, Msg *msg) {
    countRequestMessages++;
	if (is_in_cache(node, msg->request.request_static.cache.id) == 0){
        add_into_cache(node, msg->request.request_static.cache);        
    	if (node->data->key == holder){  //if it's the holder node
            int requestedChunk = msg->request.request_static.chunkId;
            if (addChildInTree(node, msg->request.request_static.creator) == -1){
                //fprintf(stdout, "request receveived from %d\n", msg->request.request_static.creator);
                lunes_send_tree_to_neighbor(node, msg->request.request_static.creator); 
            }// else {fprintf(stdout, "hhhh \n");}
            //lunes_send_request_to_neighbors()
            //lunes_send_item_to_neighbors(node, requestedChunk);
    	}
    	else if (node->data->status == 1 || node->data->status == 2){ 
    		lunes_forward_to_neighbors(node, msg,  --(msg->request.request_static.ttl),  msg->request.request_static.cache, msg->request.request_static.chunkId, msg->request.request_static.creator, forwarder);
    	}
    }
}


void lunes_user_item_event_handler(hash_node_t *node, int forwarder, Msg *msg) {
    countItemMessages++;
    if (node->data->status == 2){  //if it's the receiver node
        if (node->data->buffer[msg->item.item_static.chunkId] == 0){
            countDelivers++;
            node->data->buffer[msg->item.item_static.chunkId] = 1;
            for (int i = 0; i < numChildrenTree; i++){                      //for each possible child, if there is a child in the tree, send it to the child
                int child_id = node->data->treeChildren[i];
                if (child_id != -1){
                    hash_node_t * receiver = hash_lookup(stable, child_id);   
                    execute_item(simclock + FLIGHT_TIME, node, receiver,msg->item.item_static.chunkId);
                }
            }
        }
    }
}


//If possible add the node with childId as ID as one of the 5 children. If the node already has 5 children, select one and proceed recursively.
void lunes_user_tree_event_handler(hash_node_t *node, int forwarder, Msg *msg) {
    countTreeMessages++;
    if (addChildInTree (node, msg->tree.tree_static.childId) == -1){                 
        int rnd = RND_Interval(S, 0, numChildrenTree);
        hash_node_t * receiver = hash_lookup(stable, node->data->treeChildren[rnd]);
        execute_tree (simclock + FLIGHT_TIME, node, receiver, msg->tree.tree_static.childId);
    }
}

void lunes_user_broken_tree_event_handler(hash_node_t *node, int forwarder, Msg *msg) {
    countMessages++;
    lunes_send_request_to_neighbors(node, 0);  //temp! non 0 ma il primo chunk non ricevuto    
}