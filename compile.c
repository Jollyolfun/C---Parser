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
    RET,      //12
    RETDEF,   //13 RETDEF is a default return of 0
    IFST



} QUADNodeType;

typedef enum {
    INTCONT,
    IDEN,
    LAB
} InfoType;

struct QUAD {


    QUADNodeType type;

    struct QUADInfo  *src1;
    struct QUADInfo  *src2;
    struct QUADInfo  *dest;

    struct QUAD *next;


};

//holds info for each quad
struct QUADInfo {
    InfoType type;

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

    int isGlobal;

};

struct scope {
    struct symbol *symbolList;
    struct scope *nextScope;
};
int glob = 0;
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

//this function takes every character of lexeme and pushes it back to stdin.
void type();
void id_list();
struct ASTnode *opt_stmt_list();
struct ASTnode *assg_stmt();
void addToTable(struct symbol *newSymbol);
struct symbol *tableContains(char *curID);
struct symbol *containsFunc(char *curID);

int tmpNumber = 0;
char *tmpVar = "_tmp";
//generates a new QUAD node  with the provided symbol table entries 
struct QUAD *newinstr(QUADNodeType opType, struct QUADInfo *src1, struct QUADInfo *src2, struct QUADInfo *dest) {
    struct QUAD *ninstr = malloc(sizeof(struct QUAD));
    ninstr->type = opType;
    ninstr->src1 = src1; 
    ninstr->src2 = src2; 
    ninstr->dest = dest;
    ninstr->next = NULL;
    return ninstr;
}

