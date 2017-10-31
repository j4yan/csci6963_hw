
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 257
#define MAX_WORD_LEN 30

// typedef char Word[MAX_WORD_LEN];
// typedef char Line[MAX_LINE_LEN];
typedef struct _Occurence Occurence;
typedef struct _IndexedWord IndexedWord;
typedef struct _IndexArray IndexArray;

/*!
 * \brief Occurence info of an indexed word
 */
struct _Occurence {
    int doc_id;  ///< the document id
    int line_id; ///< the line position of word occurence
    int word_id; ///<  the position of word in line
};

/*!
 * \brief indexed word
 */
struct _IndexedWord {
    char *chars;
    Occurence *occurs; ///< array storing occurence  info
    int n_occurs;      ///< the actual size of \p words
    int max_occurs;    ///< the capability of \p words
} DefaultIndexedWord = {NULL, NULL, 0, 0};

/*!
 * \brief Dynamic array storing indexed words.
 */
struct _IndexArray {
    IndexedWord *words;
    int size;
    int max_size;
} DefaultIndexArray = {NULL, 0, 0};

char *createNewWord() {
    char *word = (char *)malloc(MAX_WORD_LEN * sizeof(char));

    for (int i = 0; i < MAX_WORD_LEN; ++i) {
        word[i] = '\0';
    }
    return word;
}

int trimWordPrefix(char *word);
int trimWordPostfix(char *word);
void trimWord(char *word);
int parseLine(char *line, char ***words_out_ptr);
void initIndexArray(IndexArray *indices);
void addOccurToIndexedWord(int doc_id, int line_id, int word_id, IndexedWord *word);
void addWordToIndexArray(const char *word,
                         int doc_id,
                         int line_id,
                         int word_id,
                         IndexArray *arr);
int comparaIndexedWordByOccurence(const void *a, const void *b);
void destroyIndexArray(IndexArray *indices);

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s", "Error: <Usage: 1 input file>\n");
        exit(EXIT_FAILURE);
    }
    char *fname_text = argv[1];

    FILE *fp_text = fopen(fname_text, "r");
    if (fp_text == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }

    /*
     * test function parseLine
     */
    // char *word1   = (char *)malloc(MAX_WORD_LEN * sizeof(char));
    // char *word1   = createNewWord();
    // word1[0]      = '"';
    // word1[1]      = 'B';
    // word1[2]      = '-';
    // word1[3]      = '-';
    // word1[4]      = 'C';
    // word1[5]      = 'd';
    // word1[6]      = 'e';
    // word1[7]      = '-';
    // word1[8]      = 'a';
    // word1[9]      = '.';
    // char **words1 = NULL;
    // int n_words1  = parseLine(word1, &words1);
    // for (int i = 0; i < n_words1; ++i) {
    //     printf("%s\n", words1[i]);
    // }

    IndexArray indices = DefaultIndexArray;

    int doc_id    = 0;
    int line_id   = 0;
    int word_id   = 0;
    char *line = (char*) malloc(MAX_LINE_LEN * sizeof(char));
    while (NULL != fgets(line, MAX_LINE_LEN, fp_text)) {

        ++line_id;
        char **words = NULL;
        int n_words  = parseLine(line, &words);
        for (int i = 0; i < n_words; ++i) {
            addWordToIndexArray(words[i], doc_id, line_id, word_id, &indices);
            ++word_id;
        }
        for (int i=0; i<n_words; ++i) {
            free(words[i]);
        } 
        free(words);
    }

    free(line);

    IndexedWord *words = indices.words;
    qsort(words, indices.size, sizeof(*words), comparaIndexedWordByOccurence);

    int n_total_words = 0;

    for (int i = 0; i < indices.size; ++i) {
        IndexedWord *word = indices.words + i;
        n_total_words += word->n_occurs;
    }
    printf("TOTAL WORDS: %i\n", n_total_words);
    printf("UNIQUE WORDS: %i\n", indices.size);
    for (int i = 0; i < indices.size; ++i) {
        IndexedWord *word = indices.words + i;
        n_total_words += word->n_occurs;
        printf("%i %s\n", word->n_occurs, word->chars);
    }
    for (int i = 0; i < indices.size; ++i) {
        IndexedWord *word = indices.words + i;
        if (strcmp(word->chars, "essential")) {
            continue;
        }
        for(int j=0; j<word->n_occurs; ++j) {
            Occurence* occur = word->occurs + j;
            printf("%i %i\n", occur->line_id, occur->word_id);
        }
    }

    destroyIndexArray(&indices);

    return 0;
}

