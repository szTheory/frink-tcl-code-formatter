/*
 *	$Source: /home/nlfm/Working/CVS/frink/token.c,v $
 *	$Date: 2001/07/10 15:46:34 $
 *	$Revision: 2.2 $
 *
 *------------------------------------------------------------------------
 *   AUTHOR:  Lindsay Marshall <lindsay.marshall@newcastle.ac.uk>
 *------------------------------------------------------------------------
 *    Copyright 1994-2003 The University of Newcastle upon Tyne (see COPYRIGHT)
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

extern char *currentfile;

typedef struct tbuff_s
{
    char *buff;
    int   size;
    int   length;
    int   unit;
} TBuff;

static TBuff *newTB(int size)
{
    TBuff *tp = (TBuff *) malloc(sizeof(TBuff));
    tp->buff = malloc(size);
    tp->length = 0;
    tp->unit = tp->size = size;
    return tp;
}

static void freeTB(TBuff *tp)
{
    free(tp->buff);
    free(tp);
}

static void addTB(TBuff *tp, char ch)
{
    if (tp->length == tp->size)
    {
	tp->size += tp->unit;
	tp->buff = realloc(tp->buff, tp->size);
    }
    tp->buff[tp->length] = ch;
    tp->length += 1;
}

static void capTB(TBuff *tp)
{
    if (tp->length == tp->size)
    {
	tp->size += tp->unit;
	tp->buff = realloc(tp->buff, tp->size);
    }
    tp->buff[tp->length] ='\0';
}

static void catTB(TBuff *tp, char *str)
{
    while (*str)
    {
	addTB(tp, *str++);
    }
    capTB(tp);
}

static char *tokenName[] =
{
    "ENDF",
    "ENDLINE",
    "ENDC",
    "LIST",
    "STRING",
    "CONC",
    "CALL",
    "VAR",
    "CPART",
    "SPART",
    "CONST",
    "ARRAY",
    "NL",
    "SP",
    "COMMENT",
    "HEAD",
    "LBRACK",
    "LBRACE",
    "LPAREN",
    "DOLLAR",
    "VNAME",
    "OSTART",
    "START",
    "XCONT",
    "CONT",
    "RBRACK",
    "RBRACE",
    "BLANK",
    "DQSTART",
    "DQEND",
    "SEMI",
    "EM",
    "EN",
    "ECONT",
    "SLIST",
    "SCOMMENT",
    "NOSP",
    "SRBRACE",
    "RPAREN",
    "SLBRACE",
    "PRAGMA"
};


static Token *concToken(char, char, Input *);
static Token *callToken(Input *);
static Token *varToken(Input *);

Token *newToken(TokenType t)
{
    Token *tp = malloc(sizeof(Token));
    tp->type = t;
    tp->ckind = CNSTWORD;
    tp->isconc = 0;
    tp->text = (char *) 0;
    tp->length = 0;
    tp->sequence = tp->next = noToken;
    tp->lineNo = 0;
    return tp;
}

void dumpToken(Token *tk, FILE *ops)
{
    static char *cval [] =
    {
        "word", "string", "list"
    };
    Token *tp;
    static int depth = -1;
    if (tk == noToken)
    {
	fprintf(ops, "Null Token Pointer\n");
    }
    else
    {
	depth += 1;
	fprintf(ops, "{%s ", tokenName[tk->type]);
	if (tk->type == CONST)
	{
	    fprintf(ops, "(%s) ", cval[tk->ckind]);
	}
	if (tk->isconc)
	{
	    fprintf(ops, "(conc) ");
	}
	fprintf(ops, "line %d {", tk->lineNo);
	if (tk->text != (char *) 0)
	{
	    fprintf(ops, "%s", tk->text);
	}
	else
	{
	    fprintf(ops, "(NULL POINTER)");
	}
	fprintf(ops, "} {");
	tp = tk->sequence;
	while (tp != noToken)
	{
	    dumpToken(tp, ops);
	    tp = tp->next;
	}
	fprintf(ops,"} }");
	if ((depth -= 1) < 0) { fprintf(ops, "\n"); }
    }
}

void fail(Token *tp, char *msg)
{
    if (!compout)
    {
	fprintf(stderr, "***");
    }
    if (currentfile != (char *) 0)
    {
        fprintf(stderr, " %s", currentfile);
    }
    if (!compout)
    {
        fprintf(stderr, " Error : %s", msg);
    }
    if (tp != noToken)
    {
	if (tp->text != (char *) 0)
	{
	    fprintf(stderr, "\n    %s\n", tp->text);
	}
	if (tp->lineNo != 0)
	{
	    fprintf(stderr, (compout ? " (%d) :" : " (line %d)"), tp->lineNo);
	}
    }
    if (compout)
    {
	fprintf(stderr, " Error : %s", msg);
    }
    fprintf(stderr, "\n");
    exit(1);
}

extern void makeTB(Token *hd, TBuff *tb);
 
static void makeFTB(Token *hd, TBuff *tb)
{
    if (hd == noToken)
    {
        return;
    }

    switch (hd->type)
    {
    case LBRACE :
	catTB(tb, "{");
        break;
    case RBRACE :
	catTB(tb, "}");
        break;
    case VAR :
	catTB(tb, "$");
	makeTB(hd->sequence, tb);
	break;
    case ARRAY :
	catTB(tb, "(");
	makeTB(hd->sequence, tb);
	catTB(tb, ")");
	break;
    case SCOMMENT :
        catTB(tb, ";");
    	catTB(tb, hd->text);
/*	catTB(tb, "\n"); */
	break;
    case SEMI :
	catTB(tb, ";");
	break;
    case NL :
	catTB(tb, "\n");
	break;
    case CONC :
	makeTB(hd->sequence, tb);
	break;
    case CONST:
        switch (hd->ckind)
	{
	case CNSTWORD:
	    if (hd->text != (char *) 0) { catTB(tb, hd->text); }
	    break;

	case CNSTSTRING:
	    catTB(tb, "\"");
	    if (hd->text != (char *) 0) { catTB(tb, hd->text); }
	    catTB(tb, "\"");
	    break;

	case CNSTLIST:
	    catTB(tb, "{");
	    if (hd->text != (char *) 0) { catTB(tb, hd->text); }
	    catTB(tb, "}");
	    break;
	}
        break;

    case STRING :
	catTB(tb, "\"");
	makeTB(hd->sequence, tb);
	catTB(tb, "\"");
	break;

    case SLIST :
	catTB(tb, "{");
	makeTB(hd->sequence, tb);
	catTB(tb, "}");
	break;

    case CALL :
	catTB(tb, "[");
	makeTB(hd->sequence, tb);
	catTB(tb, "]");
	break;

    case LIST :
	catTB(tb, "{");
	if (hd->text != (char *) 0) { catTB(tb, hd->text); }
	catTB(tb, "}");
	break;

    default :
	if (hd->text != (char *) 0) { catTB(tb, hd->text); }
	break;
    }
}

