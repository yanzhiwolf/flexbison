/*
 * fb3-1计算器的声明部分
 */

/*与词法分析器的接口*/
extern int yylineno;
void yyerror(char *s, ...);

/*抽象语法树中的节点*/
struct ast {
    int nodetype;
    struct ast *l;
    struct ast *r;
};
struct numval {
    int nodetype;
    double number;
};

/*构造抽象语法树*/
struct ast *newast(int nodetype, struct ast *l, struct ast *r);
struct ast *newnum(double d);

/*计算抽象语法树*/
double eval(struct ast *);

/*删除和释放抽象语法树*/
void treefree(struct ast *);
