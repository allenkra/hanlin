#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void toUpper(char* s) {
    int i=0;
    while (s[i] != '\0') {
        if ('a' <= s[i] && s[i] <= 'z')
            s[i] -= 32;
        if (s[i] == '\n'){ 
            s[i] = '\0'; // delete linebreak
            break;
        }
        i++;
    }
}

int dateCheck(char* date) {
    char *s = date;
    int len = strlen(date);
    if (len != 10)
        return 0;
    int i;
    // check YYYY
    for (i = 0; i <= 3; i++) {
        if ('0' > s[i] || s[i] >'9')
            return 0;
    }
    if (s[4] != '-')
        return 0;
    // check MM
    for (i = 5; i <= 6; i++) {
        if ('0' > s[i] || s[i] >'9')
            return 0;
    }
    if (s[7] != '-')
        return 0;
    // check DD
    for (i = 8; i <= 9; i++) {
        if ('0' > s[i] || s[i] >'9')
            return 0;
    }
    return 1;

}

void replaceStr(char* source, const char* substr, const char* replacement) {
    char buffer[512] = {0};  // buffer maxsize
    char* insert_point = &buffer[0];
    const char* tmp = source;
    int substr_len = strlen(substr);
    int replacement_len = strlen(replacement);

    while (1) {
        const char* p = strstr(tmp, substr);

        if (p == NULL) {
            strcpy(insert_point, tmp);
            break;
        }

        // copy the section from tmp to p into buffer
        memcpy(insert_point, tmp, p - tmp);
        insert_point += p - tmp;

        // copy replacement into buffer
        memcpy(insert_point, replacement, replacement_len);
        insert_point += replacement_len;

        tmp = p + substr_len;// update the pointer
    }

    // Write back to source
    strcpy(source, buffer);
}

void process_line(FILE* output, char* line) {
    replaceStr(line, "/fB", "\033[1m");
    replaceStr(line, "/fI", "\033[3m");
    replaceStr(line, "/fU", "\033[4m");
    replaceStr(line, "/fP", "\033[0m");
    replaceStr(line, "//", "/");
    fprintf(output, "       %s", line);
}


int main(int argc, char *argv[]) {
    FILE *input, *output;
    char line[256];
    char title[100];
    int section;
    char date[20];
    char output_filename[120];

    // check arguments
    if (argc != 2) {
        printf("Improper number of arguments\nUsage: ./wgroff <file>\n");
        exit(0);
    }

    // file path
    input = fopen(argv[1], "r");
    if (!input) {
        printf("File doesn't exist\n");
        exit(0);
    }

    // handle wrong imput
    if (fgets(line, sizeof(line), input) == NULL || sscanf(line, ".TH %s %d %s", title, &section, date) != 3) {
        printf("Improper formatting on line 1\n");
        fclose(input);
        exit(0);
    }

    // section must be numbers
    if (section < 0 || section > 9) {
        printf("Improper formatting on line 1\n");
        fclose(input);
        exit(0);
    }

    // check date
    if (!dateCheck(date)) {
        printf("Improper formatting on line 1\n");
        fclose(input);
        exit(0);
    }



    sprintf(output_filename, "%s.%d", title, section);
    output = fopen(output_filename, "w");

    // handle format of title line
    char format_title[512]; 
    int total_len = strlen(title) * 2 + 6; // length without spaces
    int spaces = 80 - total_len;  // The number of spaces needed

    sprintf(format_title, "%s(%d)%*s%s(%d)\n", title, section, spaces, "", title, section);
    fprintf(output, "%s", format_title);

    // handle context
    while (fgets(line, sizeof(line), input)) {
        // Skip comments
        if (line[0] == '#') continue;

        // handle section headers
        if (strncmp(line, ".SH", 3) == 0) {
            toUpper(line);
            fprintf(output, "\n\033[1m%s\033[0m\n", line + 4);

        } else {
            process_line(output, line);
        }
    }

    int padding = (80 - strlen(date)) / 2;
    fprintf(output, "%*s%s%*s\n", padding, "", date, padding, "");

    fclose(input);
    fclose(output);
    return 0;
}

/*
~cs537-1/tests/P1/test-wman.csh

~cs537-1/tests/P1/test-wapropos.csh

~cs537-1/tests/P1/test-wgroff.csh

gcc -o wgroff wgroff.c -Wall -Werror
gcc -g wgroff.c -o wgroff -Wall -Werror
*/