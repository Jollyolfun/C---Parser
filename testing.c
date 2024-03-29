#include <stdio.h>
#include "scanner.h"
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include <assert.h>
typedef enum {
    LABEL,    //0
    GOTO,     //1
    ASSGST,   //2
    ADDST,    //3
    MULST,    //4
    DIVST,    //5
    SUBST,    //6
    ENTER,    //7
    LEAVE,    //8
    CALL,     //9
    FN_CALL,  //10
    PARAM,    //11

} QUADNodeType;

struct QUAD {


    QUADNodeType type;

    QUADInfo symbol *src1;
    QUADInfo symbol *src2;
    QUADInfo symbol *dest;

    struct QUAD *next;


};

struct QUADInfo {
    QUADNodeType type;

    struct symbol *symbolPointer;
    int intConst;
};

struct ASTnode {

    NodeType type;
    char *name;

    int intConst;
    struct symbol *symbolPointer;
    

    struct ASTnode *child1;
    struct ASTnode *child2;
    struct ASTnode *child3;

    struct QUAD *code; //the pointer to the head of the code for this node
    struct symbol *place;  //the location of this node in the symbol table


};

struct argNode {
    char *argName;
    struct argNode *next; 
};

struct symbol {

    char *name;
    int val;
    int numOfArguments;
    char *typeInfo;
    struct symbol *next;
    struct argNode *args;

    int offset;
    int isIntConst;

};

struct scope {
    struct symbol *symbolList;
    struct scope *nextScope;
};

char *curFuncName;
char *curID;
extern char *lexeme;
extern int get_token();
extern int lineNum;
extern int chk_decl_flag;
extern int print_ast_flag;
extern int gen_code_flag;
int curArgs;
int currentTok;
int isError = 0;
struct scope *theScope;
char *curIdentifier;
static int label_num = 0; //used to generate unique label names

//this function takes every character of lexeme and pushes it back to stdin.
void type();
void id_list();
struct ASTnode *opt_stmt_list();
struct ASTnode *assg_stmt();
void addToTable(struct symbol *newSymbol);
struct symbol *tableContains(char *curID);
struct symbol *containsFunc(char *curID);

int tmpNumber = 0;
char *tmpVar = "tmp";
//generates a new QUAD node  with the provided symbol table entries 
struct QUAD *newinstr(QUADNodeType opType, struct symbol *src1, struct symbol *src2, struct symbol *dest) {
    struct QUAD *ninstr = malloc(sizeof(struct QUAD));
    ninstr->type = opType;
    ninstr->src1 = src1; 
    ninstr->src2 = src2; 
    ninstr->dest = dest;
    ninstr->next = NULL;
    return ninstr;
}

void printlnCode() {
    printf(".align 2\n");
    printf(".data\n");
    printf("_nl: .asciiz \"\\n\"\n");
    printf(".align 2\n");
    printf(".text\n");
    printf("# println: print out an integer followed by a newline\n");
    printf("_println:\n");
    printf("    li $v0, 1\n");
    printf("    lw $a0, 0($sp)\n");
    printf("    syscall\n");
    printf("    li $v0, 4\n");
    printf("    la $a0, _nl\n");
    printf("    syscall\n");
    printf("    jr $ra\n");
}
//creates the unused name of a tmp variable
char *createTmp() {
    char xStr[100]; 
    sprintf(xStr, "%d", tmpNumber++);
    char *newStr = malloc(strlen(tmpVar) + strlen(xStr) + 1);
    strcpy(newStr, tmpVar);  
    strcat(newStr, xStr); 

    return newStr;
}

char *createLabel(char *name) {
    char xStr[100]; 
    sprintf(xStr, "%d", label_num++);
    char *newStr = malloc(strlen(name) + strlen(xStr) + 1);
    strcpy(newStr, name);  
    strcat(newStr, xStr); 
    return newStr;
}

