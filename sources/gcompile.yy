%{
//
// FILE: gcompile.yy -- yaccer/compiler for the GCL
//
// This parser/compiler is dedicated to the memory of
// Jan L. A. van de Snepscheut, who wrote a program after which
// this code is modeled.
//
// $Id$
//

#include <stdlib.h>
#include <ctype.h>
#include "gmisc.h"

#include "gambitio.h"
#include "gcmdline.h"

#include "gstring.h"
#include "rational.h"
#include "glist.h"
#include "gstack.h"
#include "gsm.h"
#include "gsminstr.h"
#include "gsmfunc.h"
#include "portion.h"

#include "system.h"


#include "gstack.imp"

#ifdef __GNUG__
#define TEMPLATE template
#elif defined __BORLANDC__
#pragma option -Jgd
#define TEMPLATE
#endif   // __GNUG__

TEMPLATE class gStack<gString>;
TEMPLATE class gStack<int>;
TEMPLATE class gStack<char>;
TEMPLATE class gStack<gInput *>;
TEMPLATE class gStack<unsigned int>;

#include "glist.imp"

TEMPLATE class gList<bool>;
TEMPLATE class gNode<bool>;

extern GSM* _gsm;  // defined at the end of gsm.cc
gStack<gString> GCL_InputFileNames(4);

%}

%name GCLCompiler

%define MEMBERS   \
  int index; \
  gString input_text; \
  bool force_output, bval, triv, semi; \
  int statementcount; \
  gInteger ival; \
  double dval; \
  gString tval, formal, funcname, funcdesc,  paramtype, functype;  \
  gString funcbody; \
  bool record_funcbody; \
  gList<NewInstr*> program, *function, *optparam; \
  gList<gString> formals, types; \
  gList<bool> refs; \
  gList<Portion*> portions; \
  gStack<gString> formalstack; \
  gStack<int> labels, listlen; \
  gStack<char> matching; \
  gStack<gInput *> inputs; \
  gStack<int> lines; \
  GSM& gsm; \
  bool quit; \
  bool in_funcdecl; \
  \
  char nextchar(void); \
  void ungetchar(char c); \
  \
  void emit(NewInstr*); \
  bool DefineFunction(void); \
  bool DeleteFunction(void); \
  void RecoverFromError(void); \
  int ProgLength(void); \
  \
  int Parse(void); \
  int Execute(void); \
  void LoadInputs( const char* name ); 

%define CONSTRUCTOR_INIT     : record_funcbody(false), \
                               function(0), \
                               optparam(0), \
                               formalstack(4), \
                               labels(4), \
                               listlen(4), matching(4), \
                               lines(4), \
                               gsm(*_gsm), quit(false), in_funcdecl(false)

%define CONSTRUCTOR_CODE       GCL_InputFileNames.Push("stdin"); \
                               lines.Push(1); \
                               LoadInputs( "gclini.gcl" );

%token LOR
%token LAND
%token LNOT
%token EQU
%token NEQ
%token LTN
%token LEQ
%token GTN
%token GEQ
%token PLUS
%token MINUS
%token STAR
%token SLASH
%token ASSIGN
%token SEMI
%token LBRACK
%token DBLLBRACK
%token RBRACK
%token LBRACE
%token RBRACE
%token RARROW
%token LARROW
%token DBLARROW
%token COMMA
%token HASH
%token DOT
%token CARET
%token UNDERSCORE
%token AMPER
%token WRITE
%token READ

%token PERCENT
%token DIV
%token LPAREN
%token RPAREN

%token IF
%token WHILE
%token FOR
%token QUIT
%token DEFFUNC
%token DELFUNC
%token TYPEDEF
%token INCLUDE

%token NAME
%token BOOLEAN
%token INTEGER
%token FLOAT
%token TEXT
%token STDIN
%token STDOUT
%token gNULL


%token CRLF
%token EOC

%%

program: 
              toplevel EOC 	
              { return 0; }
       |      error EOC   { RecoverFromError();  return 1; }
       |      error CRLF  { RecoverFromError();  return 1; }
//       |      include    { return 0; }     

toplevel:     statements

statements:   statement               
          |   statements sep statement

