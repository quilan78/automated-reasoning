#include "sat_api.h"
#include <fcntl.h>
#include <assert.h>
#include "ParseDIMACS.h"
#include "LiteralWatch.h"
#include "VSIDS.h"
#include "ConflictAlgorithms.h"
#include "global.h"

/* (1) after deciding on a new literal (i.e., in sat_decide_literal())
 * (2) after adding an asserting clause (i.e., in sat_assert_clause(...))
 * (3) neither the above, which would imply literals appearing in unit clauses */
#define CASE1 1
#define CASE2 2
#define CASE3 3


/* GLOBALS */
BOOLEAN FLAG_CASE1_UNIT_RESOLUTION = 0;
BOOLEAN FLAG_CASE2_UNIT_RESOLUTION = 0;
BOOLEAN FLAG_CASE3_UNIT_RESOLUTION = 1;  //TODO: set this to 1 in the final version



void print_clause(Clause* clause){
	printf("clause index %ld:\t", clause->cindex);
	//printf("num literals in a clause (%ld): ",clause->num_literals_in_clause);
	for(unsigned long i = 0; i < clause->num_literals_in_clause; i++){
		printf("%ld(c:",clause->literals[i]->sindex);
		//printf(" num of containing clause %ld  ,",clause->literals[i]->num_containing_clause);
		for(unsigned long j =0; j < clause->literals[i]->num_containing_clause; j++){
			printf("%ld,", clause->literals[i]->list_of_containing_clauses[j]);
		}
		printf(")(w:");

		for(unsigned long w =0; w < clause->literals[i]->num_watched_clauses; w++){
			printf("%ld,", clause->literals[i]->list_of_watched_clauses[w]);
		}
		printf(")(d:");
		for(unsigned long d = 0; d < clause->literals[i]->num_dirty_watched_clauses; d++){
			printf("%ld,", clause->literals[i]->list_of_dirty_watched_clauses[d]);
		}

		printf(")(a:");
		if(sat_literal_var(clause->literals[i])->antecedent != 0)
			printf("%ld",sat_literal_var(clause->literals[i])->antecedent);
		printf(")(l:%ld)\t",  clause->literals[i]->decision_level);

	}


//	if(clause->L1 != NULL)
//		printf("Watching over L1: %ld\t", clause->L1->sindex);
//	if(clause->L2 != NULL)
//		printf("Watching over L2: %ld", clause->L2->sindex);


	if(clause->L1 != NULL && clause->L2!=NULL)
		printf("Watching over: %ld, %ld", clause->L1->sindex, clause->L2->sindex);
	else if(clause->L1 != NULL && clause->L2 == NULL)
		printf("Watching over: %ld", clause->L1->sindex);

	printf("\n");
}

void print_watching_clauses_in_list(Lit* lit){
	printf("watching clauses on lit:%ld, ", lit->sindex);
	for(unsigned long d = 0; d < lit->num_watched_clauses; d++){
		printf("%ld\t",lit->list_of_watched_clauses[d]);
	}
	printf("\n");
}

void print_all_clauses(SatState* sat_state){
	printf("---------------------------------\n");
	printf("Debugging all clauses: at level %ld\n", sat_state->current_decision_level);
	for(unsigned long i =0; i< sat_state->num_clauses_in_delta; i++){
		print_clause(&sat_state->delta[i]);
	}

//	for(unsigned long i =0; i<sat_state->num_variables_in_cnf; i++){
//		Var* var = &sat_state->variables[i];
//		Lit* poslit = var->posLit;
//		print_watching_clauses_in_list(poslit);
//		Lit* neglit = var->negLit;
//		print_watching_clauses_in_list(neglit);
//	}
	printf("---------------------------------\n");
}

void print_current_decisions(SatState* sat_state){
	printf("Current decisions: ");
	for(unsigned long i = 0; i< sat_state->num_literals_in_decision; i++){
		printf("%ld\t", sat_state->decisions[i]->sindex);
	}
	printf("\n");
}

void print_clause_containing_literal(Lit* lit){
	printf("Clauses containing this literal: %ld:\t", lit->sindex);
	for(unsigned long i =0; i< lit->num_containing_clause; i++){
		printf("%ld\t", lit->list_of_containing_clauses[i]);
	}
	printf("\n");
}


/******************************************************************************
 * We explain here the functions you need to implement
 *
 * Rules:
 * --You cannot change any parts of the function signatures
 * --You can/should define auxiliary functions to help implementation
 * --You can implement the functions in different files if you wish
 * --That is, you do not need to put everything in a single file
 * --You should carefully read the descriptions and must follow each requirement
 ******************************************************************************/

/******************************************************************************
 * Variables
 ******************************************************************************/

//returns a variable structure for the corresponding index
// index starts from 1 to n
Var* sat_index2var(c2dSize index, const SatState* sat_state) {
#ifdef TEST_C2D
	printf("Calling sat_index2var\n"); fflush(stdout);
#endif
	return &sat_state->variables[index-1];
}

