/*
 *	$Source: /home/nlfm/Working/CVS/frink/vars.h,v $
 *	$Date: 2002/08/27 10:25:34 $
 *	$Revision: 2.3 $
 *
 *------------------------------------------------------------------------
 *   AUTHOR:  Lindsay Marshall <lindsay.marshall@newcastle.ac.uk>
 *------------------------------------------------------------------------
 *    Copyright 1994-2003 The University of Newcastle upon Tyne (see COPYRIGHT)
 *========================================================================
 *
 */

#ifndef FRINK_VARS_h
#define FRINK_VARS_H

enum VarType
{
    VLOCAL	= 0,		/* local variable			*/
    VGLOBAL	= 1,		/* global variable			*/
    VVAR	= 2,		/* namespace variable			*/
    VFOR	= 3,		/* foreach variable			*/
    VARG 	= 4		/* argument				*/
};

typedef struct vardata_s
{
    char		*name;
    enum VarType	type;
    int			used;
    int			set;
    int			dval;
    int			array;
} VarData;

enum varCheck
{
    VC_NONE	= 0x0000,
    VC_SET	= 0x0001,
    VC_USE	= 0x0002,
    VC_BEFORE	= 0x0004,
    VC_DECL	= 0x0010,
    VC_UNKNOWN	= 0x0040,
    VC_ARRAY	= 0x0080,
};


extern void checkName(Token *, int);
extern VarData *varKnown(char *);
extern VarData *declareVar(Token *, enum VarType, int);
extern VarData *useVar(Token *, enum VarType, int);
extern VarData *setVar(Token *, enum VarType, int);

#endif
