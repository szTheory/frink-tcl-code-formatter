/*
 *	$Source: /home/nlfm/Frink/frink.c,v $
 *	$Date: 2003/10/02 20:17:47 $
 *	$Revision: 1.1 $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "frink.h"

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

int failed	= 0;
int lineNumber	= 0;
int makespec	= 0;
int checksw	= 0;
int checkrx	= 0;
int warndyn	= 0;

int fascist	= 0;

int compout	= 0;
int resvsplit	= 0;
int noquotes	= 0;
int spaceout	= 0;
int trystrings	= 0;
int nocommand	= 0;
int testonly	= 0;
unsigned int heuristics	= 0x04; /* we need this to make some of the stuff work!! */
int noheuristics = 1;   /* added to silence the reporting from the previous ..... */
int lmargin	= 0;
int tclX	= 0;
int addSpaces	= 0;
int width	= 80;
int indent	= 4;
int contdent	= 2;
int nocomments	= 0;
int obfuscate	= 0;
int tabsOn	= 1;
int putElse	= 0;
int putThen	= 0;
int tabStops	= 8;
int minimise	= 0;
int xf		= 0;
int oneliner	= 0;
int doBind	= 1;
int haltonWarn	= 0;
int doExpr	= 1;
int debrace	= 0;
int embrace	= 0;
int trace	= 0;
int varbrace	= 0;
int noblanks	= 0;
int procnls	= 0;
int switchIn	= 0;
int elseif	= 0;
int doTime	= 1;
int olcomments	= 1;
int ifelse	= 0;
int nonlelsif	= 0;
int extract	= 0;
int internat	= 0;

char *style		= (char *) 0;
char *pstyle		= (char *) 0;
char *contString 	= "";

char *locale		= (char *) 0;
FILE *msgfile		= NULL;
FILE *specfile		= NULL;

int pragma		= 0;
int praghold		= 0;

char *currentfile	= (char *) 0;

List *skiplist = (List *) 0;

#ifndef __FreeBSD__
extern int getopt(int, char*const*, const char*);
#endif

extern int optind;
extern char *optarg;
extern int fclose(FILE*);
extern void readconfig(char *);
extern void stringconfig(char *);
extern void clearState(void);

static void doVersion(void)
{
    printf("Frink " VMAJOR "." VMINOR "." VSUB " patch level " VPATCH "\n");
    exit(0);
}