//creates a new symbol table entry for a tmp variable and adds it to the table
struct symbol *newTemp() {
    struct symbol *ntmp = malloc(sizeof(struct symbol));
    ntmp->name = strdup(createTmp());
    ntmp->typeInfo=strdup("int");
    ntmp->next = NULL;
    ntmp->args = NULL;
    ntmp->numOfArguments = -1;
    addToTable(ntmp);


    //add the new node to the symbol table
    return ntmp;
}

//params start at 8
//locals start at -4
int debugging = 1;
void genCode(struct ASTnode *node) {
    switch(node->type) {

        case ASSG: {
            //generating code for children.
            struct symbol *LHSVar = node->symbolPointer; //LHS
            genCode(node->child2); //RHS
            struct QUAD *RHS = node->child2->code;
            struct QUAD *newInstr = newinstr(ASSGST, node->child2->place, NULL, LHSVar);

            struct QUAD *cur;
            for (cur=RHS; cur->next != NULL; cur = cur->next);
            cur->next = newInstr;
            
        }

        case IDENTIFIER: {
            struct symbol *theIDSymbol = tableContains(node->name);
            node->place = theIDSymbol;
            node->code = NULL;
        }

        case INTCONST: {
            struct symbol *newPlace = newTemp();
            node->place = newPlace;

            struct QUAD *newInstr = newinstr(ASSG, NULL, NULL, node->place);


        }



    }

}





//returning type of funtion
NodeType ast_node_type(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->type;
}
//returning function name
char *func_def_name(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->name;
}
//returning number of args
int func_def_nargs(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->symbolPointer->numOfArguments;
}

//getting the nth argname from the function
char *func_def_argname(void *ptr, int n) {

    
    struct ASTnode *ast = ptr;

    struct argNode *curArg;
    int i = 1;
    for (curArg = ast->symbolPointer->args; curArg != NULL; curArg = curArg->next) {
        if (i == n) {
            return curArg->argName;
        }
        i += 1;
    }
    return "UNDEFINED";
}

void * func_def_body(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child1;
}

char * func_call_callee(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->name;
}

void * func_call_args(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child1;
}

void * stmt_list_head(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child1;
}
void * stmt_list_rest(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child2;  
}

void * expr_list_head(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child1;

}
void * expr_list_rest(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child2;
}

char *expr_id_name(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->name;
}
int expr_intconst_val(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->intConst;
}
void * expr_operand_1(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child1;
}
void * expr_operand_2(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child2;
}
void * stmt_if_expr(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child1;
}

void * stmt_if_then(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child2;
}
void * stmt_if_else(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child3; 
}

char *stmt_assg_lhs(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->name;
}
void *stmt_assg_rhs(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child1;
}

void *stmt_while_expr(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child1;
}

void *stmt_while_body(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child2;  
}

void *stmt_return_expr(void *ptr) {
    struct ASTnode *ast = ptr;
    return ast->child1; 
}








void addToTable(struct symbol *newSymbol) {
    if (theScope->symbolList == NULL) {
        theScope->symbolList = newSymbol;
    }
    //if there are symbols in the scope, go to the end and add it
    else {
        struct symbol *curSymbol = NULL;
        for (curSymbol = theScope->symbolList; curSymbol->next != NULL; curSymbol = curSymbol->next) {
        }
        curSymbol->next = newSymbol;
    }

}


