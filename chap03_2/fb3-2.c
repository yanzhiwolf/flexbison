#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "fb3-2.h"

static unsigned symhash(char *sym)
{
    unsigned int hash = 0;
    unsigned c;
    while (c = *sym++) {
        hash = hash * 9 ^ c;
    }
    return hash;
}
struct symbol *lookup(char *sym)
{
    struct symbol *sp = &symtab[symhash(sym) % NHASH];
    int scount = NHASH;

    while (--scount >= 0) {
        if (sp->name && !strcmp(sp->name, sym)) {
            return sp;
        }
        if (!sp->name) {
            sp->name = strdup(sym);
            sp->value = 0;
            sp->func = NULL;
            sp->syms = NULL;
            return sp;
        }
        if (++sp >= symtab+NHASH) {
            sp = symtab;
        }
    }

    yyerror("symbol table overflow\n");
    abort();
}

struct ast *newast(int nodetype, struct ast *l, struct ast *r)
{
    struct ast *a = malloc(sizeof(*a));
    if (!a) {
        yyerror("out of space");
        exit(0);
    }

    a->nodetype = nodetype;
    a->l = l;
    a->r = r;
    return a;
}

struct ast *newnum(double d)
{
    struct numval *a = malloc(sizeof(*a));
    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'K';
    a->number = d;
    return (struct ast*)a;
}

struct ast *newcmp(int cmptype, struct ast *l, struct ast *r)
{
    struct ast *a = malloc(sizeof(*a));
    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = '0' + cmptype;
    a->l = l;
    a->r = r;
    return a;
}

struct ast *newfunc(int functype, struct ast *l)
{
    struct fncall *a = malloc(sizeof(*a));
    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'F';
    a->l = l;
    a->functype = functype;
    return (struct ast*)a;
}

struct ast *newcall(struct symbol *s, struct ast *params)
{
    struct ufncall *a = malloc(sizeof(*a));
    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'C';
    a->params = params;
    a->s = s;
    return (struct ast*)a;
}

struct ast *newref(struct symbol *s)
{
    struct symref *a = malloc(sizeof(*a));
    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = 'N';
    a->s = s;
    return (struct ast*)a;
}

struct ast *newasgn(struct symbol *s, struct ast *v)
{
    struct symasgn *a = malloc(sizeof(*a));
    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = '=';
    a->s = s;
    a->v = v;
    return (struct ast*)a;
}

struct ast *newflow(int nodetype, struct ast *cond, struct ast *tl, struct ast *el)
{
    struct flow *a = malloc(sizeof(*a));
    if (!a) {
        yyerror("out of space");
        exit(0);
    }
    a->nodetype = nodetype;
    a->cond = cond;
    a->tl = tl;
    a->el = el;
    return (struct ast*)a;
}

/*用户自定义函数*/
void dodef(struct symbol *name, struct symlist *syms, struct ast *func) {
    if (name->syms) {
        symlistfree(name->syms);
    }
    if (name->func) {
        treefree(name->func);
    }
    name->syms = syms;
    name->func = func;
}

void treefree(struct ast *a)
{
    switch (a->nodetype) {
        /*两颗子树*/
        case '+': case '-': case '*': case '/':
        case '1': case '2': case '3': case '4': case '5': case '6':
        case 'L':
            treefree(a->r);
        /*一颗子树*/
        case '|':
        case 'M': case 'C': case 'F':
            treefree(a->l);

        /*没有子树*/
        case 'K': case 'N':
            break;

        case '=':
            free(((struct symasgn*)a)->v);
            break;

        /*最多三颗子树*/
        case 'I': case 'W': {
                struct flow *f = (struct flow*)a;
                free(f->cond);
                if (f->tl) {
                    free(f->tl);
                }
                if (f->el) {
                    free(f->el);
                }
            } break;
        default:
            printf("internal error: free bad node %c\n", a->nodetype);
    }
    free(a);
}

struct symlist *newsymlist(struct symbol *sym, struct symlist *next) {
    struct symlist *sl = malloc(sizeof(*sl));
    if (!sl) {
        yyerror("out of space");
        exit(0);
    }
    sl->sym = sym;
    sl->next = next;
    return sl;
}
void symlistfree(struct symlist *sl) {
    struct symlist *nsl;
    while (sl) {
        nsl = sl->next;
        free(sl);
        sl = nsl;
    }
}

