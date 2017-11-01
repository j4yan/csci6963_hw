
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LEN 257
#define MAX_WORD_LEN 30

// typedef char Word[MAX_WORD_LEN];
// typedef char Line[MAX_LINE_LEN];
typedef struct _Bigram Bigram;
typedef struct _Trigram Trigram;
typedef struct _BigramArray BigramArray;
typedef struct _TrigramArray TrigramArray;
typedef struct _Occurence Occurence;
typedef struct _IndexedWord IndexedWord;
typedef struct _IndexArray IndexArray;

struct _Bigram {
    char *word1;
    char *word2;
    int n_occurs;
} DefaultBigram = {NULL, NULL, 0};

struct _Trigram {
    char *word1;
    char *word2;
    char *word3;
    int n_occurs;
} DefaultTrigram = {NULL, NULL, NULL, 0};

struct _BigramArray {
    Bigram *bigrams;
    int size;
    int max_size;
} DefaultBigramArray = {NULL, 0, 0};

struct _TrigramArray {
    Trigram *trigrams;
    int size;
    int max_size;
} DefaultTrigramArray = {NULL, 0, 0};

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

void addBigram(const char *word1, const char *word2, BigramArray *bigrams);
void addTrigram(const char *word1,
                const char *word2,
                const char *word3,
                TrigramArray *trigrams);

char *createNewWord();
int parseLine(char *line, char ***words_out_ptr);
void initIndexArray(IndexArray *indices);
void addOccurToIndexedWord(int doc_id,
                           int line_id,
                           int word_id,
                           IndexedWord *word);
void addWordToIndexArray(const char *word,
                         int doc_id,
                         int line_id,
                         int word_id,
                         IndexArray *arr);
void createInvertedIndex(const char* fname, IndexArray* indices);
void createNGrams(const char* fname, const IndexArray* indices, BigramArray* biarr, TrigramArray* triarr);
int skipWordInNgram(const char* word, const IndexArray* indices, int top);
int compareIndexedWordsByOccurence(const void *a, const void *b);
int compareBigramsByOccurence(const void *a, const void *b);
int compareTrigramsByOccurence(const void *a, const void *b);
void outputInvertedIndex(const char* fname, const IndexArray* indices, int top_n);
void outputBigrams(const char* fname, const BigramArray* arr, int top_n);
void outputTrigrams(const char* fname, const TrigramArray* arr, int top_n);
void destroyIndexArray(IndexArray *indices);

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s", "Error: <Usage: 1 input file>\n");
        exit(EXIT_FAILURE);
    }

    IndexArray indices    = DefaultIndexArray;


    BigramArray bigrams   = DefaultBigramArray;
    TrigramArray trigrams = DefaultTrigramArray;

    createInvertedIndex(argv[1], &indices);
    createNGrams(argv[1], &indices, &bigrams, &trigrams);


    outputInvertedIndex("index.dat", &indices, -1);
    outputBigrams("bigrams.dat", &bigrams, -1);
    outputTrigrams("trigrams.dat", &trigrams, -1);

    destroyIndexArray(&indices);

    return 0;
}

void createInvertedIndex(const char* fname, IndexArray* indices) {

    FILE *fp = fopen(fname, "r");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }
    int doc_id       = 0;
    int line_id      = 0;
    char *line       = (char *)malloc(MAX_LINE_LEN * sizeof(char));

    while (NULL != fgets(line, MAX_LINE_LEN, fp)) {
        ++line_id;
        // printf("%i %s", line_id, line);
        char **words = NULL;
        int n_words  = parseLine(line, &words);
        for (int i = 0; i < n_words; ++i) {
            addWordToIndexArray(words[i], doc_id, line_id, i + 1, indices);
        }
    }


    // sort index
    {
        IndexedWord *words = indices->words;
        qsort(words,
              indices->size,
              sizeof(*words),
              compareIndexedWordsByOccurence);
    }

    free(line);
    fclose(fp);
    return;
}

