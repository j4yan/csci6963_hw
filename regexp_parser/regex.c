#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 257

/*
 * forward declaration
 */
int isRepetitionSymbol(char c);
int isMetaChar(const char *c);
int matchChar(const char *regex, const char *str, int *r_next);
int runRegexMatch(const char *regex, const char *text, int first_occurence);
int regex_match(const char *filename, const char *regex, char ***matches);

int main(int argc, char **argv) {
    // char* text = "<body";
    // char* regex = "<\\w+";
    // printf("regex = %s\n", regex);
    // printf("text = %s\n", text);
    // int matched = runRegexMatch(regex, text, strlen(text));
    // printf("%i\n", matched);

    // if (argc != 3) {
    //     fprintf(stderr, "%s", "Error: <Usage: 2 input files>\n");
    //     exit(EXIT_FAILURE);
    // }
    // char* fname_regex = argv[1];
    // char* fname_text = argv[2];
    // FILE* fp_regex = fopen(fname_regex, "r");
    // FILE* fp_text = fopen(fname_text, "r");
    // char text[MAX_LINE_LEN] = {'\0'};
    // char regex[MAX_LINE_LEN] = {'\0'};
    // int matched;
    //
    // if (fp_regex == NULL || fp_text == NULL) {
    //     fprintf(stderr, "%s", "Error: <could not open files>\n");
    //     exit(EXIT_FAILURE);
    // }
    // fgets(regex, MAX_LINE_LEN, fp_regex);
    // fclose(fp_regex);
    // // printf("regex = %s\n", regex);
    //
    // while (fgets(text, MAX_LINE_LEN, fp_text) != NULL) {
    //     // printf("%s", text);
    //     matched = runRegexMatch(regex, text, strlen(text));
    //
    //     if (matched) {
    //         printf("%s", text);
    //         fflush(stdout);
    //     }
    // }
    // fclose(fp_text);
    //

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
/*!
 * \brief  Test if the leading character in \var regex matches
 *         the leading term in \var str.
 *
 * \param regex regular expression
 * \param str the string/character to be tested.
 * \param r_next next head in \var regex
 *
 * \return 1if the leading characters match; 0 if not match.
 */
int matchChar(const char *regex, const char *str, int *r_next) {
    int i, pos = -1;
    int next_offset, matched;
    const int regex_len = strlen(regex);

    if (isRepetitionSymbol(regex[0])) {
        printf("%s\n", "<regex should not start with a repetition symbol>");
        fflush(stdout);
        exit(EXIT_FAILURE);
    }

    *r_next = 1;

    if (regex[0] == '.') {
        return 1;
    }
    if (!strncmp(regex, "\\d", 2)) {
        ++(*r_next);
        if (isdigit(str[0])) {
            return 1;
        }
    }
    if (!strncmp(regex, "\\w", 2)) {
        ++(*r_next);
        if (isalpha(str[0])) {
            return 1;
        }
    }
    if (!strncmp(regex, "\\s", 2)) {
        ++(*r_next);
        if (isspace(str[0])) {
            return 1;
        }
    }

    if (regex[0] == '[') {
        for (i = 1; i < regex_len; ++i) {
            if (regex[i] == ']') {
                pos = i;
                break;
            }
        }
        *r_next += pos;
        if (pos == 1) {
            fprintf(stderr, "%s", "Error: <no char in []>\n");
            exit(EXIT_FAILURE);
        }
        if (pos < 0) {
            fprintf(stderr, "%s", "Error: <[] not closed>\n");
            exit(EXIT_FAILURE);
        }
        i = 1;
        while (i < pos) {
            matched = matchChar(regex + i, str, &next_offset);
            if (matched) {
                return 1;
            }
            i += next_offset;
        }
    }

    if (regex[0] == str[0]) {
        return 1;
    }

    return 0;
}

/*!
 * \brief The implementation of regular expression match.
 *
 * Given a regular expression \var regex and and a text \var text,
 * search the regex match in text. The first character should occur
 * in the first \var first_occurence characters in \var text.
 *
 * \param regex regular expression
 * \param text string to be tested
 * \param first_occurence the first match should occur at the first \var
 * first_occurence characters in \var text
 *
 * \return 1 if matches; 0 if does not match
 */
int runRegexMatch(const char *regex, const char *text, int first_occurence) {
    // assert(first_occurence > 0);
    static int n_calls = 0;
    // printf("n_calls = %i\n", n_calls);
    ++n_calls;
    int r_head = 0, j = 0;
    // pointer to the next non-repetition_symbol character in regex
    int r_next;
    int next_offset;
    int regex_len = strlen(regex);
    if (regex[regex_len-1] == '\0' || regex[regex_len-1] == '\n') {
        regex_len -= 1;
    }

    // const int regex_len = strlen(regex) - 1;
    const int text_len = strlen(text);
    // const int text_len = strlen(text) - 1;
    // the position of leading character in `text`
    int s_head                     = 0;
    int matched                    = 0;
    int first_occurence_repetition = 0;

    // requirement of first occurence
    for (j = 0; j < first_occurence; ++j) {
        matched = matchChar(regex, text + j, &next_offset);
        if (matched) {
            break;
        } else {
            ++s_head;
        }
    }
    if (s_head == first_occurence) {
        return 0;
    }

    while (r_head < regex_len && s_head < text_len) {
        // first test if current regex matches current text
        matched = matchChar(regex + r_head, text + s_head, &next_offset);

        // shift r_next and s_head
        r_next = r_head + next_offset;
        ++s_head;

        // in case of repetition symbol
        if (regex[r_next] == '?') {
            // If its '?' then it should alway match.
            // In addition the pointer to next character should be shifted.
            if (!matched) {
                matched = 1;
                --s_head;
            }
            ++r_next;
        } else if (regex[r_next] == '+') {
            // here we let recursion to do the rest job. Note since the next
            // search depends on the current one (continuity), ie, we should
            // restrict the occurence of first character match. This occurence
            // is defined by \v first_occurence_repetition.
            first_occurence_repetition = 1;
            ++r_next;
            if (r_next == regex_len) {
                return matched;
            }
            if (matched) {
                first_occurence_repetition = 1;
                for (j = s_head; j < text_len; ++j) {
                    matched = matchChar(regex + r_head, text + j, &next_offset);
                    if (matched) {
                        ++first_occurence_repetition;
                    } else {
                        break;
                    }
                }

                matched = runRegexMatch(regex + r_next,
                                        text + s_head,
                                        first_occurence_repetition);

                if (matched) {
                    return 1;
                }
            }
        } else if (regex[r_next] == '*') {
            ++r_next;
            if (r_next == regex_len) {
                return 1;
            }
            if (!matched) {
                matched = 1;
                --s_head;
            } else {
                first_occurence_repetition = 1;
                for (j = s_head; j < text_len; ++j) {
                    matched = matchChar(regex + r_head, text + j, &next_offset);
                    if (matched) {
                        ++first_occurence_repetition;
                    } else {
                        break;
                    }
                }
                matched = runRegexMatch(regex + r_next,
                                        text + s_head,
                                        first_occurence_repetition);
                if (matched) {
                    return 1;
                }
            }
        }

        // if it doesn't match, we restart the match for the rest of text.
        if (matched == 0) {
            matched = runRegexMatch(regex, text + 1, text_len - 1);
            if (matched) {
                return 1;
            } else {
                return 0;
            }
        }

        // ready to do next search
        r_head = r_next;
    }

    // There're 2 possible reasons we got here:
    //  1) we've looped over \v regex;
    //  2) we've looped over \v text;

    // if we've looped over \v regex
    if (r_head == regex_len - 1) {
        return 1;
    }

    // if we've looped over \v text, there's still a chance
    // the whole regex matches. For example, the rest part
    // of \v regex is "d*.*\d?"
    while (r_head < regex_len) {
        if ('.' == regex[r_head] &&
            ('?' == regex[r_head + 1] || '*' == regex[r_head + 1])) {
            r_head += 2;
        } else if (isMetaChar(regex + r_head) &&
                   ('?' == regex[r_head + 2] || '*' == regex[r_head + 2])) {
            r_head += 3;
        } else if ('?' == regex[r_head + 1] || '*' == regex[r_head + 1]) {
            r_head += 2;
        } else {
            return 0;
        }
    }
    return matched;
}

/*!
 * \brief To see if the input character is one of the metacharacters.
 *
 * \param c input character
 *
 * \return 1 if yes, 0 if not.
 */
int isMetaChar(const char *c) {
    if (c[0] == '.')
        return 1;
    if (!strncmp(c, "\\d", 2))
        return 1;
    if (!strncmp(c, "\\w", 2))
        return 1;
    if (!strncmp(c, "\\s", 2))
        return 1;

    return 0;
}

/**
 * \brief Test if a character is a repetition sysmbol.
 *
 * \param c character to be tested
 *
 * \return 1 if yes, 0 if not.
 */
int isRepetitionSymbol(char c) {
    switch (c) {
    case '?':
        return 1;
    case '+':
        return 1;
    case '*':
        return 1;
    default:
        return 0;
    }
}
/**
 * \brief Applies the given regex to each line of the file
 * specified by filename; all matching lines are stored in a
 * dynamically allocated array called matches.
 *
 * Note that it is up to the calling process to free the
 * allocated memory here
 *
 * \param filename text file name
 * \param regex regular expresion
 * \matches stores the lines that match regex
 * \return the number of lines that matched (zero or more) or
 *         -1 if an error occurred
 */
int regex_match(const char *filename, const char *regex, char ***matches) {
    char **lines;
    char **matched_lines;
    FILE *fp_text           = fopen(filename, "r");
    char text[MAX_LINE_LEN] = {'\0'};

    if (fp_text == NULL) {
        return -1;
    }
    // count the number of lines
    int n_lines = 0;
    while (fgets(text, MAX_LINE_LEN, fp_text) != NULL) {
        ++n_lines;
    }

    lines = (char **)malloc(n_lines * sizeof(char *));
    for (int i = 0; i < n_lines; ++i) {
        lines[i] = (char *)malloc(MAX_LINE_LEN * sizeof(char));
    }

    // get back to beginning of text file
    rewind(fp_text);

    int n_matched_lines = 0;
    while (fgets(text, MAX_LINE_LEN, fp_text) != NULL) {
        // printf("%s", text);
        int is_matched = runRegexMatch(regex, text, strlen(text));

        if (is_matched) {
            lines[n_matched_lines] =
                    (char *)malloc(MAX_LINE_LEN * sizeof(char));
            strcpy(lines[n_matched_lines], text);
            ++n_matched_lines;
        }
    }

    matched_lines = (char **)malloc(n_matched_lines * sizeof(char *));

    for (int i = 0; i < n_matched_lines; ++i) {
        matched_lines[i] = (char *)malloc(MAX_LINE_LEN * sizeof(char));
    }

    for (int i = 0; i < n_matched_lines; ++i) {
        strcpy(matched_lines[i], lines[i]);
    }

    for (int i = 0; i < n_lines; ++i) {
        free(lines[i]);
    }
    free(lines);

    *matches = matched_lines;

    return n_matched_lines;
}
