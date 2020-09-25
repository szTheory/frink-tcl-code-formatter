/*
 *	$Source: /home/nlfm/Working/CVS/frink/vars.c,v $
 *	$Date: 2002/08/27 10:25:34 $
 *	$Revision: 2.3 $
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
#include <string.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#include <frink.h>
#include <util.h>
#include <blocks.h>
#include <vars.h>

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

extern void warnFor(Token*, char*, char*);

void checkName(Token *cmd, int dynm)
{
    static char *badNames[] =
    {
	".",
	"after",
	"append",
	"array",
	"bell",
	"binary",
	"bind",
	"bindtags",
	"break",
	"button",
	"canvas",
	"case",
	"catch",
	"cd",
	"checkbutton",
	"clipboard",
	"clock",
	"close",
	"concat",
	"continue",
	"destroy",
	"encoding",
	"entry",
	"eof",
	"error",
	"eval",
	"event",
	"exec",
	"exit",
	"expr",
	"fblocked",
	"fconfigure",
	"fcopy",
	"file",
	"fileevent",
	"flush",
	"focus",
	"font",
	"for",
	"foreach",
	"format",
	"frame",
	"gets",
	"glob",
	"global",
	"grab",
	"grid",
	"history",
	"if",
	"image",
	"incr",
	"info",
	"interp",
	"join",
	"label",
	"lappend",
	"lindex",
	"linsert",
	"list",
	"listbox",
	"llength",
	"load",
	"lower",
	"lrange",
	"lreplace",
	"lsearch",
	"lsort",
	"menu",
	"menubutton",
	"message",
	"namespace",
	"open",
	"option",
	"pack",
	"package",
	"pid",
	"place",
	"proc",
	"puts",
	"pwd",
	"radiobutton",
	"raise",
	"read",
	"regexp",
	"regsub",
	"rename",
	"return",
	"scale",
	"scan",
	"scrollbar",
	"seek",
	"selection",
	"send",
	"set",
	"socket",
	"source",
	"spinbox",
	"split",
	"string",
	"subst",
	"switch",
	"tell",
	"text",
	"time",
	"tk",
	"tkwait",
	"toplevel",
	"trace",
	"unknown",
	"unset",
	"update",
	"uplevel",
	"upvar",
	"variable",
	"vwait",
	"while",
	"winfo",
	"wm",
        (char *) 0
    };
    char **bnp = badNames, msgb[256];

    if (cmd != noToken)
    {
        switch (cmd->type)
	{
	case CONST:
	case LIST:
	    if (doTest(HNAMING))
	    {
	        while (*bnp != (char *)0)
	        {
	            if (strcmp(cmd->text, *bnp) == 0)
		    {
			
		        warn(cmd, strcat(strcat(strcpy(msgb, "name clashes with the\" "), *bnp), "\" command"));
		        break;
		    }
		    bnp++;
		}
	    }
	    break;

	default:
	    if (doTest(HNAMING))
	    {
	        if (dynm)
	        {
	            if (warndyn) { warn(cmd, "dynamic name used"); }
	        }
	        else
	        {
	            warn(cmd, "name looks like an expression");
		}
	    }
	    break;
	}
    }
}

static VarData *newVar(char *name, enum VarType type)
{
    VarData *blp = (VarData *) malloc(sizeof(VarData));

    blp->name = newString(name);
    blp->type = type;
    blp->used = 0;
    blp->set = 0;
    blp->dval = 0;
    blp->array = 0;
    return blp;
}

static VarData *addVar(char *name, enum VarType type, int array)
{
    VarData *vp = newVar(name, type);

    lpush(&(((Blox *) lpeek(blocks))->vars), vp);
    vp->array = array;
    return vp;
}

VarData *varKnown(char *name)
{
    List *blp = blocks;
    List *lp;
    
    while (blp != noList)
    {
        lp = ((Blox *) lpeek(blp))->vars;
        while (lp != noList)
        {
	    if (strcmp(((VarData *) lpeek(lp))->name, name) == 0)
	    {
	        return (VarData *) lpeek(lp);
	    }
	    lp = lp->next;
        }
	if (((Blox *) lpeek(blp))->name != noToken || ((Blox *) lpeek(blp))->level == 0)
	{
	    break;
	}
	blp = blp->next;
    }
    return (VarData *) 0;
}

VarData *declareVar(Token *nm, enum VarType type, int array)
{
    VarData *vp = varKnown(nm->text);
    
    if (vp != (VarData *) 0)
    {
	if (vp->type != type && type != VLOCAL)
        {
	    warnFor(nm, nm->text, "redefinition of variable \"%s\"");
	    return vp;
	}
	if (vp->array == 2)
	{
	    vp->array = array;
	}
	else if (array != 2 && vp->array != array)
	{
	    warnFor(nm, nm->text, "\"%s\" used as both array and variable");
	}
    }
    return addVar(nm->text, type, array);
}

VarData *useVar(Token *nm, enum VarType type, int array)
{
    VarData *vp;
    
    if (nm->length == 0) { return (VarData *) 0; }
    vp = varKnown(nm->text);
    if (vp != (VarData *) 0 && array != 2)
    {
	if (vp->array == 2)
	{
	    vp->array = array;
        }
        else if (doTest(HARRAY))
        {
	    if (((array && !vp->array) || (!array && vp->array)) && (vp->set || vp->used))
	    {
	        warnFor(nm, nm->text, "variable \"%s\" might be used as both a variable and an array");
	    }
        }
    }
    if (vp == (VarData *) 0 || (!vp->set && !vp->used))
    {
	warnFor(nm, nm->text, "variable \"%s\" might be used before being set");
	vp = addVar(nm->text, type, array);
    }
    vp->used = 1;
    if (array) { vp->array = 1; }
    return vp;
}

VarData *setVar(Token *nm, enum VarType type, int array)
{
    VarData *vp;

    if (nm->length == 0) { return (VarData *) 0; }
    vp = varKnown(nm->text);
    if (vp == (VarData *) 0)
    {
	vp = addVar(nm->text, type, array);
    }
    else
    {
        if (type == VFOR)
        {
            vp->type = VFOR;
        }
	if (vp->array == 2 && array != 2)
	{
	    vp->array = array;
	}
	else if (doTest(HARRAY) && (vp->set || vp->used) && array != 2)
	{
	    if ((array && !vp->array) || (!array && vp->array))
	    {
		warnFor(nm, nm->text, "variable \"%s\" might be used as both a variable and an array");
	    }
	}
    }
    vp->set = 1;
    vp->used = 1;
    if (array) { vp->array = 1; }
    return vp;
}