static char *languages[] =
{
    "aa",	/* Afar	*/
    "ab",	/* Abkhazian	*/
    "af",	/* Afrikaans	*/
    "am",	/* Amharic	*/
    "ar",	/* Arabic	*/
    "as",	/* Assamese	*/
    "ay",	/* Aymara	*/
    "az",	/* Azerbaijani	*/
    "ba",	/* Bashkir	*/
    "be",	/* Byelorussian	*/
    "bg",	/* Bulgarian	*/
    "bh",	/* Bihari	*/
    "bi",	/* Bislama	*/
    "bn",	/* Bengali; Bangla	*/
    "bo",	/* Tibetan	*/
    "br",	/* Breton	*/
    "ca",	/* Catalan	*/
    "co",	/* Corsican	*/
    "cs",	/* Czech	*/
    "cy",	/* Welsh	*/
    "da",	/* Danish	*/
    "de",	/* German	*/
    "dz",	/* Bhutani	*/
    "el",	/* Greek	*/
    "en",	/* English	*/
    "eo",	/* Esperanto	*/
    "es",	/* Spanish	*/
    "et",	/* Estonian	*/
    "eu",	/* Basque	*/
    "fa",	/* Persian	*/
    "fi",	/* Finnish	*/
    "fj",	/* Fiji	*/
    "fo",	/* Faroese	*/
    "fr",	/* French	*/
    "fy",	/* Frisian	*/
    "ga",	/* Irish	*/
    "gd",	/* Scots Gaelic	*/
    "gl",	/* Galician	*/
    "gn",	/* Guarani	*/
    "gu",	/* Gujarati	*/
    "ha",	/* Hausa	*/
    "he",	/* Hebrew (formerly iw)	*/
    "hi",	/* Hindi	*/
    "hr",	/* Croatian	*/
    "hu",	/* Hungarian	*/
    "hy",	/* Armenian	*/
    "ia",	/* Interlingua	*/
    "id",	/* Indonesian (formerly in)	*/
    "ie",	/* Interlingue	*/
    "ik",	/* Inupiak	*/
    "is",	/* Icelandic	*/
    "it",	/* Italian	*/
    "iu",	/* Inuktitut	*/
    "ja",	/* Japanese	*/
    "jw",	/* Javanese	*/
    "ka",	/* Georgian	*/
    "kk",	/* Kazakh	*/
    "kl",	/* Greenlandic	*/
    "km",	/* Cambodian	*/
    "kn",	/* Kannada	*/
    "ko",	/* Korean	*/
    "ks",	/* Kashmiri	*/
    "ku",	/* Kurdish	*/
    "ky",	/* Kirghiz	*/
    "la",	/* Latin	*/
    "ln",	/* Lingala	*/
    "lo",	/* Laothian	*/
    "lt",	/* Lithuanian	*/
    "lv",	/* Latvian, Lettish	*/
    "mg",	/* Malagasy	*/
    "mi",	/* Maori	*/
    "mk",	/* Macedonian	*/
    "ml",	/* Malayalam	*/
    "mn",	/* Mongolian	*/
    "mo",	/* Moldavian	*/
    "mr",	/* Marathi	*/
    "ms",	/* Malay	*/
    "mt",	/* Maltese	*/
    "my",	/* Burmese	*/
    "na",	/* Nauru	*/
    "ne",	/* Nepali	*/
    "nl",	/* Dutch	*/
    "no",	/* Norwegian	*/
    "oc",	/* Occitan	*/
    "om",	/* (Afan) Oromo	*/
    "or",	/* Oriya	*/
    "pa",	/* Punjabi	*/
    "pl",	/* Polish	*/
    "ps",	/* Pashto, Pushto	*/
    "pt",	/* Portuguese	*/
    "qu",	/* Quechua	*/
    "rm",	/* Rhaeto-Romance	*/
    "rn",	/* Kirundi	*/
    "ro",	/* Romanian	*/
    "ru",	/* Russian	*/
    "rw",	/* Kinyarwanda	*/
    "sa",	/* Sanskrit	*/
    "sd",	/* Sindhi	*/
    "sg",	/* Sangho	*/
    "sh",	/* Serbo-Croatian	*/
    "si",	/* Sinhalese	*/
    "sk",	/* Slovak	*/
    "sl",	/* Slovenian	*/
    "sm",	/* Samoan	*/
    "sn",	/* Shona	*/
    "so",	/* Somali	*/
    "sq",	/* Albanian	*/
    "sr",	/* Serbian	*/
    "ss",	/* Siswati	*/
    "st",	/* Sesotho	*/
    "su",	/* Sundanese	*/
    "sv",	/* Swedish	*/
    "sw",	/* Swahili	*/
    "ta",	/* Tamil	*/
    "te",	/* Telugu	*/
    "tg",	/* Tajik	*/
    "th",	/* Thai	*/
    "ti",	/* Tigrinya	*/
    "tk",	/* Turkmen	*/
    "tl",	/* Tagalog	*/
    "tn",	/* Setswana	*/
    "to",	/* Tonga	*/
    "tr",	/* Turkish	*/
    "ts",	/* Tsonga	*/
    "tt",	/* Tatar	*/
    "tw",	/* Twi	*/
    "ug",	/* Uighur	*/
    "uk",	/* Ukrainian	*/
    "ur",	/* Urdu	*/
    "uz",	/* Uzbek	*/
    "vi",	/* Vietnamese	*/
    "vo",	/* Volapuk	*/
    "wo",	/* Wolof	*/
    "xh",	/* Xhosa	*/
    "yi",	/* Yiddish (formerly ji)	*/
    "yo",	/* Yoruba	*/
    "za",	/* Zhuang	*/
    "zh",	/* Chinese	*/
    "zu",	/* Zulu		*/
    (char *) 0
};