void makeTB(Token *hd, TBuff *tb)
{
    makeFTB(hd, tb);
    if (hd != noToken) { makeTB(hd->next, tb); }
}

void warn(Token *tp, char *msg)
{
    Blox *blp = (Blox *) lpeek(blocks);
    TBuff *tbp = newTB(32);

    if (!compout)
    {
        fprintf(stderr, "***");
    }
    if (currentfile != (char *) 0)
    {
        fprintf(stderr, " %s", currentfile);
    }
    if (!compout)
    {
        fprintf(stderr, " Warning : %s", msg);
    }
    if (tp != noToken)
    {
        if (compout  && tp->lineNo != 0)
        {
            fprintf(stderr, " (%d) :", tp->lineNo);
        }
        if (blp != (Blox *) 0 && blp->name != noToken)
        {
            fprintf(stderr, " in proc ");
            makeFTB(blp->name, tbp);
            fprintf(stderr, "%s", tbp->buff);
            freeTB(tbp);
        }
        if (!compout && tp->lineNo != 0)
        {
            fprintf(stderr, " (line %d)", tp->lineNo);
        }
    }
    else if (lineNumber != 0)
    {
        fprintf(stderr, (compout ? " (%d) :" : " (line %d)"), lineNumber);
    }
    if (compout)
    {
        fprintf(stderr, " Warning : %s", msg);
    }
    fprintf(stderr, "\n");
    fflush(stderr);
    if (haltonWarn)
    {
        exit(1);
    }
    failed = 1;
}

static void bwarn(int ln, char *msg)
{
    Token *tp = newToken(SP);
    tp->lineNo = ln;
    warn(tp, msg);
    freeToken(tp);
}

void streamMore(Input *file)
{
    file->pushed = 0;
    file->position = file->text;
    if (file->stream != NULL)
    {
	file->remaining = fread(file->text, 1, 64*1024, file->stream);
    }
}

static int isBlank(char ch)
{
    return ch == ' ' || ch == '\t';
}

static char *skipblank(char *cp)
{
    while (isBlank(*cp))
    {
	cp++;
    }
    return cp;
}

static char reallyGet(Input *file)
{
    char ch;
    if (file->remaining <= 0) {	streamMore(file); }
    if (file->remaining <= 0) { return '\0'; }
    file->remaining -= 1;
    if ((ch = *file->position++) == '\n')
    {
        file->lineNumber += 1;
    }
    return ch;
}

static char push(Input *file, char ch)
{
    if (ch != '\0')
    {
	file->back[file->pushed] = ch;
	file->pushed += 1;
	if (ch == '\n') { file->lineNumber -= 1; }
    }
    return ch;
}

static char getPushed(Input * file)
{
    file->pushed -= 1;
    if (file->back[file->pushed] == '\n')
    {
	file->lineNumber += 1;
    }
    return file->back[file->pushed];
}

static char chCGet(Input *file)
{
    return (file->pushed) ? getPushed(file) : reallyGet(file);
}


static char peek(Input *file)
{
    return push(file, chCGet(file));
}

