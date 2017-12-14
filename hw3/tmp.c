
        // trigrams occurence
        for (int i = 0; i < n_words - 2; ++i) {
            int n_occurs      = 0;
            int is_first      = 1;
            int target_doc_id = -1;
            int word_id       = -1;
            int n_doc         = 0;
            for (int i = 0; i < n_files; ++i) {
                occuring_doc_id[i] = -1;
            }
            for (int j = 0; j < trigrams.size; ++j) {
                if (strcmp(trigrams.grams[j].word1, words[i + 0]) == 0 &&
                    strcmp(trigrams.grams[j].word2, words[i + 1]) == 0 &&
                    strcmp(trigrams.grams[j].word3, words[i + 2]) == 0) {
                    int n_cur_occurs = indices.words[j].n_occurs;
                    n_occurs += n_cur_occurs;
                    if (is_first == 1 && trigrams.grams[j].n_occurs > 0) {
                        target_doc_id = trigrams.grams[j].occurs[0].line_id;
                        word_id       = trigrams.grams[j].occurs[0].word_id;
                    }
                    is_first += 1;

                    for (int l = 0; l < n_cur_occurs; ++l) {
                        int found = 0;
                        for (int k = 0; k < n_doc; ++k) {
                            if (occuring_doc_id[k] ==
                                indices.words[j].occurs[l].doc_id) {
                                found = 1;
                                break;
                            }
                        }
                        if (n_doc == n_files) {
                            break;
                        }
                        if (!found) {
                            occuring_doc_id[n_doc] =
                                    indices.words[i].occurs[l].doc_id;
                            ++n_doc;
                        }
                    }
                }
            }
            printf("Number of \"%s %s %s\" occurences is %i",
                   words[i],
                   words[i + 1],
                   words[i + 2],
                   n_occurs);
            if (n_occurs > 0) {
                printf(" [docID");
                for (int j = 0; j < n_doc; ++j) {
                    printf(" %i", occuring_doc_id[j]);
                }
                printf("]");
            }
            printf("\n");
            if (n_occurs > 0) {
                int cnt1 = 0;
                int cnt2 = 0;
                // if (i) printf("...");
            }
        }
        // bigrams occurence
        for (int i = 0; i < n_words - 1; ++i) {
            int n_occurs       = 0;
            int is_first       = 1;
            int target_doc_id  = -1;
            int target_line_id = -1;
            int word_id        = -1;
            int n_doc          = 0;
            for (int i = 0; i < n_files; ++i) {
                occuring_doc_id[i] = -1;
            }
            for (int j = 0; j < bigrams.size; ++j) {
                if (strcmp(bigrams.grams[j].word1, words[i + 0]) == 0 &&
                    strcmp(bigrams.grams[j].word2, words[i + 1]) == 0) {
                    int n_cur_occurs = indices.words[j].n_occurs;
                    n_occurs += n_cur_occurs;
                    if (is_first == 1 && bigrams.grams[j].n_occurs > 0) {
                        target_doc_id  = bigrams.grams[j].occurs[0].doc_id;
                        target_line_id = bigrams.grams[j].occurs[0].line_id;
                        word_id        = bigrams.grams[j].occurs[0].word_id;
                    }
                    is_first += 1;

                    for (int l = 0; l < n_cur_occurs; ++l) {
                        int found = 0;
                        for (int k = 0; k < n_doc; ++k) {
                            if (occuring_doc_id[k] ==
                                indices.words[j].occurs[l].doc_id) {
                                found = 1;
                                break;
                            }
                        }
                        if (n_doc == n_files) {
                            break;
                        }
                        if (!found) {
                            occuring_doc_id[n_doc] =
                                    indices.words[i].occurs[l].doc_id;
                            ++n_doc;
                        }
                    }
                }
            }
            printf("Number of \"%s %s\" occurences is %i",
                   words[i],
                   words[i + 1],
                   n_occurs);
            if (n_occurs > 0) {
                printf(" [docID");
                for (int j = 0; j < n_doc; ++j) {
                    printf(" %i", occuring_doc_id[j]);
                }
                printf("]");
            }
            printf("\n");
            if (n_occurs > 0) {
                int cnt1 = 0;
                int cnt2 = 0;
                // if (i) printf("...");
            }
        }