sep:          SEMI    { semi = true; }
   |          CRLF    { semi = false; }


funcdecl:     DEFFUNC { if (in_funcdecl)  YYERROR;  in_funcdecl = true; }
               LBRACK { gcmdline.SetPrompt( false ); }
              NAME
              { funcname = tval; function = new gList<NewInstr*>; 
                statementcount = 0; }
              LBRACK formallist RBRACK TYPEopt COMMA 
              { funcbody = ""; record_funcbody = true; }
              statements 
              { record_funcbody = false; 
                if( funcbody.length() > 0 )
                  funcbody.remove( funcbody.length() - 1 );
              }
              optfuncdesc
              RBRACK   { in_funcdecl = false;
			 if (!DefineFunction())  YYERROR; 
                         gcmdline.SetPrompt( true ); } 

optfuncdesc:  | { funcdesc = ""; }
              COMMA CRLFopt TEXT CRLFopt 
              { funcdesc = tval; }

delfunc:      DELFUNC { if (in_funcdecl)  YYERROR;  in_funcdecl = true; }
   	      LBRACK NAME
              { funcname = tval; function = new gList<NewInstr*>; 
                statementcount = 0; }
              LBRACK formallist RBRACK TYPEopt
              RBRACK   { in_funcdecl = false;
	                 if (!DeleteFunction())  YYERROR; } 

TYPEopt:      { functype = "ANYTYPE" }
          |   TYPEDEF { paramtype = ""; } typename 
              { functype = paramtype; }
		
formallist:
          |   formalparams

formalparams: formalparam
            | formalparams CRLFopt COMMA CRLFopt formalparam

formalparam:  NAME { formals.Append(tval); } binding 
              { paramtype = ""; } typename 
              { types.Append(paramtype); portions.Append(REQUIRED); } 
            | LBRACE NAME { formals.Append(tval); } binding
              { paramtype = ""; types.Append(paramtype); }
              optparam RBRACE

optparam:     { assert( optparam == 0 );
                optparam = new gList<NewInstr*>; }
              expression 
              { 
                if( gsm.Execute( *optparam ) == rcSUCCESS )
                {
                  Portion* _p_ = gsm.PopValue();
                  assert( _p_ != 0 );
                  if( _p_->Spec().Type != porREFERENCE )
                    portions.Append( _p_ );
                  else
                    portions.Append( REQUIRED );
                }
                else
                  portions.Append( REQUIRED );
                delete optparam;
                optparam = 0;
              }



typename:     starname
            | NAME { paramtype += tval; } optparen

starname:     NAME  { paramtype += tval; } STAR { paramtype += '*'; }

optparen:
        |     LPAREN { paramtype += '('; }  typename
              RPAREN { paramtype += ')'; }

binding:      RARROW    { refs.Append(false); }
       |      DBLARROW  { refs.Append(true); }

statement:    |  expression { triv = false; }


include:      INCLUDE LBRACK TEXT RBRACK
              { LoadInputs(tval); }


conditional:  IF { gcmdline.SetPrompt( false ); }
              LBRACK CRLFopt expression CRLFopt COMMA 
              { emit(new NewInstr(iNOT)); emit(0);
                labels.Push(ProgLength()); } statements 
              { emit(0);
		if (function)
		  (*function)[labels.Pop()] = 
                    new NewInstr(iIF_GOTO, (long) ProgLength() + 1);
		else
		  program[labels.Pop()] = 
                    new NewInstr(iIF_GOTO, (long) ProgLength() + 1);
		labels.Push(ProgLength());
	      }
              alternative RBRACK
              { emit(new NewInstr(iNOP));
		if (function)
		  (*function)[labels.Pop()] = 
                    new NewInstr(iGOTO, (long) ProgLength());
		else
		  program[labels.Pop()] = 
                    new NewInstr(iGOTO, (long) ProgLength());
              } 
              { gcmdline.SetPrompt( true ); }

alternative:   
           |  COMMA statements

CRLFopt:    | CRLFs

CRLFs:     CRLF | CRLFs CRLF

