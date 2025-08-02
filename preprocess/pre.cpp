#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// --- data struct for fn declarations ---
typedef struct Function {
    char *name;
    int decl_pos;    // pos of the start of the function body (after the "fn name ()")
    int end_pos;     // pos right after the corresponding "end name"
    struct Function *parent;
} Function;

typedef struct FuncStackEntry {
    Function *func;
    struct FuncStackEntry *next;
} FuncStackEntry;

static int is_word_boundary(const char *s, int pos, int len) {
    // s[pos..pos+len-1] is a whole word
    if (pos > 0) {
        char before = s[pos - 1];
        if (isalnum((unsigned char)before) || before == '_') return 0;
    }
    char after = s[pos + len];
    if (after != '\0') {
        if (isalnum((unsigned char)after) || after == '_') return 0;
    }
    return 1;
}

static Function *find_enclosing_function(Function **funcs, int func_count, int idx) {
    Function *best = NULL;
    for (int i = 0; i < func_count; ++i) {
        Function *f = funcs[i];
        if (f->decl_pos <= idx && idx < f->end_pos) {
            if (best == NULL) {
                best = f;
            } else {
                // choose deeper: if f is child of best, pick f
                Function *p = f->parent;
                while (p) {
                    if (p == best) {
                        best = f;
                        break;
                    }
                    p = p->parent;
                }
            }
        }
    }
    return best;
}

static int is_descendant(Function *possible_descendant, Function *ancestor) {
    Function *cur = possible_descendant;
    while (cur) {
        if (cur == ancestor) return 1;
        cur = cur->parent;
    }
    return 0;
}

