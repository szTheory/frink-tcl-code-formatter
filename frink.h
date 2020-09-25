/*
 *	$Source: /home/nlfm/Working/CVS/frink/frink.h,v $
 *	$Date: 2001/07/18 09:03:41 $
 *	$Revision: 2.0.1.20 $
 *
 *------------------------------------------------------------------------
 *   AUTHOR:  Lindsay Marshall <lindsay.marshall@newcastle.ac.uk>
 *------------------------------------------------------------------------
 *    Copyright 1994-2003 The University of Newcastle upon Tyne (see COPYRIGHT)
 *========================================================================
 *
 */
 
#define VMAJOR	"2"
#define VMINOR	"2"
#define VSUB	"2"
#define VPATCH	"4"

#include "flagvars.h"
#include "token.h"


typedef Token *(*DoFunc)(Token *, Token *);
typedef Token *(*ChFunc)(Token *);

typedef struct checkit_s
{
    TokenType	type;
    ChFunc	checkproc;
} CheckIt;

typedef enum ptype
{
    PVAR,
    PCODE,
    PSCRIPT,
    PEXPR,
    PCOND,
    PANY,
    PSINGLE,
    POPTION,
    PFLAG,
    PWINDOW,
    PARGS,
    POPTIONS,
    PSQN,
    PBLTN,
    PCMDS,
    PLEVEL,
    PSEQN0,
    PSEQN1,
    PPATTERN,
    PMANY,
    PVARLIST,
    PBLOCK,
    PBREAK,
    PNEST,
    PUNNEST,
    PCTYPE,
    PCTARGS,
    POCTYPE,
    PNONE
} ParamType;

typedef enum rtype
{
   RSINGLE, ROPTIONAL, RSTAR, RPLUS
} RepeatType;

typedef enum ctType /* some of these may be impossible!!! */
{
    CTUNQUOT = 0x000001,    /* check for unquoted value */
    CTNUMBER = 0x000002,    /* check for a number */
    CTNSNAME = 0x000004,    /* check for a namespace name */
    CTLIST   = 0x000010,    /* check for a list */
    CTSTREAM = 0x000020,    /* check for a stream name */
    CTOPTION = 0x000040    /* check for -option */
} CheckType;

typedef struct param_s
{
    ParamType		type;
    RepeatType		optional;
    List		*values;
} ParamData;

typedef struct config_s
{
    char 	*name;
    List	*params;
} ConfigData;

typedef struct segn_s
{
    List *seqn;
} SeqnData;

extern List *config;

extern List *skiplist;
extern List *blocks;