void contains(char *curID, char *typeInf) {
    if (theScope->symbolList == NULL) {
        struct symbol *newSymbol = malloc(sizeof(struct symbol));
        newSymbol->name = strdup(curID);
        newSymbol->typeInfo = strdup(typeInf);
        newSymbol->next = NULL;
        newSymbol->args = NULL;
        theScope->symbolList = newSymbol;
    }
    //if there are symbols in the scope, go to the end and add it
    else {
        struct symbol *curSymbol = NULL;
        for (curSymbol = theScope->symbolList; curSymbol->next != NULL; curSymbol = curSymbol->next) {
            if (chk_decl_flag) {
                if (strcmp(curSymbol->name, curID) == 0) {
                    fprintf(stderr, "ERROR LINE %d REPEAT SYMBOL %s\n", lineNum, curID);
                    exit(1);
                }
            }


        }
        if (chk_decl_flag) {
            if (strcmp(curSymbol->name, curID) == 0) {
                fprintf(stderr, "ERROR LINE %d REPEAT SYMBOL %s\n", lineNum, curID);
                exit(1);
            }
        }


        struct symbol *newSymbol = malloc(sizeof(struct symbol));
        newSymbol->name = strdup(curID);
        newSymbol->typeInfo = strdup(typeInf);
        newSymbol->next= NULL;
        newSymbol->args = NULL;

        curSymbol->next = newSymbol;
    }

}

struct symbol *containsFunc(char *curID) {
    if (chk_decl_flag) {
        struct scope *curScope;
        int i = 0;
        for (curScope = theScope; curScope != NULL; curScope = curScope->nextScope) {
            struct symbol *curSymbol = NULL;
            for (curSymbol = curScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {
                //if the symbol we're looking at is not a function and is in the local scope
                if (i == 0 && strcmp(curSymbol->name, curID) == 0 && strcmp(curSymbol->typeInfo, "function") != 0) {
                    //TODO ADD THIS BACK IF YOU NEED TO
                    fprintf(stderr, "ERROR LINE %d UNDECLARED FUNCTION %s\n", lineNum, curID);
                    exit(1);  
                }
                if (strcmp(curSymbol->name, curID) == 0) {
                    return curSymbol; 
                }
            }
            i += 1;
        }
        fprintf(stderr, "ERROR LINE %d UNDECLARED FUNCTION %s\n", lineNum, curID);
        exit(1);
    }
}

struct symbol *containsParameter(char *curID) {
    struct scope *curScope;
    int i = 0;
    for (curScope = theScope; curScope != NULL; curScope = curScope->nextScope) {
        struct symbol *curSymbol = NULL;
        for (curSymbol = curScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {

            if (strcmp(curSymbol->name, curID) == 0 && strcmp(curSymbol->typeInfo, "variable" ) == 0) {
                return curSymbol; 
            }
        }
        i += 1;
    }
    if (chk_decl_flag) {
        fprintf(stderr, "ERROR LINE %d PARAMETER NOT PREVIOUSLY DEFINED %s\n", lineNum, curID);
        exit(1);
    }

    return NULL;
}

//finds and returns a symbol table entry if it exists
struct symbol *tableContains(char *curID) {
    struct scope *curScope;
    int i = 0;
    for (curScope = theScope; curScope != NULL; curScope = curScope->nextScope) {
        struct symbol *curSymbol = NULL;
        for (curSymbol = curScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {

            if (strcmp(curSymbol->name, curID) == 0) {
                return curSymbol; 
            }
        }
        i += 1;
    }

    return NULL;
}


void match(char expected) {
    if (currentTok == expected) {
        currentTok = get_token();
    } 
    else {

        fprintf(stderr, "ERROR LINE %d IN match with expected: %d and actual %d with lexeme %s\n", lineNum, expected, currentTok, lexeme);
        exit(1);
    }
}

struct ASTnode *arith_exp() {
    struct ASTnode *ast = malloc(sizeof(struct ASTnode));
    ast->name = strdup(lexeme); //need to set name
    ast->intConst = 0;
    ast->symbolPointer = NULL;
    ast->child1 = NULL;
    ast->child2 = NULL;
    ast->child3 = NULL;
    if (currentTok == ID) {
        //if there is use of an ID, check if it has been previously defined
        if (chk_decl_flag) {
            ast->type = IDENTIFIER;
            ast->symbolPointer = containsParameter(lexeme);
        }
        
        match(ID);
    }
    else if (currentTok == INTCON) {
        int a = atoi(lexeme);
        ast->intConst = a;
        ast->type = INTCONST;
        match(INTCON);
    }
    else{
        fprintf(stderr, "ERROR LINE %d IN arith_expr\n", lineNum);
        exit(1); 
    }

