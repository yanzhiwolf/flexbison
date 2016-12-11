%option noyywrap nodefault yylineno
%{
#include "fb3-1.h"
#include "fb3-1.tab.h"
%}

/*浮点数指数部分*/
EXP ([Ee][-+]?[0-9]+)

%%
"+" |
"-" |
"*" |
"/" |
"|" |
"(" |
")" {return yytext[0];}
[0-9]+"."[0-9]*{EXP}? |
"."?[0-9]+{EXP}? {yylval.d = atof(yytext); return NUMBER;}
\n {return EOL;}
"//".*
[ \t] {/*ignore*/}
. {yyerror("Mystery character %c\n", *yytext);}
%%