//returns the index of a variable
c2dSize sat_var_index(const Var* var) {
#ifdef TEST_C2D
	printf("Calling sat_var_index\n"); fflush(stdout);
#endif
	return var->index;
}

//returns the variable of a literal
Var* sat_literal_var(const Lit* lit) {
#ifdef TEST_C2D
	printf("Calling sat_literal_var\n"); fflush(stdout);
#endif
//ifdef DEBUG
//	printf("sat_literal_var: get variable for lit %ld which is var: %ld\n",lit->sindex,lit->variable->index);
//#endif
	return lit->variable;
}

//returns 1 if the variable is instantiated, 0 otherwise
//a variable is instantiated either by decision or implication (by unit resolution)
BOOLEAN sat_instantiated_var(const Var* var) {
#ifdef TEST_C2D
	printf("Calling sat_instantiated_var\n"); fflush(stdout);
#endif
	// if the positive literal and the negative literal of the variable are set then the variable is instantiated
	if(var->negLit->LitState == 1 && var->posLit->LitState == 1)
		return 1;
	else
		return 0;
}

//returns 1 if all the clauses mentioning the variable are subsumed, 0 otherwise
BOOLEAN sat_irrelevant_var(const Var* var) {
#ifdef TEST_C2D
	printf("Calling sat_irrelevant_var\n"); fflush(stdout);
#endif
	//This implementation is suggested by the TA
	for(c2dSize i=0; i<sat_var_occurences(var); i++) {
	    Clause* clause = sat_clause_of_var(i,var);
	    if(!sat_subsumed_clause(clause)) return 0;
	  }
	  return 1;

//	c2dSize i;
//	SatState* sat_state = var->sat_state;
//
//	for(i =0; i < var->num_of_clauses_of_variables; i++){
//		Clause* clause = sat_index2clause(var->list_clause_of_variables[i], sat_state);
//		if(sat_subsumed_clause(clause))
//			continue;
//		else
//			break;
//	}
//	if(i == var->num_of_clauses_of_variables)
//		return 1;
//	else
//		return 0;
}

//returns the number of variables in the cnf of sat state
c2dSize sat_var_count(const SatState* sat_state) {
#ifdef TEST_C2D
	printf("Calling sat_var_count\n"); fflush(stdout);
#endif
	return sat_state->num_variables_in_cnf;
}

//returns the number of clauses mentioning a variable
//a variable is mentioned by a clause if one of its literals appears in the clause
c2dSize sat_var_occurences(const Var* var) {
#ifdef TEST_C2D
	printf("Calling sat_var_occurences\n"); fflush(stdout);
#endif
	// should return only the occurrences in the original CNF
	return var->num_of_clauses_of_variables_in_cnf;
}

//returns the index^th clause that mentions a variable
//index starts from 0, and is less than the number of clauses mentioning the variable
//this cannot be called on a variable that is not mentioned by any clause
Clause* sat_clause_of_var(c2dSize index, const Var* var) {
#ifdef TEST_C2D
	printf("Calling sat_clause_of_var\n"); fflush(stdout);
#endif
	assert(index < var->num_of_clauses_of_variables_in_cnf);

#ifdef DEBUG
	printf("sat_clause_of_var:index = %ld, num_of_clauses_of_variables_in_cnf= %ld\n", index,var->num_of_clauses_of_variables_in_cnf );
#endif

	if(index < var->num_of_clauses_of_variables_in_cnf){

		return sat_index2clause(var->list_clause_of_variables_in_cnf[index], var->sat_state) ;
	}
	else{
		return NULL;
	}

}

/******************************************************************************
 * Literals 
 ******************************************************************************/

BOOLEAN sat_is_asserted_literal(Lit* lit){
#ifdef TEST_C2D
	printf("Calling sat_is_asserted_literal\n"); fflush(stdout);
#endif
	if(lit->sindex<0){
		switch(lit->LitValue){
		case 0:
			assert(lit->LitState == 1); // asserted --> ie negative literal and has value false then it will be evaluated to true
			return 1;
			break;
		case 1:
			assert(lit->LitState == 1);
			return 0;
			break;
		default:
		//	assert(lit->LitState == 0);
			return 0;
			break;
		}
	}
	else if(lit->sindex > 0){
		switch(lit->LitValue){
		case 1:
			assert(lit->LitState == 1);
			return 1;
			break;
		case 0:
			assert(lit->LitState == 1);
			return 0;
			break;
		default:
			//assert(lit->LitState == 0);
			return 0;
			break;
		}
	}

	return 0;

}

BOOLEAN sat_is_resolved_literal(Lit* lit){
#ifdef TEST_C2D
	printf("Calling sat_is_resolved_literal\n"); fflush(stdout);
#endif
	if(lit->sindex<0){
		switch(lit->LitValue){
		case 1:
			assert(lit->LitState == 1); // resolved --> ie negative literal and has value true then it will be evaluated to false
			return 1;
			break;
		case 0:
			assert(lit->LitState == 1);
			return 0;
			break;
		default:
			//assert(lit->LitState == 0);
			return 0;
			break;
		}
	}
	else if(lit->sindex > 0){
		switch(lit->LitValue){
		case 0:
			assert(lit->LitState == 1);
			return 1;
			break;
		case 1:
			assert(lit->LitState == 1);
			return 0;
			break;
		default:
			//assert(lit->LitState == 0);
			return 0;
			break;
		}
	}

	return 0;
}