/*!
 * \brief Add a word and its info into an inverted index.
 *
 * It's the user's responsibility to guarrantee the array
 * \var iindex is large enough.
 *
 * \param word
 * \param doc_id the id of the doc where \p word is
 * \param line_id the line number
 * \param head current length of \p iindex
 * \param indices the inverted index array to put \p word in
 *
 * \return void
 */
void addWordToIndexArray(const char *line,
                         int doc_id,
                         int line_id,
                         int word_id,
                         IndexArray *indices) {
    int word_found = 0;
    for (int i = 0; i < indices->size; ++i) {
        IndexedWord *word = &indices->words[i];
        // if we already have the word
        word_found = !strcmp(word->chars, line);
        if (word_found) {
            addOccurToIndexedWord(doc_id, line_id, word_id, word);
            return;
        }
    }

    // if the word is not indexed yet
    if (indices->max_size <= 0) {
        indices->max_size = 1;
        indices->size     = 0;
        indices->words =
                (IndexedWord *)malloc(indices->max_size * sizeof(IndexedWord));
        indices->words[0] = DefaultIndexedWord;
    } else if (indices->size >= indices->max_size - 1) {
        // check the capability of indices
        indices->max_size *= 2;
        IndexedWord *ptr = realloc(indices->words,
                                   indices->max_size * sizeof(IndexedWord));
        if (ptr == NULL) {
            fprintf(stderr, "%s", "Error: <unable to reallocate memory>\n");
            exit(EXIT_FAILURE);
        } else {
            indices->words = ptr;
            for (int i = indices->size; i < indices->max_size; ++i) {
                indices->words[i] = DefaultIndexedWord;
            }
            // printf("IndexArray: memory reallocated\n");
        }
    }

    int end = indices->size;
    // indices->words[end].chars = (char *)malloc(MAX_WORD_LEN * sizeof(char));
    indices->words[end].chars = createNewWord();
    addOccurToIndexedWord(doc_id, line_id, word_id, indices->words + end);
    strcpy(indices->words[end].chars, line);
    ++(indices->size);

    return;
}

/*!
 * \brief Add Occurence to indexed word.
 *
 * \param doc_id id of occurence doc
 * \param line_id id of occurence line
 * \param word indexed word
 */
void addOccurToIndexedWord(int doc_id, int line_id, int word_id, IndexedWord *word) {
    if (word->max_occurs <= 0) {
        word->max_occurs = 1;
        word->n_occurs   = 0;
        word->occurs =
                (Occurence *)malloc(word->max_occurs * sizeof(Occurence));
    } else if (word->n_occurs >= word->max_occurs - 1) {
        // this deal with the initial case where max_docs = 0
        word->max_occurs *= 2;

        Occurence *ptr =
                realloc(word->occurs, word->max_occurs * sizeof(Occurence));

        if (ptr == NULL) {
            fprintf(stderr, "%s", "Error: <unable to reallocate memory>\n");
            exit(EXIT_FAILURE);
        } else {
            word->occurs = ptr;
            // printf("WordOccurence: memory reallocated\n");
        }
    }

    Occurence *end = word->occurs + word->n_occurs;
    end->doc_id    = doc_id;
    end->line_id   = line_id;
    end->word_id   = word_id;

    ++(word->n_occurs);
    return;
}

/*!
 * \brief free the memory allocated in indices, both directly and indirectly.
 *
 * \param indices
 *
 * \return void
 */
void destroyIndexArray(IndexArray *indices) {
    for (int i = 0; i < indices->size; ++i) {
        free(indices->words[i].occurs);
    }

    free(indices->words);
    return;
}

/*!
 * \brief Compare two indexed words by their occurence.
 *
 * \param a 1st word
 * \param b 2nd word
 *
 * \return a.n_occurs - b.n_occurs
 */
int comparaIndexedWordByOccurence(const void *a, const void *b) {
    const IndexedWord *w1 = (IndexedWord *)a;
    const IndexedWord *w2 = (IndexedWord *)b;

    return w2->n_occurs - w1->n_occurs;
}

/*!
 * \brief trim word
 */
void trimWord(char *word) {

    while (trimWordPrefix(word)) {
    };
    while (trimWordPostfix(word)) {
    };

    return;
}

/*!
 * \brief trim a word by removing the last character if it's non-alpha.
 * 
 * \param word word to be trimmed
 * \return 0 if no trim is done, otherwise, 1
 */