static char *countries[] =
{
    "AF",	/* AFGHANISTAN */
    "AL",	/* ALBANIA */
    "DZ",	/* ALGERIA */
    "AS",	/* AMERICAN SAMOA */
    "AD",	/* ANDORRA */
    "AO",	/* ANGOLA */
    "AI",	/* ANGUILLA */
    "AQ",	/* ANTARCTICA */
    "AG",	/* ANTIGUA AND BARBUDA */
    "AR",	/* ARGENTINA */
    "AM",	/* ARMENIA */
    "AW",	/* ARUBA */
    "AU",	/* AUSTRALIA */
    "AT",	/* AUSTRIA */
    "AZ",	/* AZERBAIJAN */
    "BS",	/* BAHAMAS */
    "BH",	/* BAHRAIN */
    "BD",	/* BANGLADESH */
    "BB",	/* BARBADOS */
    "BY",	/* BELARUS */
    "BE",	/* BELGIUM */
    "BZ",	/* BELIZE */
    "BJ",	/* BENIN */
    "BM",	/* BERMUDA */
    "BT",	/* BHUTAN */
    "BO",	/* BOLIVIA */
    "BA",	/* BOSNIA AND HERZEGOVINA */
    "BW",	/* BOTSWANA */
    "BV",	/* BOUVET ISLAND */
    "BR",	/* BRAZIL */
    "IO",	/* BRITISH INDIAN OCEAN TERRITORY */
    "BN",	/* BRUNEI DARUSSALAM */
    "BG",	/* BULGARIA */
    "BF",	/* BURKINA FASO */
    "BI",	/* BURUNDI */
    "KH",	/* CAMBODIA */
    "CM",	/* CAMEROON */
    "CA",	/* CANADA */
    "CV",	/* CAPE VERDE */
    "KY",	/* CAYMAN ISLANDS */
    "CF",	/* CENTRAL AFRICAN REPUBLIC */
    "TD",	/* CHAD */
    "CL",	/* CHILE */
    "CN",	/* CHINA */
    "CX",	/* CHRISTMAS ISLAND */
    "CC",	/* COCOS (KEELING) ISLANDS */
    "CO",	/* COLOMBIA */
    "KM",	/* COMOROS */
    "CG",	/* CONGO */
    "CD",	/* CONGO, THE DEMOCRATIC REPUBLIC OF THE */
    "CK",	/* COOK ISLANDS */
    "CR",	/* COSTA RICA */
    "CI",	/* COTE D'IVOIRE */
    "HR",	/* CROATIA */
    "CU",	/* CUBA */
    "CY",	/* CYPRUS */
    "CZ",	/* CZECH REPUBLIC */
    "DK",	/* DENMARK */
    "DJ",	/* DJIBOUTI */
    "DM",	/* DOMINICA */
    "DO",	/* DOMINICAN REPUBLIC */
    "TP",	/* EAST TIMOR */
    "EC",	/* ECUADOR */
    "EG",	/* EGYPT */
    "SV",	/* EL SALVADOR */
    "GQ",	/* EQUATORIAL GUINEA */
    "ER",	/* ERITREA */
    "EE",	/* ESTONIA */
    "ET",	/* ETHIOPIA */
    "FK",	/* FALKLAND ISLANDS (MALVINAS) */
    "FO",	/* FAROE ISLANDS */
    "FJ",	/* FIJI */
    "FI",	/* FINLAND */
    "FR",	/* FRANCE */
    "GF",	/* FRENCH GUIANA */
    "PF",	/* FRENCH POLYNESIA */
    "TF",	/* FRENCH SOUTHERN TERRITORIES */
    "GA",	/* GABON */
    "GM",	/* GAMBIA */
    "GE",	/* GEORGIA */
    "DE",	/* GERMANY */
    "GH",	/* GHANA */
    "GI",	/* GIBRALTAR */
    "GR",	/* GREECE */
    "GL",	/* GREENLAND */
    "GD",	/* GRENADA */
    "GP",	/* GUADELOUPE */
    "GU",	/* GUAM */
    "GT",	/* GUATEMALA */
    "GN",	/* GUINEA */
    "GW",	/* GUINEA-BISSAU */
    "GY",	/* GUYANA */
    "HT",	/* HAITI */
    "HM",	/* HEARD ISLAND AND MCDONALD ISLANDS */
    "VA",	/* HOLY SEE (VATICAN CITY STATE) */
    "HN",	/* HONDURAS */
    "HK",	/* HONG KONG */
    "HU",	/* HUNGARY */
    "IS",	/* ICELAND */
    "IN",	/* INDIA */
    "ID",	/* INDONESIA */
    "IR",	/* IRAN, ISLAMIC REPUBLIC OF */
    "IQ",	/* IRAQ */
    "IE",	/* IRELAND */
    "IL",	/* ISRAEL */
    "IT",	/* ITALY */
    "JM",	/* JAMAICA */
    "JP",	/* JAPAN */
    "JO",	/* JORDAN */
    "KZ",	/* KAZAKSTAN */
    "KE",	/* KENYA */
    "KI",	/* KIRIBATI */
    "KP",	/* KOREA, DEMOCRATIC PEOPLE'S REPUBLIC OF */
    "KR",	/* KOREA, REPUBLIC OF */
    "KW",	/* KUWAIT */
    "KG",	/* KYRGYZSTAN */
    "LA",	/* LAO PEOPLE'S DEMOCRATIC REPUBLIC */
    "LV",	/* LATVIA */
    "LB",	/* LEBANON */
    "LS",	/* LESOTHO */
    "LR",	/* LIBERIA */
    "LY",	/* LIBYAN ARAB JAMAHIRIYA */
    "LI",	/* LIECHTENSTEIN */
    "LT",	/* LITHUANIA */
    "LU",	/* LUXEMBOURG */
    "MO",	/* MACAU */
    "MK",	/* MACEDONIA, THE FORMER YUGOSLAV REPUBLIC OF */
    "MG",	/* MADAGASCAR */
    "MW",	/* MALAWI */
    "MY",	/* MALAYSIA */
    "MV",	/* MALDIVES */
    "ML",	/* MALI */
    "MT",	/* MALTA */
    "MH",	/* MARSHALL ISLANDS */
    "MQ",	/* MARTINIQUE */
    "MR",	/* MAURITANIA */
    "MU",	/* MAURITIUS */
    "YT",	/* MAYOTTE */
    "MX",	/* MEXICO */
    "FM",	/* MICRONESIA, FEDERATED STATES OF */
    "MD",	/* MOLDOVA, REPUBLIC OF */
    "MC",	/* MONACO */
    "MN",	/* MONGOLIA */
    "MS",	/* MONTSERRAT */
    "MA",	/* MOROCCO */
    "MZ",	/* MOZAMBIQUE */
    "MM",	/* MYANMAR */
    "NA",	/* NAMIBIA */
    "NR",	/* NAURU */
    "NP",	/* NEPAL */
    "NL",	/* NETHERLANDS */
    "AN",	/* NETHERLANDS ANTILLES */
    "NC",	/* NEW CALEDONIA */
    "NZ",	/* NEW ZEALAND */
    "NI",	/* NICARAGUA */
    "NE",	/* NIGER */
    "NG",	/* NIGERIA */
    "NU",	/* NIUE */
    "NF",	/* NORFOLK ISLAND */
    "MP",	/* NORTHERN MARIANA ISLANDS */
    "NO",	/* NORWAY */
    "OM",	/* OMAN */
    "PK",	/* PAKISTAN */
    "PW",	/* PALAU */
    "PS",	/* PALESTINIAN TERRITORY, OCCUPIED */
    "PA",	/* PANAMA */
    "PG",	/* PAPUA NEW GUINEA */
    "PY",	/* PARAGUAY */
    "PE",	/* PERU */
    "PH",	/* PHILIPPINES */
    "PN",	/* PITCAIRN */
    "PL",	/* POLAND */
    "PT",	/* PORTUGAL */
    "PR",	/* PUERTO RICO */
    "QA",	/* QATAR */
    "RE",	/* REUNION */
    "RO",	/* ROMANIA */
    "RU",	/* RUSSIAN FEDERATION */
    "RW",	/* RWANDA */
    "SH",	/* SAINT HELENA */
    "KN",	/* SAINT KITTS AND NEVIS */
    "LC",	/* SAINT LUCIA */
    "PM",	/* SAINT PIERRE AND MIQUELON */
    "VC",	/* SAINT VINCENT AND THE GRENADINES */
    "WS",	/* SAMOA */
    "SM",	/* SAN MARINO */
    "ST",	/* SAO TOME AND PRINCIPE */
    "SA",	/* SAUDI ARABIA */
    "SN",	/* SENEGAL */
    "SC",	/* SEYCHELLES */
    "SL",	/* SIERRA LEONE */
    "SG",	/* SINGAPORE */
    "SK",	/* SLOVAKIA */
    "SI",	/* SLOVENIA */
    "SB",	/* SOLOMON ISLANDS */
    "SO",	/* SOMALIA */
    "ZA",	/* SOUTH AFRICA */
    "GS",	/* SOUTH GEORGIA AND THE SOUTH SANDWICH ISLANDS */
    "ES",	/* SPAIN */
    "LK",	/* SRI LANKA */
    "SD",	/* SUDAN */
    "SR",	/* SURINAME */
    "SJ",	/* SVALBARD AND JAN MAYEN */
    "SZ",	/* SWAZILAND */
    "SE",	/* SWEDEN */
    "CH",	/* SWITZERLAND */
    "SY",	/* SYRIAN ARAB REPUBLIC */
    "TW",	/* TAIWAN, PROVINCE OF CHINA */
    "TJ",	/* TAJIKISTAN */
    "TZ",	/* TANZANIA, UNITED REPUBLIC OF */
    "TH",	/* THAILAND */
    "TG",	/* TOGO */
    "TK",	/* TOKELAU */
    "TO",	/* TONGA */
    "TT",	/* TRINIDAD AND TOBAGO */
    "TN",	/* TUNISIA */
    "TR",	/* TURKEY */
    "TM",	/* TURKMENISTAN */
    "TC",	/* TURKS AND CAICOS ISLANDS */
    "TV",	/* TUVALU */
    "UG",	/* UGANDA */
    "UA",	/* UKRAINE */
    "AE",	/* UNITED ARAB EMIRATES */
    "GB",	/* UNITED KINGDOM */
    "US",	/* UNITED STATES */
    "UM",	/* UNITED STATES MINOR OUTLYING ISLANDS */
    "UY",	/* URUGUAY */
    "UZ",	/* UZBEKISTAN */
    "VU",	/* VANUATU */
    "VE",	/* VENEZUELA */
    "VN",	/* VIET NAM */
    "VG",	/* VIRGIN ISLANDS, BRITISH */
    "VI",	/* VIRGIN ISLANDS, U.S. */
    "WF",	/* WALLIS AND FUTUNA */
    "EH",	/* WESTERN SAHARA */
    "YE",	/* YEMEN */
    "YU",	/* YUGOSLAVIA */
    "ZM",	/* ZAMBIA */
    "ZW",	/* ZIMBABWE */
    (char *) 0
};