static char chGet(Input *file)
{
    char ch;
    if (file->pushed) { return getPushed(file); }
    if ((ch = chCGet(file)) == '\\')
    {
	switch (ch = chCGet(file))
	{
	case '\r':
	    if ((ch = chCGet(file)) != '\n' && !isBlank(ch))
	    {
	    	(void) push(file,ch);
		ch = ' ';
		break;
	    }
	case '\n':
	    while (isBlank(ch = chCGet(file)))
	    {
	    }
	    (void) push(file, ch);
	    ch = ' ';
	    break;
	default:
	    if (ch == ' ')
	    {
	        if (peek(file) == '\n')
		{
		    bwarn(file->lineNumber, "\"\\ \" at end of line");
		}
	    }
	    (void) push(file, ch);
	    ch = '\\';
	    break;
	}
    }

    return ch;
}

void freeToken(Token *tp)
{
    if (tp != noToken)
    {
	if (tp->text != (char *) 0 && tp->text != tp->little)
	{
	    free(tp->text);
	}
	if (tp->sequence) freeToken(tp->sequence);
	if (tp->next) freeToken(tp->next);
	free(tp);
    }
}

static char *filterString(Token *tp, char *txt)
{
    char *cp = malloc(strlen(txt) + 1), *pt;
    
    if (tp->type == CONST && tp->ckind != CNSTLIST)
    {
        pt = cp;
	while (*txt)
	{
	    if (*txt != '\\')
	    {
	        *pt++ = *txt++;
	    }
	    else
	    {
	        txt++;
		if (*txt == '\0')
		{ /* this is a very weird case!!!! */
		    *pt++ = '\\';
		}
		else
		{
	            *pt++ = *txt++;
		}
	    }
	}
	*pt = '\0';
    }
    else
    {
        strcpy(cp, txt);
    }
    return cp;
}

Input *tokenise(Token *tp, int expr)
{
    Input *file = (Input *) malloc(sizeof(Input));
    file->remaining = tp->length;
    file->pushed = 0;
    file->stream = NULL;
    file->position = file->text = filterString(tp, tp->text);
    file->tcall = 0;
    file->texpr = expr;
    file->lineNumber = tp->lineNo;
    file->lineStart = 1;    
    return file;
}

Input *ftokenise(char *txt, int length, int lineNo, int expr)
{
    Token *tmp = newToken(CONST); /* gash token for filterstring */
    Input *file = (Input *) malloc(sizeof(Input));
    file->remaining = length;
    file->pushed = 0;
    file->stream = NULL;
    file->position = file->text = filterString(tmp, txt);
    file->tcall = 0;
    file->texpr = expr;
    file->lineNumber = lineNo;
    file->lineStart = 1;
    freeToken(tmp); 
    return file;
}

void untokenise(Input *file)
{
    free(file->text);
    free(file);
}

Token *tokenPop(Token **tp)
{
    Token *sp;
    if ((sp = *tp) != noToken)
    {
	*tp = sp->next;
	sp->next = noToken;
    }
    return sp;
}

void tokenPush(Token **tp, Token *v)
{
    Token * ptr;
    v->next = noToken;
    if ((ptr = *tp) == noToken)
    {
	*tp = v;
    }
    else
    {
	while (ptr->next != noToken)
	{
	    ptr = ptr->next;
	}
	ptr->next = v;
    }
}

static Token *createToken(TokenType t, char *text, Token *nxt)
{
    Token *tp = newToken(t);

    if ((tp->length = strlen(text)) < (sizeof(tp->little) - 1))
    {
	tp->text = strcpy(tp->little, text);
    }
    else
    {
	tp->text = newString(text);
    }
    tp->next = nxt;
    return tp;
}


static Token *makeToken(TokenType t, TBuff *tb, int ln)
{
    Token *tp = newToken(t);
    if ((tp->length = tb->length) < sizeof(tp->little))
    {
	tp->text = strcpy(tp->little, tb->buff);
	freeTB(tb);
    }
    else
    {
	tp->text = tb->buff;
	free(tb);
    }
    tp->lineNo = ln;
    return tp;
}

static Token *spaceToken(char ch, Input *file)
{
     TBuff *tb = newTB(128);
    int ln = file->lineNumber;
    do
    {
	addTB(tb, ch);
    }
    while ((ch = chGet(file)) == ' ' || ch == '\t');
    push(file, ch);
    capTB(tb);
    return makeToken(SP, tb, ln); 
}

