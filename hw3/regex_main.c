#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "regex.h"

#ifndef MAX_LINE_LEN
#define MAX_LINE_LEN 257
#endif



// #ifndef USE_SUBMITTY_MAIN
int main(int argc, char **argv) {
    // char* text = "<body";
    // char* regex = "<\\w+";
    // char* text = "         <!-- <div class=\"nav-column nav-image\">\n";
    // printf("regex = %s\n", regex);
    // printf("text = %s\n", text);
    // int matched = runRegexMatch(regex, text, strlen(text));
    // printf("%i\n", matched);


    if (argc != 3) {
        fprintf(stderr, "%s", "Error: <Usage: 2 input files>\n");
        exit(EXIT_FAILURE);
    }

    char *fname_regex        = argv[1];
    char *fname_text         = argv[2];
    char regex[MAX_LINE_LEN] = {'\0'};
    FILE *fp_regex           = fopen(fname_regex, "r");
    if (fp_regex == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }
    fgets(regex, MAX_LINE_LEN, fp_regex);

    char ***matched_lines = (char ***)malloc(sizeof(char **));
    int n_matched_lines   = regex_match(fname_text, regex, matched_lines);
    printf("number of matched liens = %i\n", n_matched_lines);

    for (int i = 0; i < n_matched_lines; ++i) {
        printf("%s", (*matched_lines)[i]);
    }
    for (int i = 0; i < n_matched_lines; ++i) {
        free((*matched_lines)[i]);
    }
    free(*matched_lines);
    free(matched_lines);

    return 0;
}
// #endif
