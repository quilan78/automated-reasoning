/*
 * VSIDS.c
 *
 *  Created on: May 22, 2015
 *      Author: matt
 */

#include "VSIDS.h"

#include <stdlib.h>
#include <time.h>

void initialize_vsids_scores (SatState * sat_state)
{
//	printf("14\n");
//	print_all_clauses(sat_state);
	for (unsigned long cidx = 0; cidx < sat_state->num_clauses_in_delta; cidx++)
	{
		Clause * cur_clause = & (sat_state->delta[cidx]);
		for (unsigned long lidx = 0; lidx < cur_clause->num_literals_in_clause; lidx++)
		{
			Lit * cur_lit = cur_clause->literals[lidx];
			cur_lit->vsids_score++;
		}
	}
//	print_all_clauses(sat_state);
//	printf("26\n");

	srandom(time(NULL));
}

void update_vsids_scores (SatState * sat_state)
{
	//printf("Updating!\n");

	if (random() % 4) // very rough 25% probability
	{
		// divide all the "counters" by a constant
		for (unsigned long vidx = 0; vidx < sat_state->num_variables_in_cnf; vidx++)
		{
			Var cur_var = sat_state->variables[vidx];
			Lit * pos = cur_var.posLit;
			Lit * neg = cur_var.negLit;
			pos->vsids_score = pos->vsids_score << 2;
			neg->vsids_score = neg->vsids_score << 2;
			// need to test to determine a magic number by which to decrement
		}
	}

	Clause alpha = *sat_state->alpha;
	for (unsigned long lidx = 0; lidx < alpha.num_literals_in_clause; lidx++)
	{
		alpha.literals[lidx]->vsids_score++;
	}

#ifdef DEBUG
	//printf("Finish updating scores!\n");
#endif

}

Lit* vsids_get_free_literal (SatState* sat_state) {

	Lit * max_lit = NULL;
	unsigned long max_score = 0;

	for (unsigned long vidx = 0; vidx < sat_state->num_variables_in_cnf; vidx++)
	{
		Var cur_var = sat_state->variables[vidx];

		Lit * cur_lit;

		cur_lit = cur_var.posLit;

		if (cur_lit->vsids_score > max_score)
		{
			if (!sat_implied_literal(cur_lit))
			{
				max_score = cur_lit->vsids_score;
				max_lit = cur_lit;
			}
		}
		else if (cur_lit->vsids_score == max_score)
		{
			if (!sat_implied_literal(cur_lit))
			{
				// break ties with some randomization
				if (random() % 2) {
					max_score = cur_lit->vsids_score;
					max_lit = cur_lit;
				}
			}
		}

		cur_lit = cur_var.negLit;

		if (cur_lit->vsids_score > max_score)
		{
			if (!sat_implied_literal(cur_lit))
			{
				max_score = cur_lit->vsids_score;
				max_lit = cur_lit;
			}
		}
		else if (cur_lit->vsids_score == max_score)
		{
			if (!sat_implied_literal(cur_lit))
			{
				// break ties with some randomization
				if (random() % 2) {
					max_score = cur_lit->vsids_score;
					max_lit = cur_lit;
				}
			}
		}
	}

  return max_lit;
}