// The main transformation function
char *transform_code(const char *src) {
    if (!src) return strdup("");

    int len = strlen(src);
    // parse function declarations with nesting
    Function **funcs = NULL;
    int func_capacity = 0;
    int func_count = 0;
    FuncStackEntry *stack = NULL;

    int i = 0;
    while (i < len) {
        // skip whitespace
        if (isspace((unsigned char)src[i])) { i++; continue; }

        // Check for "fn" declaration
        if (strncmp(&src[i], "fn", 2) == 0 && is_word_boundary(src, i, 2)) {
            int j = i + 2;
            // skip spaces
            while (j < len && isspace((unsigned char)src[j])) j++;
            // read identifier
            int name_start = j;
            while (j < len && (isalnum((unsigned char)src[j]) || src[j] == '_')) j++;
            if (j == name_start) {
                fprintf(stderr, "ERROR - malformed function declaration (missing name) at pos %d\n", i); 
                goto error_cleanup;
            }
            int name_len = j - name_start;
            char *name = (char*)malloc(name_len + 1);
            memcpy(name, &src[name_start], name_len);
            name[name_len] = '\0';

            // skip whitespace
            while (j < len && isspace((unsigned char)src[j])) j++;
            // Expect "()"
            if (j + 1 >= len || src[j] != '(') {
                fprintf(stderr, "ERROR - expected '(' after function name '%s' at pos %d\n", name, i);
                free(name);
                goto error_cleanup;
            }
            // Simplistically skip until matching ')'
            j++; // after '('
            while (j < len && src[j] != ')') j++;
            if (j >= len || src[j] != ')') {
                fprintf(stderr, "ERROR - missing ')' in function declaration '%s' at pos %d\n", name, i);
                free(name);
                goto error_cleanup;
            }
            j++; // after ')'

            // make fn struct
            Function *f = (Function*)malloc(sizeof(Function));
            f->name = name;
            f->parent = stack ? stack->func : NULL;
            f->decl_pos = j; // body starts after the declaration
            f->end_pos = len; // temp - will set when we find matching end

            // push onto stack
            FuncStackEntry *entry = (FuncStackEntry*)malloc(sizeof(FuncStackEntry));
            entry->func = f;
            entry->next = stack;
            stack = entry;

            // store in array
            if (func_count == func_capacity) {
                func_capacity = func_capacity ? func_capacity * 2 : 8;
                funcs = (Function**)realloc(funcs, sizeof(Function*) * func_capacity);
            }
            funcs[func_count++] = f;

            i = j;
            continue;
        }

        // Check for "end" keyword closing
        if (strncmp(&src[i], "end", 3) == 0 && is_word_boundary(src, i, 3)) {
            int j = i + 3;
            while (j < len && isspace((unsigned char)src[j])) j++;
            // read identifier
            int name_start = j;
            while (j < len && (isalnum((unsigned char)src[j]) || src[j] == '_')) j++;
            if (j == name_start) {
                fprintf(stderr, "ERROR - malformed end (missing name) at pos %d\n", i);
                goto error_cleanup;
            }
            int name_len = j - name_start;
            char tmp_name[256];
            if (name_len >= (int)sizeof(tmp_name)) {
                fprintf(stderr, "ERROR - function name too long in end at pos %d\n", i);
                goto error_cleanup;
            }
            memcpy(tmp_name, &src[name_start], name_len);
            tmp_name[name_len] = '\0';

            if (!stack) {
                fprintf(stderr, "ERROR - 'end %s' without matching 'fn' at pos %d\n", tmp_name, i);
                goto error_cleanup;
            }
            Function *top = stack->func;
            if (strcmp(top->name, tmp_name) != 0) {
                fprintf(stderr, "ERROR - mismatched end. Expected end %s but got end %s at pos %d\n", top->name, tmp_name, i);
                goto error_cleanup;
            }

            // set end_pos to just before the end keyword (so body excludes the "end name" itself)
            top->end_pos = i;

            // clean up stack
            FuncStackEntry *old = stack;
            stack = stack->next;
            free(old);

            i = j;
            continue;
        }

        i++;
    }

    if (stack) {
        fprintf(stderr, "ERROR - unclosed function '%s'\n", stack->func->name);
        goto error_cleanup;
    }

    // declaration conflicts (same name in incompatible places)
    for (int a = 0; a < func_count; ++a) {
        for (int b = a + 1; b < func_count; ++b) {
            if (strcmp(funcs[a]->name, funcs[b]->name) == 0) {
                // if neither is ancestor of the other then error bc illigal
                if (!is_descendant(funcs[a], funcs[b]) && !is_descendant(funcs[b], funcs[a])) {
                    fprintf(stderr, "ERROR - function '%s' declared in incompatible scopes.\n", funcs[a]->name);
                    goto error_cleanup;
                }
            }
        }
    }

    // usage validation (calls) outside of a fn where a fn was called
    for (int fidx = 0; fidx < func_count; ++fidx) {
        Function *called = funcs[fidx];
        size_t name_len = strlen(called->name);
        for (int pos = 0; pos + (int)name_len < len; ++pos) {
            if (strncmp(&src[pos], called->name, name_len) == 0 && is_word_boundary(src, pos, (int)name_len)) {
                // look ahead for '(' after optional whitespace
                int k = pos + name_len;
                while (k < len && isspace((unsigned char)src[k])) k++;
                if (k < len && src[k] == '(') {
                    // it is a call/usage
                    Function *encloser = find_enclosing_function(funcs, func_count, pos);
                    if (called->parent) {
                        // inner function - allowed only if encloser is parent or descendant of parent (i.e., inside parent's body)
                        if (!encloser) {
                            fprintf(stderr, "ERROR - inner function '%s' used outside its enclosing parent '%s'.\n", called->name, called->parent->name);
                            goto error_cleanup;
                        }
                        if (!is_descendant(encloser, called->parent)) {
                            fprintf(stderr, "ERROR - inner function '%s' used outside its parent scope '%s'.\n", called->name, called->parent->name);
                            goto error_cleanup;
                        }
                    }
                    // is call is top level them no restriction
                }
            }
        }
    }

    // perform replacements of whole-word "fn" and "let" with "auto" for a more dynamic feel even though it is just a strongly typed lang anyways
    // will build output dynamically
    size_t out_cap = len * 2 + 1;
    char *out = (char*)malloc(out_cap);
    size_t out_pos = 0;
    i = 0;
    while (i < len) {
        if (strncmp(&src[i], "fn", 2) == 0 && is_word_boundary(src, i, 2)) {
            // replace with "auto"
            memcpy(&out[out_pos], "auto", 4);
            out_pos += 4;
            i += 2;
        } else if (strncmp(&src[i], "let", 3) == 0 && is_word_boundary(src, i, 3)) {
            memcpy(&out[out_pos], "auto", 4);
            out_pos += 4;
            i += 3;
        } else {
            out[out_pos++] = src[i++];
        }
        if (out_pos + 10 >= out_cap) {
            out_cap *= 2;
            out = (char*)realloc(out, out_cap);
        }
    }
    out[out_pos] = '\0';

    // cleanup
    for (int k = 0; k < func_count; ++k) {
        free(funcs[k]->name);
        free(funcs[k]);
    }
    free(funcs);
    return out;

error_cleanup:
    // free everything and return empty string
    for (int k = 0; k < func_count; ++k) {
        free(funcs[k]->name);
        free(funcs[k]);
    }
    free(funcs);
    while (stack) {
        FuncStackEntry *n = stack->next;
        free(stack);
        stack = n;
    }
    return strdup("");
}