static double callbuiltin(struct fncall*);
static double calluser(struct ufncall*);
double eval(struct ast *a)
{
    double v; /*子树的计算结果*/

    if (!a) {
        yyerror("internal error, null eval");
        return 0.0;
    }

    switch (a->nodetype) {
        /*常量*/
        case 'K': {
            v = ((struct numval *)a)->number;
        } break;

        /*名字引用*/
        case 'N': {
            v = ((struct symref*)a)->s->value;
        } break;

        /*赋值*/
        case '=': {
            struct symasgn *r = (struct symasgn*)a;
            v = r->s->value = eval(r->v);
        } break;

        /*表达式*/
        case '+': {
            v = eval(a->l) + eval(a->r);
        } break;
        case '-': {
            v = eval(a->l) - eval(a->r);
        } break;
        case '*': {
            v = eval(a->l) * eval(a->r);
        } break;
        case '/': {
            v = eval(a->l) / eval(a->r);
        } break;
        case '|': {
            v = eval(a->l);
            if (v < 0) v = -v;
        }  break;
        case 'M': {
            v = -eval(a->l);
        } break;

        /*比较*/
        case '1': {
            v = (eval(a->l) > eval(a->r))? 1 : 0;
        } break;
        case '2': {
            v = (eval(a->l) < eval(a->r))? 1 : 0;
        } break;
        case '3': {
            v = (eval(a->l) != eval(a->r))? 1 : 0;
        } break;
        case '4': {
            v = (eval(a->l) == eval(a->r))? 1 : 0;
        } break;
        case '5': {
            v = (eval(a->l) >= eval(a->r))? 1 : 0;
        } break;
        case '6': {
            v = (eval(a->l) <= eval(a->r))? 1 : 0;
        } break;

        /*控制流*/
        /*if/then/else*/
        case 'I': {
            struct flow *f = (struct flow*)a;
            if (eval(f->cond) != 0) {
                v = (f->tl) ? eval(f->tl) : 0.0;
            } else {
                v = (f->el) ? eval(f->el) : 0.0;
            }
        } break;

        /*while/do*/
        case 'W': {
            struct flow *f = (struct flow*)a;
            v = 0.0;
            if (f->tl) {
                while (eval(f->cond) != 0) {
                    v = eval(f->tl);
                }
            }
        } break;

        /*语句列表*/
        case 'L': {
            eval(a->l);
            v = eval(a->r);
        } break;
        case 'F': {
            v = callbuiltin((struct fncall*)a);
        } break;
        case 'C': {
            v = calluser((struct ufncall*)a);
        } break;

        default: {
            printf("internal error: bad node %c\n", a->nodetype);
        } break;
    }
    return v;
}

static double callbuiltin(struct fncall *f) {
    enum bifs functype = f->functype;
    double v = eval(f->l);
    switch (functype) {
        case B_sqrt: return sqrt(v);
        case B_exp:  return exp(v);
        case B_log:  return log(v);
        case B_print:
            printf("= %4.4g\n", v);
            return v;
        default:
            yyerror("Unknown built-in function %d", functype);
            return 0.0;
    }
}

static double calluser(struct ufncall *f) {
    struct symbol *fn = f->s;       /*function name*/
    struct symlist *sl;             /*formal params*/
    struct ast *params = f->params; /*real params*/
    double *oldval, *newval;
    double v;
    int nargs;
    int i;

    if (!fn->func) {
        yyerror("call to undefined function", fn->name);
        return 0;
    }

    /*calc the size of params*/
    sl = fn->syms;
    for (nargs = 0; sl; sl=sl->next) {
        nargs++;
    }

    /*prepare to save params value*/
    oldval = (double*)malloc(nargs * sizeof(double));
    newval = (double*)malloc(nargs * sizeof(double));
    if (!oldval || !newval) {
        yyerror("Out of memory in %s\n", fn->name);
        return 0.0;
    }

    /*calc real params's value*/
    for (i = 0; i < nargs; i++) {
        if (!params) {
            yyerror("too few params in call to %s\n", fn->name);
            return 0.0;
        }
        if (params->nodetype == 'L') {
            newval[i] = eval(params->l);
            params = params->r;
        } else {
            newval[i] = eval(params);
            params = NULL;
        }
    }

    sl = fn->syms;
    /*save formal params*/
    for (i = 0; i < nargs; i++) {
        struct symbol *s = sl->sym;
        oldval[i] = s->value;
        s->value = newval[i];
        sl = sl->next;
    }

    v = eval(fn->func);

    /*recover formal params*/
    sl = fn->syms;
    for (i = 0; i < nargs; i++) {
        struct symbol *s = sl->sym;
        s->value = oldval[i];
        sl = sl->next;
    }

    free(oldval);
    free(newval);

    return v;
}

void yyerror(char *s, ...)
{
    va_list ap;
    va_start(ap, s);
    fprintf(stderr, "%d: error: ", yylineno);
    vfprintf(stderr, s, ap);
    fprintf(stderr, "\n");
}

int main()
{
    printf("> ");
    return yyparse();
}


