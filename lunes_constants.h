/*	##############################################################################################
 *      Advanced RTI System, ARTÌS			http://pads.cs.unibo.it
 *      Large Unstructured NEtwork Simulator (LUNES)
 *
 *      Description:
 *              Model level parameters for LUNES
 *
 *      Authors:
 *              First version by Gabriele D'Angelo <g.dangelo@unibo.it>
 *
 ############################################################################################### */

#ifndef __LUNES_CONSTANTS_H
#define __LUNES_CONSTANTS_H

//  General parameters
#define CACHE_SIZE	500
#define NUM_CHUNKS 	150

//	Dissemination protocols
#define BROADCAST                  0    // Probabilistic broadcast
#define GOSSIP_FIXED_PROB          1    // Fixed probability
#define FIXED_FANOUT		    4
#define DANDELION                  8
#define DANDELIONPLUS              6
#define DANDELIONPLUSPLUS	    5
#define DEGREE_DEPENDENT_GOSSIP    7    // Degree Dependent Gossip

#endif /* __LUNES_CONSTANTS_H */