    return ast;

}






NodeType relop() {
    if (currentTok == opEQ) {
        match(opEQ);
        return EQ;
    }    
    else if (currentTok == opNE) {
        match(opNE);
        return NE;
    }
    else if (currentTok == opLE){
        match(opLE);
        return LE;
    }    
    else if (currentTok == opLT) {
        match(opLT);
        return LT;
    }    
    else if (currentTok == opGE) {
        match(opGE);
        return GE;
    }
    else if (currentTok == opGT) {
        match(opGT);
        return GT;
    }
    else {
        fprintf(stderr, "ERROR LINE %d IN relop\n", lineNum);
        exit(1);
    }
}

struct ASTnode *bool_exp() {
    struct ASTnode *ast = malloc(sizeof(struct ASTnode));
    ast->name = strdup("bool_exp");
    ast->intConst = 0;
    ast->symbolPointer = NULL;
    ast->child1 = NULL;
    ast->child2 = NULL;
    ast->child3 = NULL;
    
    ast->child1 = arith_exp();
    NodeType type = relop();
    ast->type= type;
    ast->child2 = arith_exp();

    return ast;


}

struct ASTnode *expr_list() {
    struct ASTnode *ast = malloc(sizeof(struct ASTnode));
    ast->type= EXPR_LIST;
    ast->name = strdup("EXPR_LIST");
    ast->intConst = 0;
    ast->symbolPointer = NULL;
    ast->child1 = NULL;
    ast->child2 = NULL;
    ast->child3 = NULL;
    
    ast->child1 = arith_exp();

    curArgs += 1;
    if (currentTok == COMMA) {
        match(COMMA);
        ast->child2 = expr_list();
    }

    return ast;

}

struct ASTnode *opt_expr_list() {
    if (currentTok == RPAREN) {
        return NULL;
    }
    struct ASTnode *ast = malloc(sizeof(struct ASTnode));
    ast->type= EXPR_LIST;
    ast->name = strdup("EXPR_LIST");
    ast->intConst = 0;
    ast->symbolPointer = NULL;
    ast->child1 = NULL;
    ast->child2 = NULL;
    ast->child3 = NULL;
    ast->child1 = expr_list();
    return ast;
}
struct ASTnode *fn_call() {
    struct ASTnode *ast = malloc(sizeof(struct ASTnode));
    ast->type= FUNC_CALL;
    ast->name = strdup(curIdentifier);
    ast->intConst = 0;
    ast->symbolPointer = NULL;
    ast->child1 = NULL;
    ast->child2 = NULL;
    ast->child3 = NULL;
    containsFunc(curIdentifier);
    
    match(LPAREN);
    ast->child1 = opt_expr_list();
    match(RPAREN);
    return ast;
}

struct ASTnode *stmt();

struct ASTnode *while_stmt() {

    struct ASTnode *ast = malloc(sizeof(struct ASTnode));
    ast->type= WHILE;
    ast->name = strdup("WHILE");
    ast->intConst = 0;
    ast->symbolPointer = NULL;
    ast->child1 = NULL;
    ast->child2 = NULL;
    ast->child3 = NULL;


    match(kwWHILE);
    match(LPAREN);
    ast->child1 = bool_exp();
    match(RPAREN);
    ast->child2 = stmt();

    return ast;


}

struct ASTnode *if_stmt() {

    struct ASTnode *ast = malloc(sizeof(struct ASTnode));
    ast->type= IF;
    ast->name = strdup("IF");
    ast->intConst = 0;
    ast->symbolPointer = NULL;
    ast->child1 = NULL;
    ast->child2 = NULL;
    ast->child3 = NULL;

    match(kwIF);

    match(LPAREN);
    ast->child1 = bool_exp();
    match(RPAREN);
    ast->child2 = stmt();

