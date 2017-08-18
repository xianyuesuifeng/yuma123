/* Structure describing a word-expansion run.*/
#define YANGCLI_WORDEXP_MAXPARAMS_NUM 1024
typedef struct {
    size_t we_wordc;            /* Count of words matched.  */
    char **we_wordv;            /* List of expanded words.  */
    int   *we_word_line_offset; /* List of offsets to words in the original line */ 
  } yangcli_wordexp_t;

int yangcli_wordexp (const char* words, yangcli_wordexp_t* pwordexp, int flags);
void yangcli_wordfree (yangcli_wordexp_t * pwordexp);
void yangcli_wordexp_dump(yangcli_wordexp_t * pwordexp);