static void checkLocale(char *v)
{
    int l, i, fnd = 0;;

    for (i = 0; languages[i] != (char *) 0; i += 1)
    {
	if (strncmp(v, languages[i], 2) == 0)
	{
	    fnd = 1;
	    break;
	}
    }
    if (!fnd)
    {
	fprintf(stderr, "Warning: invalid language code in locale specification: %s\n", v);
	return;
    }
    if ((l = strlen(v)) > 2)
    {
	if (v[2] != '_')
	{
	    fprintf(stderr, "Warning: invalid locale specification format: %s\n", v);
	    return;
	}
	fnd = 0;
	for (i = 0; countries[i] != (char *) 0; i += 1)
	{
	    if (strncmp(&v[3], countries[i], 2) == 0)
	    {
		fnd = 1;
		break;
	    }
	}
	if (!fnd)
	{
	    fprintf(stderr, "Warning: invalid country code in locale specification: %s\n", v);
	    return;
	}
	if (l >5 && v[4] != '_')
	{
	    fprintf(stderr, "Warning: invalid locale specification format: %s\n", v);
	}
    }
}

static void usage(void)
{
    fprintf(stderr, "frink [flags] [files] where the flags can be"
	"\n"
	"\n-a 	: put spaces around -command code in {} and \"\". (default = OFF)"
	"\n-A	: turns OFF processing of expr calls."
	"\n-b 	: add braces (see manual page for details) (default = OFF)"
	"\n-B	: turns OFF processing of code with bind calls."
	"\n-c <n>	: set further indent for continuations to n. default = 2"
	"\n-C <f>	: generate proc specs for use by frink. default = OFF"
	"\n-d	: remove braces in certain (safe) circumstances (default = OFF)"
	"\n-D	: warn about dynamic names. default = OFF"
	"\n-e	: produce \"else\". (default = OFF)"
	"\n-E	: extract constant strings. The parameter is the locale for which"
	"\n	  the strings are currently written. If the -f flag is also used"
	"\n	  then only the constant strings that are rewritten will be output."
	"\n	  Output goes to a file called \"<locale>.msg\". (default = OFF)"
	"\n-f	: rewrite strings for msgcat (default = OFF)"
	"\n-F	: selectively control heuristics. Currently the parameter"
	"\n	  is a single hex coded number with each bit representing a test. The"
	"\n	  values you need to know are:"
	"\n		00001	: var parameter testing"
	"\n		00002	: parameter number testing"
	"\n		00004	: parameter value testing"
	"\n		00008	: regexp parameter testing"
	"\n		00010	: return checks"
	"\n		00020	: check for : or ::: in names"
	"\n		00040	: expr checks"
	"\n		00080	: foreach var checking."
	"\n		00100	: check for omitted parameters."
	"\n		00200	: check switches on commands."
	"\n		00400	: check for abbreviated options."
	"\n		00800	: check for unusedness."
	"\n		01000	: check for check for bad name choice."
	"\n		02000	: check for array usage."
	"\n		04000	: check for possible name errors."
	"\n-g	: indent switch cases. (default = OFF)"
	"\n-G	: generate compiler style error messages (default = OFF)"
	"\n-h	: print this message"
	"\n-H	: turn on all heuristic tests and warnings. (default = OFF)"
	"\n-i <n>	: set indent for each level to n. default = 4"
	"\n-I	: Treate elseif and else the same way. (default = OFF)"
	"\n-j	: remove non-essential blank lines. (default = OFF)"
	"\n-J	: Just do checks, no output. (default = OFF)"
	"\n-k	: remove non-essential braces."
	"\n-K <f>	: specify file of extra code specs."
	"\n-l	: try for one-liners (not yet implemented)"
	"\n-m	: minimise the code by removing redundant spacing. default = OFF"
	"\n-M   : warn if there is no -- on a switch statement. default = OFF" 
	"\n-n 	: do not generate tab characters (default = OFF)"
	"\n-N	: do not put a newline out before elseif. (default = OFF)"
	"\n-o	: obfuscate (not implemented yet) : default = OFF"
	"\n-O <t>   : Don't format lines starting with token \"t\""
	"\n-p <v>	: If v is a number produce that many blank lines after each"
	"\n	proc definition, otherwise produce whatever format the code"
	"\n	indicates. No codes are defined yet..... (default = do nothing)"
	"\n-P	: Turn off processing of \"time\" command (default = OFF)"
	"\n-q	: Put spaces round conditions (default = OFF)"
	"\n-Q	: warn about unquoted constants - not fully operational (default = OFF)"
	"\n-r	: remove comments (default = OFF)"
	"\n-s <c>	: format according to style \"c:\""
	"\n-S	: Don't preserve end of line comments. default = OFF"
	"\n-t <n>	: set tabstops every n characters. default = 8"
	"\n-T	: produce \"then\". default = OFF"
	"\n-u	: safe to remove brackets from elseif conds"
	"\n-U	: hardline checking enabled (default = OFF)"
	"\n-v	: put { } round variable names where appropriate."
	"\n-V	: the current version number"
	"\n-w <n>	: set line length. default = 80"
	"\n-W    	: halt on Warnings as well as errors"
	"\n-x	: produce \"xf style\" continuations"
	"\n-X	: recognise tclX constructs"
	"\n-y	: don't process -command code (default = OFF)"
	"\n-Y	: try to process dynamic code (default = OFF)"
	"\n-z	: put a single space before the \\ character on continuations."
	"\n-Z	: control heuristics that are tested. -H turns on ALL tests)"
	"\n");
}

