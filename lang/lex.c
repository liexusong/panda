
#include "lex.h"

#define CURR_CH     (lex->curr_ch)
#define NEXT_CH     (lex->next_ch)

static int to_number(const char *s, int len)
{
    int ret = 0;
    int i;

    for (i = 0; i < len; i++) {
        ret = ret * 10 + (s[i] - '0');
    }

    return ret;
}

static void lex_get_next_ch(lexer_t *lex)
{
    if (lex->line_pos >= lex->line_end) {
        int len = lex->getline(lex->line_buf, LEX_LINE_BUF_SIZE);

        if (len <= 0) {
            len = 1;
            lex->line_buf[0] = 0;
        }
        lex->line_end = len;
        lex->line_pos = 0;
        // for debug
        // lex->line_buf[len] = 0;
        // printf("get line(%d): %s\n", len, lex->line_buf);
    }

    if (lex->curr_ch == '\n') {
        lex->line++;
        lex->col = 0;
    } else {
        lex->col++;
    }

    lex->curr_ch = lex->next_ch;
    lex->next_ch = lex->line_buf[lex->line_pos ++];

    // printf("get a char: %c\n", lex->curr_ch);
}

static int lex_chk_token_type(const char *str, int len)
{
    switch(len) {
    case 2:
        if (0 == strcmp("if", str)) return TOK_IF;
        if (0 == strcmp("in", str)) return TOK_IN;
    case 3:
        if (0 == strcmp("def", str)) return TOK_DEF;
        if (0 == strcmp("var", str)) return TOK_VAR;
        if (0 == strcmp("NaN", str)) return TOK_NAN;
    case 4:
        if (0 == strcmp("else", str)) return TOK_ELSE;
        if (0 == strcmp("elif", str)) return TOK_ELIF;
        if (0 == strcmp("true", str)) return TOK_TRUE;
        if (0 == strcmp("null", str)) return TOK_NULL;
    case 5:
        if (0 == strcmp("false", str)) return TOK_FALSE;
        if (0 == strcmp("while", str)) return TOK_WHILE;
        if (0 == strcmp("break", str)) return TOK_BREAK;
    case 6:
        if (0 == strcmp("return", str)) return TOK_RET;
    case 8:
        if (0 == strcmp("continue", str)) return TOK_CONTINUE;
    case 9:
        if (0 == strcmp("undefined", str)) return TOK_UND;
    default:
        return TOK_ID;
    }
}

static void lex_eat_comments(lexer_t *lex)
{
    if ('#' == CURR_CH) {
        do {
            lex_get_next_ch(lex);
        } while (CURR_CH && '\n' != CURR_CH && '\r' != CURR_CH);
    } else
    if ('/' == NEXT_CH) {
        do {
            lex_get_next_ch(lex);
        } while (CURR_CH && '\n' != CURR_CH && '\r' != CURR_CH);
    } else
    if ('*' == NEXT_CH) {
        lex_get_next_ch(lex);
        lex_get_next_ch(lex);
        do {
            lex_get_next_ch(lex);
        } while (CURR_CH && !('*' == CURR_CH && '/' == NEXT_CH));
        lex_get_next_ch(lex);
        lex_get_next_ch(lex);
    }
}

static void lex_get_id_token(lexer_t *lex)
{
    int len  = 0;

    do {
        if (len < LEX_TOK_SIZE) {
            lex->token_str_buf[len++] = CURR_CH;
        }
        lex_get_next_ch(lex);
    } while (isalnum(CURR_CH) || '_' == CURR_CH || '$' == CURR_CH);

    lex->token_str_buf[len] = 0;
    lex->token_str_len = len;
    lex->curr_tok = lex_chk_token_type(lex->token_str_buf, len);
}

static void lex_get_str_token(lexer_t *lex)
{
    int term = CURR_CH;
    int len  = 0;

    // Eat the head ' Or "
    lex_get_next_ch(lex);

    while (CURR_CH != term) {
        if (len < LEX_TOK_SIZE) {
            lex->token_str_buf[len++] = CURR_CH;
        }
        lex_get_next_ch(lex);
    }

    // Eat the tail ' Or "
    lex_get_next_ch(lex);

    lex->token_str_buf[len] = 0;
    lex->token_str_len = len;
    lex->curr_tok = TOK_STR;
}

static void lex_get_num_token(lexer_t *lex)
{
    int len  = 0;

    do {
        if (len < LEX_TOK_SIZE) {
            lex->token_str_buf[len++] = CURR_CH;
        }
        lex_get_next_ch(lex);
    } while (isnumber(CURR_CH));

    lex->token_str_buf[len] = 0;
    lex->token_str_len = len;
    lex->curr_tok = TOK_NUM;
}

