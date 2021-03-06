/*
 * ParseDIMACS.c
 *
 *  Created on: Apr 30, 2015
 *      Author: salma
 */

#define _GNU_SOURCE
#include "ParseDIMACS.h"
#include "global.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define END_OF_CLAUSE_LINE 		0
#define INCREASE_DECISION		5

/******************
 * Helper Functions
 *******************/
#ifdef TEST_C2D
void static print_DIMACS(SatState* sat_state){
	printf("Printing DIMACS file!\n");
	FILE *f = fopen("fileDIMACS.txt", "w");
	if (f == NULL){
	    printf("Error opening file!\n");
	    exit(1);
	}
	for (unsigned long clauseidx =0; clauseidx< sat_state->num_clauses_in_cnf; clauseidx++){
		Clause curclause = sat_state->delta[clauseidx];
		for (unsigned long literalidx = 0; literalidx < curclause.num_literals_in_clause; literalidx++){
			signed long litidx = curclause.literals[literalidx]->sindex;
			fprintf(f, "%ld ", litidx);
		}
		fprintf(f,"0 \n");
	}

	fclose(f);
}
#endif


// update the list of clauses that has this variable as one of its literals
void add_clause_to_variable(Var* var, Clause* clause){
	/* Update the dynmaic list which will also keep track of the learnt clauses*/
	if(var->num_of_clauses_of_variables >= var->max_size_list_of_clause_of_variables){
		var->max_size_list_of_clause_of_variables = var->num_of_clauses_of_variables * MALLOC_GROWTH_RATE;
		var->list_clause_of_variables = (unsigned long*) realloc (var->list_clause_of_variables,sizeof(unsigned long) * var->max_size_list_of_clause_of_variables);
	}

	var->list_clause_of_variables[var->num_of_clauses_of_variables++] = clause->cindex;

}

// update the list of clauses that has this variable as one of its literals originally in the cnf
// this list will never change
void add_clause_to_variable_from_cnf(Var* var, Clause* clause){
	/* Update the static list which will only keep track of the cnf clauses*/
	if(var->num_of_clauses_of_variables_in_cnf >= var->max_size_list_of_clause_of_variables_in_cnf){
		var->max_size_list_of_clause_of_variables_in_cnf = var->num_of_clauses_of_variables_in_cnf * MALLOC_GROWTH_RATE;
		var->list_clause_of_variables_in_cnf = (unsigned long*) realloc (var->list_clause_of_variables_in_cnf,sizeof(unsigned long) * var->max_size_list_of_clause_of_variables_in_cnf);
	}

	var->list_clause_of_variables_in_cnf[var->num_of_clauses_of_variables_in_cnf++] = clause->cindex;

}



// update the list of clause that has this literal as one of its literals
void add_clause_to_literal(Lit* lit, Clause* clause){

	if(lit->num_containing_clause >= lit->max_size_list_contatining_clauses){
		lit->max_size_list_contatining_clauses = lit->max_size_list_contatining_clauses * MALLOC_GROWTH_RATE;
		lit->list_of_containing_clauses = (unsigned long*) realloc (lit->list_of_containing_clauses,sizeof(unsigned long) * lit->max_size_list_contatining_clauses);
	}

	lit->list_of_containing_clauses[lit->num_containing_clause++] = clause->cindex;
//#ifdef DEBUG
//	printf("adding clause %ld to the list of containing clauses for literal %ld\n",lit->list_of_containing_clauses[lit->num_containing_clause], lit->sindex);
//#endif

}