//returns a literal structure for the corresponding index
Lit* sat_index2literal(c2dLiteral index, const SatState* sat_state) {
#ifdef TEST_C2D
	printf("Calling sat_index2literal\n"); fflush(stdout);
#endif
	Lit * lit = NULL;
	Var corresponding_var = sat_state->variables[abs(index) - 1];
	if (index < 0) {
		lit = corresponding_var.negLit;
	} else if (index > 0) {
		lit = corresponding_var.posLit;
	}

	return lit;
}

//returns the index of a literal
c2dLiteral sat_literal_index(const Lit* lit) {
#ifdef TEST_C2D
	printf("Calling sat_literal_index\n"); fflush(stdout);
#endif
	return lit->sindex;
}

//returns the positive literal of a variable
Lit* sat_pos_literal(const Var* var) {
#ifdef TEST_C2D
	printf("Calling sat_pos_literal\n"); fflush(stdout);
#endif
	return var->posLit;
}

//returns the negative literal of a variable
Lit* sat_neg_literal(const Var* var) {
#ifdef TEST_C2D
	printf("Calling sat_neg_literal\n"); fflush(stdout);
#endif
	return var->negLit;
}

//returns 1 if the literal is implied, 0 otherwise
//a literal is implied by deciding its variable, or by inference using unit resolution
BOOLEAN sat_implied_literal(const Lit* lit) {
#ifdef TEST_C2D
	printf("Calling sat_implied_literal\n"); fflush(stdout);
#endif

	return sat_is_asserted_literal(lit);
}

//sets the literal to true, and then runs unit resolution
//returns a learned clause if unit resolution detected a contradiction, NULL otherwise
//
//if the current decision level is L in the beginning of the call, it should be updated 
//to L+1 so that the decision level of lit and all other literals implied by unit resolution is L+1
Clause* sat_decide_literal(Lit* lit, SatState* sat_state) {
#ifdef TEST_C2D
	printf("Calling sat_decide_literal\n"); fflush(stdout);
#endif
	assert(lit!=NULL);

	lit->LitState = 1;

	// Update the decision level
	sat_state->current_decision_level ++;
	lit->decision_level = sat_state->current_decision_level;


	//update the literal parameters
	//Set lit values
	if(lit->sindex < 0){
		lit->LitValue = 0;
		Lit* opposite_lit = sat_literal_var(lit)->posLit;
		opposite_lit->LitValue = 0;
		opposite_lit->LitState = 1;
		opposite_lit->decision_level = lit->decision_level;
	}
	else if (lit->sindex > 0){
		lit->LitValue = 1;
		Lit* opposite_lit = sat_literal_var(lit)->negLit;
		opposite_lit->LitValue = 1;
		opposite_lit->LitState = 1;
		opposite_lit->decision_level = lit->decision_level;
	}



	// here update the decision array of the sat_state
	sat_state->decisions[sat_state->num_literals_in_decision++] = lit;

	FLAG_CASE1_UNIT_RESOLUTION = 1;

	BOOLEAN success = sat_unit_resolution(sat_state);

#ifdef DEBUG
	printf("Is unit resolution succeeded: %ld\n",success);
#endif

	if(!success){
		Clause* learnt  = CDCL_non_chronological_backtracking_first_UIP(sat_state);
		return learnt;
	}
	else
		return NULL;
}

//undoes the last literal decision and the corresponding implications obtained by unit resolution
//
//if the current decision level is L in the beginning of the call, it should be updated 
//to L-1 before the call ends
void sat_undo_decide_literal(SatState* sat_state) {
#ifdef TEST_C2D
	printf("Calling sat_undo_decide_literal\n"); fflush(stdout);
#endif
	sat_undo_unit_resolution(sat_state);
}

/******************************************************************************
 * Clauses 
 ******************************************************************************/

//returns a clause structure for the corresponding index
Clause* sat_index2clause(c2dSize index, const SatState* sat_state) {
#ifdef TEST_C2D
	printf("Calling sat_index2clause\n"); fflush(stdout);
#endif
	assert(index <= sat_state->num_clauses_in_delta);
	return &(sat_state->delta[index - 1]);
}




//returns the index of a clause
c2dSize sat_clause_index(const Clause* clause) {
#ifdef TEST_C2D
	printf("Calling sat_clause_index\n"); fflush(stdout);
#endif
	return clause->cindex;
}

//returns the literals of a clause
Lit** sat_clause_literals(const Clause* clause) {
#ifdef TEST_C2D
	printf("Calling sat_clause_literals\n"); fflush(stdout);
#endif
	return clause->literals;
}

//returns the number of literals in a clause
c2dSize sat_clause_size(const Clause* clause) {
#ifdef TEST_C2D
	printf("Calling sat_clause_size\n"); fflush(stdout);
#endif
	return clause->num_literals_in_clause;
}

