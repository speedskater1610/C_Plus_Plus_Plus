#ifndef TRANSFORM_H
#define TRANSFORM_H

#include <stddef.h>

/**
 * idk chat gpt wrote somes docs for me bc I am lazy
 * 
 * 
 * transform_code
 * ----------------
 * Takes a source string containing function-like declarations using
 * the keywords `fn` / `let` and `end`, enforces nesting/usage rules,
 * and replaces every whole-word instance of `fn` or `let` with `auto`.
 *
 * Function declarations must be of the form:
 *     fn name ()
 *         ...
 *     end name
 *
 * Nested functions are allowed, but any inner function used outside
 * its enclosing parent scope or declared in conflicting scopes will
 * trigger an error (printed to stderr) and cause the function to
 * return an empty string.
 *
 * On error: diagnostics are printed to stderr and the return value is
 * a freshly allocated empty string ("").
 * On success: returns a newly allocated transformed string with all
 * whole-word `fn`/`let` replaced by `auto`.
 *
 * The caller is responsible for freeing the returned string.
 *
 * @param src  Null-terminated input string (e.g., file contents).
 * @return     Newly allocated transformed string (must free), or empty
 *             string on error.
 */
char *transform_code(const char *src);

#endif // TRANSFORM_H
