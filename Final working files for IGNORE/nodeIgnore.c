/*-------------------------------------------------------------------------
 *
 * nodeIgnore.c
 *
 * Note that the code closely mimics the code for limitNode.c. IGNORE is
 * handled the same way as an OFFSET clause, so the steps executed are 
 * very very similar to those for OFFSET.
 *
 *
 * Portions Copyright (c) 1996-2012, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/executor/nodeIgnore.c
 *
 *-------------------------------------------------------------------------
 */
/*
 * INTERFACE ROUTINES
 *		ExecIgnore		- extract a limited range of tuples
 *		ExecInitIgnore	- initialize node and subnodes..
 *		ExecEndIgnore	- shutdown node and subnodes
 */

#include "postgres.h"

#include "executor/executor.h"
#include "executor/nodeIgnore.h"
#include "nodes/nodeFuncs.h"

static void recompute_ignore(IgnoreState *node);
static void pass_down_bound(IgnoreState *node, PlanState *child_node);


/* ----------------------------------------------------------------
 *		ExecIgnore
 *
 *      Similar to OFFSET:
 *		"This is a very simple node which just performs IGNORE
 *		filtering on the stream of tuples returned by a subplan."
 * ----------------------------------------------------------------
 */
TupleTableSlot *				/* return: a tuple or NULL */
ExecIgnore(IgnoreState *node)
{
	ScanDirection direction;
	TupleTableSlot *slot;
	PlanState  *outerPlan;
	
	node->noCount = TRUE;

	/*
	 * get the necessary information from the node
	 */
	direction = node->ps.state->es_direction;
	outerPlan = outerPlanState(node);

	/*
	 * Main logic state machine
	 */
	switch (node->lstate)
	{
		case IGNORE_INITIAL:

			/* In the case where the ignoreNode is first called, ignore needs 
             * to be computed - hence the call to recompute_ignore.
			 */
			recompute_ignore(node);

			/* FALL THRU to IGNORE_RESCAN */

		case IGNORE_RESCAN:

			/*
			 * If backwards scan, just return NULL without changing state.
			 */
			if (!ScanDirectionIsForward(direction))
				return NULL;
            
            /*
             * Here, the subplan basically returned 0 tuples - thus, the state of ignore
             * is "EMPTY"
             */
			for (;;)
			{
				slot = ExecProcNode(outerPlan);
				if (TupIsNull(slot))
				{
					node->lstate = IGNORE_EMPTY;
					return NULL;
				}
                
                /*
                 * Else...rows are returned from the subplan until position > k.
                 */
				node->subSlot = slot;
				if (++node->position > node->k)
					break;
			}

			/*
			 * First tuple of the window.
			 */
			node->lstate = IGNORE_INWINDOW;
			break;

		case IGNORE_EMPTY:

			/*
			 * The subplan doesn't return any tuples, so NULL.
			 */
			return NULL;

            /* In this section of code, original comments from other nodes were left in, since they concisely explain
             * what is happening.
             */
		case IGNORE_INWINDOW:
			if (ScanDirectionIsForward(direction))
			{
				/*
                 * Per limitNode:
				 * "Forwards scan, so check for stepping off end of window. If
				 * we are at the end of the window, return NULL without
				 * advancing the subplan or the position variable; but change
				 * the state machine state to record having done so."
				 */
				if (!node->noCount &&
					node->position - node->k >= node->count)
				{
					node->lstate = IGNORE_WINDOWEND;
					return NULL;
				}

				/*
				 * "Get next tuple from subplan, if any."
				 */
				slot = ExecProcNode(outerPlan);
				if (TupIsNull(slot))
				{
					node->lstate = IGNORE_SUBPLANEOF;
					return NULL;
				}
				node->subSlot = slot;
				node->position++;
			}
			else
			{
				/*
				 * "Backwards scan, so check for stepping off start of window.
				 * As above, change only state-machine status if so."
				 */
				if (node->position <= node->k + 1)
				{
					node->lstate = IGNORE_WINDOWSTART;
					return NULL;
				}

				/*
				 * "Get previous tuple from subplan; there should be one!"
				 */
				slot = ExecProcNode(outerPlan);
				if (TupIsNull(slot))
					elog(ERROR, "IGNORE subplan failed to run backwards");
				node->subSlot = slot;
				node->position--;
			}
			break;

            /* In this section of code, original comments from other nodes were left in, since they concisely explain
             * what is happening.
             */
		case IGNORE_SUBPLANEOF:
			if (ScanDirectionIsForward(direction))
				return NULL;

			/*
			 * "Backing up from subplan EOF, so re-fetch previous tuple; there
			 * should be one!  Note previous tuple must be in window."
			 */
			slot = ExecProcNode(outerPlan);
			if (TupIsNull(slot))
				elog(ERROR, "IGNORE subplan failed to run backwards");
			node->subSlot = slot;
			node->lstate = IGNORE_INWINDOW;
			/* "position does not change because we didn't advance it before" */
			break;
            
            
            /* In this section of code, original comments from other nodes were left in, since they concisely explain
             * what is happening.
             */
		case IGNORE_WINDOWEND:
			if (ScanDirectionIsForward(direction))
				return NULL;

			/*
			 * "Backing up from window end: simply re-return the last tuple
			 * fetched from the subplan."
			 */
			slot = node->subSlot;
			node->lstate = IGNORE_INWINDOW;
			/* "position does not change because we didn't advance it before" */
			break;

            /* In this section of code, original comments from other nodes were left in, since they concisely explain
             * what is happening.
             */
		case IGNORE_WINDOWSTART:
			if (!ScanDirectionIsForward(direction))
				return NULL;

			/*
			 * "Advancing after having backed off window start: simply
			 * re-return the last tuple fetched from the subplan."
			 */
			slot = node->subSlot;
			node->lstate = IGNORE_INWINDOW;
			/* "position does not change because we didn't change it before" */
			break;

		default:
			elog(ERROR, "impossible IGNORE state: %d",
				 (int) node->lstate);
			slot = NULL;		/* keep compiler quiet */
			break;
	}

	/* Return the current tuple */
	Assert(!TupIsNull(slot));

	return slot;
}