//returns 1 if the clause is subsumed, 0 otherwise
BOOLEAN sat_subsumed_clause(const Clause* clause) {
#ifdef TEST_C2D
	printf("Calling sat_subsumed_clause\n"); fflush(stdout);
#endif
	return clause->is_subsumed; //TODO: don't forget to update the state of the clause after unit resolution
}

//returns the number of clauses in the cnf of sat state
c2dSize sat_clause_count(const SatState* sat_state) {
#ifdef TEST_C2D
	printf("Calling sat_clause_count\n"); fflush(stdout);
#endif
	return sat_state->num_clauses_in_cnf;
}

//returns the number of learned clauses in a sat state (0 when the sat state is constructed)
c2dSize sat_learned_clause_count(const SatState* sat_state) {
#ifdef TEST_C2D
	printf("Calling sat_learned_clause_count\n"); fflush(stdout);
#endif
	return sat_state->num_clauses_in_gamma;
}

//adds clause to the set of learned clauses, and runs unit resolution
//returns a learned clause if unit resolution finds a contradiction, NULL otherwise
//
//this function is called on a clause returned by sat_decide_literal() or sat_assert_clause()
//moreover, it should be called only if sat_at_assertion_level() succeeds
Clause* sat_assert_clause(Clause* clause, SatState* sat_state) {
#ifdef TEST_C2D
	printf("Calling sat_assert_clause\n"); fflush(stdout);
#endif
	update_vsids_scores(sat_state); // it uses alpha

	//learn clause
	// update the gamma list with the new alpha (just for performance analysis)
	add_clause_to_gamma(sat_state, clause); // it adds alpha to gamma and clears it


	Lit* free_lit = NULL;
	unsigned long num_free_literals = 0;
	// Check if the learnt clause is unit clause (the last added clause in delta) then add it directly to decisions
	for(unsigned long litidx =0; litidx < clause->num_literals_in_clause; litidx++){
		if(clause->literals[litidx]->LitState == 0){
			num_free_literals++;
			free_lit = clause->literals[litidx];
		}
	}

	assert(num_free_literals == 1);

	//update the literal parameters (decide it)
	//Set lit values
	if(free_lit->sindex < 0)
		free_lit->LitValue = 0;
	else if (free_lit->sindex > 0)
		free_lit->LitValue = 1;

	free_lit->decision_level = sat_state->current_decision_level; //1; // because it is unit (not a decision)
	free_lit->LitState = 1;

	//add it in the decision list without incrementing the decision level
	sat_state->decisions[sat_state->num_literals_in_decision++] = free_lit;
	if(clause->num_literals_in_clause == 1){
		sat_literal_var(free_lit)->antecedent=0;
	}
	else{
		sat_literal_var(free_lit)->antecedent=clause->cindex;
	}

//	if(sat_state->delta[sat_state->num_clauses_in_delta -1].num_literals_in_clause == 1){
//		Clause* unit_clause = &sat_state->delta[sat_state->num_clauses_in_delta -1];
//#ifdef DEBUG
//		printf("sat_assert_clause: the learnt clause %ld is a unit clause with literal %ld\n", unit_clause->cindex, unit_clause->literals[0]->sindex );
//#endif
//		// a unit clause
//		Lit* unit_lit = unit_clause->literals[0];
//		//update the literal parameters (decide it)
//		//Set lit values
//		if(unit_lit->sindex < 0)
//			unit_lit->LitValue = 0;
//		else if (unit_lit->sindex > 0)
//			unit_lit->LitValue = 1;
//
//		unit_lit->decision_level = sat_state->current_decision_level; //1; // because it is unit (not a decision)
//		unit_lit->LitState = 1;
//
//		//add it in the decision list without incrementing the decision level
//		sat_state->decisions[sat_state->num_literals_in_decision++] = unit_lit;
//	}
//	else{
//
//	}

	//TODO: decrease the current decision level of sat_state here before run unit resolution.
	// This is done here due to the way the main is constructed.
	// I can't decrease the current level in the undo_unit_resolution because afterwards I am checking at_asserting_level
	//sat_state->current_decision_level -- ;

#ifdef DEBUG
	printf("----- sat _assert_clause ---- before running unit resolution\n");
	print_all_clauses(sat_state);
#endif


	if(sat_state->num_literals_in_decision == 0){ //backtracking went to nothing
		return NULL;  //i.e. no new clause is learnt and decide a new literal
	}
	else
		FLAG_CASE2_UNIT_RESOLUTION = 1;

	BOOLEAN success = sat_unit_resolution(sat_state); //should use case 2 --> runs only on the decisions in the past


	if(!success){
		CDCL_non_chronological_backtracking_first_UIP(sat_state);
	}

	return sat_state->alpha;
}