// add watching clause to the list of watching clauses over a literal (used again when you learn a new clause)
void add_watching_clause(Clause* clause, Lit* lit){
	//get clause index.
	unsigned long index = clause->cindex;

	if(lit->num_watched_clauses >= lit->max_size_list_watched_clauses){
			// needs to realloc the size
			lit->max_size_list_watched_clauses =(lit->num_watched_clauses * MALLOC_GROWTH_RATE);
			lit->list_of_watched_clauses = (unsigned long*) realloc( lit->list_of_watched_clauses, sizeof(unsigned long) * lit->max_size_list_watched_clauses);
	}

	lit->list_of_watched_clauses[lit->num_watched_clauses] = index;

	lit->num_watched_clauses ++;

//Set the flag for this clause
	if(lit->num_dirty_watched_clauses >= lit->max_size_list_dirty_watched_clauses){
			// needs to realloc the size
			lit->max_size_list_dirty_watched_clauses =(lit->num_dirty_watched_clauses * MALLOC_GROWTH_RATE);
			lit->list_of_dirty_watched_clauses = (unsigned long*) realloc( lit->list_of_dirty_watched_clauses, sizeof(unsigned long) * lit->max_size_list_dirty_watched_clauses);
	}

	lit->list_of_dirty_watched_clauses[lit->num_dirty_watched_clauses] = 1;

	lit->num_dirty_watched_clauses ++;



}
void remove_watching_clause(unsigned long index, Lit* lit){
	// Clear the flag (marked dirty)
	lit->list_of_dirty_watched_clauses[index] = 0;
}


/******************************************************************************
 * Parse Problem line
 * -- Get the definition of the problem by getting the number of clauses and
 * -- number of variables
 * -- initialize space in the memory for their sizes
 ******************************************************************************/