void setOption(int flag, char *value)
{
    int not;
    switch (flag)
    {
    case 'A' :	doExpr = 0; break;
    case 'B' :	doBind = 0; break;
    case 'C' :	makespec = 1; specfile = fopen(value, "w"); break;
    case 'D' :	warndyn = 1; break;
    case 'E' :	extract = 1; checkLocale(value); locale = value; break;
    case 'F' :  if (*value == '!')
		{
		    not = 1;
		    value++;
		}
		else
	    	{
		    not = 0;
		}
		if (sscanf(value, "%x", &heuristics) != 1)
		{
		    fprintf(stderr, "Warning: bad value for -F flag\n");
		}
		if (not)
	    	{
		    heuristics = ~heuristics;
		}
		noheuristics = 0;
		break;
    case 'G' :	compout = 1; break;
    case 'H' :	heuristics = HALL; noheuristics = 0; break;
    case 'I' :	ifelse = 1; break;
    case 'J' :	testonly = 1; heuristics = HALL; noheuristics = 0; break;
    case 'K' :  readconfig(value); break;
    case 'L' :  trace = 1; break;
    case 'M' :  checksw = 1; break;
    case 'N' :  nonlelsif = 1; break;
    case 'O' :	lpush(&skiplist, newString(value)); break;
    case 'P' :  doTime = 0; break;
    case 'Q' :  noquotes = 1; break;
    case 'R' :  resvsplit = 1; break;
    case 'S' :  olcomments = 0; break;
    case 'T' :	putThen = 1; break;
    case 'U' :	fascist = 1; break;
    case 'W' :  haltonWarn = 1; break;
    case 'V' :  doVersion();
    case 'X' :	tclX = 1; break;
    case 'Y' :  trystrings = 1; break;
    case 'Z' :  break;
    case 'a' :	addSpaces = 1; break;
    case 'b' :	embrace = 1; debrace = 0; break;
    case 'c' :	contdent = atoi(value); break;
    case 'd' :	debrace = 1; embrace = 0; break;
    case 'e' :	putElse = 1; break;
    case 'f' :	internat = 1; break;
    case 'g' :	switchIn = 1; break;
    case 'i' :	indent = atoi(value); break;
    case 'j' :	noblanks = 1; break;
    case 'k' :	debrace = 1; break;
    case 'l' :	oneliner = 1; break;
    case 'm' :	minimise = 1; break;
    case 'n' :	tabsOn = 0; break;
    case 'o' :	obfuscate = 1; break;
    case 'p' :
	if (isdigit(*value))
	{
	    procnls = atoi(value);
	}
	else
	{
	    pstyle = optarg;
	}
	break;
    case 'q' :  spaceout = 1; break;
    case 'r' :	nocomments = 1; break;
    case 's' :	style = value; break;
    case 't' :	tabStops = atoi(value); break;
    case 'u' :  elseif = 1; break;
    case 'v' :	varbrace = 1; break;
    case 'w' :	width = atoi(value); break;
    case 'x' :	xf = 1; break;
    case 'y' :	nocommand = 1; break;
    case 'z' :	contString = " "; break;
    case 'h' :
    default :	usage(); exit(1);
    }
}

