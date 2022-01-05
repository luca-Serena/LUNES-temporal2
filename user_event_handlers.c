/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              -	In this file you find all the user event handlers to be used to implement a
 *                      discrete event simulation. Only the modelling part is to be inserted in this
 *                      file, other tasks such as GAIA-related data structure management are
 *                      implemented in other parts of the code.
 *              -	Some "support" functions are also present.
 *              -	This file is part of the MIGRATION-AGENTS template provided in the
 *                      ARTÌS/GAIA software distribution but some modifications have been done to
 *                      include the LUNES features.
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
#include "utils.h"
#include "msg_definition.h"
#include "lunes.h"
#include "lunes_constants.h"
#include "user_event_handlers.h"


/* ************************************************************************ */
/*          E X T E R N A L     V A R I A B L E S                           */
/* ************************************************************************ */

extern hash_t hash_table, *table;                   /* Global hash table of simulated entities */
extern hash_t sim_table, *stable;                   /* Hash table of locally simulated entities */
extern double simclock;                             /* Time management, simulated time */
extern TSeed  Seed, *S;                             /* Seed used for the random generator */
extern FILE * fp_print_trace;                       /* File descriptor for simulation trace file */
extern char * TESTNAME;                             /* Test name */
extern int    LPID;                                 /* Identification number of the local Logical Process */
extern int    local_pid;                            /* Process identifier */
extern int    NSIMULATE;                            /* Number of Interacting Agents (Simulated Entities) per LP */
extern int    NLP;                                  /* Number of Logical Processes */
// Simulation control
extern unsigned int   env_migration;                /* Migration state */
extern float          env_migration_factor;         /* Migration factor */
extern unsigned int   env_load;                     /* Load balancing */
extern float          env_end_clock;                /* End clock (simulated time) */
extern unsigned short env_max_ttl;                  /* TTL of new messages */
extern unsigned short env_dissemination_mode;       /* Dissemination mode */
extern float          env_broadcast_prob_threshold; /* Dissemination: conditional broadcast, probability threshold */
extern unsigned int   env_cache_size;               /* Cache size of each node */
extern float          env_fixed_prob_threshold;     /* Dissemination: fixed probability, probability threshold */
extern int 			  env_perc_active_nodes_;		/* Initial percentage of active node*/
extern unsigned int   env_probability_function;     /* Probability function for Degree Dependent Gossip */
extern double         env_function_coefficient;     /* Coefficient of probability function */
extern int            holder;                       /* ID of the holder node*/
extern int            env_epoch_steps;              /* Duration of a single epoch*/




/*! \brief Utility to check environment variables, if the variable is not defined then the run is aborted
 */
char *check_and_getenv(char *variable) {
    char *value;

    value = getenv(variable);
    if (value == NULL) {
        fprintf(stdout, "The environment variable %s is not defined\n", variable);
        fflush(stdout);
        exit(1);
    }else {
        return(value);
    }
}


void execute_request(double ts, hash_node_t *src, hash_node_t *dest, unsigned short ttl, int req_id, cache_element elem, unsigned int creator) {
    RequestMsg     msg;
    unsigned int message_size;

    // Defining the message type
    msg.request_static.type = 'R';
    msg.request_static.chunkId    = req_id;
    msg.request_static.cache      = elem;
    msg.request_static.ttl        = ttl;
    msg.request_static.creator    = creator;
    message_size = sizeof(struct _request_static_part);

    // Buffer check
    if (message_size > BUFFER_SIZE) {
        fprintf(stdout, "%12.2f FATAL ERROR, the outgoing BUFFER_SIZE is not sufficient!\n", simclock);
        fflush(stdout);
        exit(-1);
    }

    if (ttl > 0){
        GAIA_Send(src->data->key, dest->data->key, ts, (void *)&msg, message_size);
    }
}

void execute_item(double ts, hash_node_t *src, hash_node_t *dest, int item_id) {
    ItemMsg  msg;
    unsigned int message_size;

    // Defining the message type
    msg.item_static.type = 'I';
    msg.item_static.chunkId    = item_id;
    message_size = sizeof(struct _request_static_part);

    // Buffer check
    if (message_size > BUFFER_SIZE) {
        fprintf(stdout, "%12.2f FATAL ERROR, the outgoing BUFFER_SIZE is not sufficient!\n", simclock);
        fflush(stdout);
        exit(-1);
    }

    GAIA_Send(src->data->key, dest->data->key, ts, (void *)&msg, message_size);
    // Real send
}