    if (currentTok == kwELSE) {
        match(kwELSE);
        ast->child3 = stmt();
    }

    return ast;

}

struct ASTnode *return_stmt() {

    struct ASTnode *ast = malloc(sizeof(struct ASTnode));
    ast->type= RETURN;
    ast->name = strdup("RETURN");
    ast->intConst = 0;
    ast->symbolPointer = NULL;
    ast->child1 = NULL;
    ast->child2 = NULL;
    ast->child3 = NULL;

    match(kwRETURN);
    if (currentTok == ID || currentTok == INTCON) {
        //if the current token is an ID, set the curIdentifier to the lexeme ID
        if (currentTok == ID) {
            curIdentifier = strdup(lexeme);
        }
        ast->child1 = arith_exp();
    }
    match(SEMI);

    return ast;
}

struct ASTnode *assg_stmt() {
    struct ASTnode *ast = malloc(sizeof(struct ASTnode));
    ast->type= ASSG;
    ast->name = strdup(curIdentifier);
    ast->intConst = 0;
    ast->symbolPointer = NULL;
    ast->child1 = NULL;
    ast->child2 = NULL;
    ast->child3 = NULL;
    ast->child1 = arith_exp();
    match(SEMI);

    return ast;
}

struct ASTnode *stmt() {
    struct ASTnode *ast = NULL;
    //add other statements

    //if current token is while, the statement is a whille loop etc
    
