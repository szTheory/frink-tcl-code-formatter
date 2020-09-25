/*
 *	$Source: /home/nlfm/Working/CVS/frink/output.c,v $
 *	$Date: 2002/08/27 10:24:50 $
 *	$Revision: 2.3 $
 *
 *------------------------------------------------------------------------
 *   AUTHOR:  Lindsay Marshall <lindsay.marshall@newcastle.ac.uk>
 *------------------------------------------------------------------------
 *    Copyright 1994-2002 The University of Newcastle upon Tyne (see COPYRIGHT)
 *========================================================================
 *
 * The handling of long lines needs to be fixed in this code so that it
 * checks sizes and dynamically reallocates buffer space for them.
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
#include <vars.h>

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef hAVE_MALLOC_H
#include <malloc.h>
#endif

static char *outLine	= (char *) 0;	/* pointer to output line buffer */
static int outLeng	= 0;		/* size of buffer */
static int outInc	= 4096*16;	/* increment for increasing buffer */
static int oleng	= 0;		/* actually length in buffer */
static int odent	= 0;		/* current margin size */
static char *oposn	= (char *) 0;	/* current output position */
static int olead	= 0;		/* number of spaces wanted at start */

static TokenType olast	= HEAD;

static Token nlToken	= {BLANK, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token startToken	= {START, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token contToken	= {CONT, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token spToken	= {SP, CNSTWORD, 0, " ", 1, (Token *) 0, (Token *) 0};

void initOutput(void)
{
    if (testonly) return;
    oposn = outLine = (char *) malloc(outInc);
    outLeng = outInc;
    oleng = odent = olead = 0;
}

static void extend(void)
{
    char *oldptr = outLine;

    outLeng += outInc;
    outLine = realloc(outLine, outLeng);
    oposn = outLine + (oposn - oldptr);
}

void setIndent(void)
{
    lmargin += indent;
}

void outdent(void)
{
    lmargin -= indent;
}

static void leader(int size, int cont)
{
    olead = size;
    if (cont) { olead += contdent; }
}

static void prefix(int cont)
{
    leader(lmargin, cont);
    odent = lmargin;
}

static void contfix(int cont)
{
    leader(odent, cont);
}

static char *findBreak(void)
{
    char *bp1, *bp2, *bp3, ch;
    bp1 = &outLine[width - olead - (strlen(contString) + 2)];
    while (bp1 >= outLine &&  *bp1 != ' ') { bp1--; }
    if (bp1 < outLine) {
      /* This is a line with no spaces on it!! */
        return (char *) 0;
    }
    if ((ch = bp1[1]) != '-')
    {
	bp3 = bp1;
	if (ch != '[' && ch != '{' && ch != '"')
	{
	    bp2 = bp1;
	    while (*bp2 == ' ') { bp2--; }
	    while (bp2 >= outLine && *bp2 != ' ') { bp2--; }
	    if (bp2 >= outLine &&
	      ((ch = bp2[1]) == '[' || ch == '-' || ch == '{' || ch == '"'))
	    {
		bp1 = bp2;
	    }
	}
	while (*bp3 == ' ') { bp3--; }
	while (bp3 >= outLine && *bp3 != ' ') { bp3--; }
	if (bp3 >= outLine && (bp3[1]) == '-')
	{
	    bp1 = bp3;
	}
    }
    
    *bp1++ = '\0';
    return bp1;
}

static void endline(void)
{
    char *cp;
    int l;

    if (testonly) return;
    if (oleng != 0)
    {
	*oposn = '\0';
	if (!minimise)
	{
	    while ((olead + oleng) > width)
	    {
		if ((cp = findBreak()) == (char *) 0) { break; }
		oposn = outLine;
		if (olead != 0)
		{
		    if (tabsOn)
		    {
			for (l = olead / tabStops; l > 0; l -= 1)
			{
			    putchar('\t');
			}
			for (l = olead % tabStops; l > 0; l -= 1)
			{
			    putchar(' ');
			}
		    }
		    else
		    {
			while (olead-- > 0) { putchar(' '); }
		    }
		    olead = 0;
		}
		printf("%s%s\\\n", oposn, contString);
		contfix(1);
		strcpy(oposn, cp);
		oleng = strlen(outLine);
	    }
	}
	oposn = outLine;
	if (olead != 0)
	{
	    if (tabsOn)
	    {
		for (l = olead / tabStops; l > 0; l -= 1)
		{
		    putchar('\t');
		}
		for (l = olead % tabStops; l > 0; l -= 1)
		{
		    putchar(' ');
		}
	    }
	    else
	    {
		while (olead -- > 0) { putchar(' '); }
	    }
	}
	printf("%s\n", oposn);
    }
    olead = 0;
    oleng = 0;
    oposn = outLine;
    *oposn = '\0';
}

static void termline(void)
{
    char *cp;

    if (testonly) return;
    if (oleng != 0)
    {
	oleng += 1;
	if ((oleng + 1) > outLeng) { extend(); }
	*oposn++ = ';';
	*oposn = '\0';
	while (oleng > width)
	{
	    cp = &outLine[width - 2];
	    while (*cp != ' ') { cp--; }
	    *cp-- = '\0';
	    if ((*cp != '}' && *cp != '"') || cp[-1] == '\\')
	    {
		printf("%s\\\n", outLine);
	    }
	    else
	    {
		printf("%s \\\n", outLine);
	    }
	    cp += 2;
	    strcpy(outLine, cp);
	    oleng = strlen(outLine);
	    oposn = outLine + oleng;
	}
    }
}

static void printkn(char *txt, int length, int keepnl)
{
    char *cp;
    int l;
    while ((cp = strchr(txt, '\n')) != (char *) 0)
    {
	l = cp - txt;
	oleng += l;
	if ((oleng + 1) > outLeng) { extend(); }
	length -= l + 1;
	*cp++= '\0';
	strcpy(oposn, txt);
	oposn += l;
	if (!keepnl && obfuscate) { termline(); } else { endline(); }
	txt = cp;
    }
    if (length != 0)
    {
	oleng += length;
	if ((oleng + 1) > outLeng) { extend(); }
	strncpy(oposn, txt, length);
	oposn += length;
    }
}

static void printn(char *txt, int len)
{
    if (!testonly)
    {
        printkn(txt, len, 0);
    }
}

static void print(char *txt)
{
    printn(txt, strlen(txt));
}

static void brace(char *t)
{
    char *cp, *bp;
    while ((cp = strpbrk(t, "{}")) != (char *) 0)
    {
	if (cp == t)
	{
	    printn("\\", 1);
	    printn(cp, 1);
	}
	else
	{
	    printn(t, cp - t);
	    bp = cp -1;
	    while (bp != t && *bp == '\\') { bp--; }
	    if ((cp - bp) & 1) { printn("\\", 1); }
	    printn(cp, 1);
	}
	t = cp + 1;
    }
    if (*t) { print(t); }
}

void flushOutput(void)
{
    if (testonly) return;
    if (obfuscate) {
	termline();
	if (oleng > 0) { printf("%s\n", outLine); }
    }
    else
    {
	endline();
    }
    fflush(stdout);
}

void blank(void)
{
    if (noblanks || olast == LBRACK) { return; }
    if (oleng != 0) { endline(); }
    if (olast != LBRACE && olast != DQSTART && !minimise)
    {
	output(&nlToken, 1);
    }
    olast = NL;
}

void comment(Token *hd)
{
    int exec = 0;
    if (!nocomments || (exec = (olast == HEAD && hd->text[1] == '!')))
    {
	output(hd, 0);
	if (exec)
	{
	    endline();
	    olast = NL;
	}
    }
}

static void needSpace(void)
{
    switch (olast)
    {
    case HEAD :
    case LBRACK :
    case LBRACE :
    case DQSTART :
    case LPAREN :
    case DOLLAR :
    case EM :
    case EN :
    case NOSP :
    case SP :
    case CONT :
    case ECONT :
    case NL :
	break;	
    default :
	output(&spToken, 1);
    }
}

static void putSeq(Token *hd, int comp, int str)
{
    Token *tp = hd->sequence;
    while (tp != noToken)
    {
        if (str)
	{
	    switch (tp->type)
	    {
	    case LBRACE : tp->type = SLBRACE; break;
	    case RBRACE : tp->type = SRBRACE; break;
	    default : break;
	    }
	}
	output(tp, comp);
	tp = tp->next;
    }
}

static void putString(Token *seq)
{
    int qts = 1;
    needSpace();
    if (qts) { printn("\"", 1); }
    putSeq(seq, 0, 1);
    if (qts) { printn("\"", 1); }
}

static Token dollarToken = {DOLLAR, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token endlineToken = {ENDLINE, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token lbrackToken = {LBRACK, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token rbrackToken = {RBRACK, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};

void output(Token *token, int compress)
{
    int i;

    if (token == noToken)
    {
    	return;
    }    
    switch (token->type)
    {
    case SEMI:
	printn(";", 1);
	break;
    case DOLLAR:
	printn("$", 1);
	break;
    case VNAME:
	printn(token->text, token->length);
	break;
    case VAR :
        if (token->isconc)
	{
	    needSpace();
	}
	if (token->sequence->type == LIST ||
	    (varbrace && token->sequence->next == (Token *) 0))
	{
	    printn("${", 2);
	    printn(token->sequence->text, token->sequence->length);
	    printn("}", 1);
	}
	else
	{
	    output(&dollarToken, 1);
	    putSeq(token, 1, 1);
	}
	useVar(token->sequence, VLOCAL, (token->sequence->next != noToken));
	break;
    case LPAREN:
	printn("(", 1);
	break;
    case RPAREN:
	printn(")", 1);
	break;
    case ARRAY :
	printn("(", 1);
	olast = LPAREN;
	putSeq(token, 0, 1);
	printn(")", 1);
	break;
    case SCOMMENT:
        if (!nocomments)
	{
	    if (olcomments)
	    {
	    	needSpace();
	        printn(";", 1);
	        printn(token->text, token->length);
	    }
	    else
	    {
	        endline();
		output(&startToken, 1);
		printn(token->text, token->length);
	    }
	}
	break;
    case PRAGMA:
    case COMMENT :
	output(&startToken, 1);
	printn(token->text, token->length);
	break;
    case OSTART :
	if (minimise)
	{
	    output(&spToken, 1);
	}
	else
	{
	    output(&startToken , 1);
	}
	return;
    case START :
	switch (olast)
	{
	case HEAD :
	case ENDLINE :
	case NL :
	case SEMI :
	    break;
	case DQSTART :
	case LBRACE :
	    if (!minimise) { endline(); }
	    break;
	case EM :
	case EN :
	case SP :
	case NOSP :
	case LBRACK :
	    olast = START;
	case CONT :
	case ECONT :
	    return;
	default :
	    output(&endlineToken, 0);
	}
	if (!minimise) { prefix(0); }
	break;
    case XCONT :
	if (!minimise)
	{
	    output(&contToken, 0);
	    for (i = 0; i < contdent; i += 1)  { printn(" ", 1);  }
	}
	else
	{
	    return;
	}
	break;
    case ECONT :
    case CONT :
	if (!minimise)
	{
	    print(contString);
	    printn("\\", 1);
	    endline();
	    prefix(token->type == CONT);
	}
	else
	{
	    return;
	}
	break;
    case ENDLINE :
	if (obfuscate)
	{
	    termline();
	}
	else
	{
	    endline();
	}
	break;
    case NL :
	endline();
	break;
    case BLANK :
	endline();
	if (!testonly)
	{
	    printf("\n");
	}
	olast = NL;
	return;
    case CONC :
	needSpace();
	putSeq(token, 1, 1);
	break;
    case CONST :
	needSpace();
	switch (token->ckind)
	{
	case CNSTSTRING:
	    printn("\"", 1);
	    printn(token->text, token->length);
	    printn("\"", 1);
	    break;
	case CNSTLIST:
	    printn("{", 1);
	    printn(token->text, token->length);
	    printn("}", 1);
	    break;
	case CNSTWORD:
	    if (minimise && lmargin > 0)
	    {
	        brace(token->text);
	    }
	    else
	    {
	        printn(token->text, token->length);
	    }
	}
	break;
    case STRING :
        putString(token);
	break;
    case SLIST:
	needSpace();
	printn("{", 1);
	putSeq(token, 0, 0);
	printn("}", 1);
	break;

    case CALL :
        if (token->isconc)
	{
	    needSpace();
	}
	output(&lbrackToken, 1);
	body(token, 0, 0);
	output(&rbrackToken, 1);
	break;

    case LBRACK :
	printn("[", 1);
	break;
    case RBRACK :
	printn("]", 1);
	break;
    case DQSTART :
	needSpace();
	printn("\"", 1);
	break;
    case DQEND :
	if (olast == DQSTART)
	{
	    if (!compress && !minimise) { output(&spToken, 0); }
	}
	else if (!compress && !minimise) { output(&startToken, 0); }
	printn("\"", 1);
	break;
    case LBRACE :
	needSpace();
    case SLBRACE :
	printn("{", 1);
	break;
    case RBRACE :
	if (olast == LBRACE)
	{
	    if (!compress && !minimise) { output(&spToken, 0); }
	}
	else if (!compress && !minimise) { output(&startToken, 0); }
    case SRBRACE:
	printn("}", 1);
	break;
    case LIST :
	needSpace();
	printn("{", 1);
	printkn(token->text, token->length, 1);
	printn("}", 1);
	break;
    case NOSP:
	break;
    case EM :
	if (!minimise) { printn(" ", 1); }
	break;
    case EN :
	if (addSpaces && !minimise) { printn(" ", 1); }
	break;
    case SP :
	switch (olast)
	{
	case CONT :
	case XCONT :
	case START :
	    return;
	case SP  :
	case ENDLINE :
	case NL :
	    if (!compress) { printn(token->text, token->length); }
	    break;
	case LBRACE :
	    if (!compress) { printn(token->text, token->length); }
	    break;
	default :
	    if (compress)
	    {
		printn(" ", 1);
	    }
	    else
	    {
		printn(token->text, token->length);
	    }
	}
	break;
    case CPART :
	if (minimise && lmargin > 0)
	{
	    brace(token->text);
	    break;
	}
    default :
	printn(token->text, token->length);
	break;
    }
    olast = token->type;
}
