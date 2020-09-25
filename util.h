/*
 *	$Source: /home/nlfm/Working/CVS/frink/util.h,v $
 *	$Date: 2001/07/18 09:03:46 $
 *	$Revision: 2.2 $
 *
 *------------------------------------------------------------------------
 *   AUTHOR:  Lindsay Marshall <lindsay.marshall@newcastle.ac.uk>
 *------------------------------------------------------------------------
 *    Copyright 1994-2002 The University of Newcastle upon Tyne (see COPYRIGHT)
 *========================================================================
 *
 */

#ifndef FRINK_UTIL_H
#define FRINK_UTIL_H

typedef struct list_s
{
    struct list_s	*next;
    void		*item;
} List;

#define noList ((List *) 0)

enum Heuristics
{
    HVAR	= 0x000001,	/* check for non-var 1st param		*/
    HNUMBER	= 0x000002,	/* check for a number			*/
    HVALUE	= 0x000004,	/* check option values			*/
    HREGEXP	= 0x000008,	/* check regexp parameters		*/
    HRETURN	= 0x000010,	/* check for value/novalue returns	*/
    HNSNAME	= 0x000020,	/* check for : instead of :: in names	*/
    HEXPR	= 0x000040,	/* check various expr issues		*/
    HFOREACH	= 0x000080,	/* check various foreach issues		*/
    HNOPARAM	= 0x000100,	/* check missing parameters		*/
    HSWITCH	= 0x000200,	/* check switches on commands		*/
    HABBREV	= 0x000400,	/* check for abbreviated options	*/
    HUNUSED	= 0x000800,	/* check for unusedness			*/
    HNAMING	= 0x001000,	/* check for bad name choice		*/
    HARRAY	= 0x002000,	/* check for array usage		*/
    HNAMERR	= 0x004000,	/* check for possible name erros	*/
    HALL	= 0xffffff	/* the whole lot			*/
};

enum Pragmas
{
    NOTREACHED	= 0x0001,	/* point not reached			*/
    NOCHECK	= 0x0002,	/* don't check the next line		*/
    NOFORMAT	= 0x0004,	/* don't try to format the next line	*/
    RETURNOK	= 0x0008	/* straight line return is OK		*/
};

extern void lpush(List **, void *);
extern void *lpeek(List *);
extern void *lpop(List **);
extern void lappend(List **, void*);
extern void ldel(List **, int);
extern void lapply(List *, void(*)(void *));

extern char *newString(char *);
extern int doTest(enum Heuristics);

#endif