whileloop:    WHILE { gcmdline.SetPrompt( false ); }
              LBRACK CRLFopt { labels.Push(ProgLength() + 1); }
              expression { emit(new NewInstr(iNOT)); emit(0);
			   labels.Push(ProgLength()); }
              CRLFopt COMMA statements RBRACK 
              { if (function)
		  (*function)[labels.Pop()] = 
                    new NewInstr(iIF_GOTO, (long) ProgLength() + 2);
		else
		  program[labels.Pop()] = 
                    new NewInstr(iIF_GOTO, (long) ProgLength() + 2);
		emit(new NewInstr(iGOTO, (long) labels.Pop()));
		emit(new NewInstr(iNOP));
              }
              { gcmdline.SetPrompt( true ); }

forloop:      FOR { gcmdline.SetPrompt( false ); }
              LBRACK CRLFopt exprlist CRLFopt COMMA CRLFopt 
              { labels.Push(ProgLength() + 1); }
              expression CRLFopt COMMA CRLFopt
              {  index = labels.Pop();   // index is loc of begin of guard eval
                 emit(new NewInstr(iNOT));
                 // slot for guard-false jump
                 emit(0); labels.Push(ProgLength());
                 // push location of increment 
                 labels.Push(ProgLength() + 2);
                 // slot for guard-true jump
                 emit(0); 
                 labels.Push(ProgLength()); labels.Push(index);
              }
              exprlist CRLFopt COMMA
              { // emit jump to beginning of guard eval
                emit(new NewInstr(iGOTO, (long) labels.Pop())); 
                // link guard-true jump
                if (function)
                  (*function)[labels.Pop()] = 
                    new NewInstr(iGOTO, (long) ProgLength() + 1);
		else
		  program[labels.Pop()] = 
                    new NewInstr(iGOTO, (long) ProgLength() + 1);
                semi = false;
              }
              statements RBRACK
              { 
                // emit jump to beginning of increment step
                emit(new NewInstr(iGOTO, (long) labels.Pop()));
		// link guard-false branch to end of code
                if (function)
		  (*function)[labels.Pop()] = 
                    new NewInstr(iIF_GOTO, (long) ProgLength() + 1);
		else
		  program[labels.Pop()] = 
                    new NewInstr(iIF_GOTO, (long) ProgLength() + 1);
		emit(new NewInstr(iNOP));
	      }
              { gcmdline.SetPrompt( true ); }

exprlist:     expression  { emit(new NewInstr(iPOP)); }
        |     exprlist SEMI expression  { emit(new NewInstr(iPOP)); }

expression:   Ea
          |   WRITE expression  { emit(new NewInstr(iOUTPUT)); }
          |   Ea ASSIGN expression { emit(new NewInstr(iASSIGN)); }
          |   Ea ASSIGN { emit(new NewInstr(iUNASSIGN)); }
          |   conditional
          |   whileloop
          |   forloop
          |   funcdecl { emit(new NewInstr(iPUSH_BOOL, (bool)true)); }  
          |   delfunc   { emit(new NewInstr(iPUSH_BOOL, (bool)true)); }
          |   include   { emit(new NewInstr(iPUSH_BOOL, (bool)true)); }
          ;

Ea:           E0
  |           Ea WRITE E0   { emit(new NewInstr(iWRITE)); }
  |           Ea READ E0    { emit(new NewInstr(iREAD)); }
  ; 

E0:           E1
  |           E0 LOR E1  { emit(new NewInstr(iOR)); }
  ;

E1:           E2
  |           E1 LAND E2  { emit(new NewInstr(iAND)); } 
  ;

E2:           E3
  |           LNOT E2     { emit(new NewInstr(iNOT)); }
  ;

E3:           E4       
  |           E3 EQU E4    { emit(new NewInstr(iEQU)); } 
  |           E3 NEQ E4    { emit(new NewInstr(iNEQ)); }
  |           E3 LTN E4    { emit(new NewInstr(iLTN)); }
  |           E3 LEQ E4    { emit(new NewInstr(iLEQ)); }
  |           E3 GTN E4    { emit(new NewInstr(iGTN)); } 
  |           E3 GEQ E4    { emit(new NewInstr(iGEQ)); }
  ;

E4:           E5
  |           E4 PLUS E5   { emit(new NewInstr(iADD)); }
  |           E4 MINUS E5  { emit(new NewInstr(iSUB)); }
  |           E4 AMPER E5  { emit(new NewInstr(iCONCAT)); }
  ;