static Token *listToken(Input *file, int term)
{
    int bufSize = 16*1024;
    char ch;
    TBuff *tb = newTB(bufSize);
    Token *tp;
    int bcount = 0, ln = file->lineNumber;
    
    for (;;)
    {
      switch (ch = chCGet(file)) /* needs to be chCGet here */
	{
	case '\\' :
	    addTB(tb, ch);
	    ch = chCGet(file);  /* ditto - this avoids backslash processing */
	    break;
	case '{' :
	    bcount += 1;
	    break;
	case '}' :
	    if ((bcount -= 1) < 0) { goto done; }
	    break;
	case '\0' :
	    bwarn(file->lineNumber, "Missing }");
	    goto done;
	}
	addTB(tb, ch);
    }
done:
    capTB(tb);
    tp = makeToken(LIST, tb, ln);
    if (term)
    {
	ch = chGet(file);
	push(file, ch);
	if (!isspace(ch))
	{
	    switch (ch)
	    {
	    case ')' :
	        if (!file->texpr)
		{
		    fail(tp, "Extra characters after '}'. (Missing '(' ?)");
		}
		break;
	    case ';' :
	    case '\0': break;
	    case ']' :
		if (!file->tcall)
		{
		    fail(tp, "Extra characters after '}'. (Missing '[' ?)");
		}
		break;
	    default :
		fail(tp, "Extra characters after '}'.");
		break;
	    }
	}
    }
    return tp;
    
}

static Token *handleSemi(Input*, char);

Token *stringToken(Input *file, char lst, TokenType res, int term)
{
    int bufSize = 16*1024;
    Token *tp = newToken(res), *nt;
    char ch;
    TBuff *tb = newTB(bufSize);
    static char errmsg [] ="Missing X";
    int variable = (res == ARRAY);

    tp->lineNo = file->lineNumber;
    for(;;)
    {
	if ((ch = chGet(file)) == lst) { break; }
	switch (ch)
	{
	case '\r':
	    if ((ch = chGet(file)) != '\n')
	    {
	    	push(file, ch);
	        addTB(tb, '\r');
	        continue;
	    }
	case '\n' : nt = newToken(NL); break;
	case ';' :  nt = handleSemi(file, lst);  break;
	case ' ' :
	case '\t':  nt = spaceToken(ch, file); break;
	case '[' :  nt = callToken(file); variable = 1; break;
	case '$' :
	    if ((nt = varToken(file)) == noToken)
	    {
		addTB(tb, '$');
		continue;
	    }
	    variable = 1;
	    break;
	case '\0' :
	    errmsg[8] = lst;
	    bwarn(file->lineNumber, errmsg);
	    goto done;
	case '{' : nt = newToken(LBRACE); break;
	case '}' : nt = newToken(RBRACE); break;
	case '\\' :
	    addTB(tb, ch);
	    ch = chGet(file);
	default :
	    addTB(tb, ch);
	    continue;
	}
	if (tb->length != 0)
	{
	    capTB(tb);
	    tokenPush(&tp->sequence, makeToken(SPART, tb, file->lineNumber));
	    tb = newTB(bufSize);
	}
	nt->lineNo = file->lineNumber;
	tokenPush(&tp->sequence, nt);
    }
done:
    if (term)
    {
	ch = chGet(file);
	push(file, ch);
	if (!isspace(ch))
	{
	    switch (ch)
	    {
	    case ')' :
	        if (file->texpr)
		{
		    break;
		}
		fail(tp, "Extra characters after '}'. (Missing '(' ?)");
		break;
	    case ';' :
	    case '\0': break;
	    case ']' :
		if (file->tcall)
		{
		    break;
		}
		fail(tp, "Extra characters after quoted string. (Missing '[' ?)");
		break;
	    default :
		fail(tp, "Extra characters after quoted string.");
		break;
	    }
	}
    }
    
    capTB(tb);
    if (res != ARRAY && tp->sequence == noToken)
    {
	freeToken(tp);
	tp = makeToken(CONST, tb, file->lineNumber);
	tp->ckind = CNSTSTRING;
	return tp;
    }
    if (tb->length != 0)
    {
	tokenPush(&tp->sequence, makeToken(SPART, tb, file->lineNumber));
    }
    else
    {
	freeTB(tb);
    }
    if (!variable)
    {
	tb = newTB(bufSize);
	makeTB(tp->sequence, tb);
	freeToken(tp);
	tp = makeToken(CONST, tb, file->lineNumber);
	tp->ckind = CNSTSTRING;
    }
    return tp;
}


int tokEqual(Token *tp, char *val)
{
    if (tp != noToken)
    {
        switch (tp->type)
	{
	case CONST:
	case LIST :
	    return (tp->text != (char *) 0 && strcmp(tp->text, val) == 0);
	default:
	    break;
	}
    }
    return 0;
}

int tokNEqual(Token *tp, char *val, int n)
{
    if (tp != noToken)
    {
        switch (tp->type)
	{
	case CONST:
	case LIST :
	    return (tp->text != (char *) 0 && strncmp(tp->text, val, n) == 0);
	default:
	    break;
	}
    }
    return 0;
}

static int isPragma(TBuff *tp)
{
    char *cp;
    
    cp = skipblank(tp->buff + 1); /* start after # character */
    return (strncmp(cp, "PRAGMA", 6) == 0 || strncmp(cp, "FRINK", 5) == 0);
}

