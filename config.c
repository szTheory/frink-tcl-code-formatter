/*
 *	$Source: /home/nlfm/Working/CVS/frink/config.c,v $
 *	$Date: 2002/08/27 10:26:46 $
 *	$Revision: 2.0.1.12 $
 *
 *------------------------------------------------------------------------
 *   AUTHOR:  Lindsay Marshall <lindsay.marshall@newcastle.ac.uk>
 *------------------------------------------------------------------------
 *    Copyright 2000-2002 The University of Newcastle upon Tyne (see COPYRIGHT)
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

#include "frink.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

extern Token *tokenPop(Token **);
extern void freeToken(Token *);
extern void streamMore(Input *);

List *config = noList;

static struct {
	char		*name;
	ParamType	code;
} pvals[] ={
	{"var",		PVAR},
	{"code",	PCODE},
	{"script",	PSCRIPT},
	{"expr",	PEXPR},
	{"cond",	PCOND},
	{"any",		PANY},
	{"single",	PSINGLE},
	{"option",	POPTION},
	{"flag",	PFLAG},
	{"window",	PWINDOW},
	{"args",	PARGS},
	{"options",	POPTIONS},
	{"seqn",	PSQN},
	{"builtin",	PBLTN},
	{"cmds",	PCMDS},
	{"level",	PLEVEL},
	{"*",		PSEQN0},
	{"+",		PSEQN1},
	{"pattern",	PPATTERN},
	{"|",		PMANY},
	{"varlist",	PVARLIST},
	{"block",	PBLOCK},
	{"break",	PBREAK},
	{"nest",	PNEST},
	{"unnest",	PUNNEST},
	{"ctype",	PCTYPE},
	{"ctargs",	PCTARGS},
	{"ctype?",	POCTYPE},
	{"none",	PNONE},
	{(char *) 0,	0}
};

void dumpPdata(ParamData *pt, FILE *op)
{
    int i;
    char *pad;
    List *lp;

    for (i=0; pvals[i].name != (char *) 0; i += 1)
    {
        pad = "";
	if (pt->type == pvals[i].code)
	{
	    if ((lp = pt->values) != noList)
	    {
	        fprintf(op, "{");
	    }
	    fprintf(op, "%s%s", pad, pvals[i].name);
	    if (lp != noList)
	    {
	        while (lp != noList)
		{
		    fprintf(op, "%x ", (int) lp->item);
		    lp = lp->next;
		}
	        fprintf(op, "}");
	    }
	    pad = " ";
	    return;
	}
    }
    fprintf(op, " Unknown ");

}

void dumpConf(ConfigData *cp, FILE *op)
{
    List *plp;

    fprintf(op, "%s {", cp->name);
    plp = cp->params;
    while (plp != noList)
    {
        dumpPdata((ParamData *) lpeek(plp), op);
	plp = plp->next;
    }
    fprintf(op, "}\n");
}

static void pcode(ParamData *ppt, char *str)
{
    int i;

    switch (*str)
    {
    case '?':
        ppt->optional = ROPTIONAL;
	str++;
        break;
    case '*':
        ppt->optional = RSTAR;
	str++;
        break;
    case '+':
        ppt->optional = RPLUS;
	str++;
        break;
    default:
        break;
    }
    for (i=0; pvals[i].name != (char *) 0; i += 1)
    {
	if (!strcmp(str, pvals[i].name))
	{
	    if ((ppt->type = pvals[i].code) == POCTYPE)
	    {
		ppt->optional = ROPTIONAL;
	    }
	    return;
	}
    }
    fprintf(stderr, "\"%s\" is an invalid config item\n", str);
    ppt->type = PANY;
}

static ParamData *newpdata(void)
{
    ParamData *ppt = (ParamData *) malloc(sizeof(ParamData));

    ppt->values = noList;
    ppt->optional = RSINGLE;
    return ppt;
}

static void parlist(Token *tp, List **plist);

static void param(Token *tp, List **plist)
{
    Token *vp, *vp2;
    ParamData *ppt;
    SeqnData *sd;
    void *dp;
    extern void dumpToken(Token *, FILE *);
    
    ppt = newpdata();
    switch (tp->type)
    {
    case CONST:
        if (tp->ckind == CNSTWORD)
	{
            pcode(ppt, tp->text);
            break;
	}
	/**** fall through ****/
    case LIST:
	vp2 = vp = tokacc(tp, 0, 1);
	pcode(ppt, vp->text);
	for (vp = vp->next ; vp != noToken ; vp = vp->next)
	{
	    switch (vp->type)
	    {
	    case NL:
	        continue;

	    case LIST:
	    case CONST:
		switch (ppt->type)
		{
		case PMANY:
		case PSEQN0:
		case PSEQN1:
		    sd = (SeqnData *) malloc(sizeof(SeqnData));
		    sd->seqn = noList;
		    parlist(vp, &sd->seqn);
		    lappend(&ppt->values, sd);
		    break;

		default:
		    if (strncmp(vp->text, "0x", 2) == 0)
		    {
		        sscanf(vp->text, "%x", (unsigned int *) &dp);
		    }
		    else
		    {
		        dp = newString(vp->text);
		    }
		    lappend(&ppt->values, dp);
		    break;
		}
		break;

	    default:
        	dumpToken(vp, stderr);
		fprintf(stderr, "Error in config file, line %d\n", vp->lineNo);
		exit(1);
	    }
	}
	freeToken(vp2);
	break;

    case ENDF:
        free(ppt);
	return;

    default:
        dumpToken(tp, stderr);
	fprintf(stderr, "Error in config\n");
	exit(1);
    }
    lappend(plist, ppt);
}