E5:           E6
  |           E5 STAR E6    { emit(new NewInstr(iMUL)); }
  |           E5 SLASH E6   { emit(new NewInstr(iDIV)); }
  |           E5 PERCENT E6 { emit(new NewInstr(iMOD)); }
  |           E5 DIV E6     { emit(new NewInstr(iINTDIV)); }
  |           E5 DOT E6     { emit(new NewInstr(iDOT)); }
  |           E5 CARET E6   { emit(new NewInstr(iPOWER)); } 
  ;

E6:           PLUS E7
  |           MINUS E7      { emit(new NewInstr(iNEG)); }
  |           E7

E7:           E8
  |           E7 HASH E8   { emit(new NewInstr(iCHILD)); }
  |           E7 DBLLBRACK expression RBRACK RBRACK 
                 { emit(new NewInstr(iSUBSCRIPT)); }
  |           E7 UNDERSCORE E8
		 { emit(new NewInstr(iSUBSCRIPT)); }
  ;

E8:           E9   
  |           LPAREN expression RPAREN
  |           NAME          { emit(new NewInstr(iPUSHREF, tval)); }
  |           function      { emit(new NewInstr(iCALL_FUNCTION)); }
  |           { gcmdline.SetPrompt( false ); }
              list
              { gcmdline.SetPrompt( true ); }
                                { emit(new NewInstr(iPUSHLIST, 
                                (long) listlen.Pop())); }
  ;

E9:           BOOLEAN  { emit(new NewInstr(iPUSH_BOOL, bval)); }
  |           INTEGER  { emit(new NewInstr(iPUSH_INTEGER, ival.as_long())); }
  |           FLOAT    { emit(new NewInstr(iPUSH_FLOAT, dval)); }
  |           TEXT     { emit(new NewInstr(iPUSH_TEXT, tval)); }
  |           STDIN    { emit(new NewInstr(iPUSHINPUT, &gin)); }
  |           STDOUT   { emit(new NewInstr(iPUSHOUTPUT, &gout)); }
  |           gNULL    { emit(new NewInstr(iPUSHOUTPUT, &gnull)); }
  |           QUIT     { emit(new NewInstr(iQUIT)); }
  ;

function:     NAME LBRACK { emit(new NewInstr(iINIT_CALL_FUNCTION, tval)); } 
              arglist RBRACK

arglist:
       |      unnamed_args
       |      unnamed_args COMMA named_args
       |      named_args

unnamed_args: unnamed_arg
            | unnamed_args COMMA unnamed_arg

unnamed_arg:  expression  { emit(new NewInstr(iBIND)); }

named_args:   named_arg
          |   named_args COMMA named_arg

named_arg:    NAME RARROW { formalstack.Push(tval); } expression
                           { emit(new NewInstr(iBINDVAL, formalstack.Pop())); }
         |    NAME DBLARROW  { formalstack.Push(tval); } name_or_io
                           { emit(new NewInstr(iBINDREF, formalstack.Pop())); }

name_or_io:   NAME     { emit(new NewInstr(iPUSHREF, tval)); }
         |    STDIN    { emit(new NewInstr(iPUSHINPUT, &gin)); }
         |    STDOUT   { emit(new NewInstr(iPUSHOUTPUT, &gout)); }
         |    gNULL    { emit(new NewInstr(iPUSHOUTPUT, &gnull)); }

list:         LBRACE CRLFopt  { listlen.Push(0); } listels CRLFopt RBRACE 
    |         LBRACE CRLFopt  { listlen.Push(0); } RBRACE 

listels:      listel
       |      listels CRLFopt COMMA CRLFopt listel

listel:       expression   { listlen.Push(listlen.Pop() + 1); }

%%


const char CR = (char) 10;

char GCLCompiler::nextchar(void)
{
  char c;

  while (inputs.Depth() && inputs.Peek()->eof())  {
    delete inputs.Pop();
    GCL_InputFileNames.Pop();
    lines.Pop();
  }

  if (inputs.Depth() == 0)
    // gin >> c;
    gcmdline >> c;
  else
    *inputs.Peek() >> c;

  if (c == CR)
    lines.Peek()++;

  if( record_funcbody )
    funcbody += c;

  return c;
}

