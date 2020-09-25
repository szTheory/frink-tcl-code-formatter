/*
 *	$Source: /home/nlfm/Working/CVS/frink/util.c,v $
 *	$Date: 2001/07/06 15:29:54 $
 *	$Revision: 2.1 $
 *
 *------------------------------------------------------------------------
 *   AUTHOR:  Lindsay Marshall <lindsay.marshall@newcastle.ac.uk>
 *------------------------------------------------------------------------
 *    Copyright 1994-2002 The University of Newcastle upon Tyne (see COPYRIGHT)
 *========================================================================
 *
 */
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#include <token.h>

void lpush(List **l, void *v)
{
    List *dt = (List *) malloc(sizeof(List));
    
    dt->next = *l;
    dt->item = v;
    *l = dt;
}

void *lpop(List **l)
{
    List *dt = *l;
    void *vp;
    
    if (dt == (List *) 0)
    {
        fprintf(stderr, "Error: attempt to pop empty list!\n");
	exit(1);
    }
    *l = dt->next;
    vp = dt->item;
    free(dt);
    return vp;
}

void *lpeek(List *l)
{    
    if (l == (List *) 0)
    {
        fprintf(stderr, "Error: attempt to peek at empty list!\n");
	exit(1);
    }
    return l->item;
}

void lappend(List **l, void *v)
{
    List *lp;

    if ((lp = *l) == (List *) 0)
    {
        lpush(l, v);
    }
    else
    {
        while (lp->next != (List *) 0)
	{
	    lp = lp->next;
	}
	lpush(&lp->next, v);
    }
}

void ldel(List **l, int fr)
{
    void *vp;

    while (*l != (List *) 0)
    {
        vp = lpop(l);
	if (fr)
	{
	    free(vp);
	}
    }
}

void lapply(List * l, void (*fn)(void *))
{
    while (l != (List *) 0)
    {
        fn(l->item);
	l = l->next;
    }
}

char * newString(char *t)
{
    return strcpy(malloc(strlen(t)+1), t);
}


int doTest(enum Heuristics x)
{
   extern int pragma;
   extern int heuristics;

   return (!(pragma & NOCHECK) && (heuristics & (int) x));
}

