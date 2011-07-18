#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

int main(int argc, char *argv[])
{
    char base[100], opath[100], *name, *p;
    FILE *ifp, *ofp;
    int size, cnt;
	int32_t word;

    if (argc < 2 || argc > 3) {
        fprintf(stderr, "usage: bin2xbasic <infile> [ <outfile> ]\n");
        exit(1);
    }

    strcpy(base, argv[1]);
    if ((p = strrchr(base, '.')) != NULL)
        *p = '\0';

    if (argc >= 3)
        strcpy(opath, argv[2]);
    else {
        strcpy(opath, base);
        strcat(opath, ".bas");
    }

    if ((name = strrchr(base, '/')) != NULL)
        ++name;
    else
        name = base;

    if (!(ifp = fopen(argv[1], "rb"))) {
        fprintf(stderr, "error: can't open: %s\n", argv[1]);
        exit(1);
    }

    if (!(ofp = fopen(opath, "wb"))) {
        fprintf(stderr, "error: can't create: %s\n", argv[2]);
        exit(1);
    }

    fprintf(ofp, "dim %s_array() = {\n", name);

    size = cnt = 0;
 	while (fread(&word, sizeof(word), 1, ifp) == 1) {
        if (cnt != 0)
			fprintf(ofp, ",");
		fprintf(ofp, " 0x%08x", word);
		if (++cnt == 4) {
            putc('\n', ofp);
            cnt = 0;
        }
		++size;
    }

    if (cnt > 0)
        putc('\n', ofp);
    
    fprintf(ofp, "\
}\n\
def %s_size = %d\n\
", name, size);

    fclose(ifp);
    fclose(ofp);

    return 0;
}
