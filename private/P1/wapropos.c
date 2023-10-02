#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

int searchFileForKeyword(const char *filename, const char *keyword) {
    FILE *fp = fopen(filename, "r");
    if (!fp) return 0;
    
    int flag_key = 0; // flag = 1 when keyword found
    int in_description = 0; // flag = 1 when inside DESCRIPTION section
    char line[1024];
    char *name_line = NULL;
    
    while (fgets(line, sizeof(line), fp)) {
        if (strncmp(line, "\033[1mDESCRIPTION", 12) == 0) {
            in_description = 1;
            continue;
        }

        if (in_description) {
            if (strstr(line, keyword)) {
                char temp_filename[256];
                strncpy(temp_filename, filename, sizeof(temp_filename));
                char *page = strtok(temp_filename, ".");
                printf("%s (%s) -%s", strrchr(page, '/') + 1, strrchr(filename, '.') + 1, strchr(name_line, '-') + 1); // filename (section) - NAME
                flag_key = 1; // keyword found, return after printf
                break;
            }

            // ending the DESCRIPTION section if find"\033[1m]", which hints beginning
            if (strncmp(line, "\033[1m", 4) == 0) {
                in_description = 0;
            }
        }

        if (strncmp(line, "\033[1mNAME", 5) == 0) {
            if (fgets(line, sizeof(line), fp)) {
                name_line = strdup(line); // get the name line
            }
        }

        if (flag_key) break;
    }

    if (name_line) free(name_line);
    fclose(fp);
    return flag_key;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("wapropos what?\n");
        return 0;
    }

    const char *keyword = argv[1];
    int found = 0;

    for (int i = 1; i <= 9; i++) {
        char path[64];
        sprintf(path, "./man_pages/man%d", i);

        DIR *dir = opendir(path); //get info of the directory
        if (dir) {
            struct dirent *entry;
            while ((entry = readdir(dir))) {
                if (entry->d_type == DT_REG) {
                    char filepath[500];
                    snprintf(filepath, sizeof(filepath), "%s/%s", path, entry->d_name);
                    found = searchFileForKeyword(filepath, keyword);
                }
            }
            closedir(dir);
        }
    }

    if (!found) {
        printf("nothing appropriate\n");
    }

    return 0;
}




/*
~cs537-1/tests/P1/test-wman.csh

~cs537-1/tests/P1/test-wapropos.csh

~cs537-1/tests/P1/test-wgroff.csh

gcc -o wapropos wapropos.c -Wall -Werror
gcc -g wapropos.c -o wapropos -Wall -Werror
*/