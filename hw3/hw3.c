#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "comm.h"
#include "index.h"
#include "merge_sort.h"
#include "ngrams.h"

struct _InvertedIndex;
typedef struct _InvertedIndex IndexedIndex;
int readSurroundingLinesIntoWords(const char *fname,
                                  int line_id,
                                  int npre,
                                  int npost,
                                  char ***words_ptr);

int copyFiles(const char *to, const char *from);
// #ifndef USE_SUBMITTY_MAIN
int main(int argc, char **argv)
{
    // if (argc != 3) {
    //     fprintf(stderr, "%s", "Error: <Usage: 2 input files>\n");
    //     exit(EXIT_FAILURE);
    // }

    // FILE *fp = fopen("documents/contents.txt", "r");
    FILE *fp = fopen("contents.txt", "r");
    if (fp == NULL) {
        fprintf(stderr, "%s", "Error: <could not open contents.txt>\n");
        exit(EXIT_FAILURE);
    }

    // first count the number of lines
    char *line  = createNewLine();
    int n_files = 0;
    while (NULL != fgets(line, MAX_LINE_LEN, fp)) {
        int len = strlen(line);
        if (len == 0) {
            continue;
        }
        ++n_files;
    }
    // printf("number of files = %i\n", n_files);
    rewind(fp);

    char **fnames       = (char **)malloc(n_files * sizeof(char *));
    char **fnames1       = (char **)malloc(n_files * sizeof(char *));
    char **index_fnames = (char **)malloc(n_files * sizeof(char *));
    // const int len_prefix = 12;
    const int len_prefix = 12;
    for (int i = 0; i < n_files; ++i) {
        fnames[i] = createNewWord();
        // index_fnames[i] = createNewWord();
        // strncpy(fnames[i], "./documents/", len_prefix);
        // strncpy(index_fnames[i], "./documents/", len_prefix);
    }
    for (int i = 0; i < n_files; ++i) {
        fnames1[i]       = createNewWord();
        index_fnames[i] = createNewWord();
        strncpy(fnames1[i], "./documents/", len_prefix);
        strncpy(index_fnames[i], "./documents/", len_prefix);
    }

    // clock_t start = clock();

    int *doc_id = (int *)malloc(n_files * sizeof(int *));
    for (int i = 0; i < n_files; ++i) {
        // int flag = fscanf(fp, "%i %s", doc_id + i, fnames[i] + len_prefix);
        int flag = fscanf(fp, "%i %s", doc_id + i, fnames[i]);
        if (flag != 2) {
            // printf("%i\n", flag);
            fprintf(stderr, "%s", "Error: <wrong format in contents.txt>\n");
            exit(EXIT_FAILURE);
        }
        int len = strlen(fnames[i]);
        if (len == 0) {
            continue;
        }
        if (fnames[i][len - 1] == '\n') {
            fnames[i][len - 1] = '\0';
        }
        for (int j = 0; j < len; ++j) {
            index_fnames[i][j + len_prefix] = fnames[i][j];
            fnames1[i][j + len_prefix] = fnames[i][j];
            if (strncmp(fnames[i] + j, ".txt", 4) == 0) {
                strncpy(index_fnames[i] + j + len_prefix, ".index", 6);
                strncpy(fnames1[i] + j + len_prefix, ".txt", 4);
                break;
            }
        }

        // printf("%s\n", index_fnames[i]);
        fprintf(stdout,
                "Reading docID %i %s...\n",
                doc_id[i],
                index_fnames[i] + len_prefix);
        fflush(stdout);
        // printf("%i %s %s\n", doc_id[i], fnames[i], index_fnames[i]);
    }
    for (int i = 0; i < n_files; ++i) {
        copyFiles(fnames[i], fnames1[i]);
    }

    IndexArray indices = DefaultIndexArray;
    for (int i = 0; i < n_files; ++i) {
        createInvertedIndex(fnames1[i], doc_id[i], &indices);
    }

    // IndexedWord *words = indices.words;
    merge_sort(indices.words,
               indices.size,
               sizeof(*(indices.words)),
               compareIndexedWordsByOccurence);

    BigramArray bigrams   = DefaultBigramArray;
    TrigramArray trigrams = DefaultTrigramArray;
    for (int i = 0; i < n_files; ++i) {
        createNGrams(fnames1[i], i + 1, &indices, &bigrams, &trigrams);
    }

    // sort bigrams and trigrams
    // Bigram *bigrams_ptr = bigrams.grams;
    merge_sort(bigrams.grams,
               bigrams.size,
               sizeof(*(bigrams.grams)),
               compareBigramsByOccurence);

    // Trigram *trigrams_ptr = trigrams.grams;
    merge_sort(trigrams.grams,
               trigrams.size,
               sizeof(*(trigrams.grams)),
               compareTrigramsByOccurence);

    const int NPRE               = 10;
    const int NPOST              = 10;
    const int STENCIL_LINES      = NPRE + NPOST + 1;
    const int MAX_WORDS_PER_LINE = 20;
    const int STENCIL_WORDS      = MAX_WORDS_PER_LINE * STENCIL_LINES;
    char **sur_words = (char **)malloc(STENCIL_WORDS * sizeof(char *));
    for (int i = 0; i < STENCIL_WORDS; ++i) {
        sur_words[i] = createNewWord();
    }
    // for (int i=0; i<indices.size; ++i) {
    //     if (!strcmp(indices.words[i].chars, "abc")) {
    //         printf("doc_id(acc) = %i\n", indices.words[i].occurs[0].doc_id);
    //     }
    // }

    char **stopwords      = getStopWords();
    char **query_bigrams  = (char **)malloc(4 * sizeof(char **));
    char **query_trigrams = (char **)malloc(3 * sizeof(char **));
    for (int i = 0; i < 4; ++i) {
        query_bigrams[i] = createNewWord();
    }
    for (int i = 0; i < 3; ++i) {
        query_trigrams[i] = createNewWord();
    }
    // start searching words
    for (;;) {
        fprintf(stdout, "SEARCH: ");
        char *input_raw = createNewWord();
        fgets(input_raw, MAX_WORD_LEN, stdin);
        char *input = (char *)malloc(strlen(input_raw) * sizeof(input_raw));
        strcpy(input, input_raw);
        if (strcmp(input, "QUIT\n") == 0) {
            return EXIT_SUCCESS;
        }
        // printf("%s", input_raw);
        // printf("%s", input);

        char **query_words = NULL;
        int n_query_words  = parseLine(input, &query_words);
        // we only deal with up to 5 words
        const int max_words = 5;
        if (n_query_words > max_words) {
            n_query_words = max_words;
        }

        int *n_unigram_occurs = (int *)malloc(n_query_words * sizeof(int));
        int *n_unigram_docs   = (int *)malloc(n_query_words * sizeof(int));
        int *n_trigram_occurs = (int *)malloc(n_query_words * sizeof(int));
        int *n_trigram_docs   = (int *)malloc(n_query_words * sizeof(int));
        int *n_bigram_occurs  = (int *)malloc(n_query_words * sizeof(int));
        int *n_bigram_docs    = (int *)malloc(n_query_words * sizeof(int));
        int **unigram_occ_doc_id =
                (int **)malloc(n_query_words * sizeof(int *));
        int **unigram_occ_line_id =
                (int **)malloc(n_query_words * sizeof(int *));
        int **bigram_occ_doc_id = (int **)malloc(n_query_words * sizeof(int *));
        int **bigram_occ_line_id =
                (int **)malloc(n_query_words * sizeof(int *));
        int **trigram_occ_doc_id =
                (int **)malloc(n_query_words * sizeof(int *));
        int **trigram_occ_line_id =
                (int **)malloc(n_query_words * sizeof(int *));
        int n_stop_words  = 0;
        int nonstop_id[5] = {-1};
        {
            int cnt = 0;
            for (int i = 0; i < n_query_words; ++i) {
                int found_stop_word = 0;
                for (int j = 0; j < NUM_STOPWORDS; ++j) {
                    if (0 == strcmp(query_words[i], stopwords[j])) {
                        found_stop_word = 1;
                        ++n_stop_words;
                        break;
                    }
                }
                if (!found_stop_word) {
                    nonstop_id[cnt++] = i;
                }
            }
        }
        int n_trigrams = n_query_words - 2 - n_stop_words;
        int n_bigrams  = n_query_words - 1 - n_stop_words;
        // printf("n_trigrams = %i, n_bigrams = %i\n", n_trigrams, n_bigrams);
        // printf("nonstop_id = %i %i %i %i %i \n",
        //        nonstop_id[0],
        //        nonstop_id[1],
        //        nonstop_id[2],
        //        nonstop_id[3],
        //        nonstop_id[4]);

        // trigram occurrence
        for (int i = 0; i < n_trigrams; ++i) {
            trigram_occ_doc_id[i]  = (int *)malloc(n_files * sizeof(int));
            trigram_occ_line_id[i] = (int *)malloc(n_files * sizeof(int));
            n_trigram_occurs[i]    = 0;
            n_trigram_docs[i]      = 0;
            // int found_query_word   = 0;

            int id1 = nonstop_id[i];
            int id2 = nonstop_id[i + 1];
            int id3 = nonstop_id[i + 2];

            for (int j = 0; j < trigrams.size; ++j) {
                // printf("%s %s %s\n", trigrams.grams[j].word1,
                // trigrams.grams[j].word2, trigrams.grams[j].word3);
                // printf("%s %s %s\n", query_words[id1], query_words[id2],
                // query_words[id3]);
                if (strcmp(trigrams.grams[j].word1, query_words[id1]) != 0 ||
                    strcmp(trigrams.grams[j].word2, query_words[id2]) != 0 ||
                    strcmp(trigrams.grams[j].word3, query_words[id3]) != 0) {
                    continue;
                }
                // printf("found trigram\n");
                // found_query_word = 1;
                int n_cur_occurs = trigrams.grams[j].n_occurs;
                n_trigram_occurs[i] += n_cur_occurs;

                for (int occ_i = 0; occ_i < n_cur_occurs; ++occ_i) {
                    Occurence *occ = &(trigrams.grams[j].occurs[occ_i]);
                    // check if we already have the docID
                    int found = 0;
                    for (int k = 0; k < n_trigram_docs[i]; ++k) {
                        if (trigram_occ_doc_id[i][k] == occ->doc_id) {
                            found = 1;
                            break;
                        }
                    }
                    // check if we already have all the docs, break
                    if (!found) {
                        trigram_occ_doc_id[i][n_trigram_docs[i]] = occ->doc_id;
                        trigram_occ_line_id[i][n_trigram_docs[i]] =
                                occ->line_id;
                        ++n_trigram_docs[i];
                    }
                    if (n_trigram_docs[i] == n_files) {
                        break;
                    }
                }
                // arriving here indicates that we already have found the query
                // words
                break;
            }

            // if (!found_query_word) {
            // printf("there is something wrong!!!\n");
            // return EXIT_FAILURE;
            // }

            fprintf(stdout,
                    "Number of \"%s %s %s\" occurrences is %i",
                    query_words[id1],
                    query_words[id2],
                    query_words[id3],
                    n_trigram_occurs[i]);
            if (n_trigram_occurs[i] > 0) {
                fprintf(stdout, " [docID");
                for (int j = 0; j < n_trigram_docs[i]; ++j) {
                    fprintf(stdout, " %i", trigram_occ_doc_id[i][j]);
                }
                fprintf(stdout, "]");
            }

            fprintf(stdout, "\n");
        }
        // trigram occurrence
        for (int i = 0; i < n_bigrams; ++i) {
            bigram_occ_doc_id[i]  = (int *)malloc(n_files * sizeof(int));
            bigram_occ_line_id[i] = (int *)malloc(n_files * sizeof(int));
            n_bigram_occurs[i]    = 0;
            n_bigram_docs[i]      = 0;
            // int found_query_word   = 0;

            int id1 = nonstop_id[i];
            int id2 = nonstop_id[i + 1];

            for (int j = 0; j < bigrams.size; ++j) {
                // printf("%s %s\n", bigrams.grams[j].word1,
                // bigrams.grams[j].word2);
                // printf("%s %s\n", query_words[id1], query_words[id2]);
                if (strcmp(bigrams.grams[j].word1, query_words[id1]) != 0 ||
                    strcmp(bigrams.grams[j].word2, query_words[id2]) != 0) {
                    continue;
                }
                // printf("found trigram\n");
                // found_query_word = 1;
                int n_cur_occurs = bigrams.grams[j].n_occurs;
                n_bigram_occurs[i] += n_cur_occurs;

                for (int occ_i = 0; occ_i < n_cur_occurs; ++occ_i) {
                    Occurence *occ = &(bigrams.grams[j].occurs[occ_i]);
                    // check if we already have the docID
                    int found = 0;
                    for (int k = 0; k < n_bigram_docs[i]; ++k) {
                        if (bigram_occ_doc_id[i][k] == occ->doc_id) {
                            found = 1;
                            break;
                        }
                    }
                    // check if we already have all the docs, break
                    if (!found) {
                        bigram_occ_doc_id[i][n_bigram_docs[i]]  = occ->doc_id;
                        bigram_occ_line_id[i][n_bigram_docs[i]] = occ->line_id;
                        ++n_bigram_docs[i];
                    }
                    if (n_bigram_docs[i] == n_files) {
                        break;
                    }
                }
                // arriving here indicates that we already have found the query
                // words
                break;
            }

            // if (!found_query_word) {
            //     printf("there is something wrong!!!\n");
            // return EXIT_FAILURE;
            // }

            fprintf(stdout,
                    "Number of \"%s %s\" occurrences is %i",
                    query_words[id1],
                    query_words[id2],
                    n_bigram_occurs[i]);
            if (n_bigram_occurs[i] > 0) {
                fprintf(stdout, " [docID");
                for (int j = 0; j < n_bigram_docs[i]; ++j) {
                    fprintf(stdout, " %i", bigram_occ_doc_id[i][j]);
                }
                fprintf(stdout, "]");
            }

            fprintf(stdout, "\n");
        }

        // words occurrence
        for (int i = 0; i < n_query_words; ++i) {
            unigram_occ_doc_id[i]  = (int *)malloc(n_files * sizeof(int));
            unigram_occ_line_id[i] = (int *)malloc(n_files * sizeof(int));
            n_unigram_occurs[i]    = 0;
            n_unigram_docs[i]      = 0;
            // int found_query_word   = 0;
            for (int j = 0; j < indices.size; ++j) {
                if (strcmp(indices.words[j].chars, query_words[i])) {
                    continue;
                }
                // found_query_word = 1;
                int n_cur_occurs = indices.words[j].n_occurs;
                n_unigram_occurs[i] += n_cur_occurs;

                for (int occ_i = 0; occ_i < n_cur_occurs; ++occ_i) {
                    Occurence *occ = &(indices.words[j].occurs[occ_i]);
                    // check if we already have the docID
                    int found = 0;
                    for (int k = 0; k < n_unigram_docs[i]; ++k) {
                        if (unigram_occ_doc_id[i][k] == occ->doc_id) {
                            found = 1;
                            break;
                        }
                    }
                    // check if we already have all the docs, break
                    if (!found) {
                        unigram_occ_doc_id[i][n_unigram_docs[i]] = occ->doc_id;
                        unigram_occ_line_id[i][n_unigram_docs[i]] =
                                occ->line_id;
                        ++n_unigram_docs[i];
                    }
                    if (n_unigram_docs[i] == n_files) {
                        break;
                    }
                }
                // arriving here indicates that we already have found the query
                // words
                break;
            }

            // if (!found_query_word) {
            //     printf("there is something wrong!!!\n");
            //     return EXIT_FAILURE;
            // }

            fprintf(stdout,
                    "Number of \"%s\" occurrences is %i",
                    query_words[i],
                    n_unigram_occurs[i]);
            if (n_unigram_occurs[i] > 0) {
                fprintf(stdout, " [docID");
                for (int j = 0; j < n_unigram_docs[i]; ++j) {
                    fprintf(stdout, " %i", unigram_occ_doc_id[i][j]);
                }
                fprintf(stdout, "]");
            }

            fprintf(stdout, "\n");
        }
        //
        // print snippets of trigrams
        //
        for (int i = 0; i < n_trigrams; ++i) {
            if (n_trigram_occurs[i] == 0) {
                continue;
            }

            int id1 = nonstop_id[i];
            // int id2 = nonstop_id[i + 1];
            int id3 = nonstop_id[i + 2];

            for (int doc_j = 0; doc_j < n_trigram_docs[i]; ++doc_j) {
                int n_stencil_words = readSurroundingLinesIntoWords(
                        fnames1[trigram_occ_doc_id[i][doc_j] - 1],
                        trigram_occ_line_id[i][doc_j],
                        NPRE,
                        NPOST,
                        &sur_words);
                // loop over stencil words, find out the index of query word.
                // index of pass-the-end word in snippet
                int query_word_id1   = -1;
                int query_word_id3   = -1;
                int word_id0         = -1;  // index of first word in snippet
                int word_id1         = -1;  // index of last word in snippet
                int found_query_word = 0;
                char *tmp_word       = createNewWord();
                for (int j = 0; j < n_stencil_words; ++j) {
                    char **sub_words = NULL;
                    strcpy(tmp_word, sur_words[j]);
                    // int n_sub_words  = parseLine(sur_words[j], &sub_words);
                    int n_sub_words = parseLine(tmp_word, &sub_words);
                    for (int k = 0; k < n_sub_words; ++k) {
                        if (!strcmp(sub_words[k], query_words[id1])) {
                            // since we added into sur_words NPRE lines before
                            // and NPOST lines after the occuring line, these 2
                            // indices propably are not negative.
                            query_word_id1   = j;
                            found_query_word = 1;
                            break;
                        }
                    }
                    if (found_query_word) {
                        break;
                    }
                }
                if (found_query_word == 0) {
                    fprintf(stderr, "%s", "Error: <could not find word>\n");
                    // printf("%s\n", fnames[unigram_occ_doc_id[i][doc_j] - 1]);
                    // printf("line_id = %i\n", trigram_occ_line_id[i][doc_j]);
                    // for (int j = 0; j < n_stencil_words; ++j) {
                    //     printf("%s ", sur_words[j]);
                    // }
                    // printf("\n");
                    exit(EXIT_FAILURE);
                }
                found_query_word = 0;
                for (int j = query_word_id1 + 1; j < n_stencil_words; ++j) {
                    char **sub_words = NULL;
                    strcpy(tmp_word, sur_words[j]);
                    // int n_sub_words  = parseLine(sur_words[j], &sub_words);
                    int n_sub_words = parseLine(tmp_word, &sub_words);
                    for (int k = 0; k < n_sub_words; ++k) {
                        if (!strcmp(sub_words[k], query_words[id3])) {
                            // since we added into sur_words NPRE lines before
                            // and NPOST lines after the occuring line, these 2
                            // indices propably are not negative.
                            query_word_id3   = j;
                            found_query_word = 1;
                            break;
                        }
                    }
                    if (found_query_word) {
                        break;
                    }
                }
                if (found_query_word == 0) {
                    fprintf(stderr, "%s", "Error: <could not find word>\n");
                    // printf("%s", fnames[unigram_occ_doc_id[i][doc_j] - 1]);
                    exit(EXIT_FAILURE);
                }
                word_id0 = query_word_id1;
                word_id1 = query_word_id3;
                // find word_id0 and word_id1
                int n_tot_sub_words = 0;
                for (int j = 0; j < 100; j++) {
                    char **sub_words = NULL;
                    int id           = query_word_id1 - 1 - j;
                    if (id < 0) {
                        break;
                    }
                    strcpy(tmp_word, sur_words[id]);
                    int n_sub_words = parseLine(tmp_word, &sub_words);
                    n_tot_sub_words += n_sub_words;
                    word_id0 = id;
                    if (n_tot_sub_words >= 5) {
                        break;
                    }
                }
                n_tot_sub_words = 0;
                for (int j = 0; j < 100; j++) {
                    char **sub_words = NULL;
                    int id           = query_word_id3 + j + 1;
                    if (id > n_stencil_words - 1) {
                        break;
                    }
                    strcpy(tmp_word, sur_words[id]);
                    int n_sub_words = parseLine(tmp_word, &sub_words);
                    n_tot_sub_words += n_sub_words;
                    word_id1 += n_sub_words;
                    word_id1 = id;
                    if (n_tot_sub_words >= 5) {
                        break;
                    }
                }
                if (word_id0 < 0) {
                    word_id0 = 0;
                }
                if (word_id1 > n_stencil_words) {
                    word_id1 = n_stencil_words;
                }
                int len = strlen(sur_words[word_id1]);
                if (!isalpha(sur_words[word_id1][len-1])) {
                    sur_words[word_id1][len-1] = '\0';
                }
                fprintf(stdout, "...");
                for (int j = word_id0; j < word_id1; ++j) {
                    fprintf(stdout, "%s ", sur_words[j]);
                }
                fprintf(stdout, "%s...\n", sur_words[word_id1]);
            }
        }

        //
        // print snippets of bigrams
        //
        for (int i = 0; i < n_bigrams; ++i) {
            if (n_bigram_occurs[i] == 0) {
                continue;
            }

            int id1 = nonstop_id[i];
            int id2 = nonstop_id[i + 1];

            for (int doc_j = 0; doc_j < n_bigram_docs[i]; ++doc_j) {
                int n_stencil_words = readSurroundingLinesIntoWords(
                        fnames1[bigram_occ_doc_id[i][doc_j] - 1],
                        bigram_occ_line_id[i][doc_j],
                        NPRE,
                        NPOST,
                        &sur_words);
                // loop over stencil words, find out the index of query word.
                // index of pass-the-end word in snippet
                int query_word_id1   = -1;
                int query_word_id2   = -1;
                int word_id0         = -1;  // index of first word in snippet
                int word_id1         = -1;  // index of last word in snippet
                int found_query_word = 0;
                char *tmp_word       = createNewWord();
                for (int j = 0; j < n_stencil_words; ++j) {
                    char **sub_words = NULL;
                    strcpy(tmp_word, sur_words[j]);
                    // int n_sub_words  = parseLine(sur_words[j], &sub_words);
                    int n_sub_words = parseLine(tmp_word, &sub_words);
                    for (int k = 0; k < n_sub_words; ++k) {
                        if (!strcmp(sub_words[k], query_words[id1])) {
                            // since we added into sur_words NPRE lines before
                            // and NPOST lines after the occuring line, these 2
                            // indices propably are not negative.
                            query_word_id1   = j;
                            found_query_word = 1;
                            break;
                        }
                    }
                    if (found_query_word) {
                        break;
                    }
                }
                if (found_query_word == 0) {
                    fprintf(stderr, "%s", "Error: <could not find word>\n");
                    // printf("%s\n", fnames[unigram_occ_doc_id[i][doc_j] - 1]);
                    // printf("line_id = %i\n", trigram_occ_line_id[i][doc_j]);
                    // for (int j = 0; j < n_stencil_words; ++j) {
                    //     printf("%s ", sur_words[j]);
                    // }
                    // printf("\n");
                    exit(EXIT_FAILURE);
                }
                found_query_word = 0;
                for (int j = query_word_id1 + 1; j < n_stencil_words; ++j) {
                    char **sub_words = NULL;
                    strcpy(tmp_word, sur_words[j]);
                    // int n_sub_words  = parseLine(sur_words[j], &sub_words);
                    int n_sub_words = parseLine(tmp_word, &sub_words);
                    for (int k = 0; k < n_sub_words; ++k) {
                        if (!strcmp(sub_words[k], query_words[id2])) {
                            // since we added into sur_words NPRE lines before
                            // and NPOST lines after the occuring line, these 2
                            // indices propably are not negative.
                            query_word_id2   = j;
                            found_query_word = 1;
                            break;
                        }
                    }
                    if (found_query_word) {
                        break;
                    }
                }
                if (found_query_word == 0) {
                    fprintf(stderr, "%s", "Error: <could not find word>\n");
                    // printf("%s", fnames[unigram_occ_doc_id[i][doc_j] - 1]);
                    exit(EXIT_FAILURE);
                }
                word_id0 = query_word_id1;
                word_id1 = query_word_id2;
                // printf("%i %i\n", word_id0, word_id1);
                // find word_id0 and word_id1
                int n_tot_sub_words = 0;
                for (int j = 0; j < 100; j++) {
                    char **sub_words = NULL;
                    int id           = query_word_id1 - 1 - j;
                    if (id < 0) {
                        break;
                    }
                    strcpy(tmp_word, sur_words[id]);
                    int n_sub_words = parseLine(tmp_word, &sub_words);
                    n_tot_sub_words += n_sub_words;
                    word_id0 = id;
                    if (n_tot_sub_words >= 5) {
                        break;
                    }
                }
                n_tot_sub_words = 0;
                for (int j = 0; j < 100; j++) {
                    char **sub_words = NULL;
                    int id           = query_word_id2 + j + 1;
                    if (id > n_stencil_words - 1) {
                        break;
                    }
                    strcpy(tmp_word, sur_words[id]);
                    int n_sub_words = parseLine(tmp_word, &sub_words);
                    n_tot_sub_words += n_sub_words;
                    word_id1 += n_sub_words;
                    word_id1 = id;
                    if (n_tot_sub_words >= 5) {
                        break;
                    }
                }
                if (word_id0 < 0) {
                    word_id0 = 0;
                }
                if (word_id1 > n_stencil_words) {
                    word_id1 = n_stencil_words;
                }
                int len = strlen(sur_words[word_id1]);
                if (!isalpha(sur_words[word_id1][len-1])) {
                    sur_words[word_id1][len-1] = '\0';
                }
                // printf("%i %i\n", word_id0, word_id1);
                fprintf(stdout, "...");
                for (int j = word_id0; j < word_id1; ++j) {
                    fprintf(stdout, "%s ", sur_words[j]);
                }
                fprintf(stdout, "%s...\n", sur_words[word_id1]);
            }
        }

        //
        // print snippets of unigrams
        //
        for (int i = 0; i < n_query_words; ++i) {
            if (n_unigram_occurs[i] == 0) {
                continue;
            }

            for (int doc_j = 0; doc_j < n_unigram_docs[i]; ++doc_j) {
                int n_stencil_words = readSurroundingLinesIntoWords(
                        fnames1[unigram_occ_doc_id[i][doc_j] - 1],
                        unigram_occ_line_id[i][doc_j],
                        NPRE,
                        NPOST,
                        &sur_words);
                // for (int i=0; i<n_stencil_words; ++i) {
                //     printf("%s ", sur_words[i]);
                // }
                // printf("\n");
                // loop over stencil words, find out the index of query word.
                int query_word_id =
                        -1;  // index of pass-the-end word in snippet
                int word_id0         = -1;  // index of first word in snippet
                int word_id1         = -1;  // index of last word in snippet
                int found_query_word = 0;
                char *tmp_word       = createNewWord();
                for (int j = 0; j < n_stencil_words; ++j) {
                    char **sub_words = NULL;
                    strcpy(tmp_word, sur_words[j]);
                    // int n_sub_words  = parseLine(sur_words[j], &sub_words);
                    int n_sub_words = parseLine(tmp_word, &sub_words);
                    for (int k = 0; k < n_sub_words; ++k) {
                        if (!strcmp(sub_words[k], query_words[i])) {
                            // since we added into sur_words NPRE lines before
                            // and NPOST lines after the occuring line, these 2
                            // indices propably are not negative.
                            query_word_id    = j;
                            found_query_word = 1;
                            break;
                        }
                    }
                    if (found_query_word) {
                        break;
                    }
                }
                if (found_query_word == 0) {
                    fprintf(stderr, "%s", "Error: <could not find word>\n");
                    // printf("%s", fnames[unigram_occ_doc_id[i][doc_j] - 1]);
                    exit(EXIT_FAILURE);
                }
                word_id0 = query_word_id;
                word_id1 = query_word_id;
                // find word_id0 and word_id1
                int n_tot_sub_words = 0;
                for (int j = 0; j < 100; j++) {
                    char **sub_words = NULL;
                    int id           = query_word_id - 1 - j;
                    if (id < 0) {
                        break;
                    }
                    strcpy(tmp_word, sur_words[id]);
                    int n_sub_words = parseLine(tmp_word, &sub_words);
                    n_tot_sub_words += n_sub_words;
                    word_id0 = id;
                    if (n_tot_sub_words >= 5) {
                        break;
                    }
                }
                n_tot_sub_words = 0;
                for (int j = 0; j < 100; j++) {
                    char **sub_words = NULL;
                    int id           = query_word_id + j + 1;
                    if (id > n_stencil_words - 1) {
                        break;
                    }
                    strcpy(tmp_word, sur_words[id]);
                    int n_sub_words = parseLine(tmp_word, &sub_words);
                    n_tot_sub_words += n_sub_words;
                    word_id1 += n_sub_words;
                    word_id1 = id;
                    if (n_tot_sub_words >= 5) {
                        break;
                    }
                }
                if (word_id0 < 0) {
                    word_id0 = 0;
                }
                if (word_id1 > n_stencil_words) {
                    word_id1 = n_stencil_words;
                }
                int len = strlen(sur_words[word_id1]);
                if (!isalpha(sur_words[word_id1][len-1])) {
                    sur_words[word_id1][len-1] = '\0';
                }
                fprintf(stdout, "...");
                for (int j = word_id0; j < word_id1; ++j) {
                    fprintf(stdout, "%s ", sur_words[j]);
                }
                fprintf(stdout, "%s...\n", sur_words[word_id1]);
            }
        }
        fflush(stdout);
        for (int i = 0; i < n_query_words; ++i) {
            free(unigram_occ_doc_id[i]);
            free(unigram_occ_line_id[i]);
        }
        for (int i = 0; i < n_trigrams; ++i) {
            free(trigram_occ_doc_id[i]);
            free(trigram_occ_line_id[i]);
        }
        for (int i = 0; i < n_bigrams; ++i) {
            free(bigram_occ_doc_id[i]);
            free(bigram_occ_line_id[i]);
        }
        free(unigram_occ_doc_id);
        free(unigram_occ_line_id);
        free(bigram_occ_doc_id);
        free(bigram_occ_line_id);
        free(trigram_occ_doc_id);
        free(trigram_occ_line_id);
        free(input_raw);
        free(input);
    }

    free(line);
    for (int i = 0; i < 4; ++i) {
        free(query_bigrams[i]);
    }
    for (int i = 0; i < 3; ++i) {
        free(query_trigrams[i]);
    }
    free(query_bigrams);
    free(query_trigrams);

    destroyIndexArray(&indices);
    destroyBigramArray(&bigrams);
    destroyTrigramArray(&trigrams);

    for (int i = 0; i < STENCIL_WORDS; ++i) {
        free(sur_words[i]);
    }
    free(sur_words);
    for (int i = 0; i < n_files; ++i) {
        free(fnames[i]);
        free(index_fnames[i]);
    }
    free(fnames);
    free(index_fnames);
    return 0;
}