void globs() {
    //adding global vars
    struct scope *scopeCur;
    struct symbol *cur;
    printf(".align 2\n");
    printf(".data\n");
    for (scopeCur = theScope; scopeCur->nextScope != NULL; scopeCur = scopeCur->nextScope);
    for (cur = scopeCur->symbolList; cur != NULL; cur = cur->next) {
        if (strcmp(cur->typeInfo, "variable") == 0) {
            cur->isGlobal = 1;
            printf("_%s: .space 4\n", cur->name);
        }

    }  
}
void printlnCode() {
    struct scope *scopeCur;
    struct symbol *cur;

    //adding global vars
    //for (scopeCur = theScope; scopeCur->nextScope != NULL; scopeCur = scopeCur->nextScope);
    // for (cur = scopeCur->symbolList; cur != NULL; cur = cur->next) {
    //     if (strcmp(cur->typeInfo, "variable") == 0) {
    //         cur->isGlobal = 1;
    //         printf("_%s: .space 4\n", cur->name);
    //     }

    // }
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

int labelNum = 0;
char *labVar = "_label_";
char *createLabel() {
    char xStr[100]; 
    sprintf(xStr, "%d", labelNum++);
    char *newStr = malloc(strlen(labVar) + strlen(xStr) + 1);
    strcpy(newStr, labVar);  
    strcat(newStr, xStr); 

    return newStr;
}

//creates a new symbol table entry for a tmp variable and adds it to the table
struct symbol *newLabel() {
    struct symbol *ntmp = malloc(sizeof(struct symbol));
    ntmp->name = strdup(createLabel());
    ntmp->typeInfo=strdup("LABEL");
    ntmp->next = NULL;
    ntmp->args = NULL;
    ntmp->numOfArguments = -1;
    addToTable(ntmp);


    //add the new node to the symbol table
    return ntmp;
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



struct QUADInfo *newQuadInfo(InfoType type, struct symbol *STPointer, int intConst) {

    struct QUADInfo *info = malloc(sizeof(struct QUADInfo));
    info->type = type;
    info->symbolPointer = STPointer;
    info->intConst = intConst;
    return info;

}
int debugging = 1;
void genCode(struct ASTnode *node) {
    //printf("\nTYPE: %d\n", node->type);
    switch(node->type) {
       

        case ASSG: {
            //todo for some reason my code is reading global variables instead of local variables
            struct symbol *LHSVar = tableContains(node->name); //LHS
            struct QUADInfo *LHSInfo = newQuadInfo(IDEN, LHSVar, -1);
            genCode(node->child1); //RHS
            struct QUAD *RHS = node->child1->code;
            

            struct QUADInfo *RHSInfo = newQuadInfo(IDEN, node->child1->place, -1);
            struct QUAD *newInstr = newinstr(ASSGST, RHSInfo, NULL, LHSInfo);
            if (RHS != NULL) {
                struct QUAD *cur;
                for (cur=RHS; cur->next != NULL; cur = cur->next);
                cur->next = newInstr;
                node->code = RHS;
            }
            else {
                node -> code = newInstr;
            }

            
            break;
            
        }

        case INTCONST: {
            struct symbol *temp = newTemp();
            node->place = temp;
            struct QUADInfo *src1Info = newQuadInfo(INTCONT, NULL, node->intConst);
            struct QUADInfo *destInfo = newQuadInfo(IDEN, temp, -1);

            struct QUAD *newInstr = newinstr(ASSGST, src1Info, NULL, destInfo);

            node->code = newInstr;

            break;
        }

        case IDENTIFIER: {
            struct symbol *theSymbol = tableContains(node->name);
            node->place = theSymbol;
            node->code = NULL;
            break;
        }

        case FUNC_CALL: {
            
            //child1 is the opt_expr_list (the arguments)
            struct QUAD *listToBuild = NULL;

            //the node's child1 is opt_expr_list, opt_expr_list child 1 is expr_list

            //if there are arguments, do this
            if (node->child1 != NULL) {
                struct ASTnode *curExpr = node->child1->child1;

                

                while (curExpr != NULL) {
                    genCode(curExpr->child1);
                    

                    curExpr->code = curExpr->child1->code;
                    curExpr->place = curExpr->child1->place;
                    //from the current expression, this is how you get to the intConst: curExpr->code->src1->intConst 
                    struct QUADInfo *paramInstr;

                    if (curExpr->child1->type == INTCONST) {
                        paramInstr = newQuadInfo(INTCONT, curExpr->child1->place, curExpr->code->src1->intConst);
                    }
                    else {
                        paramInstr = newQuadInfo(IDEN, curExpr->child1->place, -1);
                        
                    }
                    //add curExpr->child1->code to stack
                    struct QUAD *newInstr = newinstr(PARAM, NULL, NULL, paramInstr);
                    // newInstr->next = listToBuild;
                    // listToBuild = newInstr;

                    struct QUAD *curLis;
                    if (listToBuild == NULL) {
                        listToBuild = newInstr;
                    }
                    else {
                        for (curLis=listToBuild; curLis->next != NULL; curLis = curLis->next);
                        curLis->next = newInstr;
                    }


                    //in the case that curExpr->child1->code is NULL, this gives a segfault.
                    //code is null when we have an identifier
                    struct QUAD *eq = curExpr->child1->code;
                    if (eq != NULL) {
                        eq->next = listToBuild;
                        listToBuild = eq;
                    }
                    

                    //printf("curExpr Type is: %d\n", curExpr->child1->child2->type);
                    
                    curExpr = curExpr->child2;
                    


                }
                //creating the instr for the function call
                struct symbol *location = tableContains(node->name);
                struct QUADInfo *instrInfo = newQuadInfo(IDEN, location, -1);
                struct QUAD *newInstr = newinstr(CALL, instrInfo, NULL, NULL);
                //adding the instr after the params
                struct QUAD *curAdd;
                for (curAdd = listToBuild; curAdd->next != NULL; curAdd = curAdd->next);
                curAdd->next = newInstr;

                node->code = listToBuild;
                node->place = location;
                
                break;
            }
            else {
                //if there are not arguments, just do the call
                struct symbol *location = tableContains(node->name);
                struct QUADInfo *instrInfo = newQuadInfo(IDEN, location, -1);
                struct QUAD *newInstr = newinstr(CALL, instrInfo, NULL, NULL);
                node->code = newInstr;
                node->place = location;
                break;
            }


        }



        case FUNC_DEF: {
            //printf("\n\n generating code for function %s\n\n", node->name);
            //if the node's child is null, there is nothing in the body
            if (node->child1 != NULL) {
                genCode(node->child1);
                

                struct symbol *funcName = tableContains(node->name);
                struct QUADInfo *funcInfo = newQuadInfo(LAB, funcName, -1);

                struct QUAD *newInstr = newinstr(ENTER, NULL, NULL, funcInfo);

                newInstr->next = node->child1->code;


                node->code = newInstr;
                struct QUAD *leaveInstr = newinstr(RETDEF, NULL, NULL, NULL);
                struct QUAD *cur;
                for (cur = newInstr; cur->next != NULL; cur = cur->next);
                cur->next = leaveInstr;


                break;
            }
            //if there is nothing in the body
            else {
                
                struct symbol *funcName = tableContains(node->name);
                struct QUADInfo *funcInfo = newQuadInfo(LAB, funcName, -1);

                struct QUAD *newInstr = newinstr(ENTER, NULL, NULL, funcInfo);
                struct QUAD *leaveInstr = newinstr(RET, NULL, NULL, NULL);
                newInstr->next = leaveInstr;
                node->code = newInstr;
                break;

            }

            
        }

        case STMT_LIST: {
            //printf("\n\nin stmt_list...\n\n");
            //could be something funky with the cur->child1 or cur->child2 but check later
            struct QUAD *listToBuild = NULL;
            struct ASTnode *cur = node;

            while (cur != NULL) {
                genCode(cur->child1);
                //if the child's code is not null, do this
                if (cur->child1->code != NULL) {
                    cur->code = cur->child1->code;

                    //adding cur expre to the linked list
                    if (listToBuild == NULL) {
                        //printf("\n\nlistToBuild is null\n\n");
                        listToBuild = cur->child1->code;
                    }
                    else {
                        struct QUAD *curAdd;
                        for(curAdd = listToBuild; curAdd->next != NULL; curAdd = curAdd->next);
                        curAdd->next = cur->child1->code;
                        //printf("\n\added to listToBuild with %s\n\n", cur->child1->child1->place->name);
                    }
                }
                //if the code is null, 
                else {
                    
                }


                //if the next child is null, exit

                cur = cur->child2;

            }
            //might have to add place

            node->code = listToBuild;
            break;
        }





        case EXPR_LIST: {

            struct QUAD *listToBuild = NULL;
            struct ASTnode *cur = node->child1;

            while (cur != NULL) {

                //generating code for the cur pointer
                genCode(cur);
                //printf("segfault below\n");
                cur->place = cur->place;
                //printf("segfault above\n");
                cur->code = cur->code;

                
                //adding cur expre to the linked list
                if (listToBuild == NULL) {
                    listToBuild = cur->code;
                }
                else {
                    struct QUAD *curAdd;
                    for(curAdd = listToBuild; curAdd->next != NULL; curAdd = curAdd->next);
                    curAdd->next = cur->code;
                }

                cur = cur->child2;
            }
            //might have to add place
            node->code = listToBuild;
            break;


        }
        //probably unnecessary
        case OPT_EXPR_LIST: {
            genCode(node->child1);
            break;
        }
        case RETURN: {
            //if child1 is not null, generate the code for it

            if (node->child1 != NULL) {
                //generating the code for child 1 which is an arith expr
                genCode(node->child1);
                struct QUADInfo *newQuadInf = NULL;


                if (node->child1->type == INTCONST) {
                    
                    newQuadInf = newQuadInfo(INTCONT, node->child1->place, -1);
                }
                else {
                    newQuadInf = newQuadInfo(IDEN, node->child1->place, -1);
                }
                struct QUAD *newInstr = newinstr(RET, newQuadInf, NULL, NULL);
                node->code = newInstr;
                //printf("\ncreated return\n\n");

            }
            break;
        }
        case IF: {
            struct QUAD *listToBuild = NULL;
            //node->child1 is the bool expr
            //node->child2 is the statement if the condition is true

            //generating code for the bool expr. if->child1 = bool_expr, bool_expr->child1 = arith same with child2
            //node->child1->child1 IS THE BOOL_EXPR LHS
            //node->child1->child2 IS THE BOOL_EXPR RHS
            genCode(node->child1->child1);
            genCode(node->child1->child2);
            //setting the code

            //if the left child is not null, set both children's code (if child2->code is null, add anyway at the end)
            if (node->child1->child1->code != NULL) {
                listToBuild = node->child1->child1->code;
                listToBuild->next = node->child1->child2->code;
            }
            else {
                listToBuild = node->child1->child2->code;
            }

            //getting the type of the bool expression, <, >, ==, etc.
            NodeType boolOperator = node->child1->type;

            //generating code for conditional statement body
            genCode(node->child2);

            //setting boolOperator as the 3rd parameter because i think I can get away with using it to check my if statements
            //here since i never use the INTCONST field of my QUADInfo struct
            struct QUADInfo *LHS = newQuadInfo(IDEN, node->child1->child1->place, boolOperator);
            struct QUADInfo *RHS = newQuadInfo(IDEN, node->child1->child2->place, boolOperator);
            //jump to this label if it is true
            struct symbol *trueLabel = newLabel();
            struct QUADInfo *label = newQuadInfo(LAB, trueLabel, -1);

            //LHS, RHS, label which is the label we will jump to
            //todo might mess up our offsets since we're adding to the symbol table. look at later if cases fail for weird reason
            //newInstr = if LHS boolOperator RHS, goto label
            struct QUAD *trueInstr = newinstr(IFST, LHS, RHS, label);
            //jump to this label if it is false
            struct symbol *falseLabel = newLabel();
            struct QUADInfo *label2 = newQuadInfo(LAB, falseLabel, -1);
            
            struct QUAD *falseInstr = newinstr(GOTO, label2, NULL, NULL);
            struct QUAD *curAdd;
            
            //if neither of the children have code, set the list
            if (listToBuild == NULL)  {
                listToBuild = trueInstr;
            }
            //
            else {
                for (curAdd = listToBuild; curAdd->next != NULL; curAdd = curAdd->next);
                curAdd->next = trueInstr;
            }
            
            

            
            for (curAdd = listToBuild; curAdd->next != NULL; curAdd = curAdd->next);
            curAdd->next = falseInstr;
            

            //HERE WE WILL CREATE OUR LABELS. PREVIOUSLY WE JUMPED TO THEM BUT NOW WE WILL CREATE WHERE THEY JUMP TO

            struct QUAD *trueLabelCreation = newinstr(LABEL, label, NULL, NULL);
            struct QUAD *falseLabelCreation = newinstr(LABEL, label2, NULL, NULL);


            //creating the true label
            for (curAdd = listToBuild; curAdd->next != NULL; curAdd = curAdd->next);
            curAdd->next = trueLabelCreation;
            //adding the code that is inside of the if statement
            
            for (curAdd = listToBuild; curAdd->next != NULL; curAdd = curAdd->next);
            curAdd->next = node->child2->code;
            //printf("%d\n", node->child2->type);


            //adding the false label to the code
            for (curAdd = listToBuild; curAdd->next != NULL; curAdd = curAdd->next);
            curAdd->next = falseLabelCreation;


            node->code = listToBuild;





            break;
        }




    }

}

char *getOpString(NodeType type) {
    switch (type) {
        case EQ: {
            return "eq";
        }
        case NE: {
            return "ne";
        }
        case GT: {
            return "gt";
        }
        case LT: {
            return "lt";
        }
        case GE: {
            return "ge";
        }
        case LE: {
            return "le";
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
    ast->type= OPT_EXPR_LIST;
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
    glob = 0;
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
    //if it is a global variable, print it
    if (glob) {
        //printf("    %s IS A GLOBAL VARIABLE\n", curID);
        struct symbol *s = tableContains(curID);
        s->isGlobal = 1;
        printf("# GLOBAL _%s\n", curID);
        printf(".align 2\n");
        printf(".data\n");
        printf("_%s: .space 4\n", curID);
    }

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


int innit = 1;
void prog() {
    //if the file is empty, exit the program with no error 
    while (1) {
        if (currentTok == EOF) {
            return;
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
                    genCode(tree);
                    //calculating the amount of variables needed when generating the mips code
                    //for storage purposes



                    
                    //gathering the number of formals for the function so later we can skip formals
                    //when generating mips code
                    struct symbol *curFunc = tableContains(funcNameForArgs);
                    int formalCount = curFunc->numOfArguments;

                    struct QUAD *TAC = tree->code;


                    // struct QUAD *cur;
                    // for (cur = TAC; cur != NULL; cur = cur->next) {
                    //     printf("the type of this node is %d\n", cur->type);
                    //     if (cur->type == 11) {
                    //         printf("name = %d\n", cur->dest->intConst);
                    //     }
                    //     // if (cur->dest->type == INTCON) {
                    //     //     printf("%s\n", cur->dest->symbolPointer->name);
                    //     // }
                    // }
                    
                    int numberOfVars = 0;
                    struct symbol *curSymbol;
                    for (curSymbol=theScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {
                        numberOfVars += 1;
                    }

                    int curOffsetLocs = -4;
                    int curOffsetParams = 8;
                    int j = 1;
                    //for every symbol in the symbol list, if it is not a formal, generate an offset of -4, -8, and so on
                    for (curSymbol = theScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {
                        //if we are currently NOT observing a formal, add the offset
                        //curSymbol->isGlobal = 0;
                        if (j > formalCount) {
                            curSymbol->offset = curOffsetLocs;
                            curOffsetLocs -= 4;

                        }
                        else {
                            curSymbol->offset = curOffsetParams;
                            curOffsetParams += 4;
                        }
                        j+= 1;
                    }
                    curOffsetParams -= 4;                    
                    j = 1;
                    //for every symbol in the symbol list, if it is not a formal, generate an offset of -4, -8, and so on
                    for (curSymbol = theScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {
                        //if we are currently NOT observing a formal, add the offset
                        if (j <= formalCount) {
                            curSymbol->offset = curOffsetParams;
                            curOffsetParams -= 4;

                        }

                        j+= 1;
                    }
                    








                    int bytesNeeded = numberOfVars * 4;
                    //params start at 8
                    //locals start at -4
                    if (innit) {
                        printlnCode();
                        innit = 0;
                    }
                    printf("\n");
                    struct QUAD *curTAC;
                    for (curTAC = TAC; curTAC != NULL; curTAC = curTAC->next) {
                        
                        //TODO generate MIPS code
                        switch (curTAC->type) {
                            

                            case ENTER: {
                                printf("# ENTER %s\n", curTAC->dest->symbolPointer->name);
                                printf(".align 2\n");
                                printf(".text\n");
                                printf("_%s:\n", curTAC->dest->symbolPointer->name);
                                printf("    la $sp, -8($sp) # allocate space for old $fp and $ra\n");
                                printf("    sw $fp, 4($sp) # save old $fp\n");
                                printf("    sw $ra, 0($sp) # save return address\n");
                                printf("    la $fp, 0($sp) # set up frame pointer\n");
                                printf("    la $sp, %d($sp) # allocate stack frame\n", curOffsetLocs);
                                printf("\n");
                                break;
                            }
                            case CALL: {

                                printf("    # CALL _%s\n", curTAC->src1->symbolPointer->name);
                                printf("    jal _%s\n", curTAC->src1->symbolPointer->name);
                                printf("    la $sp, 4($sp)\n");
                                printf("\n");
                                break;
                            }
                            case PARAM: {
                                //if it is a global param
                                if (curTAC->dest->symbolPointer->isGlobal) {
                                    printf("    # PARAM %s\n", curTAC->dest->symbolPointer->name);
                                    printf("    lw $t0, _%s\n", curTAC->dest->symbolPointer->name);
                                    printf("    la $sp, -4($sp)\n");
                                    printf("    sw $t0, 0($sp)\n");
                                }
                                //if it is not a global param
                                else {
                                    printf("    # PARAM %s\n", curTAC->dest->symbolPointer->name);
                                    printf("    lw $t0, %d($fp)\n", curTAC->dest->symbolPointer->offset);
                                    printf("    la $sp, -4($sp)\n");
                                    printf("    sw $t0, 0($sp)\n");
                                }

                                printf("\n");
                                break;
                            }
                            case ASSGST: {
                                //if we're looking at an intconst

                                if (curTAC->src1->type == INTCONT) {
                                    printf("    # _%s = %d\n", curTAC->dest->symbolPointer->name, curTAC->src1->intConst);
                                    printf("    li $t0, %d\n", curTAC->src1->intConst);
                                    printf("    sw $t0, %d($fp)\n", curTAC->dest->symbolPointer->offset);
                                }
                                else if (curTAC->src1->type == IDEN) {
                                    //if we are setting a global to a global
                                    if (curTAC->dest->symbolPointer->isGlobal == 1 && curTAC->src1->symbolPointer->isGlobal == 1) {
                                        printf("    # _%s = _%s\n", curTAC->dest->symbolPointer->name, curTAC->src1->symbolPointer->name);
                                        printf("    lw $t0, _%s\n", curTAC->src1->symbolPointer->name);
                                        printf("    sw $t0, _%s\n", curTAC->dest->symbolPointer->name);
                                    }
                                    //if we are setting a global to a local
                                    else if (curTAC->dest->symbolPointer->isGlobal == 1 && curTAC->src1->symbolPointer->isGlobal != 1) {
                                        printf("    # _%s = _%s\n", curTAC->dest->symbolPointer->name, curTAC->src1->symbolPointer->name);
                                        printf("    lw $t0, %d($fp)\n", curTAC->src1->symbolPointer->offset);
                                        printf("    sw $t0, _%s\n", curTAC->dest->symbolPointer->name);
                                    }
                                    //if we are setting a local to a global
                                    else if (curTAC->dest->symbolPointer->isGlobal != 1 && curTAC->src1->symbolPointer->isGlobal == 1) {
                                        printf("    # _%s = _%s\n", curTAC->dest->symbolPointer->name, curTAC->src1->symbolPointer->name);
                                        printf("    lw $t0, _%s\n", curTAC->src1->symbolPointer->name);
                                        printf("    sw $t0, %d($fp)\n", curTAC->dest->symbolPointer->offset);
                                    }
                                    //setting a local to a local
                                    else {
                                        printf("    # _%s = _%s\n", curTAC->dest->symbolPointer->name, curTAC->src1->symbolPointer->name);
                                        printf("    lw $t0, %d($fp)\n", curTAC->src1->symbolPointer->offset);
                                        printf("    sw $t0, %d($fp)\n", curTAC->dest->symbolPointer->offset);
                                    }

                                }
                    
                                printf("\n");
                                break;
                            }
                            case RETDEF: {
                                printf("    #ASSIGNING DEFAULT RETURN VAL 0\n");
                                printf("    li $t0, 0\n");
                                printf("    sw $t0, -4($fp)\n");
                                printf("\n");
                                printf("    # RETURN\n");
                                printf("    lw $v0, -4($fp)\n");
                                printf("    la $sp, 0($fp)\n");
                                printf("    lw $ra, 0($sp)\n");
                                printf("    lw $fp, 4($sp)\n");
                                printf("    la $sp, 8($sp)\n");
                                printf("    jr $ra\n");
                                printf("\n");
                                break;
                            }
                            case RET: {

                                printf("    # RETURN _%s\n", curTAC->src1->symbolPointer->name);
                                printf("    lw $v0, %d($fp)\n", curTAC->src1->symbolPointer->offset);
                                printf("    la $sp, 0($fp)\n");
                                printf("    lw $ra, 0($sp)\n");
                                printf("    lw $fp, 4($sp)\n");
                                printf("    la $sp, 8($sp)\n");
                                printf("    jr $ra\n");
                                printf("\n");
                                break;
                            }
                            case IFST: {
                                char *op = getOpString(curTAC->src1->intConst);
                                //prints, for instance IF p lt tmp0 GOTO label_0
                                printf("    # IF %s %s %s GOTO %s\n", curTAC->src1->symbolPointer->name, op, curTAC->src2->symbolPointer->name, curTAC->dest->symbolPointer->name);
                                printf("    lw $t0, %d($fp)\n", curTAC->src1->symbolPointer->offset);
                                printf("    lw $t1, %d($fp)\n", curTAC->src2->symbolPointer->offset);
                                printf("    b%s $t0, $t1, %s\n", op, curTAC->dest->symbolPointer->name);
                                printf("\n");
                            }
                            case LABEL: {
                                if (curTAC->src1->type == LAB) {
                                    printf("    #LABEL %s\n", curTAC->src1->symbolPointer->name);
                                    printf("    %s:\n", curTAC->src1->symbolPointer->name);
                                    printf("\n");
                                    break;
                                }
                                
                            }
                            case GOTO: {
                                if (curTAC->src1->type == LAB) {
                                    printf("    #GOTO %s\n", curTAC->src1->symbolPointer->name);
                                    printf("    j %s\n", curTAC->src1->symbolPointer->name);
                                    printf("\n");
                                    break;
                                }
                                
                            }

                        }
                        

                            
                        
                    }

                }




                //pop the scope from the scope list
                theScope = theScope->nextScope;


                
            }
            //if the current token is a SEMI or COMMA
            else if (currentTok == SEMI || currentTok == COMMA) {

                //checks if curID (the current ID) is in the current scope or not
                
                contains(curID, "variable");

                struct symbol *con = tableContains(curID);
                con->isGlobal = 1;


                //the call to declare variables
                glob = 1;
                printf("# GLOBAL _%s\n", curID);
                printf(".align 2\n");
                printf(".data\n");
                printf("_%s: .space 4\n", curID);
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



int parse() {

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
    //what should go before globals

    currentTok = get_token();
    prog();
    if (gen_code_flag) {
        printf("main:\n    j _main\n");
    }

        
    //if it reaches here then an error was never reached
    return 0;
}