static Token *getComment(Input *file, TBuff *tb, char lst)
{
    char ch;
    TokenType tt = COMMENT;

    for(;;)
    {
        if ((ch = chCGet(file)) == lst)
	{
	    break;
	}
	switch (ch)
	{
	case '\r' :
	    if (lst != '\n')
	    {
	        break;
	    }
	    if ((ch = chGet(file)) != '\n')
	    {
	        push(file, ch);
		ch = '\r';
		break;
	    }
	case '\0' :
	    goto xit;

	case '\\' :
	    addTB(tb, '\\');
	    ch = chCGet(file);
	}
	addTB(tb, ch);
    }
xit:
    push(file, ch);
    capTB(tb);
    if (isPragma(tb))
    {
       tt = PRAGMA;
    }
    return makeToken(tt, tb, file->lineNumber);
}

static Token *handleSemi(Input *file, char term)
{
    char ch;
    Token *tp;
    TBuff *tb;

    if ((ch = chCGet(file)) != '#')
    {
        push(file, ch);
	return newToken(SEMI);
    }
    catTB(tb = newTB(128), "#");
    tp = getComment(file, tb, term);
    if (term == '\n' && chCGet(file) == '\r')
    {
        (void) chCGet(file);
    }
    tp->type = SCOMMENT;
    return tp;
}

Token *getToken(Input *file)
{
    char ch;
    Token *tp;
    TBuff *tb;
    
    extern int trace;

    switch (ch = chGet(file))
    {
    case '\0' : tp = newToken(ENDF) ; break;
    case ';' : tp = handleSemi(file, '\n') ; break;
    case '\r' :
    	if ((ch = chGet(file)) != '\n')
	{
	    push(file, ch);
	}
    case '\n' : tp = newToken(NL); break;
    case ' ' :
    case '\t': tp = spaceToken(ch, file); break;
    case ']':
	tp = (file->tcall) ? newToken(ENDC) : concToken(ch, '\0', file);
	break;
    case '{' : tp = listToken(file, 1); break;
    case '"' : tp = stringToken(file, '"', STRING, 1); break;
    case '#' : if (file->lineStart)
      	       {
		   catTB(tb = newTB(128), "#");
		   tp = getComment(file, tb, '\n');
	       }
	       else
	       {
	           tp = concToken(ch, '\0', file);
	       }
	       break;
    case '(' :
    case ')' :
	if (file->texpr)
	{
	    tp = newToken((ch == ')') ? RPAREN : LPAREN);
	    break;
	}
    default  : tp = concToken(ch, '\0', file); break;
    }
    if (tp->lineNo == 0)
    {
        tp->lineNo = file->lineNumber;
    }
    if (trace)
    {
        dumpToken(tp, stderr);
    }
    return tp;
}

static Token *callToken(Input *file)
{
    Token *tp = newToken(CALL), *stp;
    int tex = file->texpr;
    file->texpr = 0;
    file->tcall += 1;
    file->lineStart = 1;
    tp->lineNo = file->lineNumber;
    for(;;)
    {
	stp = getToken(file);
	switch (stp->type)
	{
	case SP :
	    freeToken(stp);
	    continue;
	case ENDF :
	    bwarn(file->lineNumber, "Missing ]");
	case ENDC :
	    freeToken(stp);
	    file->tcall -= 1;
	    file->texpr = tex;
	    return tp;
	default :
	    break;
	}
	file->lineStart = 0;
	tokenPush(&tp->sequence, stp);
    }
}

/*
 * varToken processes the name following a $. It can be:
 *
 * 1) a sequence of alphanumerics or _
 * 2) a list enclosed in {}
 * 3) a name followed by an array reference
 * 4) a namespace reference
 * 
 */
static Token *varToken(Input *file)
{
    Token *tp = newToken(VAR);
    int bufSize = 512, leng, sexpr, nsc;
    
    char ch, ch2;
    TBuff *tb;

    sexpr = file->texpr;
    file->texpr = 0;
    tp->lineNo = file->lineNumber;
    if ((ch = chGet(file)) == '{')	/* it's a name in {} */
    {
	tokenPush(&tp->sequence, listToken(file, 0));
	file->texpr = sexpr;
	return tp;
    }

    tb = newTB(bufSize);
    nsc = 0;
    while (isalnum(ch) || ch == '_' || ch == ':') /* alphanumeric name */
    {
        if (ch == ':')
	{
	    nsc += 1;
	}
	else
	{
	    if (doTest(HNSNAME))
	    {
	        if (nsc == 1)
	        {
	            warn(tp, "Single ':' in variable name!");
	        }
		else if (nsc > 2)
		{
	            warn(tp, "More than two consecutive ':' characters in variable name!");
		}
	    }
	    nsc = 0;
	}
	addTB(tb, ch);
  	ch = chGet(file);
    }
/* have to back off trailing :s */
    if (nsc > 0)
    {
        push(file, ch);
	tb->length -= nsc;
        while (nsc > 1)
        {
	    push(file, ':');
	    nsc -= 1;
	}
	ch = ':';
    }
    capTB(tb);
    if ((leng = tb->length) == 0)
    {
        if (doTest(HNAMERR))
        {
            ch2 = peek(file);
            if (isalnum(ch2) || ch2 == '_' || ch2 == ':')
            {

                warn(tp, "Possible variable name error");
            }
        }
	file->texpr = sexpr;
	push(file, ch);
	freeToken(tp);
	freeTB(tb);
	return noToken;
    }
    tokenPush(&tp->sequence, makeToken(VNAME, tb, file->lineNumber));
    if (ch == '(')
    {
	if (leng == 0) { fail(noToken, "Variable error"); }
	tokenPush(&tp->sequence, stringToken(file, ')', ARRAY, 0));
    }
    else
    {
	push(file, ch);
    }
    file->texpr = sexpr;
    return tp;
}

