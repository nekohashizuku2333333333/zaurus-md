#include "MdRichText.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int compare_files(const char *a, const char *b)
{
    FILE *left = fopen(a, "rb"), *right = fopen(b, "rb");
    char x[4096], y[4096];
    size_t nx, ny;
    int result = 0;
    if (!left || !right) {
        if (left) fclose(left);
        if (right) fclose(right);
        return 2;
    }
    do {
        nx = fread(x, 1, sizeof(x), left); ny = fread(y, 1, sizeof(y), right);
        if (nx != ny || memcmp(x, y, nx)) { result = 1; break; }
    } while (nx);
    if (ferror(left) || ferror(right)) result = 2;
    fclose(left); fclose(right);
    return result;
}

int main(int argc, char **argv)
{
    unsigned size = 0, capacity = 4096, n, offset;
    char *source, *html = NULL, *next;
    int result;
    if (argc == 4 && !strcmp(argv[1], "--compare")) return compare_files(argv[2], argv[3]);
    source = (char *)malloc(capacity);
    if (!source) return 2;
    while ((n = fread(source + size, 1, capacity - size, stdin)) != 0) {
        size += n;
        if (size == capacity) {
            if (capacity > 8u * 1024u * 1024u) { free(source); return 2; }
            capacity *= 2;
            next = (char *)realloc(source, capacity);
            if (!next) { free(source); return 2; }
            source = next;
        }
    }
    if (argc == 2) {
        result = md_task_offset(source, size, (unsigned)atoi(argv[1]), &offset);
        if (result) printf("%u\n", offset);
        free(source);
        return result ? 0 : 3;
    }
    result = md_rich_text(source, size, &html);
    if (!result) fputs(html, stdout);
    else fprintf(stderr, "render error: %d\n", result);
    free(html); free(source);
    return result ? 2 : 0;
}