void GCLCompiler::ungetchar(char c)
{
  if (inputs.Depth() == 0)
    // gin.unget(c);
    gcmdline.unget(c);
  else
    inputs.Peek()->unget(c);

  if (c == CR)
    lines.Peek()--;

  if( record_funcbody )
    if( funcbody.length() > 0 )
      funcbody.remove( funcbody.length() - 1 );
}

typedef struct tokens  { long tok; char *name; };

void GCLCompiler::yyerror(char *s)
{
static struct tokens toktable[] =
{ { LOR, "OR or ||" },  { LAND, "AND or &&" }, { LNOT, "NOT or !" },
    { EQU, "=" }, { NEQ, "!=" }, { LTN, "<" }, { LEQ, "<=" },
    { GTN, ">" }, { GEQ, ">=" }, { PLUS, "+" }, { MINUS, "-" },
    { STAR, "*" }, { SLASH, "/" }, { ASSIGN, ":=" }, { SEMI, ";" },
    { LBRACK, "[" }, { DBLLBRACK, "[[" }, { RBRACK, "]" },
    { LBRACE, "{" }, { RBRACE, "}" }, { RARROW, "->" },
    { LARROW, "<-" }, { COMMA, "," }, { HASH, "#" },
    { DOT, "." }, { CARET, "^" }, { UNDERSCORE, "_" },
    { AMPER, "&" }, { WRITE, "<<" }, { READ, ">>" },
    { IF, "If" }, { WHILE, "While" }, { FOR, "For" },
    { QUIT, "Quit" }, 
    { DEFFUNC, "NewFunction" }, 
    { DELFUNC, "DeleteFunction" },
    { TYPEDEF, "=:" },
    { INCLUDE, "Include" },
    { PERCENT, "%" }, { DIV, "DIV" }, { LPAREN, "(" }, { RPAREN, ")" },
    { CRLF, "carriage return" }, { EOC, "carriage return" }, { 0, 0 }
};


  gerr << s << ": " << GCL_InputFileNames.Peek() << ':'
       << ((yychar == CRLF || yychar == EOC) ? lines.Peek() - 1 : lines.Peek()) << " at ";

  for (int i = 0; toktable[i].tok != 0; i++)
    if (toktable[i].tok == yychar)   {
      gerr << toktable[i].name << '\n';
      return;
    }

  switch (yychar)   {
    case NAME:
      gerr << "identifier " << tval << '\n';
      break;
    case BOOLEAN:
      gerr << ((bval) ? "True" : "False") << '\n';
      break;
    case FLOAT:
      gerr << "floating-point constant " << dval << '\n';
      break;
    case INTEGER:
      gerr << "integer constant " << ival << '\n';
      break;
    case TEXT:
      gerr << "text string " << tval << '\n';
      break;
    case STDIN:
      gerr << "StdIn\n";
      break;
    case STDOUT:
      gerr << "StdOut\n";
      break;
    case gNULL:
      gerr << "NullOut\n";
      break;
    default:
      gerr << yychar << '\n';
      break;
  }    
}