    //if the currentTok == ID then it can be an assg_stmt or a fn_call
    if (currentTok == ID) {
        curIdentifier = strdup(lexeme);
        match(ID);
        if (currentTok == opASSG) {
            //if it is an assignment, check if the curidentifier is in the table
            if (chk_decl_flag) {
                containsParameter(curIdentifier);
            }
            match(opASSG);
            ast = assg_stmt();
            return ast;
        }
        curArgs = 0;
        ast = fn_call();
        



        //here we will check if the number of arguments for the function matches
        //the number of arguments for the function call
        if (chk_decl_flag) {

        
            struct scope *curScope;
            int stop = 0;
            for (curScope = theScope; curScope != NULL; curScope = curScope->nextScope) {
                struct symbol *curSymbol;
                for (curSymbol = curScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {
                    //if the current symbol is a function and the current symbol's name matches curI
                    //and the number of arguments matchthen set its arg count to curArgs
                    if (strcmp(curSymbol->typeInfo, "function") == 0 && strcmp(curSymbol->name, curIdentifier) == 0 && curSymbol->numOfArguments == curArgs) {
                        stop = 1;
                        break;
                    }
                    if (chk_decl_flag && strcmp(curSymbol->typeInfo, "function") == 0 && strcmp(curSymbol->name, curIdentifier) == 0 && curSymbol->numOfArguments != curArgs) {
                        printf("num of args in call: %d and num of args in func: %d\n", curArgs, curSymbol->numOfArguments);
                        fprintf(stderr, "ERROR LINE %d IN stmt, FUNCTION CALL'S NUMBER OF ARGUMENTS DOES NOT MATCH DEFINITION\n", lineNum);
                        exit(1);
                    }
                }
                if (stop) {
                    break;
                }
            }
        }
        curArgs = 0;




        match(SEMI);
    }
    else if (currentTok == kwWHILE) {
        ast = while_stmt();
    }
    else if (currentTok == kwIF) {
        ast = if_stmt();
    }
    else if (currentTok == SEMI) {
        match(SEMI);
        ast = NULL;
    }
    else if (currentTok == kwRETURN) {
        ast = return_stmt();
    }
    else if (currentTok == LBRACE) {
        match(LBRACE);
        ast = opt_stmt_list();
        match(RBRACE);
    }
    else {
        fprintf(stderr, "ERROR LINE %d IN stmt\n", lineNum);
        exit(1);
    }
    return ast;

}

struct ASTnode *opt_stmt_list() {
    if (currentTok != RBRACE) {
        struct ASTnode *ast = malloc(sizeof(struct ASTnode));
        ast->type = STMT_LIST;
        ast->name = strdup("stmt");
        ast->intConst = 0;
        ast->symbolPointer = NULL;
        ast->child1 = NULL;
        ast->child2 = NULL;
        ast->child3 = NULL;
        ast->child1 = stmt();
        ast->child2 = opt_stmt_list();
        return ast;
    }
    //if we are at the end, return NULL
    else {
        return NULL;
    }
    
    
    


}

void formals() {
    type();
    char *curID = strdup(lexeme);
    //adding the variable to the symbol table
    contains(curID, "variable");

    //TODO no todo, just keep in mind that this assume functions are only global
    struct scope *currentScope;
    for(currentScope = theScope; currentScope->nextScope != NULL; currentScope = currentScope->nextScope);

    //add the current argument to the args struct for that function
    struct symbol *curSymbol;
    //search for the function named curFuncName

    for (curSymbol = currentScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {
        //if we encounter the function with the name to add an arg to
        if (strcmp(curSymbol->typeInfo, "function") == 0 && strcmp(curFuncName, curSymbol->name) == 0) {
            struct argNode *newArg = malloc(sizeof(struct argNode));
            newArg->argName = strdup(curID);
            newArg->next = NULL;
            if (curSymbol->args == NULL) {
                curSymbol->args = newArg;
        
            }
            else{
                //go to end of list and add to the end
                struct argNode *curArg;
                for (curArg = curSymbol->args; curArg->next != NULL; curArg = curArg->next) {
        
                }
                //adding the new argument to the end of the argument list for the current function
                curArg->next = newArg;

            }


        }
    }

    

    curArgs += 1;

    match(ID);
    //if we're at the end of the formals, return out
    if (currentTok == RPAREN) {
        return;
    }
    else if (currentTok == COMMA) {
        match(COMMA);
        formals();
    }
    else {
        fprintf(stderr, "ERROR LINE %d IN formals\n", lineNum);
        exit(1);
    }
}


void opt_formals() {
    //if there is nothing inside the paren, return
    if (currentTok == RPAREN) {
        return;
    }
    formals();
}


void var_decl_in_bracket() {   
    type();
    id_list();
}

void opt_vars_decls() {

    if (currentTok == kwINT) {
        var_decl_in_bracket();
        opt_vars_decls();
    }
    else if (currentTok == EOF) {
        fprintf(stderr, "ERROR LINE %d IN opt_vars_decls\n", lineNum);
        exit(1);
    }
    
}

//match function definition
struct ASTnode *func_defn() {
    //creating the AST node 
    struct ASTnode *ast = malloc(sizeof(struct ASTnode));
    ast->type = FUNC_DEF;
    ast->name = strdup(curFuncName);
    ast->symbolPointer = NULL;
    ast->intConst = 0;
    ast->child1 = NULL;
    ast->child2 = NULL;
    ast->child3 = NULL;


    match(LPAREN);
    opt_formals();
    match(RPAREN);
    //end of arguments, now curArguments should have tge proper num
    //goal: find node with curID as name, replace its argument count with curArgs
    struct scope *curScope;
    int stop = 0;
    for (curScope = theScope; curScope != NULL; curScope = curScope->nextScope) {
        struct symbol *curSymbol;
        for (curSymbol = curScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {
            //if the current symbol is a function and the current symbol's name matches curI
            //then set its arg count to curArgs
            if (strcmp(curSymbol->typeInfo, "function") == 0 && strcmp(curSymbol->name, curID) == 0) {
                
                //setting the ASTs symbolTable pointer for this function to the function's table entry
                ast->symbolPointer = curSymbol;
                
                
                curSymbol->numOfArguments = curArgs;
                stop = 1;
                break;
            }
        }
        if (stop) {
            break;
        }
    }
    curArgs = 0;
    
    


    match(LBRACE);
    opt_vars_decls();
    ast->child1 = opt_stmt_list();
    match(RBRACE);

    return ast;
}

void id_list() {
    //id list must stard with id

    char *curID = strdup(lexeme);
    //this is where lexeme is the ID, check here
    //if the scope's list is null, add this symbol, otherwise check if it's there

    contains(curID, "variable");

    match(ID);

    if (currentTok == SEMI) {
        match(SEMI);
        return;
    }
    else if (currentTok == COMMA) {
        match(COMMA);
        id_list();
    }
    else {
        fprintf(stderr, "ERROR LINE %d AT id_list\n", lineNum);
        exit(1);
    }
    
}
void var_decl() {   

    if (currentTok == SEMI) {
        match(SEMI);
        return;
    }
    else if (currentTok == COMMA) {
        match(COMMA);
            
        //recursing into id_list
        id_list();
    }
    else {
        fprintf(stderr, "ERROR LINE %d AT var_decl\n", lineNum);
        exit(1);
    }
}



void prog() {
    //if the file is empty, exit the program with no error 
    while (1) {
        if (currentTok == EOF) {
            exit(0);
        }
        //we have to match kwINT and ID first
        else if (currentTok == kwINT) {
            //since both var_decls and func_defn start with kwINT and ID, match both
            
            match(kwINT);
            
            curID = strdup(lexeme);
            match(ID);


            //from here, we've matched, for instance, 'int x'
            //now, if the next token is (, we know it's a function
            //definition, otherwise it's a variable decl
            if (currentTok == LPAREN) {


                //adding the function name to the symbol table

                contains(curID, "function");
                //constructing and adding the new scope to the top of the stack
                struct scope *newScope = malloc(sizeof(struct scope));
                newScope->symbolList = NULL; 
                newScope->nextScope = theScope; 
                theScope = newScope;

                curArgs = 0;
                curFuncName = strdup(curID);
                //used for getting arg numbner for code gen
                char *funcNameForArgs = strdup(curID);
                struct ASTnode *tree = func_defn();
                
                if (print_ast_flag) {
                    print_ast(tree);
                }
                
                //TODO generate the 3 address code
                if (gen_code_flag) {

                    //calculating the amount of variables needed when generating the mips code
                    //for storage purposes



                    
                    //gathering the number of formals for the function so later we can skip formals
                    //when generating mips code
                    struct symbol *curFunc = tableContains(funcNameForArgs);
                    int formalCount = curFunc->numOfArguments;

                    struct QUAD *TAC = tree->code;
                    
                    // int numberOfVars = 0;
                    // struct symbol *curSymbol;
                    // for (curSymbol=theScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {
                    //     numberOfVars += 1;
                    // }
                    // printf("numberOfVars: %d\n", numberOfVars);

                    // int curOffset = -4;
                    // int j = 1;
                    // //todo DONE assign offsets
                    // //for every symbol in the symbol list, if it is not a formal, generate an offset of -4, -8, and so on
                    // for (curSymbol = theScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {
                    //     //if we are currently NOT observing a formal, add the offset
                    //     if (j > formalCount) {
                    //         curSymbol->offset = curOffset;
                    //         curOffset -= 4;
                    //     }
                    //     j+= 1;
                    // }
                    
                    // curOffset += 4; //resetting curOffset
                    // curOffset *= -1; //turn the offset to positive.
                    // j = 1;
                    // for (curSymbol = theScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {
                    //     //if we are currently NOT observing a formal, add the offset
                    //     if (j <= formalCount) {
                    //         curSymbol->offset = curOffset;
                    //         curOffset += 4;
                    //     }
                    //     j+= 1;
                    // }








                    // int bytesNeeded = numberOfVars * 4;

                    // printlnCode();
                    // struct QUAD *curTAC;
                    // for (curTAC = TAC; curTAC != NULL; curTAC = curTAC->next) {
                        
                        
                    //     //TODO generate MIPS code
                    //     switch (curTAC->type) {
                            
                    //         case LABEL: {
                    //             printf("# LABEL %s\n", curTAC->dest->name);
                    //             printf(".align 2\n");
                    //             printf(".text\n");
                    //             printf("_%s:\n", curTAC->dest->name);
                    //             break;
                    //         }
                    //         case ENTER: {
                    //             printf("    la $sp, -8($sp) # allocate space for old $fp and $ra\n");
                    //             printf("    sw $fp, 4($sp) # save old $fp\n");
                    //             printf("    sw $ra, 0($sp) # save return address\n");
                    //             printf("    la $fp, 0($sp) # set up frame pointer\n");
                    //             printf("    la $sp, -%d($sp) # allocate stack frame\n", bytesNeeded);
                    //             printf("\n");
                    //             break;
                    //         }
                    //         case CALL: {
                    //             printf("\n");
                    //             struct symbol *cFunc = tableContains(curTAC->src1->name);
                    //             printf("    jal _%s\n", curTAC->src1->name);
                    //             printf("    la $sp, %d($sp)\n", cFunc->numOfArguments * 4);
                    //             printf("\n");
                    //         }
                    //         case PARAM: {
                    //             printf("\n");
                    //             printf("    la $sp, -4($sp)\n");
                    //             printf("    sw $t0, 0($sp)\n");
                    //             printf("\n");
                    //         }
                    //         case ASSGST: {
                    //            //if the rhs of the assignment statement (src1) is not null, it is an intconst
                    //             printf("\n");
                    //             if (curTAC->dest != NULL && curTAC->dest->isIntConst) {
                    //                 printf("    li $t0, %d\n", curTAC->dest->val);
                    //                 printf("    sw $t0, %d($fp)\n", curTAC->dest->offset);
                    //             } 
                    //             //if it is not an intConst
                    //             else if (curTAC->src1 != NULL && curTAC->src1->isIntConst == 0) {
                    //                 printf("not an int\n");
                    //             }
                    //             printf("\n");


                    //         }

                    //     }
                        

                            
                        
                    // }
                    // printf("main:\n    j _main\n");
                }




                //pop the scope from the scope list
                theScope = theScope->nextScope;


                
            }
            //if the current token is a SEMI or COMMA
            else if (currentTok == SEMI || currentTok == COMMA) {

                //checks if curID (the current ID) is in the current scope or not
                contains(curID, "variable");

                //the call to declare variables
                var_decl();

            }
            else {
                fprintf(stderr, "ERROR LINE %d AT ELSE IN prog\n", lineNum);
                exit(1);
            }

        }
        else {
            fprintf(stderr, "ERROR LINE %d AT ELSE IN prog\n", lineNum);
            exit(1);
        }
        
    }

}


void type() {
    //if the current token matches some ID, see if it matches.
    if (currentTok == kwINT) {
        match(kwINT);
    }
    //this allows us to enter in other types later as else if statements.
    else {

        fprintf(stderr, "ERROR LINE %d IN type\n", lineNum);
        exit(1);
    }
}
    // char *name;
    // int val;
    // int numOfArguments;
    // char *typeInfo;
    // struct symbol *next;
    // struct argNode *args;


int parse() {
    //going until the end of a file
    //here is where tok will represent the current token, and we
    //can see if it matches patterns
    curArgs = 0;
    //global scope 
    theScope = malloc(sizeof(struct scope));
    theScope->symbolList = NULL;
    theScope->nextScope = NULL;

    struct symbol *printlnStruct = malloc(sizeof(struct symbol));
    printlnStruct->name = strdup("println");
    printlnStruct->typeInfo = strdup("function");
    printlnStruct->next = NULL;
    printlnStruct->numOfArguments = 1;
    theScope->symbolList = printlnStruct;
    currentTok = get_token();
    prog();
        
    //if it reaches here then an error was never reached
    return 0;
}