// #endif

int readSurroundingLinesIntoWords(const char *fname,
                                  int line_id,
                                  int npre,
                                  int npost,
                                  char ***words_ptr)
{
    char **words = *words_ptr;
    // print out snippets
    int line_id0 = line_id - npre;
    int line_id1 = line_id + npost;
    if (line_id0 < 1) {
        line_id0 = 1;
    }

    FILE *fp_occ = fopen(fname, "r");
    if (fp_occ == NULL) {
        fprintf(stderr,
                "%s",
                "Error: <could not open file to read surrounding strins>\n");
        exit(EXIT_FAILURE);
    }

    char *cur_line = (char *)malloc(MAX_LINE_LEN * sizeof(char));
    for (int l = 1; l < line_id0; ++l) {
        char *fgets_ptr = fgets(cur_line, MAX_LINE_LEN, fp_occ);
        // char *fgets_ptr = fgets(cur_line, 2, fp_occ);

        if (fgets_ptr == NULL) {
            fprintf(stderr,
                    "%s",
                    "Error: <trying to read line that does not exist>\n");
            exit(EXIT_FAILURE);
        }
    }
    int word_cnt = 0;
    int char_cnt = 0;
    for (int l = line_id0; l <= line_id1; ++l) {
        // char *cur_line  = raw_lines[l - line_id0];
        char *fgets_ptr = fgets(cur_line, MAX_LINE_LEN, fp_occ);
        if (fgets_ptr == NULL) {
            break;
        }
        // printf("%s", cur_line);
        // copy all stencil lines into all_lines
        size_t cur_line_len = strlen(cur_line);
        if (cur_line_len == 0) {
            continue;
        }
        for (size_t k = 0; k < cur_line_len; ++k) {
            if (cur_line[k] == '\n' || isspace(cur_line[k])) {
                if (char_cnt == 0) {
                    continue;
                }
                else {
                    words[word_cnt][char_cnt] = '\0';
                    ++word_cnt;
                    char_cnt = 0;
                }
            }
            else {
                words[word_cnt][char_cnt++] = cur_line[k];
            }
        }
    }

    // for (int i=0; i<word_cnt; ++i) {
    //     int len = strlen(words[i]);
    //     if (words[i][len-1] == '.') {
    //         words[i][len-1] = '\0';
    //     }
    // }

    // for (int i=0; i<word_cnt; ++i) {
    //     printf("%s ", words[i]);
    // }
    // printf("\n");
    fclose(fp_occ);
    free(cur_line);
    return word_cnt;
}

int copyFiles(const char *to, const char *from)
{
    int fd_to, fd_from;
    char buf[4096];
    ssize_t nread;
    int saved_errno;

    fd_from = open(from, O_RDONLY);
    if (fd_from < 0) return -1;

    fd_to = open(to, O_WRONLY | O_CREAT | O_EXCL, 0666);
    if (fd_to < 0) goto out_error;

    while (nread = read(fd_from, buf, sizeof buf), nread > 0) {
        char *out_ptr = buf;
        ssize_t nwritten;

        do {
            nwritten = write(fd_to, out_ptr, nread);

            if (nwritten >= 0) {
                nread -= nwritten;
                out_ptr += nwritten;
            }
            else if (errno != EINTR) {
                goto out_error;
            }

        } while (nread > 0);
    }

    if (nread == 0) {
        if (close(fd_to) < 0) {
            fd_to = -1;
            goto out_error;
        }
        close(fd_from);

        /* Success! */
        return 0;
    }

out_error:
    saved_errno = errno;

    close(fd_from);
    if (fd_to >= 0) close(fd_to);

    errno = saved_errno;
    return -1;
}