int GCLCompiler::yylex(void)
{
  char c;

  if (force_output)  {
    force_output = false;
    return WRITE;
  }	

I_dont_believe_Im_doing_this:

  while (1)  {
    char d;
    do  {
      c = nextchar();
    }  while (isspace(c) && c != CR);
    if (c == '/')  {
      if ((d = nextchar()) == '/')  {
	while ((d = nextchar()) != CR);
	if (matching.Depth())
	  return CRLF;
	else
	  return EOC;
      }
      else if (d == '*')  {
	int done = 0;
	while (!done)  {
	  while ((d = nextchar()) != '*');
	  if ((d = nextchar()) == '/')  done = 1;
	}
      }
      else  {
	ungetchar(d);
	return SLASH;
      }
    }
    else
      break;
  }

  if (c == '\\')   {
    while (isspace(c = nextchar()) && c != CR);
    if (c == CR)
      goto I_dont_believe_Im_doing_this;
    else  {
      ungetchar(c);
      return '\\';
    }
  }

  if (isalpha(c))  {
    gString s(c);
    c = nextchar();
    while (isalpha(c) || isdigit(c))   {
      s += c;
      c = nextchar();
    }
    ungetchar(c);

    if (s == "True")   {
      bval = true;
      return BOOLEAN;
    }
    else if (s == "False")  {
      bval = false;
      return BOOLEAN;
    }
    else if (s == "StdIn")  return STDIN;
    else if (s == "StdOut") return STDOUT;
    else if (s == "NullOut")   return gNULL;
    else if (s == "AND")    return LAND;
    else if (s == "OR")     return LOR;
    else if (s == "NOT")    return LNOT;
    else if (s == "DIV")    return DIV;
    else if (s == "MOD")    return PERCENT;
    else if (s == "If")     return IF;
    else if (s == "While")  return WHILE;
    else if (s == "For")    return FOR;
    else if (s == "Quit")   return QUIT;
    else if (s == "NewFunction")   return DEFFUNC;
    else if (s == "DeleteFunction")   return DELFUNC;
    else if (s == "Include")   return INCLUDE;
    else  { tval = s; return NAME; }
  }

  if (c == '"')   {
    gcmdline.SetPrompt( false );
    tval = "";
    c = nextchar();
    bool lastslash = false;
	
    while (c != '"' || lastslash)   {
      if (lastslash && c == '"')  
        tval[tval.length() - 1] = '"';
      else
        tval += c;

      lastslash = (c == '\\');
      c = nextchar();
    }
    gcmdline.SetPrompt( true );
    return TEXT;
  }

  if (isdigit(c))   {
    gString s(c);
    c = nextchar();
    while (isdigit(c))   {
      s += c;
      c = nextchar();
    }

    if (c == '.')   {
      s += c;
      c = nextchar();
      while (isdigit(c))  {
	s += c;
	c = nextchar();
      }

      ungetchar(c);
      dval = atof((char *) s);
      return FLOAT;
    }
    else  {
      ungetchar(c);
      ival = atoi((char *) s);
      return INTEGER;
    }
  }

  switch (c)  {
    case ',':   return COMMA;
    case '.':   c = nextchar();
      if (c < '0' || c > '9')  { ungetchar(c);  return DOT; }
      else  {
	gString s(".");
	s += c;
        c = nextchar();
        while (isdigit(c))  {
	  s += c;
	  c = nextchar();
        }

        ungetchar(c);
        dval = atof((char *) s);
        return FLOAT;
      }

    case ';':   return SEMI;
    case '_':   return UNDERSCORE;
    case '(':   matching.Push('(');  return LPAREN;
    case ')':   if (matching.Depth() > 0 && matching.Peek() == '(')
                  matching.Pop();
                return RPAREN;
    case '{':   matching.Push('{');  return LBRACE;
    case '}':   if (matching.Depth() > 0 && matching.Peek() == '{')
                  matching.Pop();
                return RBRACE;
    case '+':   return PLUS;
    case '-':   c = nextchar();
                if (c == '>')  return RARROW;
                else  { ungetchar(c);  return MINUS; }
    case '*':   return STAR;
    case '/':   return SLASH;
    case '%':   return PERCENT;
    case '=':   c = nextchar();
                if (c == ':')  return TYPEDEF;
                else   { ungetchar(c);  return EQU; }  
    case '#':   return HASH;
    case '^':   return CARET;
    case '[':   matching.Push('[');
                c = nextchar();
                if (c == '[')   {
		  matching.Push('[');
		  return DBLLBRACK;
		}
                else   {
		  ungetchar(c);
		  return LBRACK;
		}
    case ']':   if (matching.Depth() > 0 && matching.Peek() == '[')
                  matching.Pop();
                return RBRACK;
    case ':':   c = nextchar();
                if (c == '=')  return ASSIGN;
                else   { ungetchar(c);  return ':'; }  
    case '!':   c = nextchar();
                if (c == '=')  return NEQ;
		else   { ungetchar(c);  return LNOT; }
    case '<':   c = nextchar();
                if (c == '=')  return LEQ;
	        else if (c == '<')  return WRITE; 
                else if (c != '-')  { ungetchar(c);  return LTN; }
                else   { 
		  c = nextchar();
		  if (c == '>')   return DBLARROW;
		  ungetchar(c);
		  return LARROW;
		}
    case '>':   c = nextchar();
                if (c == '=')  return GEQ;
                else if (c == '>')  return READ;
                else   { ungetchar(c);  return GTN; }
    case '&':   c = nextchar();
                if (c == '&')  return LAND;
                else   { ungetchar(c);  return AMPER; }
    case '|':   c = nextchar();
                if (c == '|')  return LOR;
                else   { ungetchar(c);  return '|'; }
    case CR:    if (matching.Depth())
                  return CRLF;
    case EOF:   return EOC;
    default:    if ((inputs.Depth() == 0 && gcmdline.eof()) ||
                    (inputs.Depth() > 0 && inputs.Peek()->eof())) return EOC;
                return c;
  }
}