static void lex_get_next_token(lexer_t *lex)
{
    int tok;

TOKEN_LOCATE:
    // eat space
    while (isspace(CURR_CH)) {
        lex_get_next_ch(lex);
    }

    tok = CURR_CH;
    // eat comments
    if (tok == '#' || (tok == '/' && (NEXT_CH == '/' || NEXT_CH == '*'))) {
        lex_eat_comments(lex);
        goto TOKEN_LOCATE;
    }

    if (isalpha(tok) || '_' == tok || '$' == tok) {
        lex_get_id_token(lex);
    } else
    if ('\'' == tok || '"' == tok) {
        lex_get_str_token(lex);
    } else
    if (isnumber(tok)) {
        lex_get_num_token(lex);
    } else {
        if (tok) {
            lex_get_next_ch(lex);

            if (CURR_CH == '=') {
                switch (tok) {
                case '!': lex_get_next_ch(lex); tok = TOK_NE; break;
                case '>': lex_get_next_ch(lex); tok = TOK_GE; break;
                case '<': lex_get_next_ch(lex); tok = TOK_LE; break;
                case '=': lex_get_next_ch(lex); tok = TOK_EQ; break;

                case '-': lex_get_next_ch(lex); tok = TOK_SUBASSIGN; break;
                case '+': lex_get_next_ch(lex); tok = TOK_ADDASSIGN; break;
                case '*': lex_get_next_ch(lex); tok = TOK_MULASSIGN; break;
                case '/': lex_get_next_ch(lex); tok = TOK_DIVASSIGN; break;
                case '%': lex_get_next_ch(lex); tok = TOK_MODASSIGN; break;
                case '&': lex_get_next_ch(lex); tok = TOK_ANDASSIGN; break;
                case '|': lex_get_next_ch(lex); tok = TOK_ORASSIGN; break;
                case '^': lex_get_next_ch(lex); tok = TOK_XORASSIGN; break;
                case '~': lex_get_next_ch(lex); tok = TOK_NOTASSIGN; break;
                default: break;
                }
            } else
            if (tok == '&' && CURR_CH == '&') {
                lex_get_next_ch(lex);
                tok = TOK_LOGICAND;
            } else
            if (tok == '|' && CURR_CH == '|') {
                lex_get_next_ch(lex);
                tok = TOK_LOGICOR;
            } else
            if (tok == '>' && CURR_CH == '>') {
                lex_get_next_ch(lex);
                if (CURR_CH == '=') {
                    lex_get_next_ch(lex);
                    tok = TOK_RSHIFTASSIGN;
                } else {
                    tok = TOK_RSHIFT;
                }
            } else
            if (tok == '<' && CURR_CH == '<') {
                lex_get_next_ch(lex);
                if (CURR_CH == '=') {
                    lex_get_next_ch(lex);
                    tok = TOK_LSHIFTASSIGN;
                } else {
                    tok = TOK_LSHIFT;
                }
            }
        } else {
            // Token is Eof
        }

        lex->curr_tok = tok;
    }
}

intptr_t lex_init(lexer_t *lex, int (*getline)(void *, int))
{
    if (lex && getline) {
        lex->line_end = 0;
        lex->line_pos = 0;
        lex->getline = getline;
        lex->curr_tok = TOK_EOF;
        lex->token_str_len = 0;

        lex->line = 0;
        lex->col = 0;
        lex_get_next_ch(lex);
        lex_get_next_ch(lex);
        lex_get_next_token(lex);

        return (intptr_t) lex;
    }

    return 0;
}

int lex_deinit(lexer_t *lex)
{
    return 0;
}

int lex_token(intptr_t l, token_t *tok)
{
    lexer_t *lex = (lexer_t *)l;

    if (tok) {
        tok->type = lex->curr_tok;
        tok->line = lex->line;
        tok->col  = lex->col;
        tok->text = lex->token_str_buf;

        if (tok->type == TOK_ID || tok->type == TOK_STR) {
            tok->value = lex->token_str_len;
        } else
        if (tok->type == TOK_NUM) {
            tok->value = to_number(lex->token_str_buf, lex->token_str_len);
        }
    }

    return lex->curr_tok;
}

int lex_match(intptr_t l, int tok)
{
    lexer_t *lex = (lexer_t *)l;

    if (lex->curr_tok == tok) {
        lex_get_next_token(lex);
        return 1;
    } else {
        return 0;
    }
}
