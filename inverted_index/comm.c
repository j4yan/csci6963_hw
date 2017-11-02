#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "comm.h"

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
 * \brief Parse a line using word delimiters.
 *
 * \param line word to be parsed
 * \param words_out_ptr a pointer to store new words
 * \return number of new words
 */
int parseLine(char *line, char ***words_out_ptr) {
    size_t len = strlen(line);
    // empty line
    if (len < 2) {
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
