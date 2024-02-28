
/*
file: scanner.c
author: cole perry
description: this file creates a scanner for G0. It allows
pattern matching for very basic C tokens such as +, -, (), {},
among others. Once a pattern is found, it returns the corresponding
value in the enum typedef which gives us the corresponding name 
of the token, along with the lexeme which i set to whatever the matched
lexeme is.
class: csc453
*/
 
#include <stdio.h>
#include "scanner.h"
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

char *lexeme = NULL;
int lineNum = 1;
/*
get_token()

purpose: this function searches for tokens from input taken from
standard in, and returns said tokens.

parameters: none

return: some token location in the enum typedef, also setting lexeme
*/
int get_token() {
    int currentChar = getchar();

    //skipping whitespace
    if (isspace(currentChar)) {
        if (currentChar == '\n') {
            lineNum += 1;
        }
        while (isspace(currentChar)) {
            
            currentChar = getchar();
            if (currentChar == '\n') {
                lineNum += 1;
            }

        }
    }
    

    if (currentChar == '/') {
        int nextChar = getchar();
        if (nextChar == '*') {
            while ((currentChar = getchar())) {
                if (currentChar == EOF) {
                    return EOF;
                }

                if (currentChar == '\n') {
                    lineNum += 1;
                }

                if (currentChar == '*') {
                    int nextNextChar = getchar();
                    if (nextNextChar == '/') {
                        currentChar = getchar();
                        break;
                    } 
                    else if (nextNextChar == '\n') {
                        lineNum += 1;  
                    } 
                    else {
                        ungetc(nextNextChar, stdin);
                    }
                }
            }
        } 
        else {
            ungetc(nextChar, stdin);
            lexeme = strdup("/");
            return opDIV;
        }
    } 
    //skipping whitespace
    if (isspace(currentChar)) {
        if (currentChar == '\n') {
            lineNum += 1;
        }
        while (isspace(currentChar)) {
            
            currentChar = getchar();
            if (currentChar == '\n') {
                lineNum += 1;
            }

        }
    }


    


    //checking symbols, all of this is the same except for two chatacter
    //symbols, in which we check the next character as well
    if (currentChar == '(') {
        lexeme = strdup("(");
        return LPAREN;
        //return '(';
    }
    else if (currentChar == ')') {
        lexeme = strdup(")");
        return RPAREN;
        //return ')';
    }
    else if (currentChar == '{') {
        lexeme = strdup("{");
        return LBRACE;
        //return '{';
    }
    else if (currentChar == '}') {
        lexeme = strdup("}");
        return RBRACE;
        //return '}';
    }
    else if (currentChar == ',') {
        lexeme = strdup(",");
        return COMMA;
        //return ',';
    }
    else if (currentChar == ';') {
        lexeme = strdup(";");
        return SEMI;
        //return ';';
    }
    else if (currentChar == '+') {
        lexeme = strdup("+");
        return opADD;
        //return '+';
    }
    else if (currentChar == '-') {
        lexeme = strdup("-");
        return opSUB;
        //return '-';
    }
    else if (currentChar == '/') {
        lexeme = strdup("/");
        return opDIV;
        //return '/';
    }

    else if (currentChar == '*') {
        lexeme = strdup("*");
        return opMUL;
        //return '*';
    }
    else if (currentChar == '!') {
        int nextChar = getchar();
        if (nextChar == '=') {
            //return '!=';
            lexeme = strdup("!=");
            return opNE;
        }
        ungetc(nextChar, stdin);
        lexeme = strdup("!");
        return opNOT;
        //return '!';
    }
    else if (currentChar == '=') {
        int nextChar = getchar();
        if (nextChar == '=') {
            //return '==';
            lexeme = strdup("==");
            return opEQ;
        }
        ungetc(nextChar, stdin);
        //return '=';
        lexeme = strdup("=");
        return opASSG;
    }
    else if (currentChar == '<') {
        int nextChar = getchar();
        if (nextChar == '=') {
            lexeme = strdup("<=");
            return opLE;
        }
        ungetc(nextChar, stdin);
        lexeme = strdup("<");
        return opLT;
    }
    else if (currentChar == '>') {
        int nextChar = getchar();
        if (nextChar == '=') {
            lexeme = strdup(">=");
            return opGE;
        }
        ungetc(nextChar, stdin);
        lexeme = strdup(">");
        return opGT;
    }
    int nextChar = getchar();
    if (currentChar == '&' && nextChar == '&') {
        lexeme = strdup("&&");
        return opAND;
    }
    else {
        ungetc(nextChar, stdin);
    }
    nextChar = getchar();

    if (currentChar == '|' && nextChar == '|') {
        lexeme = strdup("||");
        return opOR;
    }
    else {
        ungetc(nextChar, stdin);
    }
    
    //if the next character could be the start of an int constant,
    //build the int
    if (isdigit(currentChar)) {

        //building intConst and checking if it is the full integer
        //since we don't know w hat size it is, we rebuild and
        //reallocate space for the string whenever needed
        char* intConst = malloc(10 * sizeof(char));

        intConst[0] = currentChar;  
        int i = 1;  
        size_t theSize = 10;

        while ((currentChar = getchar()) != EOF) {
            //if the current character is not a number, unget it and return the lexeme
            if (!(isdigit(currentChar))) {
                ungetc(currentChar, stdin);
                intConst[i] = '\0'; 
                lexeme = strdup(intConst);
                free(intConst);
                return INTCON;
            }

            //if we reach the end of the character sequence and there are more characters, resize
            //the dynamically allocated memory to be bigger, just times 2 for efficiency's sake
            if (i == theSize - 1) {
                theSize *= 2;
                intConst = realloc(intConst, theSize * sizeof(char));
            }

            //otherwise, add the current character to the string
            intConst[i] = currentChar;
            i++;
        }
    

        free(intConst);
    }
    else if (isalpha(currentChar)) {

        //building intConst and checking if it is the full integer
        //since we don't know w hat size it is, we rebuild and
        //reallocate space for the string whenever needed
        char* id = malloc(10 * sizeof(char));

        id[0] = currentChar;  
        int i = 1;  
        size_t theSize = 10;

        while ((currentChar = getchar()) != EOF) {
            //if the character is not alphanumeric, terminate loop
            if (!isalpha(currentChar) && !isdigit(currentChar) && currentChar != '_') {
                ungetc(currentChar, stdin);
                id[i] = '\0'; 
                lexeme = strdup(id);

                if (strcmp(id, "if") == 0) {
                    return kwIF;
                }
                else if (strcmp(id, "else") == 0) {
                    return kwELSE;
                }
                else if (strcmp(id, "while") == 0) {
                    return kwWHILE;
                }
                else if (strcmp(id, "return") == 0) {
                    return kwRETURN;
                }
                else if (strcmp(id, "int") == 0) {
                    return kwINT;
                }

                free(id);
                return ID;

            }
            //if we reach the end of the character sequence and there are more characters, resize
            //the dynamically allocated memory to be bigger, just times 2 for efficiency's sake
            if (i == theSize - 1) {
                theSize *= 2;
                id = realloc(id, theSize * sizeof(char));
            }

            //otherwise, add the current character to the string
            id[i] = currentChar;
            i++;

        }

        free(id);
    }
    else if (currentChar == EOF) {
        return EOF;
    }
    else {
        char cur[2];
        cur[0] = currentChar;
        cur[1] = '\0';
        lexeme = strdup(cur);
        return UNDEF;
    }

    return EOF;
}

// shrimp posanda mild
// veggie tikka masala 
// chicken tikka masala hot

// chicken biryani hot
// chicken tikka masala hot

// 4 regular naan