//added API: Update the state of the clause if a literal is decided or implied i.e. asserted
void sat_update_clauses_state(Lit* lit, SatState* sat_state){
#ifdef TEST_C2D
	printf("Calling sat_update_clauses_state\n"); fflush(stdout);
#endif
	if(sat_is_asserted_literal(lit)){
		for(unsigned long i =0; i<lit->num_containing_clause; i++){
			Clause* clause = sat_index2clause(lit->list_of_containing_clauses[i], sat_state);
			clause->is_subsumed = 1;
#ifdef DEBUG
			printf("Clause: %ld is subsumed now\n", clause->cindex);
#endif
		}
	}
}
//added API: Undo state of clause of undecided literal which was asserted called at undo unit resolution took place
void sat_undo_clauses_state(Lit* lit, SatState* sat_state){
#ifdef TEST_C2D
	printf("Calling sat_undo_clauses_state\n"); fflush(stdout);
#endif
	if(sat_is_asserted_literal(lit)){
		for(unsigned long i =0; i<lit->num_containing_clause; i++){
			Clause* clause = sat_index2clause(lit->list_of_containing_clauses[i], sat_state);;
			clause->is_subsumed = 0;
		}
	}

}


/******************************************************************************
 * A SatState should keep track of pretty much everything you will need to
 * condition/uncondition variables, perform unit resolution, and do clause learning
 *
 * Given an input cnf file you should construct a SatState
 *
 * This construction will depend on how you define a SatState
 * Still, you should at least do the following:
 * --read a cnf (in DIMACS format, possible with weights) from the file
 * --initialize variables (n of them)
 * --initialize literals  (2n of them)
 * --initialize clauses   (m of them)
 *
 * Once a SatState is constructed, all of the functions that work on a SatState
 * should be ready to use
 *
 * You should also write a function that frees the memory allocated by a
 * SatState (sat_state_free)
 ******************************************************************************/

//constructs a SatState from an input cnf file
SatState* sat_state_new(const char* file_name) {
#ifdef TEST_C2D
	printf("Calling sat_state_new\n"); fflush(stdout);
#endif
	//initialization
  SatState* sat_state = (SatState *) malloc (sizeof (SatState));

  //sat_state->decisions, sat_state->variables, sat_state->implications, sat_state->delta
  // and their corresponding parameters are set in parseDIMACS

  sat_state->gamma = (unsigned long*) malloc(sizeof(unsigned long)); // will be realloc when it expands
  sat_state->alpha = (Clause *) malloc(sizeof(Clause));

  sat_state->current_decision_level = 1; // this is by description
  sat_state->num_clauses_in_delta = 0;

  sat_state->num_clauses_in_gamma = 0;
  sat_state->max_size_list_gamma = 1;

  sat_state->num_literals_in_decision = 0;
  sat_state->num_literals_in_implications = 0;
  sat_state->num_variables_in_cnf = 0;


  FILE* cnf_file = fopen(file_name, "r");
  
  if (cnf_file == 0){
	  perror("Cannot open the CNF file");
	  exit(-1);
  }
  else{
	  // call the parser
	  parseDIMACS(cnf_file, sat_state);
#ifdef DEBUG
	  printf("Number of clauses in delta = %ld\n",sat_state->num_clauses_in_delta);

#endif
  }

  fclose(cnf_file);


  initialize_vsids_scores(sat_state);

  return sat_state;
}

//frees the SatState
void sat_state_free(SatState* sat_state) {
#ifdef TEST_C2D
	printf("Calling sat_state_free\n"); fflush(stdout);
#endif

#ifdef DEBUG
	printf("Start freeing sat ----\n");
#endif
	// all three of these are guaranteed to be allocated
	// based on the implementation of sat_state_new()
	FREE(sat_state->delta);
	FREE(sat_state->gamma);
	FREE(sat_state->alpha);

	// clean up the variables
	for(unsigned long vidx = 0; vidx < sat_state->num_variables_in_cnf; vidx++) {
		variable_cleanup(sat_state->variables + vidx);
	}

	// free the variables
	FREE(sat_state->variables);
}

// void clause_cleanup (Clause * clause) {
// 	// can ignore the literals variable
// 	// as literals exist outside clauses
// 
// 	// can ignore the L1 and L2 pointers for the same reason
// 
// 	// right now, this function should do nothing!
// }

// FREEs everything within a variable, but not the variable itself
void variable_cleanup (Var * variable) {
#ifdef TEST_C2D
//	printf("Calling variable_cleanup\n"); fflush(stdout);
#endif
	// right now the only allocated objects within a variable are
	// the literals to which it points:
	literal_free(variable->posLit);
	literal_free(variable->negLit);
	FREE(variable->list_clause_of_variables);
}

void literal_free (Lit * literal) {
#ifdef TEST_C2D
	//printf("Calling literal_free\n"); fflush(stdout);
#endif
	FREE(literal->list_of_watched_clauses);
	FREE(literal->list_of_dirty_watched_clauses);
	FREE(literal->list_of_containing_clauses);

	// no need to free the antecedent clause though,
	// as it may exist outside this literal
	// (either in delta, gamma, alpha, or conflict_clause)

	FREE(literal);
}

