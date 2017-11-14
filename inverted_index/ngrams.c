#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "comm.h"
#include "index.h"
#include "merge_sort.h"
#include "ngrams.h"
#include "regex.h"

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
                  int doc_id,
                  const IndexArray *indices,
                  BigramArray *biarr,
                  TrigramArray *triarr) {

    int top  = 50; // top words to skip in \v indices
    FILE *fp = fopen(fname, "r");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open file>\n");
        exit(EXIT_FAILURE);
    }
    int line_id      = 0;
    int n_words_old  = 0;
    char **words_old = NULL;
    char *line_raw   = (char *)malloc(MAX_LINE_LEN * sizeof(char));
    char *line       = createNewLine();

    while (NULL != fgets(line_raw, MAX_LINE_LEN, fp)) {
        ++line_id;
        for (int i = 0; i < MAX_LINE_LEN; ++i) {
            line[i] = '\n';
        }
        int script_flag = trimLine(line_raw, line);
        if (script_flag == 1) {
            while (NULL != fgets(line_raw, MAX_LINE_LEN, fp)) {
                ++line_id;
                for (int i = 0; i < MAX_LINE_LEN; ++i) {
                    line[i] = '\n';
                }
                int flag = trimLine(line_raw, line);
                if (flag == 2) {
                    break;
                }
            }
            continue;
        } else if (script_flag == 2) {
            fprintf(stderr, "%s", "Error: <we should never get here>\n");
            exit(EXIT_FAILURE);
        }

        char **words = NULL;
        int n_words  = parseLine(line, &words);

        // First count the total number of words we may have in
        // generating bigrams and trigrams
        int n_tot_words = n_words;
        // since we may add 2 words into current line from last line,
        // this variable indicate where in \v tot_words to start
        // counting bigrams
        int bigram_start = 0;
        if (n_words_old >= 2) {
            n_tot_words += 2;
            bigram_start = 1;
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
        int i = 0;
        while (i < n_tot_words - 2) {
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

            addTrigram(tot_words[i],
                       tot_words[i + 1],
                       tot_words[i + 2],
                       doc_id,
                       triarr);
            ++i;
        }

        // bigram
        i = bigram_start;
        while (i < n_tot_words - 1) {
            if (skipWordInNgram(tot_words[i + 1], indices, top)) {
                i += 2;
                continue;
            }
            if (skipWordInNgram(tot_words[i], indices, top)) {
                i += 1;
                continue;
            }
            // if (!strcmp(tot_words[i], "hallward\'s")) {
            //     printf("%i\n", line_id);
            // }
            addBigram(tot_words[i], tot_words[i + 1], doc_id, biarr);
            ++i;
        }

        // update the old words, only when the current line is not empty
        if (n_words > 0) {
            for (int i = 0; i < n_words_old; ++i) {
                free(words_old[i]);
            }
            free(words_old);
            words_old   = tot_words;
            n_words_old = n_tot_words;
        }
    }

    // sort bigrams and trigrams
    // Bigram *bigrams_ptr = biarr->grams;
    // merge_sort(bigrams_ptr,
    //            biarr->size,
    //            sizeof(*bigrams_ptr),
    //            compareBigramsByOccurence);
    // Trigram *trigrams = triarr->grams;
    // merge_sort(trigrams,
    //            triarr->size,
    //            sizeof(*trigrams),
    //            compareTrigramsByOccurence);

    free(line_raw);
    free(line);
    fclose(fp);

    return;
}

void destroyBigramArray(BigramArray *arr) {
    for (int i = 0; i < arr->size; ++i) {
        free(arr->grams[i].occurs);
    }

    free(arr->grams);
    return;
}