/*
 * Evaluate ignore at IGNORE_INITIAL or IGNORE_SCAN.
 */
static void
recompute_ignore(IgnoreState *node)
{
	ExprContext *econtext = node->ps.ps_ExprContext;
	Datum		val;
	bool		isNull;

	if (node->ignoreClause)
	{
		val = ExecEvalExprSwitchContext(node->ignoreClause,
										econtext,
										&isNull,
										NULL);
		/* if k = NULL, change it to k = 0 */
		if (isNull)
			node->k = 0;
		else
		{
            /* Get the value for k! */
			node->k = DatumGetInt64(val);
            
            /* If k is a negative value, return an error */
			if (node->k < 0)
				ereport(ERROR,
				 (errcode(ERRCODE_INVALID_ROW_COUNT_IN_LIMIT_CLAUSE),
				  errmsg("IGNORE must not be negative")));
		}
	}
	else
	{
		/* No IGNORE supplied */
		node->k = 0;
	}

	/* Reset position to start-of-scan */
	node->position = 0;
	node->subSlot = NULL;

	/* Set state-machine state */
	node->lstate = IGNORE_RESCAN;

	/* Notify child node about ignore, if useful */
	pass_down_bound(node, outerPlanState(node));
}


/*
 * If we have a COUNT, and our input is a Sort node, notify it that it can
 * use bounded sort.  Also, if our input is a MergeAppend, we can apply the
 * same bound to any Sorts that are direct children of the MergeAppend,
 * since the MergeAppend surely need read no more than that many tuples from
 * any one input.  We also have to be prepared to look through a Result,
 * since the planner might stick one atop MergeAppend for projection purposes.
 *
 * This is a bit of a kluge, but we don't have any more-abstract way of
 * communicating between the two nodes; and it doesn't seem worth trying
 * to invent one without some more examples of special communication needs.
 *
 * Note: it is the responsibility of nodeSort.c to react properly to
 * changes of these parameters.  If we ever do redesign this, it'd be a
 * good idea to integrate this signaling with the parameter-change mechanism.
 */
