/*
 *	$Source: /home/nlfm/Working/CVS/frink/blocks.h,v $
 *	$Date: 2001/07/10 15:46:29 $
 *	$Revision: 2.2 $
 *
 *------------------------------------------------------------------------
 *   AUTHOR:  Lindsay Marshall <lindsay.marshall@newcastle.ac.uk>
 *------------------------------------------------------------------------
 *    Copyright 1994-2002 The University of Newcastle upon Tyne (see COPYRIGHT)
 *========================================================================
 *
 */

#ifndef FRINK_BLOCKS_H
#define FRINK_BLOCKS_H

typedef struct block_s
{
    Token	*name;
    int		level;
    int		unreachable;
    
    int		result;
    int		infloop;
    int		returns;
    int		breaks;
    int		conditional;
    int		continues;
    List	*args;
    List	*vars;
} Blox;

extern Blox *pushBlock(Token *, int, int, int);
extern void delBlock(Blox *);
extern void popBlock(int);
extern int isUnreachable();
extern void setUnreachable(int);

#endif