void destroyTrigramArray(TrigramArray *arr) {
    for (int i = 0; i < arr->size; ++i) {
        free(arr->grams[i].occurs);
    }

    free(arr->grams);
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
 * \param a 1st trigram
 * \param b 2nd trigram
 *
 * \return a.n_occurs - b.n_occurs
 */
int compareTrigramsByOccurence(const void *a, const void *b) {
    const Trigram *g1 = (Trigram *)a;
    const Trigram *g2 = (Trigram *)b;

    return g2->n_occurs - g1->n_occurs;
    // int diff = g2->n_occurs - g1->n_occurs;
    // return diff == 0 ? (int)(g1 - g2) : diff;
}

/*!
 * \brief add a bigram into bigram array.
 *
 * \param word1 the 1st word in bigram
 * \param word2 the 2nd word in bigram
 * \param arr bigram array
 * \return nothing
 */
void addBigram(const char *word1,
               const char *word2,
               int doc_id,
               BigramArray *arr) {
    int found = 0;
    for (int i = 0; i < arr->size; ++i) {
        Bigram *bigram = arr->grams + i;

        found = strcmp(word1, bigram->word1) == 0 &&
                strcmp(word2, bigram->word2) == 0;

        if (found) {
            // bigram->n_occurs += 1;
            addOccurToBigram(doc_id, bigram);
            return;
        }
    }
    if (arr->max_size <= 0) {
        arr->max_size            = 1;
        arr->size                = 0;
        int max_size             = arr->max_size;
        arr->grams             = (Bigram *)malloc(max_size * sizeof(Bigram));
        arr->grams[0].word1    = createNewWord();
        arr->grams[0].word2    = createNewWord();
        arr->grams[0].n_occurs = 0;
        arr->grams[0].max_occurs = 0;
    } else if (arr->size >= arr->max_size - 1) {
        arr->max_size *= 2;
        Bigram *ptr = realloc(arr->grams, arr->max_size * sizeof(Bigram));

        if (ptr == NULL) {
            fprintf(stderr, "%s", "Error: <unable to reallocate memory>\n");
            exit(EXIT_FAILURE);
        } else {
            arr->grams = ptr;

            for (int i = arr->size; i < arr->max_size; ++i) {
                arr->grams[i].word1    = createNewWord();
                arr->grams[i].word2    = createNewWord();
                arr->grams[i].n_occurs = 0;
                arr->grams[i].max_occurs = 0;
            }
        }
    }

    int end = arr->size;
    strcpy(arr->grams[end].word1, word1);
    strcpy(arr->grams[end].word2, word2);
    // arr->grams[end].n_occurs += 1;
    addOccurToBigram(doc_id, arr->grams + end);
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
                int doc_id,
                TrigramArray *arr) {
    int found = 0;
    for (int i = 0; i < arr->size; ++i) {
        Trigram *trigram = arr->grams + i;

        found = strcmp(word1, trigram->word1) == 0 &&
                strcmp(word2, trigram->word2) == 0 &&
                strcmp(word3, trigram->word3) == 0;

        if (found) {
            // trigram->n_occurs += 1;
            addOccurToTrigram(doc_id, trigram);
            return;
        }
    }

    if (arr->max_size <= 0) {
        arr->max_size          = 1;
        arr->size              = 0;
        int max_size           = arr->max_size;
        arr->grams          = (Trigram *)malloc(max_size * sizeof(Trigram));
        arr->grams[0].word1 = createNewWord();
        arr->grams[0].word2 = createNewWord();
        arr->grams[0].word3 = createNewWord();
        arr->grams[0].n_occurs = 0;
        arr->grams[0].max_occurs = 0;
    } else if (arr->size >= arr->max_size - 1) {
        arr->max_size *= 2;
        Trigram *ptr = realloc(arr->grams, arr->max_size * sizeof(Trigram));

        if (ptr == NULL) {
            fprintf(stderr, "%s", "Error: <unable to reallocate memory>\n");
            exit(EXIT_FAILURE);
        } else {
            arr->grams = ptr;

            for (int i = arr->size; i < arr->max_size; ++i) {
                arr->grams[i].word1    = createNewWord();
                arr->grams[i].word2    = createNewWord();
                arr->grams[i].word3    = createNewWord();
                arr->grams[i].n_occurs = 0;
                arr->grams[i].max_occurs = 0;
            }
        }
    }

    int end = arr->size;
    strcpy(arr->grams[end].word1, word1);
    strcpy(arr->grams[end].word2, word2);
    strcpy(arr->grams[end].word3, word3);
    // arr->grams[end].n_occurs += 1;
    addOccurToTrigram(doc_id, arr->grams + end);
    ++(arr->size);

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
    const Bigram *g1 = (Bigram *)a;
    const Bigram *g2 = (Bigram *)b;

    return g2->n_occurs - g1->n_occurs;
    // int diff = g2->n_occurs - g1->n_occurs;
    // return diff == 0 ? (int)(g1 - g2) : diff;
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
        Bigram *gram = arr->grams + i;
        n_total_grams += gram->n_occurs;
    }
    fprintf(fp, "TOTAL BIGRAMS: %i\n", n_total_grams);
    fprintf(fp, "UNIQUE BIGRAMS: %i\n", arr->size);
    for (int i = 0; i < n_output; ++i) {
        Bigram *gram = arr->grams + i;
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
        Trigram *gram = arr->grams + i;
        n_total_grams += gram->n_occurs;
    }
    fprintf(fp, "TOTAL TRIGRAMS: %i\n", n_total_grams);
    fprintf(fp, "UNIQUE TRIGRAMS: %i\n", arr->size);
    for (int i = 0; i < n_output; ++i) {
        Trigram *gram = arr->grams + i;
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

void addOccurToBigram(int doc_id, Bigram *gram) {

    if (gram->max_occurs <= 0) {
        gram->max_occurs = 1;
        gram->n_occurs   = 0;
        gram->occurs =
                (Occurence *)malloc(gram->max_occurs * sizeof(Occurence));
    } else if (gram->n_occurs >= gram->max_occurs - 1) {
        // this deal with the initial case where max_docs = 0
        gram->max_occurs *= 2;

        Occurence *ptr =
                realloc(gram->occurs, gram->max_occurs * sizeof(Occurence));

        if (ptr == NULL) {
            fprintf(stderr, "%s", "Error: <unable to reallocate memory>\n");
            exit(EXIT_FAILURE);
        } else {
            gram->occurs = ptr;
            // printf("WordOccurence: memory reallocated\n");
        }
    }

    Occurence *end = gram->occurs + gram->n_occurs;
    end->doc_id    = doc_id;
    end->line_id   = -1;
    end->word_id   = -1;

    ++(gram->n_occurs);
    return;
}

void addOccurToTrigram(int doc_id, Trigram *gram) {

    if (gram->max_occurs <= 0) {
        gram->max_occurs = 1;
        gram->n_occurs   = 0;
        gram->occurs =
                (Occurence *)malloc(gram->max_occurs * sizeof(Occurence));
    } else if (gram->n_occurs >= gram->max_occurs - 1) {
        // this deal with the initial case where max_docs = 0
        gram->max_occurs *= 2;

        Occurence *ptr =
                realloc(gram->occurs, gram->max_occurs * sizeof(Occurence));

        if (ptr == NULL) {
            fprintf(stderr, "%s", "Error: <unable to reallocate memory>\n");
            exit(EXIT_FAILURE);
        } else {
            gram->occurs = ptr;
            // printf("WordOccurence: memory reallocated\n");
        }
    }

    Occurence *end = gram->occurs + gram->n_occurs;
    end->doc_id    = doc_id;
    end->line_id   = -1;
    end->word_id   = -1;

    ++(gram->n_occurs);
    return;
}