static void options(int argc, char *argv[])

{
    int flg;

    while ((flg = getopt(argc, argv ,"ABCDE:F:GHIJK:LMNO:PQRSTUVWXYZ:abc:defghi:jklmnop:qrs:t:uvw:xyz")) != -1)
    {
	setOption(flg, optarg);
    }
}

static void setStyle(void)
{
    if (style != (char *) 0)
    {
	if (strcmp(style, "ouster") == 0)
	{
	    switchIn = 1;
	    putElse = 1;
	    debrace = 1;
	}
    }
    if (obfuscate || minimise)
    {
	minimise = 1;
	debrace = 1;
	embrace = 0;
	noblanks = 1;
	nocomments = 1;
	putElse = 0;
	putThen = 0;
	oneliner = 1;
	indent = 0;
	contdent = 0;
    }
}

static void readrc(char *file)
{
    char *opts[50], buff[128], *cp;
    FILE *desc;
    int leng, i;

    if ((desc = fopen(file, "r")) != NULL)
    {
	leng = fread(buff, 1, 127, desc);
	fclose(desc);
	buff[leng] = '\0';
	cp = buff;
	opts[0] = "";
	leng = 1;
	while (*cp)
	{
	    while (isspace(*cp)) { cp++; }
	    opts[leng++] = cp;
	    while (*cp && !isspace(*cp)) { cp++; }
	    if (*cp) { *cp++ = '\0'; }
	}
	for (i = 0; i < leng; i += 1)
	{
	    if (*opts[i] == '-')
	    {
		setOption(opts[i][1], opts[i+1]);
	    }
	}
    }
}