int GCLCompiler::Parse(void)
{
  int command = 1;

  while (!quit && (inputs.Depth() > 0 || !gcmdline.eof()))  {

    while (inputs.Depth() && inputs.Peek()->eof())  {
      delete inputs.Pop();
      GCL_InputFileNames.Pop();
      lines.Pop();
    }

    if (inputs.Depth() == 0)  {
      // gout << "GCL" << command << ": ";
      /*
      if (gsm.Verbose())  {
        gout << "<< ";
	force_output = true;
      }	
      */
    }
    matching.Flush();
    if (!yyparse())  {
      if (Execute() == rcQUIT)  quit = true;
      if (inputs.Depth() == 0) command++;
    }
    else 
      while (program.Length() > 0)   delete program.Remove(1);
  }
  gsm.Clear();
  return 1;
}


void GCLCompiler::emit(NewInstr* op)
{
  // the encoding for the line number is decoded in GSM::ExecuteUserFunc()
  if(op) op->LineNumber = statementcount * 65536 + lines.Peek();
  if( optparam )
    optparam->Append( op );
  else if (function)
    function->Append(op);
  else
    program.Append(op);
}

int GCLCompiler::ProgLength(void)
{
  return (function) ? function->Length() : program.Length();
}

void GCLCompiler::RecoverFromError(void)
{
  if (function)   {
    while (function->Length())   delete function->Remove(1);
    delete function;  function = 0;
    formals.Flush();
    types.Flush();
    refs.Flush();
    portions.Flush();
  }
  labels.Flush();
  listlen.Flush();

  while (inputs.Depth())   {
    delete inputs.Pop();
    GCL_InputFileNames.Pop();
    lines.Pop();
  }

  gcmdline.ResetPrompt();
}
    

bool GCLCompiler::DefineFunction(void)
{
  FuncDescObj *func = new FuncDescObj(funcname, 1);
  bool error = false;

  PortionSpec funcspec;

  funcspec = TextToPortionSpec(functype);
  if (funcspec.Type != porERROR) {
    FuncInfoType funcinfo = 
      FuncInfoType(function, funcspec, formals.Length());
    funcinfo.Desc = funcbody;
    if( funcdesc.length() > 0 )
      funcinfo.Desc += "\n\n" + funcdesc;
    funcdesc = "";
    func->SetFuncInfo(0, funcinfo);
  }
  else {
    error = true;
    gerr << "Error: Unknown type " << functype << ", " << 
      PortionSpecToText(funcspec) << " as return type in declaration of " << 
      funcname << "[]\n";
  }

//  function->Dump(gout);

  for (int i = 1; i <= formals.Length(); i++)   {
    PortionSpec spec;
    if(portions[i])
      spec = portions[i]->Spec();
    else
      spec = TextToPortionSpec(types[i]);

    if (spec.Type != porERROR)   {
      if (refs[i])
	func->SetParamInfo(0, i - 1, 
                          ParamInfoType(formals[i], spec,
			                portions[i], BYREF));
      else
	func->SetParamInfo(0, i - 1, 
                          ParamInfoType(formals[i], spec,
			                portions[i], BYVAL));
    }
    else   {
      error = true;
      gerr << "Error: Unknown type " << types[i] << ", " << 
	PortionSpecToText(spec) << " for parameter " << formals[i] <<
	 " in declaration of " << funcname << "[]\n";
      break;
    }
  }


  if( !error )
    gsm.AddFunction(func);
  formals.Flush();
  types.Flush();
  refs.Flush();
  portions.Flush();
  function = 0;
  return !error;
}


