#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comm.h"
#include "index.h"
#include "ngrams.h"

const Bigram DefaultBigram             = {NULL, NULL, 0};
const BigramArray DefaultBigramArray   = {NULL, 0, 0};
const Trigram DefaultTrigram           = {NULL, NULL, NULL, 0};
const TrigramArray DefaultTrigramArray = {NULL, 0, 0};
/*!
 * \brief Creates bigrams and trigrams.
 *
 * \param fname file name of document to be analyzed
 * \param indices inverted index which gives top 50 words to be skipped.
 * \param biarr bigram array
 * \param triarr trigram array
 * \return void
 */
void createNGrams(const char *fname,
                  const IndexArray *indices,
                  BigramArray *biarr,
                  TrigramArray *triarr) {

    FILE *fp = fopen(fname, "r");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }
    int line_id      = 0;
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

        // count the total number of words we may have in generating
        // bigrams/trigrams
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
        int i   = 0;
        while (i < n_tot_words - 2) {
            // for (int i = 0; i < n_tot_words - 2; ++i) {
            if (skipWordInNgram(tot_words[i + 2], indices, top)) {
                i += 3;
                continue;
            }
            if (skipWordInNgram(tot_words[i + 1], indices, top)) {
                i += 2;
                continue;
            }
            if (skipWordInNgram(tot_words[i], indices, top)) {
                i += 1;
                continue;
            }
            addTrigram(
                    tot_words[i], tot_words[i + 1], tot_words[i + 2], triarr);
            ++i;
        }

        // bigram
        i = 0;
        while (i < n_tot_words - 1) {
            // for (int i = 0; i < n_tot_words - 1; ++i) {
            if (skipWordInNgram(tot_words[i + 1], indices, top)) {
                i += 2;
                continue;
            }
            if (skipWordInNgram(tot_words[i], indices, top)) {
                i += 1;
                continue;
            }
            addBigram(tot_words[i], tot_words[i + 1], biarr);
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
 * \brief Test if the input word should be skipped according to its
 *        frequency in inverted index.
 *
 * Note the inverted index should be sorted before calling this function.
 *
 * \param word word to be tested
 * \param indices the inverted index
 * \param top number of words at top of inverted index with high frequency
 * \return 1 if skipping this word, 0 if not.
 */
int skipWordInNgram(const char *word, const IndexArray *indices, int top) {
    int n_top = top < indices->size ? top : indices->size;

    for (int i = 0; i < n_top; ++i) {
        IndexedWord *word_i = indices->words + i;
        if (!strcmp(word, word_i->chars)) {
            return 1;
        }
    }

    return 0;
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
 * \brief add a bigram into bigram array.
 *
 * \param word1 the 1st word in bigram
 * \param word2 the 2nd word in bigram
 * \param arr bigram array
 * \return nothing
 */
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

/*!
 * \bried add a trigram into trigram array.
 *
 * \param word1 the 1st word in trigram
 * \param word2 the 2nd word in trigram
 * \param word3 the 2nd word in trigram
 * \param arr trigram array
 * \return nothing
 */
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
 * \brief output the top \p top_n bigrams with most occurence and its inverted
 * index info.
 *
 * \param fname output file name
 * \param arr trigram array
 * \param top_n number of grams to be output
 * \return void
 */
void outputBigrams(const char *fname, const BigramArray *arr, int top_n) {
    assert(top_n <= arr->size);
    FILE *fp = fopen(fname, "w");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }

    int n_output      = top_n < 0 ? arr->size : top_n;
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

/*!
 * \brief output the top \p top_n trigrams with most occurence and its inverted
 * index info.
 *
 * \param fname output file name
 * \param arr trigram array
 * \param top_n number of grams to be output
 * \return void
 */
void outputTrigrams(const char *fname, const TrigramArray *arr, int top_n) {
    assert(top_n <= arr->size);
    FILE *fp = fopen(fname, "w");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }

    int n_output      = top_n < 0 ? arr->size : top_n;
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
        fprintf(fp,
                "%i %s %s %s\n",
                gram->n_occurs,
                gram->word1,
                gram->word2,
                gram->word3);
    }

    fclose(fp);
    return;
}