CSCI 4707: Lab 4
==========

**Note:** I am making this repository public, since it appears that the course is not doing this lab 		anymore. I am not certain whether this is an assignment that is used at other schools. If it is, please contact me if 	you would like me to remove our solution and I will be happy to do so.


1. OBJECTIVE
------------
Implement the keyword IGNORE in postgreSQL. The general form of an IGNORE statement is as follows,

			SELECT (target list)
			FROM (table)
			WHERE (condition clause)
			IGNORE k

where k is a positive integer. When IGNORE is used, the first k tuples of the result are ommitted before the rest of 		the result is returned. 

IGNORE behaves like LIMIT/OFFSET when only offset is used (eg. SELECT / FROM / WHERE / OFFSET k is pretty much 		IGNORE.) There's a post about it on the moodle forum. So, all of the IGNORE code is very similar to the OFFSET
code, with changes as necessary. 
    
2. EDITED FILES
---------------

A list of files edited and the rationale behind the edits, as well as the file locations in the postgresql-9.2.1 		directory.

#### 2.1 PARSER

1. **/src/backend/parser/gram.y**: Contains the PostgreSQL grammar.
	Should be working for simple IGNORE statements, as listed below.
	
2. **/src/include/parser/kwlist.h**: Contains the added IGNORE keyword.
	This file contains a simple list of keywords in alphabetical order.
	
3. **/src/include/nodes/parsenodes.h**: Adds a node for the IGNORE clause 
	(*ignoreClause) to the Query struct so that it can be formulated into 
	a query tree for further processing by the rewriter and the planner. 
	Also adds an ignoreClause node to the struct for SELECT statements.
	
4. **/src/backend/parser/analyze.c**: Transforms an *ignoreClause node into 
	something that can be parsed by the query tree. Adds *ignoreClause 
    	to general SELECT statements in isGeneralSelect, and a call to
    	transformLimitClause for *ignoreClause. See the following file for 
    	further explaination. 

5. **/src/backend/parser/parse_clause.c**: This file did not actually need 
	to be edited per the lab handout, since transformLimitClause is 
	actually a general method designed to transform LIMIT and 'any 
	allied expression' into type bigint. Thus, IGNORE / *ignoreClause can be 
	transformed in analyze.c using this method.
	
#### 2.2 PLANNER

1. **/src/backend/optimizer/plan/planner.c**: Added preprocess_IgnoreClause. 
	Prepares IGNORE for the query tree. Similar to the LIMIT/OFFSET code, but changed
	to be for IGNORE. See comments in the code for specifics.

2. **/src/backend/optimizer/plan/createplan.c**: Builds an Ignore plan node.

3. **/src/backend/optimizer/plan/setrefs.c**: Added code for IGNORE which is 
	very similar to the code for OFFSET.

4. **/src/include/nodes/plannodes.h**: Definition of the Ignore node struct for the 
    	planner.

5. **/src/include/nodes/nodes.h**: Just adds the Ignore nodes to a list of nodes.

6. **/src/include/optimizer/planmain.h**: Prototype of make_Ignore which is used 
	in createplan.c

#### 2.3 EXECUTOR

1. **/src/backend/executor/nodeIgnore.c**: Contains the Definition of IGNORE. It 
	contains the basic function to Execute Ignore such as ExecIgnore, ExecInitIgnore 
	and ExecEndIgnore. This extracts a range of tuples, initilizes the nodes and 
	subnodes and then shutsdown the nodes and subnodes after execution. 

2. **/src/backend/executor/execProcnode.c**: Indicates which process should execute 
	when given an Ignore node. Updated to include nodeIgnore.h. Added case for the 
	Ignore node so the executor knows which states to run on each of the ExecInit, 
	ExecProcNode, and ExecEndNode. ExecInit to run the ExecInitIgnore from the 
	previous step. ExecProcNode to execute ExecIgnore which executes the given node. 
	ExecEndNode to execute the function ExecEndIgnore so it can clean up all the nodes.

3. **/src/backend/executor/MakeFile**: Updated to include nodeIgnore.c and 
	nodeIgnore.h.

4. **/src/backend/nodes/copyfuncs.c**: Added _copyIgnore. Copies the ignore node from 
	the superclass and copies the remainder of the node. 

5. **/src/backend/nodes/equalfuncs.c**: Added some calls for any ignore nodes. 
	Added calls for ignore in _equalQuery and _equalSelectStmt.  

6. **/src/backend/nodes/outfuncs.c**: Added _outIgnore. Added an output function for 
	ignore. 

7. **/src/include/executor/nodeIgnore.h**: Header file for nodeIgnore.c.

8. **/src/backend/nodes/nodeFuncs.c**: Not in the lab handout, but needed to 
	be edited to avoid crashing/compiling without errors. Added two lines for 
	IGNORE clauses. 

9. **/src/backend/nodes/readfuncs.c**: Not in lab handout, but needed to avoid compling 
	errors. Added only 1 line for IGNORE. This file was necessary to edit because 
	every node that has a output function must also have a input function. 

10. **/src/include/nodes/execnodes.h**: Added some structs for IgnoreState in order to 
	perform initialize, rescan, empty, inwindow, subplaneog, windowend, and windowstart
	on an Ignore node. These are all necessary functions for executing  


3. TESTING
----------

The test cases that we tried were simple SQL queries of the following form:

		SELECT (target list)
		FROM (table)
		WHERE (condition clause)
		IGNORE k

These cases worked correctly for positive integer and float values of k. See below for special cases, and their results. 

Note that IGNORE will NOT work with any additional clauses other than those defined in the gram.y file as belonging to a 'simple select'. For example, if we were to run the above query but want to add an additional LIMIT clause, or GROUP BY clause, it will not work and a syntax error will be returned. We also cannot have an IGNORE clause in a nested select statement.

The reason for this is that we have defined only one such variation of a SELECT query with an IGNORE clause in the grammar of the database. It is possible to add additional cases within the gram.y file so that IGNORE can be run with other clauses, with very minimal to no editing of additional files; the reason we did not implement these cases, however, was due to strange precedence errors that would result. We wanted to make sure that we could properly implement simple SELECT...IGNORE queries such as the one above, and didn't have time to implement any others.

We also tested a number of special cases, which are as follows:

1. **IGNORE for k = 0** : Returns the correct result, which is equivalent to having no IGNORE clause in the SELECT query. No rows are ignored, and the resulting table is returned as usual according to any other constraints.

2. **IGNORE for k < 0** : If we enter a negative k value, the result will be an error message, "ERROR: IGNORE must not be negative". This is the correct behavior.

3. **IGNORE for k greater than the number of tuples in the FROM table (or the number of tuples returned by the conditions in the SELECT and WHERE clauses)** : This query will return an empty table. This is correct, since ignoring more rows than are in a table is the same as ignoring all the rows in the table.

4. **IGNORE for float k values** : In this case, k will be rounded to the closest integer (where a decimal value of 0.5 is the cutoff - i.e. 0.5 will be rounded up)

5. **IGNORE for k values that are not numbers** : If we have an IGNORE clause like something of the form 'IGNORE a', or 'IGNORE age', or 'IGNORE name = 'John Doe', an error will be returned, "ERROR: argument of IGNORE must not contain variables". This is the correct behavior.



