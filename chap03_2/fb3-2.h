/*
 * fb3-1计算器的声明部分
 */

/*与词法分析器的接口*/
extern int yylineno;
void yyerror(char *s, ...);

/*固定大小的简单的符号表*/
/*符号表*/
struct ast;
struct symlist;
struct symbol {
    char *name;
    double value;
    struct ast *func;
    struct symlist *syms;   /*formal params*/
};

#define NHASH 9997
struct symbol symtab[NHASH];
struct symbol *lookup(char*);

/*符号列表，作为参数列表*/
struct symlist {
    struct symbol *sym;
    struct symlist *next;
};
struct symlist *newsymlist(struct symbol *sym, struct symlist *next);
void symlistfree(struct symlist *sl);

/* 节点类型说明
 + - * / |
 0-7 比较操作符，位编码：04 等于，02 小于， 01 大于
 M 单目符号
 L 表达式或者语句列表
 I IF语句
 W WHILE语句
 N 符号引用
 = 赋值
 S 符号列表
 F 内置函数调用
 C 用户函数调用
*/
enum bifs { /*内置函数*/
    B_sqrt = 1,
    B_exp,
    B_log,
    B_print
};

/*抽象语法树中的节点，所有节点都有公共的初始nodetype*/
struct ast {
    int nodetype;
    struct ast *l;
    struct ast *r;
};

struct fncall { /*内置函数*/
    int nodetype;   /*类型F*/
    struct ast *l;
    enum bifs functype;
};

struct ufncall { /*用户自定义函数*/
    int nodetype;   /*类型C*/
    struct ast *params;
    struct symbol *s;
};

struct flow { /*类型I或者N*/
    int nodetype;
    struct ast *cond;   /*条件*/
    struct ast *tl;     /*then分支或者do语句*/
    struct ast *el;     /*可选的else分支*/
};

struct numval {
    int nodetype;   /*类型K*/
    double number;
};

struct symref {
    int nodetype;       /*类型N*/
    struct symbol *s;
};

struct symasgn {
    int nodetype;       /*类型=*/
    struct symbol *s;
    struct ast *v;      /*值*/
};

/*构造抽象语法树*/
struct ast *newast(int nodetype, struct ast *l, struct ast *r);
struct ast *newcmp(int cmptype, struct ast *l, struct ast *r);
struct ast *newfunc(int functype, struct ast *l);
struct ast *newcall(struct symbol *s, struct ast *params);
struct ast *newref(struct symbol *s);
struct ast *newasgn(struct symbol *s, struct ast *v);
struct ast *newnum(double d);
struct ast *newflow(int nodetype, struct ast *cond, struct ast *tl, struct ast *el);

/*定义函数*/
void dodef(struct symbol *name, struct symlist *syms, struct ast *stmts);

/*计算抽象语法树*/
double eval(struct ast *);

/*删除和释放抽象语法树*/
void treefree(struct ast *);
