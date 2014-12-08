4707 Lab 4
==========

IGNORE is exactly like LIMIT/OFFSET when only offset is used (eg. SELECT / FROM / WHERE / OFFSET k is pretty much IGNORE.) There's a post about it on the moodle forum. So, all of the IGNORE code is very similar to the OFFSET
    code, with changes as necessary. 


List of files edited, as well as their locations in the postgresql-9.2.1 
directory:

### 2.1 PARSER

1. **/src/backend/parser/gram.y**: Contains the PostgreSQL grammar.
	Should be working for simple IGNORE statements, as listed below.
	
2. **/src/backend/parser/gram.output**: Used for debugging purposes with 
	bison in order to resolve shift/reduce and reduce/reduce conflicts 
	in the grammar caused by the precedence of the IGNORE operation. 
    This file has since been deleted, as it is no longer needed for 
    debugging purposes.
	
3. **/src/backend/parser/gram.dot**: Graphical representation of the 
	grammar. Do not open, it will crash. This file has since been 
    deleted, as it is no longer needed for debugging purposes.
	
4. **/src/include/parser/kwlist.h**: Contains the added IGNORE keyword.
    This file contains a simple list of keywords in alphabetical order.
	
5. **/src/include/nodes/parsenodes.h**: Adds a node for the IGNORE clause (*ignoreClause) 
	to the Query struct so that it can be formulated into a query tree 
	for further processing by the rewriter and the planner. Also adds
    adds an ignoreClause node to the struct for SELECT statements.
	
6. **/src/backend/parser/analyze.c**: Transforms an *ignoreClause node into 
	something that can be parsed by the query tree. Adds *ignoreClause 
    to general SELECT statements in isGeneralSelect, and a call to
    transformLimitClause for *ignoreClause. See the following file for 
    further explaination. 

7. **/src/backend/parser/parse_clause.c**: This file did not actually need 
	to be edited per the lab handout, since transformLimitClause is 
	actually a general method designed to transform LIMIT and 'any 
	allied expression' into type bigint. Thus, IGNORE / *ignoreClause can be 
	transformed in analyze.c using this method.
	
### 2.2 PLANNER

1. **/src/backend/optimizer/plan/planner.c**: Added preprocess_IgnoreClause. 
	Prepares IGNORE for the query tree. Similar to the LIMIT/OFFSET code, but changed
	to be for IGNORE. See comments in the code for specifics.

2. **/src/backend/optimizer/plan/createplan.c**: Builds an Ignore plan node.

3. **/src/backend/optimizer/plan/setrefs.c**: Added code for IGNORE which is 
	basically a copy of the OFFSET code. this file probably needs tweaking...

4. **/src/include/nodes/plannodes.h**: Definition of the Ignore node struct for the 
    planner.

5. **/src/include/nodes/nodes.h**: Just adds the Ignore nodes to a list of nodes.

6. **/src/include/optimizer/planmain.h**: Prototype of make_Ignore which is used 
	in createplan.c

### 2.3 EXECUTOR

1. **/src/backend/executor/nodeIgnore.c**: Contains the Definition of IGNORE. It contains the basic function to Execute Ignore such as ExecIgnore, ExecInitIgnore and ExecEndIgnore. This extracts a range of tuples, initilizes the nodes and subnodes and then shutsdown the nodes and subnodes after execution. 

2. **/src/backend/executor/execProcnode.c**: Indicates which process should execute 
	when given an Ignore node. Updated to include nodeIgnore.h. Added case for the Ignore node so the executor knows which states to run on each of the ExecInit, ExecProcNode, and ExecEndNode. ExecInit to run the ExecInitIgnore from the previous step. ExecProcNode to execute ExecIgnore which executes the given node. ExecEndNode to execute the function ExecEndIgnore so it can clean up all the nodes.

3. **/src/backend/executor/MakeFile**: Updated to include nodeIgnore.c and 
	nodeIgnore.h.

4. **/src/backend/nodes/copyfuncs.c**: Added _copyIgnore

5. **/src/backend/nodes/equalfuncs.c**: Added some calls for any ignore nodes. This 
	probably won't need further editing.

6. **/src/backend/nodes/outfuncs.c**: Added _outIgnore

7. **/src/include/executor/nodeIgnore.h**: Header file for nodeIgnore.c.

8. **/src/backend/nodes/nodeFuncs.c**: Not in the lab handout, but needed to 
	be edited to avoid crashing/compiling without errors. Added two lines for 
	IGNORE clauses. Don't think this file will need further editing.

9. **/src/backend/nodes/readfuncs.c**: Not in the lab handout, but needed to 
	be edited to avoid crashing/compiling without errors. Added only 1 line for 
	IGNORE. This file probably won't need further editing.

10. **/src/include/nodes/execnodes.h**: Added some structs for IgnoreState. This 
	file will likely need some editing, since it's a copy/paste of LIMIT/OFFSET.
