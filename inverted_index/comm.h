#ifndef COMM_H
#define COMM_H 

#ifndef MAX_LINE_LEN
#define MAX_LINE_LEN 1025
#endif

#ifndef MAX_WORD_LEN
#define MAX_WORD_LEN 30
#endif

char *createNewWord();
char *createNewLine();
int parseLine(char *line, char ***words_out_ptr);

int trimLine(const char *line_in, char *line_out);
void removeSpecialChars(char*);
#endif