static void findrc(void)
{
    char *home, file[512];
    if ((home = getenv("HOME")) != (char *) 0)
    {
	strcat(strcpy(file, home),"/.frinkrc");
	readrc(file);
    }
    readrc("./.frinkrc");
}

static void process(FILE *desc)
{
    extern void flushOutput(void);
    extern int handle(Token *line);
    extern void streamMore(Input *);

    Input file;

    file.text = (char *) malloc(64*1024);
    file.stream = desc;
    file.tcall = file.texpr = 0;
    file.lineNumber = 1;
    file.lineStart = 1;    
    streamMore(&file);
    while (handle(collect(&file)))
    {
    }
    flushOutput();
    free(file.text);
}

static void initCmds(void)
{
    static char *cmdspec =
	"set {{var 0x0017} {ctype? 0x0001}}\n"
	"global {{varlist 0x0150}}\n"
	"append {{var 0x0017} any args}\n"
	"break {{break}}\n"
	"continue  {{break 0x1}}\n"
	"incr {{var 0x0017} {ctype? 0x0002}}\n"
	"time {script {ctype? 0x0002}}\n"
	"catch {script {?var 0x0013}}\n"
	"gets {any {?var 0x0011}}\n"
	"lappend {{var 0x0107} {ctype 0x0001} args}\n"
	"error {any ?any ?any block}\n"
	"exit {{ctype? 0x0002} block}\n"
	"while {{cond 0x06} code}\n"
	"for {seqn {cond 0x06} seqn code}\n"
	"uplevel {?level cmds}\n"
	"scan {{ctype 0x0001} {ctype 0x0001} {varlist 0x0001}}\n"
	"bind {{builtin 0x1}}\n"
	"common {{builtin 0x2}}\n"
	"constructor {{builtin 0x3}}\n"
	"destructor {{builtin 0x4}}\n"
	"expr {{builtin 0x6}}\n"
	"foreach {{builtin 0x7}}\n"
	"if {{builtin 0x8}}\n"
	"itcl_class {{builtin 0x0a}}\n"
	"loop {{builtin 0x0b}}\n"
	"method {{builtin 0x0c}}\n"
	"proc {{builtin 0x0e}}\n"
	"protected {{builtin 0x0f}}\n"
	"public {{builtin 0x10}}\n"
	"regexp {{builtin 0x13}}\n"
	"regsub {{builtin 0x0d}}\n"
	"return {{builtin 0x11}}\n"
	"switch {{builtin 0x12}}\n"
	"unset	{{builtin 0x16}}\n"
	"upvar	{{builtin 0x14}}\n"
	"variable {{builtin 0x15}}\n"
	"string {{| args"
		" {{option bytelength length} {ctype 0x0001}}"
		" {{option index repeat wordend wordstart} {ctype 0x0001} {ctype 0x0001}}"
		" {{option compare equal is} args}"
		" {{option first last map match} {ctype 0x0001} {ctype 0x0001} {ctype? 0x0001}}"
		" {{option range} {ctype 0x0001} {ctype 0x0001} {ctype 0x0001}}"
		" {{option replace} {ctype 0x0001} {ctype 0x0001} {ctype 0x0001} {ctype? 0x0001}}"
		" {{option tolower totitle toupper} {ctype 0x0001} {ctype? 0x0001} {ctype? 0x0001}}"
		" {{option trim trimright trimleft} {ctype 0x0001} {ctype? 0x0001}}"
	    "}}\n"
	"file {{| args"
		" {{option atime mtime channels} any ?any}"
		" {{option attributes } any args}"
		" {{option copy delete rename} args}"
		" {{option dirname executable exists extension isdirectory isfile nativename normalize owned pathtype readable readlink rootname size split tail type writable} any}"
		" {{option join mkdir} any args}"
		" {{option lstat stat} any {var 0x0001}}"		
		" {{option volume}}"
	    "}}\n"
	"namespace {{| args"
		" {{option children} ?any ?any}"
		" {{option code} script}"
		" {{option current}}"
		" {{option delete export import forget} args}"
		" {{option eval} any {nest 0x1} cmds {unnest 0x1}}"
		" {{option inscope} any any args}"
		" {{option exists origin qualifiers tail} any}"
		" {{option parent} ?any}"
		" {{option which} any ?any ?any}"
	    "}}\n"
	"interp {{| args"
		" {{option alias} any any args}"
		" {{option aliases issafe} ?any}"
		" {{option create delete slaves} args}"
		" {{option eval} any cmds}"
		" {{option expose hide} any any args}"
		" {{option exists hidden marktrusted} any}"
		" {{option exists invokehidden} any any args}"
		" {{option share target} any any}"
		" {{option transfer} any any any}"
	    "}}\n"
	"array {{| args"
		" {{option set} {var 0x0093} any}"
		" {{option anymore donesearch nextelement} {var 0x0082} any}"
		" {{option exists} any}"
		" {{option size startsearch statistics} {var 0x082}}"
		" {{option names} {var 0x0082} ?any ?any}"
		" {{option get unset} {var 0x0082} ?any}"
	    "}}\n"
	"binary {{| args"
		" {{option format} {ctype 0x0001} args}"
		" {{option scan} any {ctype 0x0001} {varlist 0x0001}}"
	    "}}\n"
	"cd {?any}\n"
	"close {any}\n"
	"concat {args}\n"
	"encoding {{| args"
		" {{option convertfrom convertto} any ?any}"
		" {{option names}}"
		" {{option system} ?any}"
	    "}}\n"
	"eof {any}\n"
	"fblocked {any}\n"
	"fconfigure {any args}\n"
	"fileevent {any {option readable writable} args}\n"
	"fcopy {any any args}\n"
	"flush {any}\n"
	"format {any args}\n"
	"glob {args}\n"
	"rename {any any}\n"
	"lindex {{ctype 0x0010} ?any}\n"
	"lrange {{ctype 0x0010} any any}\n"
	"info {{| args"
		" {{option cmdcount hostname library nameofexecutable patchlevel sharedlibextension tclversion}}"
		" {{option args body complete} any}"
		" {{option exists} {var 0x0011}}"
		" {{option commands globals level loaded locals procs script vars} ?any}"
		" {{option default} any any {var 0x0001}}"
	    "}}\n"
	"clock {{| args"
		" {{option format scan} any args}"
	        " {{option clicks} ?any}"
		" {{option seconds}}"
	    "}}\n"
        "package {{| args"
                " {{option forget} args}"
		" {{option ifneeded} any any ?script}"
                " {{option names}}"
                " {{option provide} any ?any}"
                " {{option present require} any ?any ?any}"
                " {{option unknown} ?any}"
                " {{option vcompare vsatisfies} any any}"
                " {{option versions} any}"
	"}}\n"



	"console {{| args"
	        " {{option title} ?any}"
	        " {{option eval hide show}}"
	"}}\n"
	"history {{| args"
	        " {{option add change clear event info keep nextid redo} args}"
	        " {?any}"
	"}}\n";
/* after is tricky because of the non-text possibilities for 1st arg
	"after {{| args"
		" {{option cancel} args}"
		" {{option idle} args}"
		" {{option info} ?any}"
	    "}}\n";
*/
    stringconfig(cmdspec);
}

int main(int argc, char **argv)
{
    FILE  *desc;
    extern void initOutput(void);

    initCmds();
    findrc();
    options(argc, argv);
    setStyle();
    initOutput();
    if (argv[optind] == (char *) 0)
    {
	clearState();
	process(stdin);
    }
    else
    {
	while (argv[optind] != (char *) 0)
	{
	    currentfile = argv[optind];
	    if ((desc = fopen(argv[optind], "r")) == NULL)
	    {
		fprintf(stderr, "\"%s\" cannot be accessed!!\n", argv[optind]);
	    }
	    else
	    {
	    	clearState();
		process(desc);
		fclose(desc);
	    }
	    optind += 1;
	}
    }
    if (msgfile != NULL)
    {
	fclose(msgfile);
    }
    if (specfile != NULL)
    {
	fclose(specfile);
    }
    return failed;
}
