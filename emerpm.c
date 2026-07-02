#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PKG_DIR ".emerald_packages"

static void usage(const char* prog) {
    fprintf(stderr, "emerpm - Emarald package manager (prototype)\n");
    fprintf(stderr, "Usage:\n");
    fprintf(stderr, "  %s init <name> <version>      Create a simple package manifest\n", prog);
    fprintf(stderr, "  %s pack <path>                Pack directory into .emer (tar.gz)\n", prog);
    fprintf(stderr, "  %s install <pkg.emer>         Install package into %s\n", prog, PKG_DIR);
    fprintf(stderr, "  %s list                       List installed packages\n", prog);
}

static int cmd_init(int argc, char** argv) {
    if (argc < 4) {
        fprintf(stderr, "init requires <name> <version>\n");
        return 1;
    }
    const char* name = argv[2];
    const char* version = argv[3];
    FILE* f = fopen("emerald.toml", "w");
    if (!f) {
        perror("fopen");
        return 1;
    }
    fprintf(f, "name = \"%s\"\nversion = \"%s\"\n", name, version);
    fclose(f);
    printf("Created emerald.toml for %s %s\n", name, version);
    return 0;
}

static int cmd_pack(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "pack requires <path>\n");
        return 1;
    }
    const char* path = argv[2];
    /* Read manifest if present to name package */
    char name[128] = {0};
    char version[64] = {0};
    char manifest[1024];
    snprintf(manifest, sizeof(manifest), "%s/emerald.toml", path);
    FILE* mf = fopen(manifest, "r");
    if (mf) {
        /* crude parse */
        char line[256];
        while (fgets(line, sizeof(line), mf)) {
            if (strncmp(line, "name", 4) == 0) {
                char* p = strchr(line, '"');
                if (p) { p++; char* q = strchr(p, '"'); if (q) { *q='\0'; strncpy(name,p,sizeof(name)-1); }}
            }
            if (strncmp(line, "version", 7) == 0) {
                char* p = strchr(line, '"');
                if (p) { p++; char* q = strchr(p, '"'); if (q) { *q='\0'; strncpy(version,p,sizeof(version)-1); }}
            }
        }
        fclose(mf);
    }
    if (name[0] == '\0') strncpy(name, "package", sizeof(name)-1);
    if (version[0] == '\0') strncpy(version, "0.0.0", sizeof(version)-1);
    char outname[256];
    snprintf(outname, sizeof(outname), "%s-%s.emer", name, version);
    /* Use tar to create archive */
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "tar -czf %s -C %s .", outname, path);
    int r = system(cmd);
    if (r != 0) {
        fprintf(stderr, "tar failed (exit %d)\n", r);
        return 1;
    }
    printf("Created package %s\n", outname);
    return 0;
}

static int ensure_pkg_dir(void) {
    struct stat st;
    if (stat(PKG_DIR, &st) == 0) return 0;
    if (mkdir(PKG_DIR, 0755) != 0) {
        perror("mkdir");
        return 1;
    }
    return 0;
}

static int cmd_install(int argc, char** argv) {
    if (argc < 3) {
        fprintf(stderr, "install requires <pkg.emer>\n");
        return 1;
    }
    const char* pkg = argv[2];
    if (ensure_pkg_dir() != 0) return 1;
    char cmd[1024];
    snprintf(cmd, sizeof(cmd), "tar -xzf %s -C %s", pkg, PKG_DIR);
    int r = system(cmd);
    if (r != 0) {
        fprintf(stderr, "extract failed (exit %d)\n", r);
        return 1;
    }
    printf("Installed %s into %s\n", pkg, PKG_DIR);
    return 0;
}

static int cmd_list(int argc, char** argv) {
    if (ensure_pkg_dir() != 0) return 1;
    char cmd[256];
    snprintf(cmd, sizeof(cmd), "ls -1 %s 2>/dev/null || true", PKG_DIR);
    int r = system(cmd);
    (void)r;
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) { usage(argv[0]); return 1; }
    if (strcmp(argv[1], "init") == 0) return cmd_init(argc, argv);
    if (strcmp(argv[1], "pack") == 0) return cmd_pack(argc, argv);
    if (strcmp(argv[1], "install") == 0) return cmd_install(argc, argv);
    if (strcmp(argv[1], "list") == 0) return cmd_list(argc, argv);
    usage(argv[0]);
    return 1;
}