/******************************************************************************
 * Given a SatState, which should contain data related to the current setting
 * (i.e., decided literals, subsumed clauses, decision level, etc.), this function 
 * should perform unit resolution at the current decision level 
 *
 * It returns 1 if succeeds, 0 otherwise (after constructing an asserting
 * clause)
 *
 * There are three possible places where you should perform unit resolution:
 * (1) after deciding on a new literal (i.e., in sat_decide_literal())
 * (2) after adding an asserting clause (i.e., in sat_assert_clause(...)) 
 * (3) neither the above, which would imply literals appearing in unit clauses
 *
 * (3) would typically happen only once and before the other two cases
 * It may be useful to distinguish between the above three cases
 * 
 * Note if the current decision level is L, then the literals implied by unit
 * resolution must have decision level L
 *
 * This implies that there must be a start level S, which will be the level
 * where the decision sequence would be empty
 *
 * We require you to choose S as 1, then literals implied by (3) would have 1 as
 * their decision level (this level will also be the assertion level of unit
 * clauses)
 *
 * Yet, the first decided literal must have 2 as its decision level
 ******************************************************************************/
static void add_literal_to_list(Lit** list, Lit* lit, unsigned long* capacity, unsigned long* num_elements){
#ifdef TEST_C2D
	printf("Calling add_literal_to_list\n"); fflush(stdout);
#endif
	unsigned long cap = *capacity;
	unsigned long num = *num_elements;

	if(num >= cap){
		// needs to realloc the size
		cap =num * MALLOC_GROWTH_RATE;
		list = (Lit**) realloc(list , sizeof(Lit*) * cap);
	}

		list[num++] = lit;

		*num_elements = num;
		*capacity = cap;
}
static BOOLEAN unit_resolution_case_1(SatState* sat_state){
//#ifdef TEST_C2D
	//printf("Calling unit_resolution_case_1\n"); fflush(stdout);
//#endif

#ifdef DEBUG
	printf("Unit resolution case 1\n");
#endif
	// This is called after decide literal
		FLAG_CASE1_UNIT_RESOLUTION = 0;


		//prepare the list of literals on which the unit resolution will run
		//loop on all decided literals that have the same last level
		//TODO: do I really need to malloc
		//Lit** literals_in_last_decision = (Lit**)malloc(sizeof(Lit*));
		//unsigned long max_size_last_decision_list = 1;
		//unsigned long num_last_decision_lit = 0;


//	#ifdef DEBUG
//		printf("Number of literals in the decision array = %ld ", sat_state->num_literals_in_decision);
//	#endif

		//for(unsigned long i =0; i<sat_state->num_literals_in_decision; i++){
		//	if(sat_state->decisions[i]->decision_level != sat_state->current_decision_level)
		//		continue;
		//	else
			 //TODO: Enhance: I can just add all the elements after this point without having to check the level of each one again because levels are incremental
				//while(i < sat_state->num_literals_in_decision){
		//			add_literal_to_list(literals_in_last_decision, sat_state->decisions[i], &max_size_last_decision_list, &num_last_decision_lit);
				//	i++;
				//}

//		}
//	#ifdef DEBUG
//		printf("number of literals in the last decision level: %ld\n", num_last_decision_lit);
//	#endif


//#ifdef DEBUG
	//printf("Decisions in the list for the two literal watch: ");
	//for(unsigned long i =0; i<num_last_decision_lit; i++){
	//	printf("%ld\t", literals_in_last_decision[i]->sindex);
	//}
	//printf("\n");
//#endif
		// run the two literal watch based on the new decision
		//BOOLEAN ret = two_literal_watch(sat_state, literals_in_last_decision, num_last_decision_lit);
		BOOLEAN ret = two_literal_watch(sat_state, sat_state->decisions, sat_state->num_literals_in_decision, CASE1);

#ifdef DEBUG
	printf("----- sat _decide_literal ---- after running unit resolution\n");
	print_all_clauses(sat_state);
	print_current_decisions(sat_state);
	printf("-------------------------------------------------------------");
#endif

	return ret;
}

static BOOLEAN unit_resolution_case_2(SatState* sat_state){
	// this is called after adding an asserting clause
	//reset the flag
#ifdef DEBUG
	printf("Unit resolution case 2\n");
	//reset the flag

	//prepare the list of literals on which the unit resolution will run
//	Lit** literals_in_decision = (Lit**)malloc(sizeof(Lit*) * sat_state->num_literals_in_decision );
//	unsigned long max_size_decision_list = 1;
//	unsigned long num_decision_lit = 0;
//

	//pass all the decision list again because the last level is cleared after the contradiction
//	literals_in_decision = s
#endif
	FLAG_CASE2_UNIT_RESOLUTION = 0;

	//prepare the list of literals on which the unit resolution will run
//	Lit** literals_in_decision = (Lit**)malloc(sizeof(Lit*) * sat_state->num_literals_in_decision );
//	unsigned long max_size_decision_list = 1;
//	unsigned long num_decision_lit = 0;
//

	//pass all the decision list again because the last level is cleared after the contradiction
//	literals_in_decision = sat_state->decisions;
//	max_size_decision_list = sat_state->num_literals_in_decision; // for now just put max size = num_literals
//	num_decision_lit = sat_state->num_literals_in_decision;


#ifdef DEBUG
	printf("Decisions in the list for the two literal watch: ");
	for(unsigned long i =0; i<sat_state->num_literals_in_decision; i++){
		printf("%ld\t", sat_state->decisions[i]->sindex);
	}
	printf("\n");
#endif

	BOOLEAN ret = two_literal_watch(sat_state,sat_state->decisions, sat_state->num_literals_in_decision, CASE2);

#ifdef DEBUG
	printf("----- sat _assert_clause ---- after running unit resolution\n");
	print_all_clauses(sat_state);
	print_current_decisions(sat_state);
	printf("-------------------------------------------------------------");
#endif

	return  ret;

}

