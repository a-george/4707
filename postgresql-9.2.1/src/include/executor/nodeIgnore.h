/*-------------------------------------------------------------------------
 *
 * nodeIgnore.h
 *
 *
 *
 * Portions Copyright (c) 1996-2012, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/executor/nodeIgnore.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef NODEIGNORE_H
#define NODEIGNORE_H

#include "nodes/execnodes.h"

extern IgnoreState *ExecInitIgnore(Ignore *node, EState *estate, int eflags);
extern TupleTableSlot *ExecIgnore(IgnoreState *node);
extern void ExecEndIgnore(IgnoreState *node);
extern void ExecReScanIgnore(IgnoreState *node);

#endif   /* NODEIGNORE_H */
