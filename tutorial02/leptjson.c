#include "leptjson.h"
#include <assert.h>  /* assert() */
#include <ctype.h>  // isspace
#include <errno.h>  // errno, ERANGE
#include <math.h>  // HUGE_VAL
#include <stdlib.h>  /* NULL, strtod() */

#define EXPECT(c, ch)       do { assert(*c->json == (ch)); c->json++; } while(0)
#define ISDIGIT(ch) ((ch) >= '0' && (ch) <= '9')
#define ISDIGIT1TO9(ch) ((ch) >= '1' && (ch) <= '9')

typedef struct {
    const char* json;
}lept_context;

static void lept_parse_whitespace(lept_context* c) {
    const char *p = c->json;
    while (isspace(*p))
        p++;
    c->json = p;
}

// literal : null
//         | true
//         | false
static int lept_parse_literal(lept_context* c, lept_value* v, const char* literal, lept_type type) {
    EXPECT(c, literal[0]);
    size_t i;
    for (i = 0; literal[i + 1]; ++i)
        if (c->json[i] != literal[i + 1])
            return LEPT_PARSE_INVALID_VALUE;
    c->json += i;
    v->type = type;
    return LEPT_PARSE_OK;
}

// number : ["-"] int [frac] [exp]
// int    : 0
//        | ["1"-"9"] ["0"-"9"]*
// frac   : "." ["0"-"9"]+
// exp    : ("e" | "E") ["-" | "+"] ["0"-"9"]+
static int lept_parse_number(lept_context* c, lept_value* v) {
    // validate number
    const char* p = c->json;
    // negative
    if (*p == '-') ++p;
    // int
    if (*p == '0') {
        ++p;
    } else if (ISDIGIT1TO9(*p)) {
        do {
            ++p;
        } while (ISDIGIT(*p));
    } else {
        goto invalid;
    }
    // frac
    if (*p == '.') {
        ++p;
        if (!ISDIGIT(*p))
            goto invalid;
        do {
            ++p;
        } while (ISDIGIT(*p));
    }
    // exp
    if (*p == 'e' || *p == 'E') {
        ++p;
        if (*p == '-' || *p == '+')
            ++p;
        if (!ISDIGIT(*p))
             goto invalid;
        do {
            ++p;
        } while (ISDIGIT(*p));
    }

    errno = 0;
    double res = strtod(c->json, NULL);
    if (errno == ERANGE && (res == HUGE_VAL || res == -HUGE_VAL))
        return LEPT_PARSE_NUMBER_TOO_BIG;
    v->n = res;
    c->json = p;
    v->type = LEPT_NUMBER;
    return LEPT_PARSE_OK;

invalid:
    return LEPT_PARSE_INVALID_VALUE;
}

static int lept_parse_value(lept_context* c, lept_value* v) {
    switch (*c->json) {
        case 't':  return lept_parse_literal(c, v, "true", LEPT_TRUE);
        case 'f':  return lept_parse_literal(c, v, "false", LEPT_FALSE);
        case 'n':  return lept_parse_literal(c, v, "null", LEPT_NULL);
        case '\0': return LEPT_PARSE_EXPECT_VALUE;
        default:   return lept_parse_number(c, v);
    }
}

int lept_parse(lept_value* v, const char* json) {
    lept_context c;
    int ret;
    assert(v != NULL);
    c.json = json;
    v->type = LEPT_NULL;
    lept_parse_whitespace(&c);
    if ((ret = lept_parse_value(&c, v)) == LEPT_PARSE_OK) {
        lept_parse_whitespace(&c);
        if (*c.json != '\0') {
            v->type = LEPT_NULL;
            ret = LEPT_PARSE_ROOT_NOT_SINGULAR;
        }
    }
    return ret;
}

lept_type lept_get_type(const lept_value* v) {
    assert(v != NULL);
    return v->type;
}

double lept_get_number(const lept_value* v) {
    assert(v != NULL && v->type == LEPT_NUMBER);
    return v->n;
}