static int parseProblemLine(char* line, SatState* sat_state){
	char* pch = NULL;
	int countparameters = 0;
#ifdef DEBUG
	printf("%s\n",line);
#endif

	//separate the string into space separated tokens
	pch = strtok(line, " \t"); //TODO: some problem lines have two spaces

	while (pch != NULL){
		// skip the p cnf part of the line
		// skip if it is space or new line ot tab


		if ( !strstr(pch, "p")  &&  !strstr(pch, "cnf") ) {
#ifdef DEBUG
			printf("%s\n", pch);

#endif
			//if(pch[0] == ' ') continue; // if the first character in the token is space then neglect this token

			if(countparameters == 0 ){
				sat_state->num_variables_in_cnf = atoi(pch);
				//allocate memory for n of variables and at the end the number of implications will equal to the number of variables;
				sat_state->variables =( (Var*) malloc(sizeof(Var) * sat_state->num_variables_in_cnf) );
				//sat_state->implications =( (Lit **) malloc(sizeof(Lit*) * sat_state->num_variables_in_state) );
				sat_state->decisions =( (Lit **) malloc(sizeof(Lit*) * sat_state->num_variables_in_cnf   + INCREASE_DECISION) );
			}
			else{
				sat_state->num_clauses_in_delta = atoi(pch);
				sat_state->num_clauses_in_cnf = atoi(pch);
				// allocate memory for m clauses
				sat_state->delta =( (Clause *) malloc(sizeof(Clause) * sat_state->num_clauses_in_delta) );
				// initialize the max_size_list_delta with the original number of clauses. This number will increase by adding the learnt clause
				sat_state->max_size_list_delta = sat_state->num_clauses_in_delta;
			}
			countparameters++;
		}

		pch = strtok (NULL, " \t");
	}

//	if(pch)
//	 free(pch);


	// initialize the n variables and 2n literals;
	for(unsigned long i = 0; i < sat_state->num_variables_in_cnf; i++ ){
#ifdef DEBUG
		printf("in loop i = %ld\n", i);
#endif

		/* Initialize the variables*/
		sat_state->variables[i].index = i+1; 								// variables start with index 1 to n ;

		sat_state->variables[i].max_size_list_of_clause_of_variables_in_cnf = 1;
		sat_state->variables[i].num_of_clauses_of_variables_in_cnf = 0;
		sat_state->variables[i].list_clause_of_variables_in_cnf = (unsigned long*) malloc(sizeof(unsigned long) );

		sat_state->variables[i].max_size_list_of_clause_of_variables = 1;
		sat_state->variables[i].num_of_clauses_of_variables_in_cnf = 0;
		sat_state->variables[i].list_clause_of_variables = (unsigned long*) malloc(sizeof(unsigned long) );

		sat_state->variables[i].antecedent = 0;
		sat_state->variables[i].sat_state = sat_state;
		sat_state->variables[i].mark = 0;	// THIS FIELD AS IS IN THE REQUIREMENTS

		/* Initialize negative literals*/
		sat_state->variables[i].negLit = (Lit*) malloc(sizeof(Lit) );
		sat_state->variables[i].negLit->sindex = (i+1) - (2*(i+1));			// negative literals start with -1 to -n;
		sat_state->variables[i].negLit->LitState = 0; 						// free literal
		sat_state->variables[i].negLit->decision_level = 1;
		sat_state->variables[i].negLit->LitValue = 'u'; 					// initialize to unassigned literal
		sat_state->variables[i].negLit->num_watched_clauses = 0;
		sat_state->variables[i].negLit->list_of_watched_clauses = (unsigned long*) malloc(sizeof(unsigned long)); // will be realloc when it expands
		sat_state->variables[i].negLit->max_size_list_watched_clauses = 1;
		sat_state->variables[i].negLit->num_dirty_watched_clauses = 0;
		sat_state->variables[i].negLit->list_of_dirty_watched_clauses = (unsigned long*) malloc(sizeof(unsigned long)); // will be realloc when it expands
		sat_state->variables[i].negLit->max_size_list_dirty_watched_clauses = 1;
		sat_state->variables[i].negLit->num_containing_clause = 0;
		sat_state->variables[i].negLit->list_of_containing_clauses = (unsigned long*) malloc(sizeof(unsigned long));// will be realloc when it expands
		sat_state->variables[i].negLit->max_size_list_contatining_clauses = 1;
		sat_state->variables[i].negLit->variable = &(sat_state->variables[i]);
		sat_state->variables[i].negLit->vsids_score = 0;

		//sat_state->variables[i].negLit->antecedent = NULL;
		//sat_state->variables[i].negLit->order_of_implication = 0;

		/* Initialize positive literals */
		sat_state->variables[i].posLit = (Lit*) malloc(sizeof(Lit) );
		sat_state->variables[i].posLit->sindex = i+1;						// positive literals start with 1 to n;
		sat_state->variables[i].posLit->LitState = 0; 						// free literal
		sat_state->variables[i].posLit->decision_level = 1;
		sat_state->variables[i].posLit->LitValue = 'u';
		sat_state->variables[i].posLit->num_watched_clauses = 0;// initialize to free literal
		sat_state->variables[i].posLit->list_of_watched_clauses = (unsigned long*) malloc(sizeof(unsigned long)); // will be realloc when it expands
		sat_state->variables[i].posLit->max_size_list_watched_clauses = 1;
		sat_state->variables[i].posLit->num_dirty_watched_clauses = 0;
		sat_state->variables[i].posLit->list_of_dirty_watched_clauses = (unsigned long*) malloc(sizeof(unsigned long)); // will be realloc when it expands
		sat_state->variables[i].posLit->max_size_list_dirty_watched_clauses = 1;
		sat_state->variables[i].posLit->num_containing_clause = 0;
		sat_state->variables[i].posLit->list_of_containing_clauses = (unsigned long*) malloc(sizeof(unsigned long)); // will be realloc when it expands
		sat_state->variables[i].posLit->max_size_list_contatining_clauses = 1;
		sat_state->variables[i].posLit->variable = &(sat_state->variables[i]);
		sat_state->variables[i].posLit->vsids_score = 0;
		//sat_state->variables[i].posLit->antecedent = NULL;
		//sat_state->variables[i].posLit->order_of_implication = 0;
	}

	return 1;
}


/******************************************************************************
 * Parse Clause line
 * -- For each line in the DIMACS file construct a new clause struct
 * -- initialize the array of literals this clause contains
 ******************************************************************************/
