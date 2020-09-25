/*
 *	$Source: /home/nlfm/Working/CVS/frink/tcl.c,v $
 *	$Date: 2001/07/10 15:46:33 $
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
#include <string.h>

#include <frink.h>
#include <util.h>
#include <blocks.h>
#include <vars.h>

static int nest[32]; /* = = class, 1 = namespace */
static int inproc = 0;

static Token lbraceToken	= {LBRACE, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token rbraceToken	= {RBRACE, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token xcontToken		= {XCONT, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token ostartToken	= {OSTART, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token startToken		= {START, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token contToken		= {CONT, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token econtToken		= {ECONT, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token emToken		= {EM, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token nospToken		= {NOSP, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token *olsToken		= &emToken;
static Token spToken		= {SP, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token lbrackToken	= {LBRACK, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token rbrackToken	= {RBRACK, CNSTWORD, 0, (char *) 0, 0, (Token *) 0, (Token *) 0};
static Token msgcatToken	= {CONST, CNSTWORD, 0, "::msgcat::mc", 12, (Token *) 0, (Token *) 0};

static Token dqStart		= {DQSTART, CNSTWORD, 0, (char *) 0, 0, noToken, noToken};
static Token dqEnd		= {DQEND, CNSTWORD, 0, (char *) 0, 0, noToken, noToken};
static Token thenToken		= {CONST, CNSTWORD, 0, "then", 4, noToken, noToken};
static Token procToken		= {CONST, CNSTWORD, 0, "proc", 4, noToken, noToken};
static Token elseToken		= {CONST, CNSTWORD, 0, "else", 4, noToken, noToken};
static Token elseifToken	= {CONST, CNSTWORD, 0, "elseif", 6, noToken, noToken};
static Token methodToken	= {CONST, CNSTWORD, 0, "method", 6, noToken, noToken};
static Token semiToken		= {SEMI, CNSTWORD, 0, ";", 1, noToken, noToken};
static Token argsToken		= {SEMI, CNSTWORD, 0, "args", 4, noToken, noToken};
static Token argvToken		= {SEMI, CNSTWORD, 0, "argv", 4, noToken, noToken};
static Token argv0Token		= {SEMI, CNSTWORD, 0, "argv0", 5, noToken, noToken};
static Token platfToken		= {SEMI, CNSTWORD, 0, "tcl_platform", 12, noToken, noToken};

List *blocks  = noList;

static CheckIt **noChecks = (CheckIt **) 0;

extern void setIndent(void);
extern void outdent(void);
extern int isVarToken(Token *);
extern int isSwitch(Token *);
extern int isSingleCall(Token *, char *);
extern int tokEqual(Token *, char *);

/*
 * If there are no more tokens, print a useful message to the user and
 * exit.
 */
void failIfNullToken(Token *token, char *part, char *command, int ln)
{
    char msg[1024];

    if (token == noToken)
    {
	if (ln == 0)
	{
	    sprintf(msg, "Missing \"%s\" part in call of %s", part, command);
	}
	else
	{
	    sprintf(msg, "Missing \"%s\" part in call of %s, starting line %d",
	      part, command, ln);
	}
	fail(token, msg);
    }
}

/*
 * If there are more tokens after this one, print a useful warning to
 * the user.
 */
void warnIfArgsAfter(Token *token, char *part, char *command)
{
    char msg[1024];

    if (token != 0 && token->next != 0)
    {
	if (token->next->type == SCOMMENT)
	{
	    output(token->next, 0);
	}
	else
	{
	    if (*part != '\0')
	    {
		sprintf(msg, "Extra arguments after \"%s\" part in call of %s",
		  part, command);
	    }
	    else
	    {
		sprintf(msg, "Extra arguments in call of `%s'", command);
	    }
	    warn(token, msg);
	    if (token->next != noToken && token->next->text != (char *) 0 &&
	      token->next->text[0] != '\0')
	    {
		sprintf(msg, "Extra token is `%s'", token->next->text);
		warn(token->next, msg);
	    }
	}
    }
}

void warnExpr(Token *cmd, char *txt)
{
    if (doTest(HEXPR) && cmd != noToken && cmd->type != LIST)
    {
	warn(cmd, txt);
    }
}

void warnFor(Token *token, char *cmd, char *txt)
{
    char msg[1024];

    sprintf(msg, txt, cmd);
    warn(token, msg);
}

static void warnIFor(Token *token, int val, char *txt)
{
    char msg[1024];

    sprintf(msg, txt, val);
    warn(token, msg);
}
/*
 * reset various variables so that multiple file processing doesn't get
 * screwed up
 */
void clearState(void)
{
    int i;

    for (i=0; i < 32; i+= 1)
    {
	nest[i] = 0;
    }
    while (blocks != noList)
    {
	popBlock(0);
    }
    pushBlock(noToken, 0, 0, 0);
    setVar(&argsToken, VGLOBAL, 0);
    setVar(&argvToken, VGLOBAL, 0);
    setVar(&argv0Token, VGLOBAL, 0);
    setVar(&platfToken, VGLOBAL, 1);
}

static int isNumber(char *cp)
{
    if (*cp == '-' || *cp == '+')
    {
	cp++;
    }
    while (*cp)
    {
        if (!isdigit(*cp) && *cp != '.')
        {
            return 0;
        }
        cp++;
    }
    return 1;    
}

static int tokIsLevel(Token *tp)
{
    char *cp;
    if (tp == noToken || !(tp->type == CONST || tp->type == LIST) || tp->text == (char *) 0)
    {
	return 0;
    }
    cp = tp->text;
    /* some people write \#0 !!! */
    if (*cp == '\\')
    {
	cp++;
    }
    if (*cp == '#')
    {
	cp++;
    }
    return isNumber(cp);
}

static int oneLine(Token *seq, int semis)
{
    while (seq != noToken)
    {
	if (seq->type == SCOMMENT || seq->type == NL || 
	  (semis && seq->type == SEMI))
	{
	    return 0;
	}
	seq = seq->next;
    }
    return 1;
}

static int checkSpecial(char *val)
{
    char ch;

    if (*val == '{') return 0;
    while ((ch = *val++))
    {
	switch (ch)
	{
	case '$' :
	case '[' :
	    return 0;
	}
    }
    return 1;
}

static void checkUnquoted(Token *str)
{
    char msg[128];
    if (noquotes && str->type == CONST && str->ckind == CNSTWORD && !isNumber(str->text))
    {
        sprintf(msg, "Unquoted constant - \"%s\"", str->text);
        warn(str, msg);
    }
}

static void checkType(Token *cmd, int flags)
{
    if (cmd->type == CONST)
    {
	if (cmd->ckind == CNSTWORD)
	{
            if (flags & CTUNQUOT)
	    {
		checkUnquoted(cmd);
	    }
            if (flags & CTNUMBER && !isNumber(cmd->text))
            {
                warn(cmd, "Number expected!");
	    }
        }	
    }
}

static int single(Token * tp)
{
    if (tp != noToken && tp->next == noToken)
    {
	switch (tp->type)
	{
	case CONST :
	    return checkSpecial(tp->text);
	case CONC :
	case CALL :
	case VAR :
	    return 1;
	default :
	    return 0;
	}
    }
    return 0;
}

static int sconstant(Token *tp)
{
    return (tp != noToken && tp->next == noToken
	&& (tp->type == CONST || tp->type == LIST) && !checkSpecial(tp->text));
}

static int constantString(Token * tp, int any)
{
    Token *sq;
    if (tp == noToken) return 0;
    switch (tp->type)
    {
    case CONST :
	return 1;
    
    case LIST :
	return any;

    case STRING :
	sq = tp->sequence;
	return (sq->next == 0 && sq->type == SPART);

    default :
	return 0;
    }
}

void loopstart(int inf, int cond)
{
    pushBlock(noToken, inf, -1, cond);
}

void loopend(void)
{
    Blox *bp = lpeek(blocks);
    int il = bp->infloop, su = (bp->breaks == 0);

    popBlock(0);
    if (il)
    {
	setUnreachable(su);
    }
}

static void msgsave(Token *tp, int any)
{
    char filename[128];

    if (extract && constantString(tp, any))
    {
	if (msgfile == NULL)
	{
	    if ((msgfile = fopen(strcat(strncpy(filename, locale, 123), ".msg"),
	      "a")) == NULL)
	    {
	    	warn(noToken, "cannot open msg file");
		return;
	    }
	}
	switch (tp->type)
	{
	case CONST:
	case LIST:
	    fprintf(msgfile, "::msgcat::mcset %s \"%s\"\n", locale, tp->text);
	    break;

	case STRING:
	    fprintf(msgfile, "::msgcat::mcset %s \"%s\"\n", locale,
	      tp->sequence->text);
	    break;

	default: /* shut up the compiler warnings */
	    break;
	}
    }
}

void sptclop(Token *hd)
{
    List *cpt = config;
    extern void doUser(Token *hd, Token *cmd, ConfigData *cpt, int nostart);
    ConfigData *cdp;

    while (cpt != noList)
    {
        cdp = (ConfigData *) lpeek(cpt);
	if (!strcmp(hd->text, cdp->name))
	{
	   doUser(hd, noToken, cdp, 1);
	   return;
	}
	cpt = cpt->next;
    }
    output(hd, 1);
}

void body(Token *bod, int addBraces, int popcont)
{
    Token *bl;
    int lnr;
    extern void sprocess(Token *, int);
    Blox *blp = (Blox *) 0;

    switch (bod->type)
    {
    case CONST :
	if (bod->ckind == CNSTWORD)
	{
	    if (embrace)
	    {
		output(&lbraceToken, 1);
		setIndent();
		output((oneliner) ? olsToken : &startToken, 0);
	    }
	    sptclop(bod);
	    if (embrace)
	    {
		outdent();
		if (oneliner) { output(olsToken, 0); }
		output(&rbraceToken, oneliner);
	    }
	    break;
	}
	if (bod->ckind == CNSTSTRING)
	{
	    bl = tokacc(bod, 0, 1);

	    output(&dqStart, 1);
	    setIndent();
	    if ((lnr = (oneliner && oneLine(bl, 0))))
	    {
		output(olsToken, 0);
	    }
	    lprocess(bl, 1);
	    if (lnr) { output(olsToken, 0); }
	    outdent();
	    output(&dqEnd, 0);
	    bod->sequence = noToken;
	    break;
	}
	/* drop through - must be a constant list */
    case LIST :
	bl = tokacc(bod, 0, 1);
	if (debrace && bl != noToken && bl->next == 0
	  && bl->type == CONST && !checkSpecial(bl->text))
	{
	    output(bl, 1);
	    freeToken(bl);
	}
	else if (oneliner && oneLine(bl, 0))
	{
	    output(&lbraceToken, 1);
	    setIndent();
	    output(olsToken, 0);
	    lprocess(bl, 1);
	    outdent();
	    output(olsToken, 0);
	    output(&rbraceToken, 1);
	}
	else
	{
	    output(&lbraceToken, 0);
	    setIndent();
	    lprocess(bl, 1);
	    outdent();
	    output(&rbraceToken, 0);
	}
	break;
	
    case STRING :
        if (popcont) { blp = lpop(&blocks); } /* must evaluate in right context */
	if (!trystrings)
	{
	    output(bod, 0);
	}
	else
	{
	    output(&dqStart, 1);
	    setIndent();
	    if ((lnr = oneliner && oneLine(bod->sequence, 1)))
	    {
		output(olsToken, 0);
	    }
	    sprocess(bod->sequence, 1);
	    if (lnr) { output(olsToken, 0); }
	    outdent();
	    output(&dqEnd, 0);
	    bod->sequence = noToken;
	}
	if (popcont) { lpush(&blocks, blp); }
	break;

    case VAR :
    case CONC :
        if (popcont) { blp = lpop(&blocks); } /* must evaluate in right context */
	if (addBraces && embrace)
	{
	    output(&lbraceToken, 1);
	    setIndent();
	    output((oneliner) ? olsToken : &startToken, 0);
	}
	output(bod , 1);
	if (addBraces && embrace)
	{
	    outdent();
	    if (oneliner) { output(olsToken, 0); }
	    output(&rbraceToken, oneliner);
	}
	if (popcont) { lpush(&blocks, blp); }
	break;

    case SLIST :
	output(bod, 0);
	break;

    case CALL :
        if (popcont) { blp = lpop(&blocks); } /* must evaluate in right context */
	lprocess(bod->sequence, 0);
	bod->sequence = noToken;
	if (popcont) { lpush(&blocks, blp); }
	break;

    case SCOMMENT:
	output(bod, 1);
	break;
    case SEMI:
    case NL:
	blank();
	break;

    default :
	fail(bod, "Block error");
    }
}

typedef enum flags_e {
    NOBRACE	= 001,
    ADDBRACES	= 002,
    SEMIS	= 004,
    PAREN	= 010,
    SPACEOUT	= 020
} PressFlags;

static void press(Token *v , PressFlags flags, CheckIt **checkp)
{
    Input *idx;
    Token *token = noToken, *lst = noToken;
    int parcnt = 0;

    switch (v->type)
    {
    case SLIST:
	output(v, 1);
	break;
    case LIST :
	idx = tokenise(v, flags);
	for(;;)
	{
	    token = getToken(idx);
	    switch (token->type)
	    {
	    case ENDF :
		freeToken(token);
		goto done;
	    case NL :
	    case SEMI :
	    case SCOMMENT :
		if (flags & SEMIS)
		{
		    tokenPush(&lst, token);
		    break;
		}
	    case PRAGMA:
	    case COMMENT :
	    case SP :
		freeToken(token);
	    	break;
	    case LPAREN:
		parcnt += 1;
		tokenPush(&lst, token);
		break;
	    case RPAREN :
		if ((parcnt -= 1) < 0)
		{
		    warn(v, "Possible missing '('");
		}
		tokenPush(&lst, token);
		break;
	    default :
		tokenPush(&lst, token);
	    }
	    idx->lineStart = 0;
	}
done :
	untokenise(idx);
	if (parcnt > 0)
	{
	    warn(v, "Possible missing ')'");
	}
	if (((flags & NOBRACE) && debrace && single(lst)) || sconstant(lst))
	{
	    output(lst, 1);
	}
	else
	{
	    output(&lbraceToken, 1);
	    if (flags & SPACEOUT)
	    {
		output(&emToken, 1);
	    }
	    token = lst;
	    while (token != noToken)
	    {
		switch (token->type)
		{
		case SEMI :
		    if (flags && SEMIS)
		    {
			output(&semiToken, 1);
		    }
		    else
		    {
			output(&startToken, 1);
		    }
		    break;
		default :
		    output(token, 1);
		}
		token = token->next;
	    }
	    if (flags & SPACEOUT)
	    {
		output(&emToken, 1);
	    }
	    output(&rbraceToken, 1);
	}
	freeToken(lst);
	break;
	
    case SCOMMENT:
	output(v, 1);
	break;
	
    case SEMI :
    case NL :
	if (flags & SEMIS) { output(&semiToken, 1); }
    	break;

    default :
	if (embrace && (flags & ADDBRACES)) { output(&lbraceToken, 1); }
	output(v, 1);
	if (embrace && (flags & ADDBRACES)) { output(&rbraceToken, 1); }
    }
}

void etcetera(Token *cmd, int v)
{
    while (cmd != noToken)
    {
	output(cmd, v);
	cmd = cmd->next;
    }
}

void catbin(Token *tp)
{
    int sem, oln;
    Token *sols;

    if (tp != noToken)
    {
	switch (tp->type)
	{
	case LIST :
	    if (tp->length == 0)
	    {
		output(tp, 1); /* empty list - just output it */
		return;
	    }
	    break;
	case STRING :
	default :
	    if (!trystrings)
	    {
		output(tp, 1);
		return;
	    }
	    break;
	}
	oln = oneliner;
	oneliner = 1;
	sem = embrace;
	embrace = 0;
	sols = olsToken;
	olsToken = &nospToken;
	body(tp, 0, 0);
	olsToken = sols;
	embrace = sem;
	oneliner = oln;
    }
}

static void handleVar(Token *hd, Token *cmd, int flags, int paramCount)
{
    enum VarType vt;
    VarData *vp = (VarData *) 0;
    char msg[1024];
    Token *ap = noToken, *ac;
    int array = 0;

    if (isVarToken(cmd))
    {
        if (doTest(HVAR))
	{
	    sprintf(msg, "Parameter %d of call on %s is not a variable name",
	      paramCount, hd->text);
	    warn(cmd, msg);
	}
    }
    else
    {
	ac = cmd;
        vt = ((flags & 0x0f00) >> 8);
	if (flags & VC_ARRAY)
	{
	    array = 1;
	}
	else if ((ap = isArray(cmd)) != noToken)
	{
	    array = 1;
	    ac = ap;
	}
	else if (flags & VC_UNKNOWN)
	{
	    array = 2 ;
	}
	
	if (flags & VC_DECL)
	{
	    vp = declareVar(ac, vt, array);
	}
	if (flags & VC_SET)
	{ /* variable is set here */
	    vp = setVar(ac, vt, array);
	}
	if (flags & VC_USE)
	{
	    vp = useVar(ac, vt, array);
	}
	if (vp != (VarData *) 0)
	{
	    if ((flags & VC_BEFORE) && !vp->set)
	    {
	        warnFor(cmd, cmd->text, "variable \"%s\" may be used before being set");
	    }
	}
	if (ap != noToken) { freeToken(ap); }
    }
    output(cmd, 1);
}

static void handlescom(Token *tp)
{
    if (tp != noToken)
    {
	output(tp, 1);
    }
}

static Token *handleOpt(Token *arg)
{
    Token *ap = arg;

    arg = arg->next;
    if ((!nocommand || pragma & NOFORMAT) && tokEqual(ap, "-command"))
    {
        pushBlock(noToken, 0, -1, 0);
	catbin(arg);
	popBlock(1);
    }
    else if (tokEqual(ap, "-text") || tokEqual(ap, "-label"))
    {
	if (!isSingleCall(arg, "::msgcat::mc"))
	{
	    if (internat)
	    {
		output(&spToken, 1);
		output(&lbrackToken, 1);
		output(&msgcatToken, 1);
		output(arg, 1);
		output(&rbrackToken, 1);
	    }
	    else
	    {
		output(arg, 1);
	    }
	    msgsave(arg, 1);
	}
	else
	{
	    output(arg, 1);
	}
    }
    else
    {
	output(arg, 1);
    }
    return arg;
}

void makeCall (Token *prc, Token *arg)
{
    if (prc != noToken)
    {
    	output(&startToken, 0);
	if (tokEqual(prc, "}")) { warn(prc, "unmatched } found"); }
	output(prc, 1);
    }
    while (arg != noToken)
    {
	if ((arg->type == CONST || arg->type == LIST) && arg->text[0] == '-' &&
	  arg->next != noToken)
	{
/*
 * Be careful here - watch out for the case where -option is used as an
 * access function rather than to set the command!!
 */
	    if (xf)
	    {
		output(&xcontToken, 1);
	    }
	    output(arg, 1);
	    arg = handleOpt(arg);
	}
	else
	{
            checkUnquoted(arg);
	    output(arg, 1);
	}
	arg = arg->next;
    }
}

Token *doswitch(Token *cmd, Token *leadin)
{
    Token *tp, *bod;
    int ln = leadin->lineNo, eopt = 0, dflt = 0, srtp = 1;
    Token *inbrace = &lbraceToken;
    Token *outbrace = &rbraceToken;

    tp = cmd;
    while (isSwitch(tp))
    {
	output(tp , 1 );
	if (tokEqual(tp, "--"))
	{
	    tp = tp->next;
	    eopt = 1;
	    break;
	}
	tp = tp->next;
    }
    failIfNullToken(tp, "string", "switch", ln);
    if ((fascist || checksw) && !eopt)
    {
	warn(tp, "switch statement has no --");
    }
    output(tp, 1);
    tp = tp->next;
    failIfNullToken(tp, "pattern", "switch", ln);
    if (tp->next != noToken && tp->next->type != SCOMMENT)
    { /* this the non-list format */
	if (switchIn) { setIndent(); }
	while(tp != noToken && tp->type != SCOMMENT)
	{
	    setUnreachable(0);
	    output(&contToken, 1 );
	    output(tp, 1);
	    if (tokEqual(tp, "default"))
	    {
		dflt = 1;
	    }
	    tp = tp->next;
	    failIfNullToken(tp, "body", "switch", ln);
	    if (tokEqual(tp, "-"))
	    {
		output(tp, 1);
	    }
	    else
	    {
		setIndent();
		body(tp, 0, 0);
		srtp &= isUnreachable();
		outdent();
	    }
	    tp = tp->next;
	}
	handlescom(tp);
	if (switchIn) { outdent(); }
    }
    else
    {
	switch (tp->type)
	{
	case CONST :
	    if (tp->ckind == CNSTSTRING)
	    {
		inbrace = &dqStart;
		outbrace = &dqEnd;
	    }
	case LIST :
	    tp = bod = tokacc(tp, 0, 0);
	    output(inbrace, 0);
	    if (switchIn) { setIndent(); }
	    while (tp != noToken)
	    {
		if (tp->type == COMMENT || tp->type == PRAGMA) {
		    output(tp, 1 );
		}
		else
		{
		    setUnreachable(0);
		    output(&ostartToken, 1);
		    output(tp, 1 );
		    if (tokEqual(tp, "default"))
		    {
			dflt = 1;
		    }
		    tp = tp->next;
		    failIfNullToken(tp, "body", "switch", ln);
		    if (tokEqual(tp, "-"))
		    {
			output(tp, 1);
		    }
		    else
		    {
			setIndent();
			body(tp, 1, 0);
			srtp &= isUnreachable();
			outdent();
		    }
		}
		tp = tp->next;
	    }
	    if (switchIn) { outdent(); }
	    output(outbrace, 0);
	    freeToken(bod);
	    break;
	case STRING :
/*	    break; */
	default :
	    failIfNullToken(noToken, "body", "switch", ln);
	}
    }
    if (dflt)
    {
	setUnreachable(srtp);
    }
    else
    {
	if (fascist || checksw) { warn(leadin, "switch has no default handler"); }
	setUnreachable(0);
    }
    return noToken;
}

Token *doif(Token *cmd, Token *leadin)
{
    Token *tp, *then;
    int efl = ADDBRACES, ln = leadin->lineNo, cfl = ADDBRACES | PAREN;
    int srtp;

    failIfNullToken(cmd, "condition", "if", ln);
    warnExpr(cmd, "if condition not braced.");
    if (spaceout)
    {
	cfl |= SPACEOUT;
    }
    press(cmd, cfl, noChecks);
    if (putThen) { output(&thenToken, 0); }
    then = cmd->next;
    if (tokEqual(then, "then")) { then = then->next; }
    failIfNullToken(then, "then", "if", ln);
    body(then, 0, 0);
    srtp = isUnreachable();
    tp = then->next;
    while(tokEqual(tp, "elseif"))
    {
	setUnreachable(0);
	if (!nonlelsif) { output(&econtToken, 1); }
	output(&elseifToken, 0);
	tp = tp->next;
	failIfNullToken(tp, "condition", "elseif", ln);
        warnExpr(tp, "elseif condition not braced.");
	press(tp, efl | PAREN, noChecks);
	tp = tp->next;
	failIfNullToken(tp, "body", "elseif", ln);
	body(tp, 0, 0);
	srtp &= isUnreachable();
	tp = tp->next;;
    }
    setUnreachable(0);
    if (tokEqual(tp, "else"))
    {
	if (putElse) { output(&elseToken, 0 ); }
	tp = tp->next;
	failIfNullToken(tp, "body", "else", ln);
	body(tp, 0, 0);
	srtp &= isUnreachable();
    }
    else if (tp != noToken)
    {
	if (tp->type != SCOMMENT)
	{
	    if (putElse) { output(&elseToken, 0 ); }
	    body(tp, 0, 0);
	    srtp &= isUnreachable();
	    warnIfArgsAfter(tp, "else", "if");
	}
	else
	{
	    output(tp, 0);
	    srtp = 0;
	}
    }
    else
    {
	srtp = 0;
    }
    setUnreachable(srtp);
    return noToken;
}


int isNSName(Token *cmd)
{
    char *cp;
    int cols = 0;

    if (!constant(cmd) || cmd->text == (char *) 0) { return 0; }
    cp = cmd->text;
    while (*cp)
    {
        if (*cp++ == ':')
	{
	    cols += 1;
	}
	else
	{
	    if (cols == 2)
	    {
	        return 1;
	    }
	    if (cols == 1)
	    {
	        warn(cmd, "Single ':' character in proc name");
	    }
	    else if (cols > 2)
	    {
	        warn(cmd, "More than two sequential ':' characters in proc name");
		return 1;
	    }
	    cols = 0;
	}
    }
    return 0;
}

static Token *doProc(Token *tag, Token *cmd, Token *leadin, int checkNS)
{
    int ln = leadin->lineNo, inns;
    Token *token, *tok2, *tok3;
    Blox *pr = (Blox *) 0, *tbp;
    VarData *rp;

    if (tag == &methodToken && nest[0] == 0)
    {
	makeCall(noToken, cmd);
    }
    else
    {
	failIfNullToken(cmd, "name", tag->text, ln);
	inproc = 1;
	checkName(cmd, 1);
	output(cmd, 1);
	if ((inns = isNSName(cmd)))
	{   
	    nest[1] += 1;
	}
        pr = pushBlock(cmd, 0, -1, 0);
	cmd = cmd->next;
	failIfNullToken(cmd, "args", tag->text, ln);
	switch (cmd->type)
	{
	case CONST :
	    if (cmd->ckind == CNSTWORD)
	    {
	        checkName(cmd, 0);
	        rp = declareVar(cmd, VARG, 0);
	        rp->set = 1;
	        if (embrace) { output(&lbraceToken, 1); }
	        output(cmd, 1);
	        if (embrace) { output(&rbraceToken, 1); }
	        break;
	    }
	    /* CONSTANT STRING DROPS THROUGH */
	
	case LIST:
	    output(&lbraceToken, 1);
	    tok3 = token = tokacc(cmd, 0, 0);
	    while(token != noToken)
	    {
	        switch (token->type)
		{
		case ENDF:
		    break;

		case CONST:
		    if(token->ckind == CNSTWORD)
		    {
			rp = declareVar(token, VARG, 0);
			checkName(token, 0);
			rp->set = 1;
			output(token, 1);
			break;
		    }
		/* CONSTANT STRING DROPS THROUGH */
		case LIST:
		    output(&lbraceToken, 1);
		    tok2 = tokacc(token, 0, 0);
		    rp = declareVar(tok2, VARG, 0);
		    output(tok2, 1);
		    rp->set = 1;
		    checkName(token, 0);
		    if (tok2->next != noToken)
		    {
			rp->dval = 1;
			output(tok2->next, 1);
			warnIfArgsAfter(tok2->next, "default value syntax", "proc");
		    }
		    else
		    {
			warn(tok2, "possible missing default value");
		    }
		    output(&rbraceToken, 1);
		    freeToken(tok2);
		    break;

		default:
		    output(token, 1);
		    checkName(token, 0);
		    break;
		}
		token = token->next;
	    }
	    freeToken(tok3);
	    output(&rbraceToken, 1);
	    break;
	
	default:
	    if (warndyn)
	    {
	        warn(leadin, "dynamic argument list");
	    }
	    tbp = lpop(&blocks); /* evaluate in right context !*/
	    output(cmd, 1);
	    lpush(&blocks, tbp);
	}
	cmd = cmd->next;
	failIfNullToken(cmd, "body", tag->text, ln);
	body(cmd, 0, 1);
	warnIfArgsAfter(cmd, "body", tag->text);
	
	pr = (Blox *) lpeek(blocks);
	if (doTest(HRETURN) && !(pragma & NOTREACHED))
	{
	    if (!isUnreachable() && pr->returns != 0 && pr->result)
	    {
	    	warn(cmd, "possible missing final return statement");
	    }
	}
	popBlock(1);
        if (inns) { nest[1] -= 1; }
        setUnreachable(0);
	inproc = 0;
    }
    return noToken;
}

static Token *doproc(Token *cmd, Token *leadin)
{
    return doProc(&procToken, cmd, leadin, 1);
}

static Token *domethod(Token *cmd, Token *leadin)
{
    return doProc(&methodToken, cmd, leadin, 0);
}

static Token *dodestructor(Token *cmd, Token *leadin)
{
    if (!nest[0])
    {
	makeCall(noToken, cmd);
    }
    else
    {
	failIfNullToken(cmd, "body", "destructor", 0);
	body(cmd, 0, 0);
	warnIfArgsAfter(cmd, "body", "destructor");
    }
    return noToken;
}

Token *doconstructor(Token *cmd, Token *leadin)
{
    if (!nest[0])
    {
	makeCall(noToken, cmd);
    }
    else
    {
	failIfNullToken(cmd, "args", "constructor", 0);
	press(cmd, NOBRACE | ADDBRACES, noChecks);
	cmd = cmd->next;
	failIfNullToken(cmd, "body", "constructor", 0);
	body(cmd, 0, 0);
	warnIfArgsAfter(cmd, "body", "constructor");
    }
    return noToken;
}

Token *doreturn(Token *cmd, Token *leadin)
{
    Blox *pr = (Blox *) lpeek(blocks);

    if ((heuristics & HRETURN) && !(tokEqual(cmd, "-code") && cmd->next != (Token *) 0)  )
    {
	if (pr->level != 0)
	{
	    if (cmd != noToken)
	    {
		if (pr->returns != 0 && !pr->result)
		{
	    	    warn(cmd, "inconsistent return of values");
		}
		pr->result = 1;
	    }
	    else if (pr->returns != 0 && pr->result)
	    {
	    	warn(cmd, "inconsistent return of values");
	    }
	    pr->returns += 1;
	}
	else if (!(pragma & RETURNOK))
	{
	    warn(cmd, "return used outside a proc body");
	}
    }
    for(;;)
    {
	if (!isSwitch(cmd) || tokEqual(cmd, "-1"))
	{
	    break;
	}
	output(cmd, 1);
	cmd = cmd->next;
	failIfNullToken(cmd, "flag value", "return", leadin->lineNo);
	output(cmd, 1);
	cmd = cmd->next;
    }
    
    if (cmd != noToken)
    {
	output(cmd, 1);
	warnIfArgsAfter(cmd, "value", "return");
    }
    setUnreachable(1);
    return noToken;
}

Token *doregexp(Token *cmd, Token *leadin)
{
    int eopt = 0;
    int ln = cmd->lineNo, paramCount = 0;

    while (isSwitch(cmd))
    {
	output(cmd, 1);
	paramCount += 1;
	if (tokEqual(cmd, "--"))
	{
	    cmd = cmd->next;
	    eopt = 1;
	    break;
	}
	if (tokEqual(cmd, "-start"))
	{
	    output(cmd->next, 1);
	    paramCount += 1;
	    cmd = cmd->next;
	}
	cmd = cmd->next;
    }
    if (doTest(HREGEXP) && !eopt)
    { /* warn about missing -- only if the item could start with a - */
	if (fascist || (!(cmd->type == CONST || cmd->type == LIST) &&
	  !(cmd->type == CONC && cmd->sequence->type == CONST)))
	{
	    warn(cmd, "regexp call has no --");
	}
    }
    output(cmd, 1); /* the regexp - could check it for syntax if constant. */
    failIfNullToken(cmd, "string", "regexp", ln);
    cmd = cmd->next;
    output(cmd, 1);
    /* everything else should be variable names now.... */
    paramCount += 2;
    while((cmd = cmd->next) != noToken)
    {
	handleVar(leadin, cmd, (VLOCAL << 8) | VC_SET, paramCount);
	paramCount += 1;
    }
    return noToken;
}

Token *doregsub(Token *cmd, Token *leadin)
{
    int eopt = 0;
    int ln = cmd->lineNo, paramCount = 0;
    Token *tp;

    while (isSwitch(cmd))
    {
        tp = cmd;
	cmd = cmd->next;
	output(tp, 1);
	paramCount += 1;
	if (tokEqual(tp, "--"))
	{
	    eopt = 1;
	    break;
	}
	if (tokEqual(tp, "-start"))
	{
	    output(cmd, 1);
	    paramCount += 1;
	    cmd = cmd->next;
	}
    }
    
    failIfNullToken(cmd, "regexp", "regsub", ln);
    if (doTest(HREGEXP) && !eopt)
    { /* warn about missing -- only if the item could start with a - */
	if (fascist || (!(cmd->type == CONST || cmd->type == LIST) &&
	  !(cmd->type == CONC && cmd->sequence->type == CONST)))
	{
	    warn(cmd, "regsub call has no --");
	}
    }
    paramCount += 1;
    output(cmd, 1); /* the regexp - could check it for syntax if constant. */
    failIfNullToken(cmd, "string", "regsub", ln);
    cmd = cmd->next;
    output(cmd, 1);
    cmd=cmd->next;
    failIfNullToken(cmd, "substitution", "regsub", ln);
    output(cmd, 1);
    cmd = cmd->next;
    paramCount += 3;
    failIfNullToken(cmd, "variable", "regsub", ln);
    handleVar(leadin, cmd, VC_SET, paramCount);
    return cmd->next;
}

Token *dobind(Token *cmd, Token *leadin)
{
    Token *np;
    if (!doBind || pragma & NOFORMAT)
    {
	makeCall(noToken,cmd);
    }
    else
    {
	failIfNullToken(cmd, "windowSpec", "bind", 0);
	output(cmd, 1);
	if ((np = cmd->next) != noToken)
	{
	    output(np, 1);
	    if ((np->next) != noToken)
	    { /* need to deactivate break outside a loop test. */
		pushBlock(noToken, 0, -1, 0); 
		catbin(np->next);
		popBlock(0);
		warnIfArgsAfter(np->next, "command", "bind");
	    }
	}
    }
    return noToken;
}

Token *doitcl_class(Token *cmd, Token *leadin)
{
    failIfNullToken(cmd, "className", "itcl_class", 0);
    output(cmd, 1);
    nest[0] += 1;
    cmd = cmd->next;
    failIfNullToken(cmd, "body", "itcl_class", 0);
    body(cmd, 0, 0);
    warnIfArgsAfter(cmd, "body", "itcl_class");
    nest[0] -= 1;
    return noToken;
}

Token *docvar(Token *cmd, Token *prt)
{
    if (!nest[0])
    {
	makeCall(noToken, cmd);
    }
    else
    {
	failIfNullToken(cmd, "varName", prt->text, 0);
	output(cmd, 1);
	if ((cmd = cmd->next) != noToken)
	{
	    press(cmd, NOBRACE | ADDBRACES, noChecks);
	    warnIfArgsAfter(cmd, "init", prt->text);
	}
    }
    return noToken;
}

Token *dopublic(Token *cmd, Token *leadin)
{
    if (!nest[0])
    {
	makeCall(noToken, cmd);
    }
    else
    {
	failIfNullToken(cmd, "varName", "public", 0);
	output(cmd, 1);
	if ((cmd = cmd->next) != noToken)
	{
	    press(cmd, NOBRACE | ADDBRACES, noChecks);
	    if ((cmd = cmd->next) != noToken)
	    {
		body(cmd, 0, 0);
		warnIfArgsAfter(cmd, "config", "public");
	    }
	}
    }
    return noToken;
}

Token *doprotected(Token *cmd, Token *leadin) { return docvar(cmd, leadin); }

Token *docommon(Token *cmd, Token *leadin) { return docvar(cmd, leadin); }

static void checkVar(Token *cmd, char *nm)
{
    List *bp = blocks;
    Blox *xp;
    List *vp;
    VarData *vip;

    while (bp != noList)
    {
        xp = (Blox *) lpeek(blocks);
	if (xp->name != noToken)
	{
	    break; /* we are out of any nested fors */
	}
	vp = xp->vars;
	while (vp != noList)
	{
	    vip = (VarData *) lpeek(vp);
	    if (vip->type == VFOR && strcmp((char *) vip->name, nm) == 0)
	    {
		if (doTest(HFOREACH)) { warn(cmd, "Possible nested reuse of foreach variable"); }
	    }
	    vp = vp->next; /* check against all the vars */
	}
	bp = bp->next; /* check against nested fors */
    }
}

static void addForVars(Token *cmd)
{
    Token *vr, *vp, *ap;
    VarData *rp;

    switch (cmd->type)
    {
    case CONST:
	checkVar(cmd, cmd->text);
	if ((ap = isArray(cmd)) != noToken)
	{
	    rp = setVar(ap, VFOR, 1);
	    freeToken(ap);
	}
	else
	{
	    rp = setVar(cmd, VFOR, 0);
	}
	break;

    case LIST:
	vp = vr = tokacc(cmd, 0, 0);
	while (vp != noToken)
	{
	    if (vp->type == NL || vp->type == SP || vp->type == ENDF)
	    {
		if (doTest(HFOREACH)) { warn(vp, "Possible bracing error detected"); }
	    }
	    else
	    {
		checkVar(vp, vp->text);
		if ((ap = isArray(vp)) != noToken)
		{
		    rp = setVar(ap, VFOR, 1);
		    freeToken(ap);
		}
		else
		{
		    rp = setVar(vp, VFOR, 0);
		}
	    }
	    vp = vp->next;
	}
	freeToken(vr);
	break;

    default:
	break;
    }
}

Token *doforeach(Token *cmd, Token *leadin)
{
    failIfNullToken(cmd, "varName", "foreach", 0);
    loopstart(0, 0);
    do
    {
	output(cmd, 0);
	addForVars(cmd);
	cmd = cmd->next;
	failIfNullToken(cmd, "list", "foreach", 0);
	press(cmd, NOBRACE, noChecks);
	cmd = cmd->next;
    }
    while (cmd != noToken && cmd->next != noToken && cmd->next->type != SCOMMENT);
    failIfNullToken(cmd, "body", "foreach", 0);
    body(cmd, 0, 0);
    loopend();
    warnIfArgsAfter(cmd, "body", "foreach");
    setUnreachable(0);
    return noToken;
}

Token *doloop(Token *cmd, Token *leadin)
{
    Token *tp;
    if (!tclX)
    {
	makeCall(noToken, cmd);
    }
    else
    {
	failIfNullToken(cmd, "var", "loop", 0);
	press(cmd, NOBRACE | ADDBRACES, noChecks);	/* var */
	tp = cmd->next;
	failIfNullToken(tp, "first", "loop", 0);
	warnExpr(cmd, "Unbracketed loop \"first\"");
	press(tp, ADDBRACES, noChecks);			/* first */
	tp = tp->next;
	failIfNullToken(tp, "limit", "loop", 0);
	warnExpr(cmd, "Unbracketed loop \"limit\"");
	press(tp, ADDBRACES, noChecks);			/* limit */

	tp = tp->next;
	failIfNullToken(tp, "body", "loop", 0);

	if (tp->next != noToken)
	{
	    warnExpr(cmd, "Unbracketed loop \"incr\"");
	    press(tp, NOBRACE | ADDBRACES, noChecks);		/* incr */
	    tp = tp->next;
	}
	loopstart(0, 0);
	body(tp, 0, 0);			/* body */
	loopend();
	warnIfArgsAfter(tp, "body", "loop");
    }
    setUnreachable(0);
    return noToken;
} 

Token *doexpr(Token *cmd, Token *leadin)
{
    if (!doExpr)
    {
	makeCall(noToken,cmd);
    }
    else
    {
	if (cmd == noToken)
	{
	    warn(cmd, "Missing expression body");
	}
	else if (cmd->next == noToken)
	{
	    warnExpr(cmd, "expr body not braced.");
	    press(cmd, ADDBRACES | PAREN, noChecks); /* */
	}
	else
	{
	    warn(cmd, "expr body not braced.");
	    etcetera(cmd, 1);
	}
    }
    return noToken;
}

Token *dounset(Token *cmd, Token *leadin)
{
    int pCount = 1;

    if (isSwitch(cmd))
    {
	if (tokEqual(cmd, "-nocomplain") || tokEqual(cmd, "--"))
	{
	    output(cmd, 0);
	    cmd = cmd->next;
	}
	else if (doTest(HSWITCH) && cmd->type != CONST && cmd->type != LIST)
	{
	    warn(cmd, "unset call has no -- (>=8.4)");
	}
    }
    else if (doTest(HSWITCH) && isVarToken(cmd))
    {
	warn(cmd, "unset call has no -- (>8.4)");
    }

    do
    {
        handleVar(leadin, cmd, VC_NONE, pCount);
	pCount += 1;
	cmd = cmd->next;
    }
    while (cmd != noToken);
    return noToken;
}

Token *doupvar(Token *cmd, Token *leadin)
{
    int paramCount = 0;
    int ln = leadin->lineNo;
    int glob = 0;

    if (tokIsLevel(cmd))
    {
        if (tokEqual(cmd, "#0") || tokEqual(cmd, "\\#0"))
	{
	    glob = VGLOBAL << 8;
	}
	output(cmd, 0);
	cmd = cmd->next;
	paramCount += 1;
    }
    failIfNullToken(cmd, "variable list", "upvar", ln);
    do
    {
	output(cmd, 0);
	paramCount += 1;
	cmd = cmd->next;
        failIfNullToken(cmd, "variable name", "upvar", ln);
	paramCount += 1;
	handleVar(leadin, cmd, VC_SET | VC_DECL | glob, paramCount);
    }
    while ((cmd = cmd->next) != noToken);

    return noToken;
}

Token *dovariable(Token *cmd, Token *leadin)
{
    int paramCount = 1;

    if (nest[1] == 0)
    {
	warn(leadin, "variable command used outside a namespace body");
	makeCall(noToken, cmd);
	return noToken;
    }
    failIfNullToken(cmd, "parameters", "variable", cmd->lineNo);
    do
    {
	if (cmd->next == noToken)
	{
	    handleVar(leadin, cmd, (VVAR << 8) | (inproc ? VC_SET : VC_NONE), paramCount);
	    break;
	}
	handleVar(leadin, cmd, VC_SET | (VVAR << 8), paramCount);
	output(cmd->next, 0);
	paramCount += 2;
	cmd = cmd->next->next;
    }
    while (cmd != noToken);

    return noToken;
}

static DoFunc builtins[] =
{
    0,			/* 0 */
    dobind,		/* 1 */
    docommon,		/* 2 */
    doconstructor,	/* 3 */
    dodestructor,	/* 4 */
    0,			/* 5 */
    doexpr,		/* 6 */
    doforeach,		/* 7 */
    doif,		/* 8 */
    0,			/* 9 */			
    doitcl_class,	/* 10 */
    doloop,		/* 11 */			
    domethod,		/* 12 */
    doregsub,		/* 13 */
    doproc,		/* 14 */
    doprotected,	/* 15 */
    dopublic,		/* 16 */
    doreturn,		/* 17 */
    doswitch, 		/* 18 */
    doregexp, 		/* 19 */
    doupvar, 		/* 20 */
    dovariable, 	/* 21 */
    dounset, 		/* 22 */
};

static int missable(ParamData *pt)
{
    switch (pt->type)
    {
    case PARGS:	/* these can be missing */
    case POPTIONS:
    case PSEQN0:
    case PBLOCK:
    case PBREAK:
    case PNEST:
    case PUNNEST:
    case POCTYPE:
    case PNONE :
	break;

    default:
	return (pt->optional != RSINGLE);
    }
    return 1;
}

static int valuecheck(ParamData *pdp, Token *cmd)
{
    char *mdl;
    List *vlist;

    if (pdp->type == POCTYPE || pdp->type == PCTYPE) return 1;
     
    vlist = pdp->values;
    while (vlist != noList)
    {
        mdl = (char *)lpeek(vlist);
	if (strcmp(mdl, cmd->text) == 0)
	{
	    return 1;
	}
	if (strncmp(mdl, cmd->text, cmd->length) == 0)
	{
	    if (doTest(HABBREV))
	    {
	        warnFor(cmd, mdl, "Abbreviation of %s option");
	    }
	    return 1;
	}
	vlist = vlist->next;
    }
    return 0;
}

void doBasic(ConfigData *cpt, Token *hd, Token *cmd)
{
    List *sp, *llp, *lp;
    ParamData *pt, *pdp;
    SeqnData *sd, *esc;
    Blox *blp;
    int paramCount, infloop, bcon;
    int ptpar;
    char msg[1024];

    lp = cpt->params;
    paramCount = 0;
    while (cmd != noToken && lp != noList)
    {
	pt = (ParamData *) lpeek(lp);
	if (pt != (ParamData *) 0 && pt->values != noList)
	{
	    ptpar = (int) lpeek(pt->values);
	}
	else
	{
	    ptpar = 0;
	}
	lp = lp->next;
	paramCount += 1;
	switch (pt->type)
	{
	case PVAR: /* variable name */
	    handleVar(hd, cmd, ptpar, paramCount);
	    break;

	case PVARLIST: /* a list of variable names - eats the line*/
	    do
	    {
	        handleVar(hd, cmd, ptpar, paramCount);
		paramCount += 1;
	    }
	    while ((cmd = cmd->next) != noToken);
	    break;

	case PCODE: /* loop code body */
	    if (cmd == noToken)
	    {
		warn(hd, "Missing body part");
	    }
	    else
	    {
		body(cmd, 0, 0);
	    }
	    loopend();
	    break;

	case PSCRIPT:
	    if (cmd == noToken)
	    {
		warn(hd, "Missing script part");
	    }
	    else
	    {
		catbin(cmd);
	    }
	    break;

	case PSQN:
	    if (cmd == noToken)
	    {
		warn(hd, "Missing sequence part");
	    }
	    else if (testonly)
	    {
		body(cmd, 0, 0);
	    }
	    else
	    {
		press(cmd, NOBRACE | ADDBRACES | SEMIS, noChecks); /* */
	    }
	    break;

	case PEXPR: /* an expression */
	    if (cmd == noToken)
	    {
		warn(hd, "Missing expression");
	    }
	    else
	    {
		warnExpr(cmd, "Unbracketed expression");
		press(cmd, ptpar, noChecks); /* */
	    }
	    break;

	case PCOND: /* an expression */
	    if (cmd == noToken)
	    {
		warn(hd, "Missing expression");
	    }
	    else
	    {
		infloop = tokEqual(cmd, "1");
		warnExpr(cmd, "Unbracketed expression");
		if (fascist || !infloop)
		{
		    warnExpr(cmd, "Unbracketed expression");
		}
		if (spaceout)
		{
		    ptpar |= SPACEOUT;
		}
		press(cmd, ptpar, noChecks);
		loopstart(infloop, 1);
	    }
	    break;

	case PBLTN: /* special builtin check */
	    if (ptpar <= 0)
	    {
		fail(cmd, "Internal config error!!!");
	    }
	    cmd = builtins[ptpar](cmd, hd);
	    continue;

	case PMANY:
	    llp = pt->values;
	    esc = (SeqnData *) lpeek(llp);
	    if (!doTest(HVALUE) || cmd == noToken ||
	      !(cmd->type == CONST || cmd->type == LIST))
	    { /* not enough info to process this so take first option */
		lp = esc->seqn;
	    }
	    else
	    {
		for (;;)
		{
		    if ((llp = llp->next) == noList && !noheuristics)
		    {
			sprintf(msg, "\"%s\" is an incorrect value for parameter %d of %s call",
			  cmd->text, paramCount, hd->text);
			warn(cmd, msg);
			lp = esc->seqn;
			bcon = 0;
			break;
		    }
		    sd = (SeqnData *) lpeek(llp);
		    pdp = (ParamData *) lpeek(sd->seqn);
		    if (valuecheck(pdp, cmd))
		    {
			output(cmd, 0);
			lp = sd->seqn->next;
			bcon = 1;
			break;
		    }
		}
		if (bcon)
		{
		    break;
		}
	    }
	    continue;

	case PSEQN0: /* none or more of the following */
	    break;

	case PSEQN1: /* one or more of the following */
	    break;

	case PPATTERN: /* match regular expression */
	    break;

	case PLEVEL: /* level number */
	    if (!tokIsLevel(cmd))
	    {
		if (pt->optional == ROPTIONAL)
		{
		    continue;
		}
		warn(cmd, "level number expected");
	    }
	    output(cmd, 0);
	    break;

	case PFLAG:
	    if (cmd->type == CONST || cmd->type == LIST)
	    {
		if (isSwitch(cmd))
		{
		    if (pt->optional == ROPTIONAL)
		    {
			continue;
		    }
		    warn(cmd, "flag expected");
		}
	    }
	    else
	    {
		warn(cmd, "expression result may be mistaken for flag");
	    }
	    output(cmd, 0);
	    break;

	case PCMDS: /* single list of code or a command sequence */
	    if (cmd == noToken)
	    {
		warn(hd, "Missing code");
	    }
	    else
	    {
		if (cmd->next != noToken)
		{
		    etcetera(cmd, 0);
		}
		else
		{
		    catbin(cmd);
		}
		cmd = (Token *) 0; /* eat up whole line */
	    }
	    continue;

	case PSINGLE :
	case PANY: /* anything at all */
        case PCTYPE: /* type checked anything */
        case POCTYPE: /* type checked anything */
	case POPTION:
	case PWINDOW:
	    if (cmd == noToken)
	    {
		if (pt->optional == ROPTIONAL)
		{
		    continue;
		}
		warnFor(hd, hd->text, "Call of %s with too few parameters");
		break;
	    }
	    checkType(cmd, ptpar);
	    if (doTest(HVALUE) && cmd->type == CONST
	      && pt->values != noList)
	    {
		if (!valuecheck(pt, cmd) && !noheuristics)
		{
		    sprintf(msg, "\"%s\" is an incorrect value for parameter %d of %s call",
		      cmd->text, paramCount, hd->text);
		    warn(cmd, msg);
		}
	    }
	    output(cmd, 0);
	    break;

	case PARGS:
        case PCTARGS:
	case POPTIONS:
	    etcetera(cmd, 1);
	    cmd = noToken;
	    continue;

	case PNEST:
	    nest[ptpar] += 1;
	    continue;

	case PUNNEST:
	    nest[ptpar] -= 1;
	    continue;

	case PBREAK:
	    /*
	     * a break statement exits from infinite loops
	     */
	    blp = (Blox *) lpeek(blocks);
	    if (blp->level == 0 || blp->name != noToken)
	    {
		 warnFor(hd, hd->text, "%s used outside a loop");
	    }
	    else if (ptpar)
	    {
		blp->continues += 1;
	    }
	    else
	    {
		blp->breaks += 1;
	    }
	    continue;

	case PBLOCK:
	    setUnreachable(1);	    
	    break;

        case PNONE :
	    if (cmd != noToken)
	    {
	        warnFor(hd, hd->text, "Bad argument to call of %s");
	    }
	    break;
	}
	if (cmd != noToken) { cmd = cmd->next; }
    }
    if (cmd != noToken) /* there is stuff left on the line */
    {
	if (cmd->type == SCOMMENT) /* it's a terminal comment */
	{
	    output(cmd, 1);
	}
	else if (doTest(HNUMBER)) /* oops - too many parameters */
	{
	    warnFor(hd, hd->text, "Too many arguments for call of %s");
	}
    }
    while (lp != noList) /* check for missing parameters */
    {
	pt = (ParamData *) lpeek(lp);
	if ((sp = pt->values) != noList)
	{
	    ptpar = (int) lpeek(sp);
	}
	else
	{
	    ptpar = 0;
	}
	if (!missable(pt))
	{ /* call the builtin which will either work or deal with the error*/
	    if (pt->type == PBLTN)
	    {
		if (ptpar <= 0)
		{
		    fail(cmd, "Internal config error!!!");
		}
		cmd = builtins[ptpar](cmd, hd);
	    }
	    else
	    {
		warnFor(hd, hd->text, "Call of %s with too few parameters");
		break;
	    }
	}
	else if (pt->type == PNEST)
	{
	    nest[ptpar] += 1;
	}
	else if (pt->type == PUNNEST)
	{
	    nest[ptpar] -= 1;
	}
	else if (pt->type == PBLOCK)
	{
	    setUnreachable(1);
	}
	else if (pt->type == PBREAK)
	{
	    blp = (Blox *) lpeek(blocks);
	    if (blp->level == 0 || blp->name != noToken)
	    {
		 warnFor(hd, hd->text, "%s used outside a loop");
	    }
	    else if (ptpar)
	    {
		blp->continues += 1;
	    }
	    else
	    {
		blp->breaks += 1;
	    }
	}
	
	lp = lp->next;
    }
}

void doUser(Token *hd, Token *cmd, ConfigData *cpt, int nostart)
{
    if (isUnreachable())
    {
	warn(hd, "statement is unreachable");
	setUnreachable(0);
    }
    if (!nostart)
    {
	output(&startToken, 0);
    }
    output(hd, 1);
    lineNumber = hd->lineNo;
    doBasic(cpt, hd, cmd);
    lineNumber = 0;
}

int tclop(Token *hd, Token *line)
{
    List *cpt = config;
    ConfigData *cdp;

    while (cpt != noList)
    {
        cdp = (ConfigData *) lpeek(cpt);
	if (!strcmp(hd->text, cdp->name))
	{
	   doUser(hd, line, cdp, 0);
	   return 1;
	}
	cpt = cpt->next;
    }
    return 0;
}