void execute_tree(double ts, hash_node_t *src, hash_node_t *dest,  int child_id) {
    TreeMsg  msg;
    unsigned int message_size;

    // Defining the message type
    msg.tree_static.type = 'T';
    msg.tree_static.childId    = child_id;
    message_size = sizeof(struct _tree_static_part);

    // Buffer check
    if (message_size > BUFFER_SIZE) {
        fprintf(stdout, "%12.2f FATAL ERROR, the outgoing BUFFER_SIZE is not sufficient!\n", simclock);
        fflush(stdout);
        exit(-1);
    }
        GAIA_Send(src->data->key, dest->data->key, ts, (void *)&msg, message_size);
}



void execute_ping(double ts, hash_node_t *src, int dest, int is_first) {
    PingMsg  msg;
    unsigned int message_size;

    // Defining the message type
    msg.ping_static.type = 'P';
    msg.ping_static.creator = src->data->key;
    msg.ping_static.is_first_ping = is_first;
    message_size = sizeof(struct _tree_static_part);

    // Buffer check
    if (message_size > BUFFER_SIZE) {
        fprintf(stdout, "%12.2f FATAL ERROR, the outgoing BUFFER_SIZE is not sufficient!\n", simclock);
        fflush(stdout);
        exit(-1);
    }
        GAIA_Send(src->data->key, dest, ts, (void *)&msg, message_size);
}

void execute_pong(double ts, hash_node_t *src, int dest,  int cardinality) {
    PongMsg  msg;
    unsigned int message_size;

    // Defining the message type
    msg.pong_static.type = 'O';
    msg.pong_static.num_neighbors = cardinality;
    msg.pong_static.from = src->data->key;
    message_size = sizeof(struct _tree_static_part);

    if (cardinality == -2){
        msg.pong_static.neighbor_type = 'T';
    } else {
        msg.pong_static.neighbor_type = 'O';
    }

    // Buffer check
    if (message_size > BUFFER_SIZE) {
        fprintf(stdout, "%12.2f FATAL ERROR, the outgoing BUFFER_SIZE is not sufficient!\n", simclock);
        fflush(stdout);
        exit(-1);
    }
        GAIA_Send(src->data->key, dest, ts, (void *)&msg, message_size);
}


/* ************************************************************************ */
/*      U S E R   E V E N T   H A N D L E R S			                    */
/*									                                        */
/*	NOTE: when a handler required extensive modifications for LUNES	        */
/*		then it calls another user level handerl called		                */
/*		lunes_<handler_name> and placed in the file lunes.c	                */
/* ************************************************************************ */

/*****************************************************************************
 *! \brief CONTROL: at each timestep, the LP calls this handler to permit the execution
 *      of model level interactions, for performance reasons the handler is called once
 *      for all the SE that allocated in the LP
 */
void user_control_handler() {
    int          h;
    hash_node_t *node;
    hash_node_t *tempNode;


    if ((int)simclock > EXECUTION_STEP && (int)simclock % env_epoch_steps == 0 ){   //start of an epoch: chosing each time a new aplicant and holder
        int rnd; 
        //choosing holder node
        do {
        	rnd = RND_Interval(S, 0, NLP * NSIMULATE);
            tempNode = hash_lookup(table, rnd);
        } while ( tempNode->data->status == 0);
    	holder = tempNode->data->key;
    }

    // Only if in the aggregation phase is finished &&
    // if it is possible to send messages up to the last simulated timestep then the statistics will be
    // affected by some messages that have been sent but with no time to be received
    if ((simclock >= (float)BUILDING_STEP)) {
        // For each local SE
        for (h = 0; h < stable->size; h++) {
            for (node = stable->bucket[h]; node; node = node->next) {
                // Calling the appropriate LUNES user level handler
                lunes_user_control_handler(node);
            } 
        }
    }
}

/*****************************************************************************
 *! \brief USER MODEL: when it is received a model level interaction, after some
 *         validation this generic handler is called. The specific user level
 *         handler will complete its processing
 */