static unsigned long parseClause(SatState* sat_state, char* line, Clause* clause){
	char* pch = NULL;
	unsigned long countvariables = 0;
	//Lit* literals = (Lit *) malloc(sizeof (Lit));
#ifdef DEBUG
	printf("%s\n",line);
#endif

	// initialize memory of clause literals with one element and then increase the size by double if needed
	unsigned long MIN_CAPACITY = 1;

	clause->literals = (Lit**) malloc(sizeof(Lit*) * MIN_CAPACITY);


	//separate the string into space separated tokens
	pch = strtok(line, " \t");

	while (pch != NULL){
//#ifdef DEBUG
//			printf("%s\n", pch);
//#endif
			if(atol(pch) == END_OF_CLAUSE_LINE){
				break;
			}

			if(countvariables >= MIN_CAPACITY){
				// needs to realloc the size
				MIN_CAPACITY = countvariables * MALLOC_GROWTH_RATE;
				clause->literals = (Lit**) realloc( clause->literals, sizeof(Lit*) * MIN_CAPACITY);
			}

			signed long value = atol(pch);
			 Var* var =  &(sat_state->variables[abs(value) - 1]); // because array is filled from 0 to n-1
			 if(value < 0)
				 clause->literals[countvariables] = sat_neg_literal(var);
			 else if (value > 0)
				 clause->literals[countvariables] = sat_pos_literal(var);

			 add_clause_to_variable(var, clause);
			 add_clause_to_variable_from_cnf(var, clause);
			 add_clause_to_literal(clause->literals[countvariables], clause);

//#ifdef DEBUG
//			 print_clause_containing_literal(clause->literals[countvariables]);
//#endif
			 countvariables ++;

		pch = strtok (NULL, " \t");
	}

	clause->num_literals_in_clause = countvariables;
	clause->max_size_list_literals = MIN_CAPACITY;
	clause->is_subsumed = 0;

	// For the two literal watch // just initialize here
	if(clause->num_literals_in_clause > 1){
		clause->L1 =  clause->literals[0]; // first literal
		add_watching_clause(clause, clause->L1);
		clause->L2 =  clause->literals[1]; // second literal
		add_watching_clause(clause, clause->L2);
	}
	else{ //unit clause
		clause->L1 = clause->literals[0];
		add_watching_clause(clause, clause->L1);
		clause->L2 = NULL;
	}

	clause->mark = 0; // THIS FIELD AS IT DESCRIBED BY IN THE REQUIREMENTS

	return countvariables;
}
/* **************************************************************************** */



/******************************************************************************
 * Parse the DIMACS file that contains the CNF
 * -- fill in the sat_state data structure with initialized spaces in memory
 ******************************************************************************/
void parseDIMACS(FILE* cnf_file, SatState * sat_state){

	char * line = NULL;
	size_t len = 0;
	ssize_t read;

	unsigned long clausecounter = 0; // index of clauses starts with 0 to m-1
	while((read = getline(&line, &len, cnf_file)) != -1){

		// ignore anything starts with a c or % (comment line)
		// remove the line[0] == ' '  because some clause lines start with ' '
		if (line[0] == 'c' || line[0] == '%'  || line[0] == '0' || line[0] == '\n') continue;

		// end of file
		//if (len == 0)

		// parse the line that starts with p (problem line  p cnf <number of variables n> <number of clauses m>)
		if((line)[0] == 'p'){
			parseProblemLine(line, sat_state);
#ifdef DEBUG
			printf("number of clauses: %ld, number of variables: %ld\n", sat_state->num_clauses_in_delta, sat_state->num_variables_in_cnf);
#endif
		}
		else
		{
			// read the CNF
			sat_state->delta[clausecounter].cindex = clausecounter + 1;
			unsigned long vars = parseClause(sat_state, line, &sat_state->delta[clausecounter++]);
#ifdef DEBUG
			printf("Number of variables: %ld\n", vars);
#endif

		}
	}

#ifdef TEST_C2D
	print_DIMACS(sat_state);
#endif

//	if(line)
//		free(line);

}