bool GCLCompiler::DeleteFunction(void)
{
  FuncDescObj *func = new FuncDescObj(funcname, 1);
  bool error = false;

	PortionSpec funcspec;

  funcspec = TextToPortionSpec(functype);
  if (funcspec.Type != porERROR) {
    func->SetFuncInfo(0, FuncInfoType(function, funcspec, formals.Length()));
  }
  else {
    error = true;
    gerr << "Error: Unknown type " << functype << ", " << 
      PortionSpecToText(funcspec) << " as return type in declaration of " << 
      funcname << "[]\n";
  }

//  function->Dump(gout);

  for (int i = 1; i <= formals.Length(); i++)   {
    PortionSpec spec;
    if(portions[i])
      spec = portions[i]->Spec();
    else
      spec = TextToPortionSpec(types[i]);

    if (spec.Type != porERROR)   {
      if (refs[i])
	func->SetParamInfo(0, i - 1, 
                          ParamInfoType(formals[i], spec,
			                portions[i], BYREF));
      else
	func->SetParamInfo(0, i - 1, 
                          ParamInfoType(formals[i], spec,
											portions[i], BYVAL));
    }
    else   {
      error = true;
      gerr << "Error: Unknown type " << types[i] << ", " << 
	PortionSpecToText(spec) << " for parameter " << formals[i] <<
	 " in declaration of " << funcname << "[]\n";
      break;
    }
  }

  if (!error)  gsm.DeleteFunction(func);
  formals.Flush();
  types.Flush();
  refs.Flush();
  portions.Flush();
  function = 0;
  return !error;
}


int GCLCompiler::Execute(void)
{
#ifdef ASSEMBLY
  program.Dump(gout);   gout << '\n';
#endif   // ASSEMBLY
  int result = gsm.Execute(program);
  gsm.Flush();
  return result;
}


void GCLCompiler::LoadInputs( const char* name )
{

  extern char* _SourceDir;
  const char* SOURCE = _SourceDir; 
  assert( SOURCE );

#ifdef __GNUG__
  const char SLASH = '/';
  const char SLASH1 = '/';
#elif defined __BORLANDC__
  const char * SLASH = "\\";
  const char  SLASH1 = '\\';
#endif   // __GNUG__

  bool search = false;
  bool ini_found = false;
  if( strchr( name, SLASH1 ) == NULL )
    search = true;
  gString IniFileName;

  IniFileName = (gString) name;
  inputs.Push( new gFileInput( IniFileName ) );
  if (!inputs.Peek()->IsValid())
    delete inputs.Pop();
  else
  {  
    GCL_InputFileNames.Push( IniFileName );
    ini_found = true;
  }

  if( search )
  {

    if( !ini_found && (System::GetEnv( "HOME" ) != NULL) )
    {
      IniFileName = (gString) System::GetEnv( "HOME" ) + SLASH + name;
      inputs.Push( new gFileInput( IniFileName ) );
      if (!inputs.Peek()->IsValid())
        delete inputs.Pop();
      else
      {  
        GCL_InputFileNames.Push( IniFileName );
        ini_found = true;
      }
    }

    if( !ini_found && (System::GetEnv( "GCLLIB" ) != NULL) )
    {
      IniFileName = (gString) System::GetEnv( "GCLLIB" ) + SLASH + name;
      inputs.Push( new gFileInput( IniFileName ) );
      if (!inputs.Peek()->IsValid())
        delete inputs.Pop();
      else
      {
        GCL_InputFileNames.Push( IniFileName );
        ini_found = true;
      }
    }

    if( !ini_found && (SOURCE != NULL) )
    {
      IniFileName = (gString) SOURCE + SLASH + name;
      inputs.Push( new gFileInput( IniFileName ) );
      if (!inputs.Peek()->IsValid())
        delete inputs.Pop();
      else
      {  
        GCL_InputFileNames.Push( IniFileName );
        ini_found = true;
      }
    }

  }

  if( ini_found )
    lines.Push(1);
  else
    gerr << "GCL Warning: " << name << " not found.\n";
}
