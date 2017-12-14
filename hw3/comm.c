#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include "comm.h"
#include "regex.h"


const int NUM_STOPWORDS = 30;
char** getStopWords() {
    char** stopwords = (char**) malloc(NUM_STOPWORDS * sizeof(char*));
    for (int i=0; i<NUM_STOPWORDS; ++i) {
        stopwords[i] = createNewWord();
    }

    strcpy(stopwords[0], "the");
    strcpy(stopwords[1], "of");
    strcpy(stopwords[2], "to");
    strcpy(stopwords[3], "a");
    strcpy(stopwords[4], "and");
    strcpy(stopwords[5], "in");
    strcpy(stopwords[6], "said");
    strcpy(stopwords[7], "for");
    strcpy(stopwords[8], "that");
    strcpy(stopwords[9], "was");
    strcpy(stopwords[10], "on");
    strcpy(stopwords[11], "he");
    strcpy(stopwords[12], "is");
    strcpy(stopwords[13], "with");
    strcpy(stopwords[14], "at");
    strcpy(stopwords[15], "by");
    strcpy(stopwords[16], "it");
    strcpy(stopwords[17], "from");
    strcpy(stopwords[18], "as");
    strcpy(stopwords[19], "be");
    strcpy(stopwords[20], "were");
    strcpy(stopwords[21], "an");
    strcpy(stopwords[22], "have");
    strcpy(stopwords[23], "his");
    strcpy(stopwords[24], "but");
    strcpy(stopwords[25], "has");
    strcpy(stopwords[26], "are");
    strcpy(stopwords[27], "not");
    strcpy(stopwords[28], "who");
    strcpy(stopwords[29], "they");

    return stopwords;
}
/*!
 * \brief create a new word.
 */
char *createNewWord() {
    char *word = (char *)malloc(MAX_WORD_LEN * sizeof(char));

    for (int i = 0; i < MAX_WORD_LEN; ++i) {
        word[i] = '\0';
    }
    return word;
}

/*!
 * \brief create a new word.
 */
char *createNewLine() {
    char *line = (char *)malloc(MAX_LINE_LEN * sizeof(char));

    for (int i = 0; i < MAX_LINE_LEN; ++i) {
        line[i] = '\n';
    }
    return line;
}

/*!
 * \brief Parse a line using word delimiters.
 *
 * \param line word to be parsed
 * \param words_out_ptr a pointer to store new words
 * \return number of new words
 */
int parseLine(char *line, char ***words_out_ptr) {
    size_t len = strlen(line);
    // empty line
    if (len < 1) {
        return 0;
    }

    // convert to lower case
    for (size_t i = 0; i < len; ++i) {
        line[i] = tolower((unsigned char)line[i]);
    }

    // trimWord(line);

    len = strlen(line);
    if (!len) {
        return 0;
    }

    // count the number of words we are gonna have.
    int word_cnt  = 0;
    int char_cnt  = 0;
    int quote_cnt = 0;
    for (size_t i = 0; i < len; ++i) {
        if (char_cnt == 0) {
            // got a new word
            if (isalpha(line[i])) {
                ++char_cnt;
                ++word_cnt;
            }
        } else {
            if (isalpha(line[i])) {
                // continue the current word
                ++char_cnt;
            } else if (line[i] == '\'' && quote_cnt == 0) {
                // continue the current word. remember there is at
                // most 1 single quote in each word.
                ++char_cnt;
                ++quote_cnt;
            } else {
                // in this case we must encounter some delimiters.
                char_cnt  = 0;
                quote_cnt = 0;
            }
        }
    }

    // allocate memory
    char **words_out = (char **)malloc(word_cnt * sizeof(char *));
    for (int i = 0; i < word_cnt; ++i) {
        words_out[i] = createNewWord();
    }

    // start to storing words. Use \p char_cnt to indicate
    // whether it's a new word. That's why \p word_cnt
    // starts from -1.
    char_cnt  = 0;
    quote_cnt = 0;
    word_cnt  = -1;
    for (size_t i = 0; i < len; ++i) {
        if (char_cnt == 0) {
            if (isalpha(line[i])) {
                ++word_cnt;
                words_out[word_cnt][char_cnt] = line[i];
                ++char_cnt;
            }
        } else {
            if (isalpha(line[i])) {
                words_out[word_cnt][char_cnt] = line[i];
                ++char_cnt;
            } else if (line[i] == '\'' && quote_cnt == 0) {
                // continue the current word
                words_out[word_cnt][char_cnt] = line[i];
                ++char_cnt;
                ++quote_cnt;
            } else {
                // in this case we must encounter some delimiters.
                char_cnt  = 0;
                quote_cnt = 0;
            }
        }
    }

    ++word_cnt;

    // if the word ends with '\'', trim it.
    for (int i = 0; i < word_cnt; ++i) {
        size_t len_i = strlen(words_out[i]);
        if (words_out[i][len_i - 1] == '\'') {
            words_out[i][len_i - 1] = '\0';
        }
    }

    *words_out_ptr = words_out;

    return word_cnt;
}