static void parlist(Token *tp, List **plist)
{
    Token *lp, *lp2;

    for (lp2 = lp = tokacc(tp, 0, 1); lp != noToken ; lp = lp->next)
    {
        param(lp, plist);
    }
    freeToken(lp2);
}

static int handle(Token *line)
{
    ConfigData *cpt;
    Token *hd, *tp;

    if (line == noToken)
    {
	return 1;
    }
    hd = tokenPop(&line);
    switch (hd->type)
    {
    case CONST :
	cpt = (ConfigData *) malloc(sizeof(ConfigData));
	cpt->name = newString(hd->text);
	cpt->params = noList;
	lpush(&config, cpt);
	tp = line;
	if (tp != noToken)
	{
	    switch (tp->type)
	    {
	    case CONST:
	    case LIST:
		parlist(tp, &cpt->params);
		break;

	    default:
		fprintf(stderr, "Error in config file\n");
		exit(1);
	    }
	}
	break;

    case PRAGMA :
    case COMMENT :
    case SCOMMENT :
    case SEMI :
    case NL:
	break;

    default :
	fprintf(stderr, "Warning: error in config file\n");
	exit(1);

    case ENDF :
	freeToken(hd);
	return 0;
    }
    freeToken(hd);
    freeToken(line);
    return 1;
}

void readconfig(char *str)
{
    FILE *fd;
    Input file;
    
    if ((fd = fopen(str, "r")) == NULL)
    {
	fprintf(stderr, "Warning: cannot open config file\n");
	return;
    }
/*
 * use the tokenising mechanism we already have to parse the config file
 */
    file.text = (char *) malloc(64*1024);
    file.stream = fd;
    file.tcall = file.texpr = 0;
    file.lineNumber = 1;
    file.lineStart = 1;
    streamMore(&file);
    while(handle(collect(&file)))
    {
        /* skip */
    }
    free(file.text);  
    fclose(fd);
}

void stringconfig(char *str)
{
    Input file;
/*
 * use the tokenising mechanism we already have to parse the config string
 */
    file.position = file.text = str;
    file.remaining = strlen(str);
    file.pushed = 0;
    file.stream = NULL;
    file.tcall = file.texpr = 0;
    file.lineNumber = 1;
    file.lineStart = 1;
    while(handle(collect(&file)))
    {
        /* skip */
    }
}