void createNGrams(const char* fname, const IndexArray* indices, BigramArray* biarr, TrigramArray* triarr) {

    FILE *fp = fopen(fname, "r");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }
    int line_id = 0;
    int n_words_old  = 0;
    char **words_old = NULL;
    char *line       = (char *)malloc(MAX_LINE_LEN * sizeof(char));

    // initialize
    while (n_words_old == 0) {
        if (NULL != fgets(line, MAX_LINE_LEN, fp)) {
            ++line_id;
            char **words = NULL;
            int n_words  = parseLine(line, &words);

            n_words_old = n_words;
            words_old   = words;
        }
    }
    while (NULL != fgets(line, MAX_LINE_LEN, fp)) {
        ++line_id;
        // printf("%i %s", line_id, line);
        char **words = NULL;
        int n_words  = parseLine(line, &words);

        // here we store bigrams and trigrams, that's why we need \p words_old
        // int n_tot_words = n_words + max(2, n_words_old);

        // count the total number of words we may have in generating bigrams/trigrams
        int n_tot_words = n_words;
        if (n_words_old >= 2) {
            n_tot_words += 2;
        } else if (n_words_old == 1) {
            n_tot_words += 1;
        }

        // allocate memory to store all words
        char **tot_words = (char **)malloc(n_tot_words * sizeof(char *));
        for (int i = 0; i < n_tot_words; ++i) {
            tot_words[i] = createNewWord();
        }

        // put all words in a single array
        int word_cnt = 0;
        if (n_words_old >= 2) {
            strcpy(tot_words[word_cnt], words_old[n_words_old - 2]);
            ++word_cnt;
            strcpy(tot_words[word_cnt], words_old[n_words_old - 1]);
            ++word_cnt;
        } else if (n_words_old == 1) {
            strcpy(tot_words[word_cnt], words_old[n_words_old - 1]);
            ++word_cnt;
        }

        for (int i = 0; i < n_words; ++i) {
            strcpy(tot_words[word_cnt], words[i]);
            ++word_cnt;
        }

        assert(word_cnt == n_tot_words);

        // trigram
        int top = 50;
        int i = 0;
        while( i < n_tot_words -2 ) {
        // for (int i = 0; i < n_tot_words - 2; ++i) {
            if (skipWordInNgram(tot_words[i+2], indices, top)) {
                i += 3;
                continue;
            }
            if (skipWordInNgram(tot_words[i+1], indices, top)) {
                i += 2;
                continue;
            }
            if (skipWordInNgram(tot_words[i], indices, top)) {
                i += 1;
                continue;
            }
            addTrigram(tot_words[i],
                       tot_words[i + 1],
                       tot_words[i + 2],
                       triarr);
            ++i;
        }

        // bigram
        i = 0;
        while( i < n_tot_words - 1 ) {
        // for (int i = 0; i < n_tot_words - 1; ++i) {
            if (skipWordInNgram(tot_words[i+1], indices, top)) {
                i += 2;
                continue;
            }
            if (skipWordInNgram(tot_words[i], indices, top)) {
                i += 1;
                continue;
            }
            addBigram(tot_words[i],
                       tot_words[i + 1],
                       biarr);
            ++i;
        }

        // for (int i = 1; i < n_tot_words - 2; ++i) {
        //     addBigram(tot_words[i], tot_words[i + 1], &bigrams);
        // }

        // update the old words, only when the current line is not empty
        if (n_words > 0) {
            for (int i = 0; i < n_words_old; ++i) {
                free(words_old[i]);
            }
            free(words_old);
            words_old   = words;
            n_words_old = n_words;
        }
    }
    // sort bigrams and trigrams
    {
        Bigram *bigrams_ptr = biarr->bigrams;
        qsort(bigrams_ptr,
              biarr->size,
              sizeof(*bigrams_ptr),
              compareBigramsByOccurence);
    }
    {
        Trigram *trigrams = triarr->trigrams;
        qsort(trigrams,
              triarr->size,
              sizeof(*trigrams),
              compareTrigramsByOccurence);
    }

    free(line);
    fclose(fp);
    return;
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
void addOccurToIndexedWord(int doc_id,
                           int line_id,
                           int word_id,
                           IndexedWord *word) {
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
 * To stabilize, if the occurences are the same, we compare
 * them by their address.
 *
 * \param a 1st word
 * \param b 2nd word
 *
 * \return a.n_occurs - b.n_occurs
 */
int compareIndexedWordsByOccurence(const void *a, const void *b) {
    const IndexedWord *w1 = (IndexedWord *)a;
    const IndexedWord *w2 = (IndexedWord *)b;

    int diff = w2->n_occurs - w1->n_occurs;
    // return w2->n_occurs - w1->n_occurs;
    return diff == 0 ? (int)(w1 - w2) : diff;
}

/*!
 * \brief Compare two bigrams by their occurence.
 *
 * To stabilize, if the occurences are the same, we compare
 * them by their address.
 *
 * \param a 1st bigram
 * \param b 2nd bigram
 *
 * \return a.n_occurs - b.n_occurs
 */
int compareBigramsByOccurence(const void *a, const void *b) {
    const Bigram *w1 = (Bigram *)a;
    const Bigram *w2 = (Bigram *)b;

    int diff = w2->n_occurs - w1->n_occurs;
    // return w2->n_occurs - w1->n_occurs;
    return diff == 0 ? (int)(w1 - w2) : diff;
}

/*!
 * \brief Compare two trigrams by their occurence.
 *
 * To stabilize, if the occurences are the same, we compare
 * them by their address.
 *
 * \param a 1st trigram
 * \param b 2nd trigram
 *
 * \return a.n_occurs - b.n_occurs
 */
int compareTrigramsByOccurence(const void *a, const void *b) {
    const Trigram *w1 = (Trigram *)a;
    const Trigram *w2 = (Trigram *)b;

    int diff = w2->n_occurs - w1->n_occurs;
    // return w2->n_occurs - w1->n_occurs;
    return diff == 0 ? (int)(w1 - w2) : diff;
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

char *createNewWord() {
    char *word = (char *)malloc(MAX_WORD_LEN * sizeof(char));

    for (int i = 0; i < MAX_WORD_LEN; ++i) {
        word[i] = '\0';
    }
    return word;
}
void addBigram(const char *word1, const char *word2, BigramArray *arr) {
    int found = 0;
    for (int i = 0; i < arr->size; ++i) {
        Bigram *bigram = arr->bigrams + i;
        found          = strcmp(word1, bigram->word1) == 0 &&
                strcmp(word2, bigram->word2) == 0;

        if (found) {
            bigram->n_occurs += 1;
            return;
        }
    }
    if (arr->max_size <= 0) {
        arr->max_size            = 1;
        arr->size                = 0;
        int max_size             = arr->max_size;
        arr->bigrams             = (Bigram *)malloc(max_size * sizeof(Bigram));
        arr->bigrams[0].word1    = createNewWord();
        arr->bigrams[0].word2    = createNewWord();
        arr->bigrams[0].n_occurs = 0;
    } else if (arr->size >= arr->max_size - 1) {
        arr->max_size *= 2;
        Bigram *ptr = realloc(arr->bigrams, arr->max_size * sizeof(Bigram));

        if (ptr == NULL) {
            fprintf(stderr, "%s", "Error: <unable to reallocate memory>\n");
            exit(EXIT_FAILURE);
        } else {
            arr->bigrams = ptr;

            for (int i = arr->size; i < arr->max_size; ++i) {
                arr->bigrams[i].word1    = createNewWord();
                arr->bigrams[i].word2    = createNewWord();
                arr->bigrams[i].n_occurs = 0;
            }
        }
    }

    int end = arr->size;
    strcpy(arr->bigrams[end].word1, word1);
    strcpy(arr->bigrams[end].word2, word2);
    arr->bigrams[end].n_occurs += 1;
    ++(arr->size);
    return;
}

void addTrigram(const char *word1,
                const char *word2,
                const char *word3,
                TrigramArray *arr) {
    int found = 0;
    for (int i = 0; i < arr->size; ++i) {
        Trigram *bigram = arr->trigrams + i;
        found           = strcmp(word1, bigram->word1) == 0 &&
                strcmp(word2, bigram->word2) == 0;

        if (found) {
            bigram->n_occurs += 1;
            return;
        }
    }
    if (arr->max_size <= 0) {
        arr->max_size          = 1;
        arr->size              = 0;
        int max_size           = arr->max_size;
        arr->trigrams          = (Trigram *)malloc(max_size * sizeof(Trigram));
        arr->trigrams[0].word1 = createNewWord();
        arr->trigrams[0].word2 = createNewWord();
        arr->trigrams[0].word3 = createNewWord();
        arr->trigrams[0].n_occurs = 0;
    } else if (arr->size >= arr->max_size - 1) {
        arr->max_size *= 2;
        Trigram *ptr = realloc(arr->trigrams, arr->max_size * sizeof(Trigram));

        if (ptr == NULL) {
            fprintf(stderr, "%s", "Error: <unable to reallocate memory>\n");
            exit(EXIT_FAILURE);
        } else {
            arr->trigrams = ptr;

            for (int i = arr->size; i < arr->max_size; ++i) {
                arr->trigrams[i].word1    = createNewWord();
                arr->trigrams[i].word2    = createNewWord();
                arr->trigrams[i].word3    = createNewWord();
                arr->trigrams[i].n_occurs = 0;
            }
        }
    }

    int end = arr->size;
    strcpy(arr->trigrams[end].word1, word1);
    strcpy(arr->trigrams[end].word2, word2);
    strcpy(arr->trigrams[end].word3, word3);
    arr->trigrams[end].n_occurs += 1;
    ++(arr->size);
    return;

    return;
}

void outputInvertedIndex(const char* fname, const IndexArray* indices, int top_n) {

    assert(top_n <= indices->size);
    FILE *fp = fopen(fname, "w");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }

    int n_output = top_n < 0 ? indices->size : top_n;
    int n_total_words = 0;

    for (int i = 0; i < indices->size; ++i) {
        IndexedWord *word = indices->words + i;
        n_total_words += word->n_occurs;
    }
    fprintf(fp, "TOTAL WORDS: %i\n", n_total_words);
    fprintf(fp, "UNIQUE WORDS: %i\n", indices->size);
    for (int i = 0; i < n_output; ++i) {
        IndexedWord *word = indices->words + i;
        n_total_words += word->n_occurs;
        fprintf(fp, "%i %s\n", word->n_occurs, word->chars);
    }

    fclose(fp);

    return;
}


void outputBigrams(const char* fname, const BigramArray* arr, int top_n) {
    assert(top_n <= arr->size);
    FILE *fp = fopen(fname, "w");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }


    int n_output = top_n < 0 ? arr->size : top_n;
    int n_total_grams = 0;

    for (int i = 0; i < arr->size; ++i) {
        Bigram *gram = arr->bigrams + i;
        n_total_grams += gram->n_occurs;
    }
    fprintf(fp, "TOTAL BIGRAMS: %i\n", n_total_grams);
    fprintf(fp, "UNIQUE BIGRAMS: %i\n", arr->size);
    for (int i = 0; i < n_output; ++i) {
        Bigram *gram = arr->bigrams + i;
        n_total_grams += gram->n_occurs;
        fprintf(fp, "%i %s %s\n", gram->n_occurs, gram->word1, gram->word2);
    }

    fclose(fp);

    return;
}

void outputTrigrams(const char* fname, const TrigramArray* arr, int top_n) {
    assert(top_n <= arr->size);
    FILE *fp = fopen(fname, "w");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }


    int n_output = top_n < 0 ? arr->size : top_n;
    int n_total_grams = 0;

    for (int i = 0; i < arr->size; ++i) {
        Trigram *gram = arr->trigrams + i;
        n_total_grams += gram->n_occurs;
    }
    fprintf(fp, "TOTAL TRIGRAMS: %i\n", n_total_grams);
    fprintf(fp, "UNIQUE TRIGRAMS: %i\n", arr->size);
    for (int i = 0; i < n_output; ++i) {
        Trigram *gram = arr->trigrams + i;
        n_total_grams += gram->n_occurs;
        fprintf(fp, "%i %s %s %s\n", gram->n_occurs, gram->word1, gram->word2, gram->word3);
    }

    fclose(fp);
    return;
}


int skipWordInNgram(const char* word, const IndexArray* indices, int top) {
    int n_top = top < indices->size ? top : indices->size;

    for (int i=0; i<n_top; ++i) {
        IndexedWord* word_i = indices->words + i;
        if (!strcmp(word, word_i->chars)) {
            return 1;
        }
    }

    return 0;
}
