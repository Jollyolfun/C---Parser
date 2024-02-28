#include <stdio.h>
#include "scanner.h"
#include <stdlib.h>
#include <string.h>
#include "ast.h"


struct ASTNode {

    NodeType type;
    char *name;

    struct ASTNode *child1;
    struct ASTNode *child2;
    struct ASTNode *child3;


};

struct argNode {
    char *argName;
    struct argNode *next; 
};

struct symbol {

    char *name;
    int numOfArguments;
    char *typeInfo;
    struct symbol *next;
    struct argNode *args;



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
int curArgs;
int currentTok;
int isError = 0;
struct scope *theScope;
char *curIdentifier;

//this function takes every character of lexeme and pushes it back to stdin.
void type();
void id_list();
void opt_stmt_list();
void assg_stmt();
//expected identifier, got undef
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

void containsFunc(char *curID) {
    if (chk_decl_flag) {
        struct scope *curScope;
        int i = 0;
        for (curScope = theScope; curScope != NULL; curScope = curScope->nextScope) {
            struct symbol *curSymbol = NULL;
            for (curSymbol = curScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {
                //if the symbol we're looking at is not a function and is in the local scope
                if (i == 0 && strcmp(curSymbol->name, curID) == 0 && strcmp(curSymbol->typeInfo, "function") != 0) {
                    
                    fprintf(stderr, "ERROR LINE %d UNDECLARED FUNCTION %s\n", lineNum, curID);
                    exit(1);  
                }
                if (strcmp(curSymbol->name, curID) == 0) {
                    return; 
                }
            }
            i += 1;
        }
        fprintf(stderr, "ERROR LINE %d UNDECLARED FUNCTION %s\n", lineNum, curID);
        exit(1);
    }
}

void containsParameter(char *curID) {
    if (chk_decl_flag) {
        struct scope *curScope;
        int i = 0;
        for (curScope = theScope; curScope != NULL; curScope = curScope->nextScope) {
            struct symbol *curSymbol = NULL;
            for (curSymbol = curScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {
                //if the symbol we're looking at is not a function and is in the local scope
                // if (i == 0 && strcmp(curSymbol->name, curID) == 0 && strcmp(curSymbol->typeInfo, "function") != 0) {
                //     fprintf(stderr, "ERROR LINE %d UNDECLARED FUNCTION %s\n", lineNum, curID);
                //     exit(1);  
                // }
                if (strcmp(curSymbol->name, curID) == 0 && strcmp(curSymbol->typeInfo, "variable" ) == 0) {
                    return; 
                }
            }
            i += 1;
        }
        fprintf(stderr, "ERROR LINE %d PARAMETER NOT PREVIOUSLY DEFINED %s\n", lineNum, curID);
        exit(1);
    }
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

void arith_exp() {
    if (currentTok == ID) {
        //if there is use of an ID, check if it has been previously defined
        if (chk_decl_flag) {
            containsParameter(lexeme);
        }
        
        match(ID);
    }
    else if (currentTok == INTCON) {
        match(INTCON);
    }
    else{
        fprintf(stderr, "ERROR LINE %d IN arith_expr\n", lineNum);
        exit(1); 
    }

}

void arith_exp_assg() {
    if (currentTok == ID) {
        //if there is use of an ID, check if it has been previously defined
        if (chk_decl_flag) {
            containsParameter(lexeme);
        }
        
        match(ID);
    }
    else if (currentTok == INTCON) {
        match(INTCON);
    }
    else{
        fprintf(stderr, "ERROR LINE %d IN arith_expr\n", lineNum);
        exit(1); 
    }

}




void relop() {
    if (currentTok == opEQ) {
        match(opEQ);
        return;
    }    
    else if (currentTok == opNE) {
        match(opNE);
        return;
    }
    else if (currentTok == opLE){
        match(opLE);
        return;
    }    
    else if (currentTok == opLT) {
        match(opLT);
        return;
    }    
    else if (currentTok == opGE) {
        match(opGE);
        return;
    }
    else if (currentTok == opGT) {
        match(opGT);
        return;
    }
    else {
        fprintf(stderr, "ERROR LINE %d IN relop\n", lineNum);
        exit(1);
    }
}

void bool_exp() {

    arith_exp();
    relop();
    arith_exp();


}

void expr_list() {
    arith_exp();
    curArgs += 1;
    if (currentTok == COMMA) {
        match(COMMA);
        expr_list();
        return;
    }

}

void opt_expr_list() {
    if (currentTok == RPAREN) {
        return;
    }
    expr_list();
}
void fn_call() {
    if (chk_decl_flag) {
        containsFunc(curIdentifier);
    }
    
    match(LPAREN);
    opt_expr_list();
    match(RPAREN);
}

void stmt();
void while_stmt() {

    match(kwWHILE);
    match(LPAREN);
    bool_exp();
    match(RPAREN);
    stmt();


}

void if_stmt() {
    match(kwIF);
    match(LPAREN);
    bool_exp();
    match(RPAREN);
    stmt();
    if (currentTok == kwELSE) {
        match(kwELSE);
        stmt();
    }
}

void return_stmt() {
    match(kwRETURN);
    if (currentTok == ID || currentTok == INTCON) {
        //if the current token is an ID, set the curIdentifier to the lexeme ID
        if (currentTok == ID) {
            curIdentifier = strdup(lexeme);
        }
        arith_exp();
    }
    match(SEMI);
}

void assg_stmt() {
    arith_exp();
    match(SEMI);
}

void stmt() {

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
            assg_stmt();
            return;
        }
        //function call
        curArgs = 0;
        fn_call();

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
        return;
    }
    else if (currentTok == kwWHILE) {
        while_stmt();
        return;
    }
    else if (currentTok == kwIF) {
        if_stmt();
    }
    else if (currentTok == SEMI) {
        match(SEMI);
        return;
    }
    else if (currentTok == kwRETURN) {
        return_stmt();
    }
    else if (currentTok == LBRACE) {
        match(LBRACE);
        opt_stmt_list();
        match(RBRACE);
    }
    else {
        fprintf(stderr, "ERROR LINE %d IN stmt\n", lineNum);
        exit(1);
    }

}

void opt_stmt_list() {
    //currentTok == RBRACE means that we have reached the end of the function content,
    //meaning we can use it as our exit condition
    if (currentTok == RBRACE) {
        return;
    }
    stmt();
    opt_stmt_list();


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
void func_defn() {

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
    opt_stmt_list();
    match(RBRACE);
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
                func_defn();
                




                //pop the scope from the scope list
                theScope = theScope->nextScope;

                //printing arguments
                // struct symbol *curSymbol;

                // for (curSymbol=theScope->symbolList; curSymbol != NULL; curSymbol = curSymbol->next) {
                //     if (strcmp(curSymbol->name, curFuncName) == 0) {
                        
                //         struct argNode *curArg;
                //         printf("CUR FUNCTION IS %s\n", curSymbol->name);
                //         for(curArg = curSymbol->args; curArg != NULL; curArg = curArg->next) {
                //             printf("%s\n", curArg->argName);
                //         }
                //         printf("-----done printing-----\n");

                //     }
                // }
                
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

int parse() {
    //going until the end of a file
    //here is where tok will represent the current token, and we
    //can see if it matches patterns
    curArgs = 0;
    //global scope 
    theScope = malloc(sizeof(struct scope));
    theScope->symbolList = NULL;
    theScope->nextScope = NULL;


    currentTok = get_token();
    prog();
        
    //if it reaches here then an error was never reached
    return 0;
}