void user_model_events_handler(int to, int from, Msg *msg, hash_node_t *node) {

    // A model event has been received, now calling appropriate user level handler

    // If the node should perform a DOS attack: not a miner and is an attacker
    switch (msg->type) {
    case 'R':
        lunes_user_request_event_handler(node, from, msg);
        break;
    case 'I':
        lunes_user_item_event_handler(node, from, msg);
        break;
    case 'T':
        lunes_user_tree_event_handler(node, from, msg);
        break;
    case 'P':
        lunes_user_ping_event_handler(node, from, msg);
        break;
    case 'O':
        lunes_user_pong_event_handler(node, from, msg);
        break;

    default:
        fprintf(stdout, "FATAL ERROR, received an unknown user model event type: %d\n", msg->type);
        fflush(stdout);
        exit(-1);
    }
}

void user_environment_handler() {
    // ######################## RUNTIME CONFIGURATION SECTION ####################################
    //	Runtime configuration:	migration type configuration
    env_migration = atoi(check_and_getenv("MIGRATION"));
    fprintf(stdout, "LUNES____[%10d]: MIGRATION, migration variable set to %d\n", local_pid, env_migration);
    if ((env_migration > 0) && (env_migration < 4)) {
        fprintf(stdout, "LUNES____[%10d]: MIGRATION is ON, migration type is set to %d\n", local_pid, env_migration);
        GAIA_SetMigration(env_migration);
    }else {
        fprintf(stdout, "LUNES____[%10d]: MIGRATION is OFF\n", local_pid);
        GAIA_SetMigration(MIGR_OFF);
    }

    //	Runtime configuration:	migration factor (GAIA)
    env_migration_factor = atof(check_and_getenv("MFACTOR"));
    fprintf(stdout, "LUNES____[%10d]: MFACTOR, migration factor: %f\n", local_pid, env_migration_factor);
    GAIA_SetMF(env_migration_factor);

    //	Runtime configuration:	turning on/off the load balancing (GAIA)
    env_load = atoi(check_and_getenv("LOAD"));
    fprintf(stdout, "LUNES____[%10d]: LOAD, load balancing: %d\n", local_pid, env_load);
    if (env_load == 1) {
        fprintf(stdout, "LUNES____[%10d]: LOAD, load balancing is ON\n", local_pid);
        GAIA_SetLoadBalancing(LOAD_ON);
    }else {
        fprintf(stdout, "LUNES____[%10d]: LOAD, load balancing is OFF\n", local_pid);
        GAIA_SetLoadBalancing(LOAD_OFF);
    }

    //	Runtime configuration:	number of steps in the simulation run
    env_end_clock = atof(check_and_getenv("END_CLOCK"));
    fprintf(stdout, "LUNES____[%10d]: END_CLOCK, number of steps in the simulation run -> %f\n", local_pid, env_end_clock);
    if (env_end_clock == 0) {
        fprintf(stdout, "LUNES____[%10d]:  END_CLOCK is 0, no timesteps are defined for this run!!!\n", local_pid);
    }

    env_perc_active_nodes_ = atof(check_and_getenv("ACTIVE_PERC"));
    fprintf(stdout, "LUNES____[%10d]: ACTIVE_PERC, initial percentage of active nodes -> %d\n", local_pid, env_perc_active_nodes_);
    if (env_perc_active_nodes_ <= 0) {
        fprintf(stdout, "LUNES____[%10d]: ACTIVE_PERC is <= 0, error \n", local_pid);
    }

    //	Runtime configuration:	time-to-live for new messages in the network
    env_max_ttl = atoi(check_and_getenv("MAX_TTL"));
    fprintf(stdout, "LUNES____[%10d]: MAX_TTL, maximum time-to-live for messages in the network -> %d\n", local_pid, env_max_ttl);

    env_epoch_steps= atoi(check_and_getenv("EPOCH_STEPS"));
    fprintf(stdout, "LUNES____[%10d]: EPOCH_STEPS, duration of an epoch in time-steps -> %d\n", local_pid, env_epoch_steps);

    if (env_max_ttl == 0) {
        fprintf(stdout, "LUNES____[%10d]: MAX_TTL is 0, no TTL is defined for this run!\n", local_pid);
    }


    //	Runtime configuration:	dissemination mode (gossip protocol)
    env_dissemination_mode = atoi(check_and_getenv("DISSEMINATION"));
    fprintf(stdout, "LUNES____[%10d]: DISSEMINATION, dissemination mode -> %d\n", local_pid, env_dissemination_mode);
    //
    switch (env_dissemination_mode) {
    case BROADCAST:                             //	probabilistic broadcast dissemination
        
        //	Runtime configuration:	probability threshold of the broadcast dissemination
        env_broadcast_prob_threshold = atof(check_and_getenv("BROADCAST_PROB_THRESHOLD"));
        fprintf(stdout, "LUNES____[%10d]: BROADCAST_PROB_THRESHOLD, probability of the broadcast dissemination -> %f\n", local_pid, env_broadcast_prob_threshold);
        if ((env_broadcast_prob_threshold < 0) || (env_broadcast_prob_threshold > 100)) {
            fprintf(stdout, "LUNES____[%10d]: BROADCAST_PROB_THRESHOLD is out of the boundaries!!!\n", local_pid);
        }
        break;

    case GOSSIP_FIXED_PROB:                     //	gossip with fixed probability
        
        //	Runtime configuration:	probability threshold of the fixed probability dissemination
        env_fixed_prob_threshold = atof(check_and_getenv("FIXED_PROB_THRESHOLD"));
        fprintf(stdout, "LUNES____[%10d]: FIXED_PROB_THRESHOLD, probability of the fixed probability dissemination -> %f\n", local_pid, env_fixed_prob_threshold);
        if ((env_fixed_prob_threshold < 0) || (env_fixed_prob_threshold > 100)) {
            fprintf(stdout, "LUNES____[%10d]:  FIXED_PROB_THRESHOLD is out of the boundaries!!!\n", local_pid);
        }
        break;
        

    case DEGREE_DEPENDENT_GOSSIP:

        // Runtime configuration: probability function to be applied to Degree Dependent Gossip
        env_probability_function = atoi(check_and_getenv("PROBABILITY_FUNCTION"));
        fprintf(stdout, "LUNES____[%10d]: PROBABILITY_FUNCTION, probability function -> %u\n", local_pid, env_probability_function);

        // Coefficient of probability function
        env_function_coefficient = atof(check_and_getenv("FUNCTION_COEFFICIENT"));
        fprintf(stdout, "LUNES____[%10d]: FUNCTION_COEFFICIENT, function coefficient -> %f\n", local_pid, env_function_coefficient);

        break;


    case FIXED_FANOUT:
    break;

    default:
        fprintf(stdout, "LUNES____[%10d]: FATAL ERROR, the dissemination mode [%2d] is NOT implemented in this version of LUNES!!!\n", local_pid, env_dissemination_mode);
        fflush(stdout);
        exit(-1);
        break;
    }

}

