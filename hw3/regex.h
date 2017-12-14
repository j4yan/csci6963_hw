#ifndef REGEX_H
#define REGEX_H

#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifndef MAX_LINE_LEN
#define MAX_LINE_LEN 257
#endif

int isRepetitionSymbol(char c);
int isMetaChar(const char *c);
int matchChar(const char *regex, const char *str, int *r_next);
int runRegexMatch(const char *regex, const char *text, int first_occurence);
int regex_match(const char *filename, const char *regex, char ***matches);

#endif