static BOOLEAN unit_resolution_case_3(SatState* sat_state){
#ifdef DEBUG
	printf("Unit resolution case 3\n");
#endif
	//Reset the flag in this case because I don't want to execute this unless you have a unit clause
	FLAG_CASE3_UNIT_RESOLUTION = 0;

	// The first time it is called which is case 3 is before any decision or adding assertion clause.
	// This means search in the clauses for unit literals, if found, add it in a list and decide literal on it and run unit resolution.
	for(unsigned long i=0; i<sat_state->num_clauses_in_delta; i++){
		Clause* cur_clause = &(sat_state->delta[i]);
		if (cur_clause->num_literals_in_clause == 1){
			// a unit clause
			//TODO: correct this case
			Lit* unit_lit = cur_clause->literals[0];

			if(sat_is_resolved_literal(unit_lit)){
				return 0; //contradiction
			}
			//update the literal parameters (decide it)
			//Set lit values
			if(unit_lit->sindex < 0){
				unit_lit->LitValue = 0;
				unit_lit->decision_level = 1; // because it is unit (not a decision)
				unit_lit->LitState = 1;
				Lit* opposite_lit = sat_literal_var(unit_lit)->posLit;
				opposite_lit->LitValue = 0;
				opposite_lit->LitState = 1;
				opposite_lit->decision_level = 1;
			}
			else if (unit_lit->sindex > 0){
				unit_lit->LitValue = 1;
				unit_lit->decision_level = 1; // because it is unit (not a decision)
				unit_lit->LitState = 1;
				Lit* opposite_lit = sat_literal_var(unit_lit)->negLit;
				opposite_lit->LitValue = 1;
				opposite_lit->LitState = 1;
				opposite_lit->decision_level = unit_lit->decision_level;
			}



			//add it in the decision list without incrementing the decision level
			sat_state->decisions[sat_state->num_literals_in_decision++] = unit_lit;
			sat_state->current_decision_level = 1;
		}
	}
	// now the decision list is updated with all asserted unit literals
	// run the two literal watch algorithm
	if(sat_state->num_literals_in_decision == 0){
		// there was no unit clause
		return 1; //just return and takes the next step
	}
	else{
		//prepare the list of literals on which the unit resolution will run
		//TODO: Do I really need to malloc
//		Lit** literals_in_decision = (Lit**)malloc(sizeof(Lit*) * sat_state->num_literals_in_decision);
//		unsigned long max_size_decision_list = sat_state->num_literals_in_decision; // max_size = num_literals for now
//		unsigned long num_decision_lit = sat_state->num_literals_in_decision;

		//literals_in_decision = sat_state->decisions;
		return two_literal_watch(sat_state, sat_state->decisions,sat_state->num_literals_in_decision, CASE3);
	}

}
//applies unit resolution to the cnf of sat state
//returns 1 if unit resolution succeeds, 0 if it finds a contradiction
BOOLEAN sat_unit_resolution(SatState* sat_state) {
#ifdef TEST_C2D
	printf("Calling sat_unit_resolution\n"); fflush(stdout);
#endif
	if(FLAG_CASE3_UNIT_RESOLUTION == 1){
		return unit_resolution_case_3(sat_state);
	}
	else if(FLAG_CASE2_UNIT_RESOLUTION == 1){
		return unit_resolution_case_2(sat_state);
	}
	else if(FLAG_CASE1_UNIT_RESOLUTION == 1){
		return unit_resolution_case_1(sat_state);
	}
	else
		return 0;
}

