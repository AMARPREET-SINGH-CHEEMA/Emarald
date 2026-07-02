#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Prototype WebAssembly emitter for Emarald
 * Usage: wasm_target <out.wat>
 * Emits a minimal WebAssembly text module exporting `main` returning 0.
 */

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <out.wat>\n", argv[0]);
        return 1;
    }
    const char* out = argv[1];
    FILE* f = fopen(out, "w");
    if (!f) {
        perror("fopen");
        return 1;
    }
    fprintf(f, ";; Emarald prototype WASM module\n");
    fprintf(f, "(module\n");
    fprintf(f, "  (func $main (export \"main\") (result i32)\n");
    fprintf(f, "    i32.const 0\n");
    fprintf(f, "  )\n");
    fprintf(f, ")\n");
    fclose(f);
    printf("Wrote prototype WASM module: %s\n", out);
    return 0;
}
