4707
====
List of files edited, as well as their locations in the postgresql-9.2.1 
directory:

### 2.1 PARSER

1. **/src/backend/parser/gram.y**: Contains the PostgreSQL grammar.
	
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
