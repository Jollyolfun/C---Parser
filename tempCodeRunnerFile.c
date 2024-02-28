    while ((currentChar = getchar())) {
        isLine = 1;
        if (currentChar == '\n') {
            lineNum += 1;
            continue;
        }
        else if (currentChar == EOF) {
            return EOF;
        }

        if (isspace(currentChar)) {
            continue;
        } 

        }   
        else {
            break;
        }
    }