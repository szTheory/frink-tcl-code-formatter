/*
 *	$Source: /home/nlfm/Working/CVS/frink/flagvars.h,v $
 *	$Date: 2002/08/27 10:23:24 $
 *	$Revision: 2.3 $
 *
 *------------------------------------------------------------------------
 *   AUTHOR:  Lindsay Marshall <lindsay.marshall@newcastle.ac.uk>
 *------------------------------------------------------------------------
 *    Copyright 1994-2002 The University of Newcastle upon Tyne (see COPYRIGHT)
 *========================================================================
 *
 */
 
extern int failed;

extern int lineNumber;
extern char *currentfile;

extern int compout;
extern int noquotes;
extern int makespec;
extern int warndyn;
extern int fascist;
extern int checksw;
extern int checkrx;
extern int spaceout;
extern int internat;
extern int extract;
extern int olcomments;
extern int doTime;
extern int elseif;
extern int doExpr;
extern int lineNumber;
extern int lmargin;
extern int switchIn;
extern int width;
extern int addSpaces;
extern int indent;
extern int contdent;
extern int nocomments;
extern int obfuscate;
extern int tabsOn;
extern int putElse;
extern int putThen;
extern int tabStops;
extern int tclX;
extern int minimise;
extern int xf;
extern int oneliner;
extern int doBind;
extern int haltonWarn;
extern int debrace;
extern int trace;
extern int varbrace;
extern int noblanks;
extern int embrace;
extern int procnls;
extern int ifelse;
extern int nonlelsif;
extern int testonly;
extern unsigned int heuristics;
extern int noheuristics;
extern int nocommand;
extern int trystrings;

extern char *style;
extern char *pstyle;
extern char *contString;

extern char *locale;
extern FILE *msgfile;
extern FILE *specfile;

extern int pragma;
extern int praghold;
