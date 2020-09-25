/*
 *	$Source: /home/nlfm/Working/CVS/frink/token.h,v $
 *	$Date: 2002/08/27 10:25:53 $
 *	$Revision: 2.3 $
 *
 *------------------------------------------------------------------------
 *   AUTHOR:  Lindsay Marshall <lindsay.marshall@newcastle.ac.uk>
 *------------------------------------------------------------------------
 *    Copyright 1994-2002 The University of Newcastle upon Tyne (see COPYRIGHT)
 *========================================================================
 *
 */

#ifndef FRINK_TOKEN_H
#define FRINK_TOKEN_H

#include "util.h"

typedef struct input_s
{
    char *position;
    int	 remaining;
    int  pushed;
    char back[16];
    char *text;
    FILE *stream;
    int tcall;
    int texpr;
    int lineNumber;
    int lineStart;
} Input;

typedef enum token_e {
    ENDF, ENDLINE, ENDC, LIST, STRING, CONC, CALL, VAR, CPART, SPART,
    CONST, ARRAY, NL, SP, COMMENT, HEAD, LBRACK, LBRACE, LPAREN, DOLLAR,
    VNAME, OSTART, START, XCONT, CONT, RBRACK, RBRACE, BLANK, DQSTART,
    DQEND, SEMI, EM, EN, ECONT, SLIST, SCOMMENT, NOSP, SRBRACE,
    RPAREN, SLBRACE, PRAGMA
} TokenType;

typedef enum const_e
{
    CNSTWORD, CNSTSTRING, CNSTLIST
} ConstType;

typedef struct token_s
{
    TokenType type;
    ConstType ckind;
    int isconc;
    char *text;
    int length;
    struct token_s *sequence;
    struct token_s *next;
    char little[32];
    int lineNo;
} Token;

#define noToken (Token *) 0

extern Input *tokenise(Token *, int);
extern void untokenise(Input *);
extern Token *tokacc(Token*, int, int);

extern Token *collect(Input *);
extern Token *getToken(Input *);
extern void tokenPush(Token **tp, Token *v);
extern void freeToken(Token *);
extern void lprocess(Token*, int);
extern void makeCall(Token*, Token*);
extern void body(Token *, int, int);

extern void output(Token *, int);
extern void blank(void);
extern void warn(Token *, char *);
extern void fail(Token *, char *);

extern Token *isArray(Token *);
extern int constant(Token *tp);
#endif
