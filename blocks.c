/*
 *	$Source: /home/nlfm/Working/CVS/frink/blocks.c,v $
 *	$Date: 2001/07/10 15:46:28 $
 *	$Revision: 2.2 $
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

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <frink.h>
#include <blocks.h>
#include <vars.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

extern List *blocks;
extern void warnFor(Token*, char*, char*);

Blox *pushBlock(Token *cmd, int infl, int lvl, int cond)
{
    Blox *blp = (Blox *) malloc(sizeof(Blox));

    blp->name = cmd;
    if (lvl < 0)
    {
        blp->level = ((Blox *) lpeek(blocks))->level + 1;
    }
    else
    {
	blp->level = lvl;
    }
    blp->unreachable = 0;
    blp->result = 0;
    blp->infloop = infl;
    blp->returns = 0;
    blp->breaks = 0;
    blp->conditional = cond;
    blp->continues = 0;
    blp->vars = (List *) 0;
    lpush(&blocks, blp);
    return blp;
}

void delBlock(Blox *blp)
{
    VarData *vp;
    
    if (blp != (Blox *) 0)
    {
        while(blp->vars != noList)
	{
	    vp = lpop(&blp->vars);
	    free(vp->name);
	}
    }
    free(blp);
}

void popBlock(int pcall)
{
    Blox *blp = (Blox *) lpop(&blocks);
    List *lp, *arg = noList;
    int def;
    VarData *vp, *fvp;

    if (pcall)
    {
        lp = blp->vars;
	while (lp != noList)
	{
	    vp = (VarData *) lpeek(lp);
	    if (vp->type == VARG)
	    {
		lpush(&arg, vp);
	    }
	    lp = lp->next;
	}
	if (makespec) { fprintf(specfile, "%s {", blp->name->text); }
	def = 0;
	while (arg != noList)
	{
	    vp = lpop(&arg);
	    if (strcmp("args", vp->name) == 0)
	    {
		if (makespec) { fprintf(specfile, " args"); }
		if (arg != (List *) 0)
		{
		    warn(blp->name, "args not the final argument");
		}
		vp->used |= !fascist;
	    }
	    else
	    {
		if (makespec) { fprintf(specfile, " any"); }
		if (vp->dval)
		{
		    def = 1;
		    if (makespec) { fprintf(specfile, "?"); }
		}
		else if (def)
		{
		    warn(blp->name, "argument with no default follows argument with default");
		}
	    }
	}
	if (makespec) { fprintf(specfile, "}\n"); }
        lp = blp->vars;
	while (lp != noList)
	{
	    vp = (VarData *) lpeek(lp);
	    if (doTest(HUNUSED) && !vp->used)
	    {
	        if (fascist || strncmp(vp->name, "unused", 6) != 0)
		{
		    warnFor(blp->name, vp->name, "variable \"%s\" may be unused");
		}
	    }
	    lp = lp->next;
	}
    }
    else if (blp->name == noToken && blp->level > 0)
    { /* almagamate var lists */
	while (blp->vars != noList)
	{
	    vp = lpop(&blp->vars);
	    switch (vp->type)
	    {
	    case VARG:
	    case VGLOBAL:  
	        break;

	    case VFOR:
	        if ((fvp = varKnown(vp->name)) != (VarData *) 0)
		{ /* we know about it already */
		    continue;
		}
		vp->type = VLOCAL;
	        break;

	    default: /* depends on loop type : bug!! */
	        if (blp->conditional) { vp->set = 0; }
		break;
	    }
	    lpush(&(((Blox *) lpeek(blocks))->vars), vp);
	}
    }
    delBlock(blp);
}

int isUnreachable()
{
    return ((Blox *) lpeek(blocks))->unreachable;
}

void setUnreachable(int f)
{
    ((Blox *) lpeek(blocks))->unreachable = f;
}