static void
pass_down_bound(IgnoreState *node, PlanState *child_node)
{
	if (IsA(child_node, SortState))
	{
		SortState  *sortState = (SortState *) child_node;
		int64		tuples_needed = node->count + node->k;

		/* negative test checks for overflow in sum */
		if (node->noCount || tuples_needed < 0)
		{
			/* make sure flag gets reset if needed upon rescan */
			sortState->bounded = false;
		}
		else
		{
			sortState->bounded = true;
			sortState->bound = tuples_needed;
		}
	}
	else if (IsA(child_node, MergeAppendState))
	{
		MergeAppendState *maState = (MergeAppendState *) child_node;
		int			i;

		for (i = 0; i < maState->ms_nplans; i++)
			pass_down_bound(node, maState->mergeplans[i]);
	}
	else if (IsA(child_node, ResultState))
	{
		/*
		 * An extra consideration here is that if the Result is projecting a
		 * targetlist that contains any SRFs, we can't assume that every input
		 * tuple generates an output tuple, so a Sort underneath might need to
		 * return more than N tuples to satisfy LIMIT N. So we cannot use
		 * bounded sort.
		 *
		 * If Result supported qual checking, we'd have to punt on seeing a
		 * qual, too.  Note that having a resconstantqual is not a
		 * showstopper: if that fails we're not getting any rows at all.
		 */
		if (outerPlanState(child_node) &&
			!expression_returns_set((Node *) child_node->plan->targetlist))
			pass_down_bound(node, outerPlanState(child_node));
	}
}

/* ----------------------------------------------------------------
 *		ExecInitIgnore
 *
 *		This initializes the ignore node state structures and
 *		the node's subplan.
 * ----------------------------------------------------------------
 */
IgnoreState *
ExecInitIgnore(Ignore *node, EState *estate, int eflags)
{
	IgnoreState *ignorestate;
	Plan	   *outerPlan;

	/* check for unsupported flags */
	Assert(!(eflags & EXEC_FLAG_MARK));

	/*
	 * create state structure
	 */
	ignorestate = makeNode(IgnoreState);
	ignorestate->ps.plan = (Plan *) node;
	ignorestate->ps.state = estate;

	ignorestate->lstate = IGNORE_INITIAL;

	/*
	 * Miscellaneous initialization
	 */
	ExecAssignExprContext(estate, &ignorestate->ps);

	/*
	 * initialize child expressions
	 */
	ignorestate->ignoreClause = ExecInitExpr((Expr *) node->ignoreClause,
										   (PlanState *) ignorestate);

	/*
	 * Tuple table initialization
	 */
	ExecInitResultTupleSlot(estate, &ignorestate->ps);

	/*
	 * then initialize outer plan
	 */
	outerPlan = outerPlan(node);
	outerPlanState(ignorestate) = ExecInitNode(outerPlan, estate, eflags);

	/*
	 * ignore nodes do no projections, so initialize projection info for this
	 * node appropriately
	 */
	ExecAssignResultTypeFromTL(&ignorestate->ps);
	ignorestate->ps.ps_ProjInfo = NULL;

	return ignorestate;
}

/* ----------------------------------------------------------------
 *		ExecEndIgnore
 *
 *		This shuts down the subplan and frees resources allocated
 *		to this node.
 * ----------------------------------------------------------------
 */
void
ExecEndIgnore(IgnoreState *node)
{
	ExecFreeExprContext(&node->ps);
	ExecEndNode(outerPlanState(node));
}


void
ExecReScanIgnore(IgnoreState *node)
{
	/*
	 * Recompute ignore in case parameters changed, and reset the state
	 * machine.  We must do this before rescanning our child node, in case
	 * it's a Sort that we are passing the parameters down to.
	 */
	recompute_ignore(node);

	/*
	 * if chgParam of subnode is not null then plan will be re-scanned by
	 * first ExecProcNode.
	 */
	if (node->ps.lefttree->chgParam == NULL)
		ExecReScan(node->ps.lefttree);
}