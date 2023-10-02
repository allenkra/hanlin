#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BASE_DIR "./man_pages/man"
void charToString(char *str, char ch) {
    int length = strlen(str);  
    str[length] = ch;          
    str[length + 1] = '\0';
}
int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        printf("What manual page do you want?\nFor example, try 'wman wman'\n");//wrong premeter numbers
        return 0;
    }

    char filepath[256];
    FILE *fp = NULL;

    // 1 premeter
    if (argc==2) {
        for (int i = 1; i <= 9; i++) {
            //path make
            strcpy(filepath,BASE_DIR);
            charToString(filepath,(char)(i+'0'));
            charToString(filepath,'/');
            strcat(filepath,argv[1]);
            charToString(filepath,'.');
            charToString(filepath,(char)(i+'0'));
            fp = fopen(filepath, "r");
            if (fp != NULL) {
                break;
            }
        }
    } else if (argc == 3) {// 2 premeters
        int section = atoi(argv[1]);
        if (section < 1 || section > 9) {
            printf("invalid section\n");
            return 1;
        }
        //path make
        strcpy(filepath,BASE_DIR);
        charToString(filepath,(char)(section+'0'));
        charToString(filepath,'/');
        strcat(filepath,argv[2]);
        charToString(filepath,'.');
        charToString(filepath,(char)(section+'0'));

        fp = fopen(filepath, "r");
    }

    // excption: path error
    if (fp == NULL) {
        if (argc == 2) {
            printf("No manual entry for %s\n", argv[1]);
        } else {
            printf("No manual entry for %s in section %s\n", argv[2], argv[1]);
        }
        return 0;
    }

    // correct operation
    char buffer[1000];
    while (fgets(buffer, 1000, fp) != NULL) {
        printf("%s", buffer);
    }
    fclose(fp);
    return 0;
}
/*
~cs537-1/tests/P1/test-wman.csh

~cs537-1/tests/P1/test-wapropos.csh

~cs537-1/tests/P1/test-wgroff.csh

gcc -o wman wman.c -Wall -Werror
*/