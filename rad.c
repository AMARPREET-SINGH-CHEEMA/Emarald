#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE 1024

static void trim(char* s) {
    // trim leading
    char* p = s;
    while (*p && (*p == ' ' || *p == '\t' || *p == '\r' || *p == '\n')) p++;
    if (p != s) memmove(s, p, strlen(p)+1);
    // trim trailing
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' || s[len-1] == '\r' || s[len-1] == '\n')) {
        s[len-1] = '\0';
        len--;
    }
}

int run_rad_file(const char* path) {
    FILE* f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "Could not open %s\n", path);
        return 1;
    }

    // Simple variable store for strings only
    typedef struct Var { char* name; char* value; struct Var* next; } Var;
    Var* vars = NULL;

    char line_raw[MAX_LINE];
    while (fgets(line_raw, sizeof(line_raw), f)) {
        char line[MAX_LINE];
        strncpy(line, line_raw, sizeof(line));
        trim(line);
        if (line[0] == '\0') continue;
        if (line[0] == '#') continue;
        if (strncmp(line, "//", 2) == 0) continue;

        // print("...")
        if (strncmp(line, "print(", 6) == 0) {
            char* p = strchr(line, '(');
            char* q = strrchr(line, ')');
            if (!p || !q) continue;
            *q = '\0';
            char* expr = p+1;
            trim(expr);
            // string literal
            if ((expr[0] == '"' && expr[strlen(expr)-1] == '"') || (expr[0] == '\'' && expr[strlen(expr)-1] == '\'')) {
                expr[strlen(expr)-1] = '\0';
                expr++;
                printf("%s\n", expr);
            } else {
                // variable
                Var* v = vars;
                while (v) {
                    if (strcmp(v->name, expr) == 0) {
                        printf("%s\n", v->value);
                        break;
                    }
                    v = v->next;
                }
                if (!v) printf("%s\n", expr); // fallback: print raw
            }
            continue;
        }

        // assignment: name = input("prompt") or name = "value"
        char* eq = strchr(line, '=');
        if (eq) {
            char lhs[MAX_LINE];
            strncpy(lhs, line, (eq-line)); lhs[eq-line] = '\0';
            trim(lhs);
            char rhs[MAX_LINE];
            strncpy(rhs, eq+1, sizeof(rhs)); rhs[sizeof(rhs)-1] = '\0';
            trim(rhs);

            // input("prompt")
            if (strncmp(rhs, "input(", 6) == 0) {
                char* p = strchr(rhs, '(');
                char* q = strrchr(rhs, ')');
                if (!p || !q) continue;
                *q = '\0';
                char* prompt = p+1;
                trim(prompt);
                if ((prompt[0] == '"' && prompt[strlen(prompt)-1] == '"') || (prompt[0] == '\'' && prompt[strlen(prompt)-1] == '\'')) {
                    prompt[strlen(prompt)-1] = '\0';
                    prompt++;
                }
                printf("%s", prompt);
                char buf[MAX_LINE];
                if (!fgets(buf, sizeof(buf), stdin)) buf[0] = '\0';
                trim(buf);
                // store var
                Var* v = vars;
                while (v) { if (strcmp(v->name, lhs)==0) break; v = v->next; }
                if (v) {
                    free(v->value);
                    v->value = strdup(buf);
                } else {
                    Var* n = malloc(sizeof(Var)); n->name = strdup(lhs); n->value = strdup(buf); n->next = vars; vars = n;
                }
                continue;
            }

            // string literal
            if ((rhs[0] == '"' && rhs[strlen(rhs)-1] == '"') || (rhs[0] == '\'' && rhs[strlen(rhs)-1] == '\'')) {
                rhs[strlen(rhs)-1] = '\0';
                char* val = rhs+1;
                Var* v = vars;
                while (v) { if (strcmp(v->name, lhs)==0) break; v = v->next; }
                if (v) {
                    free(v->value);
                    v->value = strdup(val);
                } else {
                    Var* n = malloc(sizeof(Var)); n->name = strdup(lhs); n->value = strdup(val); n->next = vars; vars = n;
                }
                continue;
            }

            // copy from other var
            Var* src = vars; Var* found = NULL;
            while (src) { if (strcmp(src->name, rhs)==0) { found = src; break; } src = src->next; }
            if (found) {
                Var* v = vars;
                while (v) { if (strcmp(v->name, lhs)==0) break; v = v->next; }
                if (v) {
                    free(v->value);
                    v->value = strdup(found->value);
                } else {
                    Var* n = malloc(sizeof(Var)); n->name = strdup(lhs); n->value = strdup(found->value); n->next = vars; vars = n;
                }
                continue;
            }

            // otherwise ignore
            continue;
        }

        // fallback: unrecognized line - ignore for now
    }

    // free vars
    Var* v = vars; while (v) { Var* n = v->next; free(v->name); free(v->value); free(v); v = n; }
    fclose(f);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <script.rad>\n", argv[0]);
        return 1;
    }
    return run_rad_file(argv[1]);
}