/*! \brief Trim off scripts and tags of a line.
 *
 */
int trimLine(const char *line_in, char *line_out) {

    // typedef char* REGEX;
    // REGEX regex[2];
    // regex[0] = "<\w+>";
    // regex[1] = "<\\\w+>";
    const int line_len = strlen(line_in);

    int script_flag = 0;
    if (runRegexMatch("<script", line_in, line_len)) {
        script_flag += 1;
    }
    if (runRegexMatch("/script>", line_in, line_len)) {
        script_flag += 2;
    }

    // if script is not closed
    if (script_flag == 1) {
        return 1;
    } else if (script_flag == 2) {
        return 2;
    } else if (script_flag == 3) {
        return 0;
    }

    int is_title = 0;
    for (int i = 0; i < line_len - 1; ++i) {
        if (line_in[i] == '<' && line_in[i + 1] == '?') {
            is_title += 1;
            break;
        }
    }
    for (int i = 0; i < line_len - 1; ++i) {
        if (line_in[i] == '?' && line_in[i + 1] == '>') {
            is_title += 1;
            break;
        }
    }
    if (is_title > 1) {
        return 0;
    }

    int has_tag         = 0;
    char *left_bracket  = "<\\w+";
    char *right_bracket = "</\\w+";
    if (runRegexMatch(left_bracket, line_in, line_len)) {
        has_tag += 1;
    }
    if (runRegexMatch(right_bracket, line_in, line_len)) {
        has_tag += 2;
    }
    for (int i = 0; i < line_len - 1; ++i) {
        if (line_in[i] == '<' && line_in[i + 1] == '!') {
            has_tag += 4;
            break;
        }
    }

    if (has_tag > 0) {
        int in_bracket = 0;
        int cnt        = 0;
        for (int i = 0; i < line_len; ++i) {
            if (line_in[i] == '<') {
                in_bracket = 1;
                continue;
            }
            if (line_in[i] == '>') {
                in_bracket      = 0;
                line_out[cnt++] = ' ';
                continue;
            }
            if (!in_bracket) {
                line_out[cnt++] = line_in[i];
            }
        }
        removeSpecialChars(line_out);
        return 0;
    }

    strcpy(line_out, line_in);
    removeSpecialChars(line_out);

    return 0;
}

void removeSpecialChars(char *line) {
    const int len = strlen(line);
    for (int i = 0; i < len - 4; ++i) {
        int found = !strncmp(line + i, "&amp", 4);
        if (found) {
            strncpy(line + i, "    ", 4);
        }
    }
    for (int i = 0; i < len - 5; ++i) {
        int found = !strncmp(line + i, "&nbsp", 5);
        if (found) {
            strncpy(line + i, "     ", 5);
        }
    }
    for (int i = 0; i < len - 5; ++i) {
        int found = !strncmp(line + i, "&quot", 5);
        if (found) {
            strncpy(line + i, "     ", 5);
        }
    }
    for (int i = 0; i < len - 3; ++i) {
        int found = !strncmp(line + i, "&lt", 3);
        if (found) {
            strncpy(line + i, "   ", 3);
        }
    }
    for (int i = 0; i < len - 3; ++i) {
        int found = !strncmp(line + i, "&gt", 3);
        if (found) {
            strncpy(line + i, "   ", 3);
        }
    }

    return;
}