int trimWordPostfix(char *word) {
    const int end = strlen(word) - 1;
    if (end < 0) {
        return 0;
    }
    if (isalpha(word[end])) {
        return 0;
    }
    if (isdigit(word[end])) {
        return 0;
    }

    word[end] = '\0';

    return 1;
}

/*!
 * \brief trim a word by removing the first character if it's non-alpha.
 * 
 * \param word word to be trimmed
 * \param 0 if no trim is done, otherwise, 1
 */
int trimWordPrefix(char *word) {
    const size_t len = strlen(word);
    if (!len)
        return 0;

    const int end = len - 1;

    if (end < 0) {
        return 0;
    }
    if (isalpha(word[0])) {
        return 0;
    }
    // if (isdigit(word[0])) {
    //     return 0;
    // }

    char *new_word = createNewWord();
    strcpy(new_word, word + 1);
    strcpy(word, new_word);
    free(new_word);

    return 1;
}

/*!
 * \brief Parse a word, that is, this "word" read in with space
 * as stopper may be separated into several words. 
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

    // count new words so we can allocate memory
    // int word_cnt = 1;
    //
    // for (size_t i = 0; i < len - 1; ++i) {
    //     if (line[i] == '-' && line[i + 1] != '-') {
    //         ++word_cnt;
    //     } else if (line[i] == ' ' && line[i+1] != ' ') {
    //         ++word_cnt;
    //     }
    // }
    // char **words_out = (char **)malloc(word_cnt * sizeof(char *));
    //
    // for (int i = 0; i < word_cnt; ++i) {
    //     // words_out[i] = (char *)malloc(MAX_WORD_LEN * sizeof(char));
    //     words_out[i] = createNewWord();
    // }
    //
    // word_cnt     = 0;
    // int char_cnt = 0;
    // for (size_t i = 0; i < len; ++i) {
    //     if (line[i] == '-') {
    //         if (line[i + 1] != '-') {
    //             ++word_cnt;
    //             char_cnt = 0;
    //         }
    //     } else if (line[i] == ' ') {
    //         if (line[i+1] != ' ') {
    //             ++word_cnt;
    //             char_cnt = 0;
    //         }
    //     }else {
    //         words_out[word_cnt][char_cnt] = line[i];
    //         ++char_cnt;
    //     }
    // }
    //
    // // we didn't loop over the last charactor
    // words_out[word_cnt][char_cnt] = line[len - 1];

    // count the number of words we are gonna have
    int word_cnt = 0;
    int char_cnt = 0;
    for (size_t i = 0; i < len; ++i) {
        // if (isalpha(line[i]) || line[i] == '\''){
        //     if (isalpha(line[i-1]) || line[i-1] == '\''){
        //     } else {
        //         ++word_cnt;
        //     }
        // }
        if (char_cnt == 0) {
            if (isalpha(line[i])) {
                ++char_cnt;
                ++word_cnt;
            }
        } else {
            if (isalpha(line[i]) || line[i] == '\''){
                ++char_cnt;
            } else{
                char_cnt = 0;
            }

        }
    }

    char **words_out = (char **)malloc(word_cnt * sizeof(char *));
    for (int i = 0; i < word_cnt; ++i) {
        words_out[i] = createNewWord();
    }
    // words_out[0][0] = line[0];

    char_cnt = 0;
    word_cnt = -1;
    for (size_t i = 0; i < len; ++i) {
        // if (isalpha(line[i]) || line[i] == '\''){
        //     if (isalpha(line[i-1]) || line[i-1] == '\''){
        //         words_out[word_cnt][char_cnt] = line[i];
        //         ++char_cnt;
        //     } else {
        //         // start a new word
        //         ++word_cnt;
        //         char_cnt = 0;
        //         words_out[word_cnt][char_cnt] = line[i];
        //         ++char_cnt;
        //     }
        // }

        if (char_cnt == 0) {
            if (isalpha(line[i])) {
                ++word_cnt;
                words_out[word_cnt][char_cnt] = line[i];
                ++char_cnt;
            }
        } else {
            if (isalpha(line[i]) || line[i] == '\''){
                words_out[word_cnt][char_cnt] = line[i];
                ++char_cnt;
            } else {
                char_cnt = 0;
            }

        }
    }

    for (int i = 0; i < word_cnt + 1; ++i) {
        trimWord(words_out[i]);
    }

    *words_out_ptr = words_out;

    return word_cnt + 1;
}
