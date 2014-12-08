4707
====

IGNORE is exactly like LIMIT/OFFSET when only offset is used (eg. SELECT / FROM / WHERE / OFFSET k is pretty much IGNORE.) There's a post about it on the moodle forum. So, all of the LIMIT/OFFSET code was copy-pasted and simply adjusted to be for IGNORE instead by changing variable names. This doesn't compute IGNORE yet - the code need to be cleaned up to mimic the case where OFFSET is called without LIMIT. 

List of files edited, as well as their locations in the postgresql-9.2.1 
directory:

### 2.1 PARSER

1. **/src/backend/parser/gram.y**: Contains the PostgreSQL grammar.
	Should be working for simple IGNORE statements.
	
2. **/src/backend/parser/gram.output**: Used for debugging purposes with 
	bison in order to resolve shift/reduce and reduce/reduce conflicts 
	in the grammar caused by the precedence of the IGNORE operation. 
	
3. **/src/backend/parser/gram.dot**: Graphical representation of the 
	grammar. Do not open, it will crash.
	
4. **/src/include/parser/kwlist.h**: Contains the added IGNORE keyword.
	
5. **/src/include/nodes/parsenodes.h**: Adds a node for the IGNORE clause 
	to the Query struct so that it can be formulated into a query tree 
	for further processing by the rewriter and the planner. 
	
6. **/src/backend/parser/analyze.c**: Transforms *ignoreClause into 
	something that can be parsed by the query tree.

7. **/src/backend/parser/parse_clause.c**: This file did not actually need 
	to be edited per the lab handout, since transformLimitClause is 
	actually a general method designed to transform LIMIT and 'any 
	allied expression' into type bigint. Thus, *ignoreClause can be 
	transformed in analyze.c using this method.
	
### 2.2 PLANNER

1. **/src/backend/optimizer/plan/planner.c**: Added preprocess_IgnoreClause. 
	Prepares IGNORE for the query tree. Copy of the LIMIT/OFFSET code, but changed
	to be for IGNORE. Some of the original LIMIT/OFFSET comments were left in.

2. **/src/backend/optimizer/plan/createplan.c**: Builds an Ignore plan node.

3. **/src/backend/optimizer/plan/setrefs.c**: Added code for IGNORE which is 
	basically a copy of the OFFSET code. this file probably needs tweaking...

4. **/src/include/nodes/plannodes.h**: Definition of the Ignore node struct.

5. **/src/include/nodes/nodes.h**: Just adds the Ignore nodes to a list of nodes.

6. **/src/include/optimizer/planmain.h**: Prototype of make_Ignore which is used 
	in createplan.c

### 2.3 EXECUTOR

1. **/src/backend/executor/nodeIgnore.c**: Definition of IGNORE. Basically copy 
	and pasted exactly from nodeLimit.c, but all the variable names are changed 
	for IGNORE. This file needs major edits because it's likely where all 
	the logic for IGNORE takes place.

2. **/src/backend/executor/execProcnode.c**: Indicates which process should execute 
	when given an Ignore node.

3. **/src/backend/executor/MakeFile**: Updated to include nodeIgnore.c and 
	nodeIgnore.h. Doesn't need further editing.

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