static void concopt(Token **conc)
{
    Token *tp = (*conc)->sequence;

    tp->lineNo = (*conc)->lineNo;
	
    if (tp->next == noToken)
    {
	switch (tp->type)
	{
	case CPART:
	    tp->type = CONST;
	    tp->ckind = CNSTWORD;
	    tp->isconc = 1;
	    (*conc)->sequence = noToken;
	    freeToken(*conc);
	    *conc = tp;
	    break;

	case CALL:
	case VAR:
	    tp->isconc = 1;
	    (*conc)->sequence = noToken;
	    freeToken(*conc);
	    *conc = tp;
	    break;

	default:
	    break;
	}
    }
}

static Token *concToken(char fst, char lst, Input *file)
{
    Token *tp = newToken(CONC), *vp;
    int bufSize = 512, ln;
    TBuff *tb = newTB(bufSize);
    char ch;

    tp->lineNo = file->lineNumber;
    ch = fst;
    for(;;)
    {
	if (ch == lst) { goto done; }
	switch (ch)
	{
	case '\r':
	    if ((ch = chGet(file)) != '\n')
	    {
	        push(file,ch);
		ch = '\n';
	    }
	case ';' :
	case ' ' :
	case '\t' :
	case '\n' :
	case '\0' :
	    goto done;
	case ']' :
	    if (file->tcall) { goto done; }
	    break;
	case '\\' :
	    addTB(tb, ch);
	    ch = chGet(file);
	    break;
	case '[' :
	    if (tb->length != 0)
	    {
		capTB(tb);
		tokenPush(&tp->sequence,
		  makeToken(CPART, tb, file->lineNumber));
		tb = newTB(bufSize);
	    }
	    tokenPush(&tp->sequence, callToken(file));
	    ch = chGet(file);
	    continue;
	case '$' :
	    if ((vp = varToken(file)) != noToken)
	    {
		if (tb->length != 0)
		{
		    capTB(tb);
		    tokenPush(&tp->sequence,
		      makeToken(CPART, tb, file->lineNumber));
		    tb = newTB(bufSize);
		}
		tokenPush(&tp->sequence, vp);
		ch = chGet(file);
		continue;
	    }
	    break;
	case '(' :
	case ')' :
	    if (file->texpr)
	    {
	        goto done;
	    }
	}
	addTB(tb, ch);
	ch = chGet(file);
    }
done :
    push(file, ch);
    if (tb->length != 0)
    {
	capTB(tb);
	if (tp->sequence == noToken)
	{
	    ln = tp->lineNo;
	    freeToken(tp);
	    return makeToken(CONST, tb, ln);
	}
	tokenPush(&tp->sequence, makeToken(CPART, tb, tp->lineNo));
    }
    else
    {
	freeTB(tb);
	
    }
    concopt(&tp);
    return tp;
}

static void handlePragma(Token *tp)
{
    static struct pragma_s
    {
        char		*text;
	enum Pragmas	value;
	int		nextmode;
    } pragmas[] =
    {
    	{ "notreached",		NOTREACHED,	1},
	{ "nocheck",		NOCHECK,	1},
	{ "noformat",		NOFORMAT,	1},
	{ "returnok",		RETURNOK,	1},
	{ "set",		0,		2},
	{ "unused",		0,		3},
	{ "array",		0,		4},
	{ (char *) 0,		0,		1},
    };
    struct pragma_s *prp;
    int label, mode = 1;
    char *cp;
    Token *token, *chars = newToken(CONST);
    Input *pfile;

    pragma = 0;
    cp = skipblank(tp->text+1);
    chars->text = cp;
    chars->length = strlen(cp);
    pfile=tokenise(chars, 0);
    token = getToken(pfile);
    if (tokNEqual(token, "PRAGMA", 6))
    {
        label = 1;
    }
    else if (tokNEqual(token, "FRINK", 5))
    {
        label = 2;
    }
    else
    {
        warn(tp, "Pragma syntax weirdness");
    }
    freeToken(token);

    for(;;)
    {
        token = getToken(pfile);
	switch(token->type)
	{
	    case ENDF :
	    case COMMENT :
	        goto done;

	    case CONST:
	    case LIST:
	        switch (mode)
		{
		case 1:
        	    for (prp = &pragmas[0]; prp->text != (char *) 0; prp++)
        	    {
 	                if (tokEqual(token, prp->text))
		        {
		            pragma |= prp->value;
			    mode = prp->nextmode;
		            break;
		        }
		    }
		    break;

		case 2: /* mark as set */
		    setVar(token, VLOCAL, 0);
		    break;

		case 3: /* mark as used */
		    useVar(token, VLOCAL, 0);
		    break;

		case 4: /* mark as an array */
		    setVar(token, VLOCAL, 1);
		    break;
		    
		}
	        break;

	    default:
	        break;
	}
	freeToken(token);
    }
done:
    freeToken(token);
    chars->text = (char *) 0;
    freeToken(chars);
    untokenise(pfile);
}