//undoes sat_unit_resolution(), leading to un-instantiating variables that have been instantiated
//after sat_unit_resolution()
void sat_undo_unit_resolution(SatState* sat_state) {
#ifdef TEST_C2D
	printf("Calling sat_undo_unit_resolution\n"); fflush(stdout);
#endif

#ifdef DEBUG
	printf("Undo unit resolution:\n");
#endif
	unsigned long num_reduced_decisions = 0;
	// undo the set literals at the current decision level and its complement
#ifdef DEBUG
	printf("Number of literals in decisions = %ld\n", sat_state->num_literals_in_decision);
	print_all_clauses(sat_state);
#endif

// since the order of the decision level in the decision list is not necessarily in ascending order
// so to avoid the headache of removing from inside a list I remove all the elements after the occurence of the current decision
// later on the correct implications will be implied again

	for(unsigned long i = 0; i < sat_state->num_literals_in_decision; i++){
#ifdef DEBUG
		printf("Checking decision: %ld at level %ld\n",sat_state->decisions[i]->sindex, sat_state->decisions[i]->decision_level );
#endif

		if(sat_state->decisions[i]->decision_level == sat_state->current_decision_level ){
#ifdef DEBUG
			printf("clean literal: %ld\n and any literal afterwards\n",sat_state->decisions[i]->sindex);
#endif
			// to avoid cleaning the literal twice
			//(because u may have the literal and its opposite in the decision at the contradiction)
			// I have to add this if statement (after the current modification i may remove this if)
			for(unsigned long cleanidx = i; cleanidx < sat_state->num_literals_in_decision; cleanidx++){
				if(sat_state->decisions[cleanidx]->LitState == 1){
					Var* var = sat_literal_var(sat_state->decisions[cleanidx]);
					var->antecedent = 0;
					sat_undo_clauses_state(sat_state->decisions[cleanidx], sat_state);

					Lit* poslit = var->posLit;
				//	poslit->decision_level = 1;  // don't undo it because of at_asserting_level in order not to overwrite the learning clause literals. This value will be overwritten later
					poslit->LitState = 0;
					poslit->LitValue = 'u';
					//poslit->num_watched_clauses = 0; // watched clauses should stay the same


					Lit* neglit = var->negLit;
				//	neglit->decision_level = 1;  // don't undo it because of at_asserting_level in order not to overwrite the learning clause literals
					neglit->LitState = 0;
					neglit->LitValue = 'u';
					//neglit->num_watched_clauses = 0; //watched clauses should stay the same

					num_reduced_decisions ++;
				}
			}
			break;
#ifdef DEBUG
			printf("Number of reduced decision: %ld\n",num_reduced_decisions);
#endif
		}
	}

	//update the current decision level
	sat_state->num_literals_in_decision = sat_state->num_literals_in_decision - num_reduced_decisions; // you reduce another one which is the implied driven literal at the end
#ifdef DEBUG
	for(unsigned long i = 0; i < sat_state->num_literals_in_decision; i++){
		printf("print decision list after undo: %ld at level %ld\n",sat_state->decisions[i]->sindex, sat_state->decisions[i]->decision_level );
	}
#endif

	//TODO: don't decrease this for now due to how the main function is constructed! the decision level is reduced after adding the asserting clause
	sat_state->current_decision_level -- ;

}

//returns 1 if the decision level of the sat state equals to the assertion level of clause,
//0 otherwise
//
//this function is called after sat_decide_literal() or sat_assert_clause() returns clause.
//it is used to decide whether the sat state is at the right decision level for adding clause.
BOOLEAN sat_at_assertion_level(const Clause* clause, const SatState* sat_state) {
#ifdef TEST_C2D
	printf("Calling sat_at_assertion_level\n"); fflush(stdout);
#endif

#ifdef DEBUG
	printf("At asserting level current sat level: %ld\n",sat_state->current_decision_level );
	printf(" alpha:\t");
	print_clause(clause);
#endif
	//The second highest level in a conflict-driven clause is
	//known as the assertion level of the clause
	unsigned long the_next_large_decision = 0;

	for(unsigned long i = 0; i < clause->num_literals_in_clause; i++){
		if(clause->literals[i]->decision_level > sat_state->current_decision_level)
			continue; // we want the next highest
		else if(clause->literals[i]->decision_level > the_next_large_decision ){
			the_next_large_decision = clause->literals[i]->decision_level;
		}
	}

	if(clause->num_literals_in_clause == 1 ) // I learnt a unit clause
		the_next_large_decision = 1; //back to where we start

#ifdef DEBUG
	printf("the asserting level of the alpha: %ld\n",the_next_large_decision);
#endif
	if(the_next_large_decision == sat_state->current_decision_level)
		return 1;
	else
		return 0;

//	for(unsigned long i = 0; i < sat_state->alpha->num_literals_in_clause; i++){
//		if(sat_state->alpha->literals[i]->decision_level == sat_state->current_decision_level)
//			return 1;
//		}
//	return 0;
}

/******************************************************************************
 * The functions below are already implemented for you and MUST STAY AS IS
 ******************************************************************************/

//returns the weight of a literal (which is 1 for our purposes)
c2dWmc sat_literal_weight(const Lit* lit) {
  return 1;
}

//returns 1 if a variable is marked, 0 otherwise
BOOLEAN sat_marked_var(const Var* var) {
  return var->mark;
}

//marks a variable (which is not marked already)
void sat_mark_var(Var* var) {
  var->mark = 1;
}

//unmarks a variable (which is marked already)
void sat_unmark_var(Var* var) {
  var->mark = 0;
}

//returns 1 if a clause is marked, 0 otherwise
BOOLEAN sat_marked_clause(const Clause* clause) {
  return clause->mark;
}

//marks a clause (which is not marked already)
void sat_mark_clause(Clause* clause) {
  clause->mark = 1;
}
//unmarks a clause (which is marked already)
void sat_unmark_clause(Clause* clause) {
  clause->mark = 0;
}

/******************************************************************************
 * end
 ******************************************************************************/