/*****************************************************************************
 *! \brief BOOTSTRAP: before starting the real simulation tasks, the model level
 *         can initialize some data structures and set parameters
 */
void user_bootstrap_handler() {
    #ifdef TRACE_DISSEMINATION
    char buffer[1024];

    // Preparing the simulation trace file
    sprintf(buffer, "%sSIM_TRACE_%03d.log", TESTNAME, LPID);

    fp_print_trace = fopen(buffer, "w");
    #endif
}

/*****************************************************************************
 *! \brief SHUTDOWN: Before shutting down, the model layer is able to
 *         deallocate some data structures
 */
void user_shutdown_handler() {
    #ifdef TRACE_DISSEMINATION
    char  buffer[1024];
    FILE *fp_print_messages_trace;


    sprintf(buffer, "%stracefile-messages-%d.trace", TESTNAME, LPID);

    fp_print_messages_trace = fopen(buffer, "w");

    //	statistics
    //	total number of trans on the network
    fprintf(fp_print_messages_trace, "M %010lu\n", get_total_sent_trans());
    //	total number of block broadcasted on the network
    fprintf(fp_print_messages_trace, "M %010lu\n", get_total_sent_blocks());

    fclose(fp_print_messages_trace);

    fclose(fp_print_trace);
    #endif
}