int handle(Token *line)
{
    Token *hd;

    extern int tclop(Token*, Token*);
    extern void comment(Token *);

    if (line == noToken)
    {
	if (!minimise)
	{
	    blank();
	}
	return 1;
    }
    hd = tokenPop(&line);
    switch (hd->type)
    {
    case ENDF :
	freeToken(hd);
	return 0;
    case CONST :
	if (tclop(hd, line))
	{
	    break;
	}
	/* DROP THROUGH!!!!! */
    case VAR :
    case CALL :
    case CONC :
        if (isUnreachable())
	{
	    warn(hd, "statement is unreachable");
	    setUnreachable(0);
	}
	makeCall(hd, line);
	break;
    case PRAGMA :
        handlePragma(hd);
	comment(hd);
	goto exit;
    case COMMENT :
	comment(hd);
	break;
    case SCOMMENT :
        if (nocomments)
	{
	    blank();
	}
	else
	{
	    output(hd, 1);
	}
	break;
    case SEMI :
    case NL:
	blank();
	break;
    default :
	fail(hd, "Invalid tcl program");
	/*NOT REACHED*/
    }
    pragma = 0;
exit:
    freeToken(hd);
    freeToken(line);
    return 1;
}

Token *collect(Input *file)
{
    Token *line = noToken, *tp;
    TBuff *tb;
    for(;;)
    {
	tp = getToken(file);
	file->lineStart = 0;
	switch (tp->type)
	{
	case SEMI:
	case NL :
	case ENDF :
	    if (line == noToken) { return tp; }
	    freeToken(tp);
	    file->lineStart = 1;
	    return line;
	case SCOMMENT:
	    if (line == noToken) { return tp; }
	    tokenPush(&line, tp);
	    file->lineStart = 1;
	    return line;
	case CONST :
	    if (line == noToken && tp->text[0] == '#')
	    {
		tb = newTB(128);
		catTB(tb, tp->text);
		freeToken(tp);
		tp = getComment(file, tb, '\n');
	    }
	    break;
	case CONC :
	    if (line == noToken && tp->sequence->type == CPART &&
		tp->sequence->text[0] == '#')
	    {
		tb = newTB(128);
		makeTB(tp, tb);
		freeToken(tp);
		tp = getComment(file, tb, '\n');
	    }
	    break;
	case SP :
	    freeToken(tp);
	    continue;
	default :
	    break;
	}
	file->lineStart = 0;
	tokenPush(&line, tp);
    }
}

static void sconv(Token **conc, Token **line)
{
    if (*conc != noToken)
    {
        concopt(conc);
	tokenPush(line, *conc);
	*conc = noToken;
    }
}

static Token *listconv(Token **lst)
{
    Token *tp = newToken(SLIST);

    tp->lineNo = (*lst)->lineNo;
    freeToken(tokenPop(lst));	/* remove the LBRACE token */

    for(;;)
    {
	switch ((*lst)->type)
	{
	case ENDF :
	    fail(noToken, "String body too complex");
	    /*NOT REACHED*/
	case RBRACE :
	    tp->lineNo = (*lst)->lineNo;
	    freeToken(tokenPop(lst));
	    return tp;
	case SPART :
	    (*lst)->type = CPART;
	default :
	    tokenPush(&tp->sequence, tokenPop(lst));
	}
    }
}

void sprocess(Token *lst, int nls)
{
    Token *line = noToken, *conc = noToken;
    while (lst != noToken)
    {
	switch (lst->type)
	{
	case LBRACE :
	    sconv(&conc, &line);
	    tokenPush(&line, listconv(&lst));
	    continue;
	case SPART :
	    lst->type = CPART;
	    break;
	case SEMI:
	case NL :
	    sconv(&conc, &line);
	    if (nls || line != noToken) { handle(line); }
	    line = noToken;
	    freeToken(tokenPop(&lst));
	    continue;
	case SCOMMENT :
	    sconv(&conc, &line);
	    if (nls || line != noToken) { handle(line); }
	    line = noToken;
	    freeToken(tokenPop(&lst));
	    continue;
	case SP :
	    sconv(&conc, &line);
	    freeToken(tokenPop(&lst));
	    continue;
	default :
	    break;
	}
	if (conc == noToken) { conc = newToken(CONC); }
	tokenPush(&conc->sequence, tokenPop(&lst));
    }
    sconv(&conc, &line);
    if (line != noToken) { handle(line); }
}

void lprocess(Token *lst, int nls)
{
    Token *line = noToken;

    while (lst != noToken)
    {
	switch (lst->type)
	{
	case ENDF :
	    freeToken(lst);
	    lst = noToken;
	    break;

	case SEMI :
	case NL :
	    if (nls || line != noToken) { handle(line); }
	    line = noToken;
	case SP :
	    freeToken(tokenPop(&lst));
	    break;
	case SCOMMENT:
	    tokenPush(&line, tokenPop(&lst));
	    handle(line);
	    line = noToken;
	    break;
	    
	default :
	    tokenPush(&line, tokenPop(&lst));
	    break;
	}
    }
    if (line != noToken) { handle(line); }
}

Token *accumulate(Input *file, int nl)
{
    Token *line = noToken, *hd = noToken;
    TokenType last = NL;
    TBuff *tb;
    for(;;)
    {
	hd = getToken(file);
	switch (hd->type)
	{
	case ENDF :
	    if (line == noToken)
	    {
		line = hd;
	    }
	    else
	    {
		freeToken(hd);
	    }
	    file->lineStart = 1;
	    return line;

	case SEMI:
	case NL :
	    if (!nl)
	    {
		freeToken(hd);
		continue;
	    }
	case SCOMMENT :
	    file->lineStart = 1;
	    goto seminl;

	case CONST :
	    if (last == NL && hd->text[0] == '#')
	    {
		tb = newTB(128);
		catTB(tb, hd->text);
		freeToken(hd);
		hd = getComment(file, tb, '\n');
	    }
	    break;

	case CONC :
	    if (last == NL && hd->sequence->type == CPART
		&& hd->sequence->text[0] == '#')
	    {
		tb = newTB(128);
		makeTB(hd, tb);
		freeToken(hd);
		hd = getComment(file, tb, '\n');
	    }
	    break;

	case SP :
	    freeToken(hd);
	    continue;

	default :
	    break;
	}
	file->lineStart = 0;
seminl:
	tokenPush(&line, hd);
	last = hd->type;
    }
}

Token *tokacc(Token *tp, int flag, int nl)
{
    Input *file = tokenise(tp, flag);
    Token *lp = accumulate(file, nl);

    untokenise(file);
    return lp;
}

static int chkVarToken(Token *token)
{
    return (token != noToken &&
	(token->type == VAR || token->type == CALL || token->type == CONC
	 || token->type == STRING));
}

int isVarToken(Token *token)
{
    char *cp;
    TBuff *tb;

    if (token != noToken && token->type == CONC)
    { /* It might be an array element name */
        tb = newTB(1024);
	makeTB(token->sequence, tb);
	if (tb->buff[tb->length-1] == ')')
	{
	    cp = tb->buff;
	    while (*cp)
	    {
	        switch (*cp)
		{
		case '[' :
		case '$' :
		case ' ' :
		    freeTB(tb);
		    return 1;
		case '(' :
		    freeTB(tb);
		    return 0;
		default:
		    break;
		}
	        cp++;
	    }
	}
	freeTB(tb);
    }
    return chkVarToken(token);
}

int isSingleCall(Token *token, char *proc)
{
    if (token != noToken)
    {
        switch (token->type)
	{
	case CONC :
	    if (token->sequence->type != CALL || token->sequence->next != (Token *)0)
	    {
	        break;
	    }
	    token = token->sequence;  /*!!!! Switch drops through here !!!!*/
	case CALL:
	    return tokEqual(token->sequence, proc);
	default :
	    break;
	}
    }
    return 0;
}

int isSwitch(Token *token)
{
    if (token != noToken)
    {
        switch (token->type)
	{
	case CONC :
	    token = token->sequence;
	    if (token == noToken || token->type != CONST)
	    {
	        break;
	    }
	/* ********** DROP THROUGH ************** */    
	case LIST :
	case CONST:
	    return tokNEqual(token, "-", 1);

	default :
	    break;
	}
    }
    return 0;
}

Token *isArray(Token *tp)
{
    Token *res = noToken;
    TBuff *tbp = (TBuff *) 0;
    char *txt, *cp, ch;
    int len;

    if (tp != noToken && (tp->text != (char *) 0 || tp->type == CONC))
    {
	switch (tp->type)
	{
	case CONC:
	    tbp = newTB(64);
	    makeTB(tp->sequence, tbp);
	    txt = tbp->buff;
	    len = tbp->length;
	    break;

	case CONST:
	case LIST:
	    txt = tp->text;
	    len = tp->length;
	    break;

	default:
	fprintf(stderr, "default\n");
	    return noToken;
	}
	if (txt[len-1] == ')' && *txt != '(')
	{ /* could be an array reference.... */
	    cp = txt;
	
	    for(;;)
	    {
	        ch = *cp;
	        if (ch == '(')
		{
		    *cp = '\0';
		    res = createToken(CONST, txt, noToken);
		    *cp = '(';
		    break;
		}
		if (ch == '\0') { break; }
		cp++;
	    }
	}
	if (tbp != (TBuff *) 0) { freeTB(tbp); }
    }
    return res;
}

int constant(Token *tp)
{
    return (tp != noToken && (tp->type == CONST || tp->type == LIST));
}
