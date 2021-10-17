/* This file contains functions that are not part of the visible "interface".
 * They are essentially helper functions.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simfs.h"

/* Internal helper functions first.
 */

FILE *
openfs(char *filename, char *mode)
{
    FILE *fp;
    if((fp = fopen(filename, mode)) == NULL) {
        perror("openfs");
        exit(1);
    }
    return fp;
}

void
closefs(FILE *fp)
{
    if(fclose(fp) != 0) {
        perror("closefs");
        exit(1);
    }
}

int space_finder(fnode fnodes[MAXBLOCKS]) {
    int space = 0;
    for (int i = 0; i < MAXBLOCKS; i ++) {
        if (fnodes[i].blockindex < 0) {
            space += BLOCKSIZE;
        }
    }

    return space;
}

int block_finder(fnode fnodes[MAXBLOCKS]) {
    for (int i = 0; i < MAXBLOCKS; i ++) {
        if (fnodes[i].blockindex < 0) {
            return i;
        }
    }
    return -1;
}

/* File system operations: creating, deleting, reading, and writing to files.
 */

void createfile(char *fsname, char *filename) {
    int length = strnlen(filename, 15);
    if (length > 11) {
        fprintf(stderr, "Error: Filename too big\n");
        exit(1);
    }
    FILE *fp;
    fp = openfs(fsname, "r+");
    fentry files[MAXFILES];
    if (fread(files, sizeof(fentry), MAXFILES, fp) == 0) {
        fprintf(stderr, "Error: Could not read file entries\n");
        closefs(fp);
        exit(1);
    }

    for (int i = 0; i < MAXFILES; i ++) {
        if (strncmp(files[i].name, filename, 11) == 0) {
            fprintf(stderr, "Error: File already exists\n");
            closefs(fp);
            exit(1);
        }
        if (files[i].name[0] == '\0') {
            strncpy(files[i].name, filename, 11);
            break;
        }
    }

    fseek(fp, 0, SEEK_SET);
    if(fwrite(files, sizeof(fentry), MAXFILES, fp) < MAXFILES) {
        fprintf(stderr, "Error: Write failed on init\n");
        closefs(fp);
        exit(1);
    }

    closefs(fp);

}

void deletefile(char *fsname, char *filename) {

    FILE *fp;
    fp = openfs(fsname, "r+");
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];

//    fseek(fp, 0, SEEK_SET);
    if (fread(files, sizeof(fentry), MAXFILES, fp) == 0) {
        fprintf(stderr, "Error: Could not read file entries\n");
        closefs(fp);
        exit(1);
    }

//    fseek(fp, MAXFILES * sizeof(fentry), SEEK_SET);
    if (fread(fnodes, sizeof(fnode), MAXBLOCKS, fp) == 0) {
        fprintf(stderr, "Error: Could not read file entries\n");
        closefs(fp);
        exit(1);
    }


    int count = 0;
    int block = -1;
    for (int i = 0; i < MAXFILES; i ++) {
        if (strncmp(filename, files[i].name, 11) == 0) {
            strncpy(files[i].name, "", 11);
            block = files[i].firstblock;
            files[i].firstblock = -1;
            files[i].size = 0;
            count ++;
            break;
        }
    }

    if (count == 0) {
        fprintf(stderr, "Error: File does not exist\n");
        closefs(fp);
        exit(1);
    }

    fseek(fp, 0, SEEK_SET);
    if(fwrite(files, sizeof(fentry), MAXFILES, fp) < MAXFILES) {
        fprintf(stderr, "Error: Write failed on init\n");
        closefs(fp);
        exit(1);
    }

//    if (block == -1) {
//        fprintf(stderr, "Error: file does not exist\n");
//        closefs(fp);
//        exit(1);
//    }
    int new_block;
    while (block != -1) {
        fnodes[block].blockindex = -1 * block;
//        fseek(fp, sizeof(fentry) * MAXFILES, SEEK_SET);
//        if (fwrite(fnodes, BLOCKSIZE, 1, fp) < 1) {
//            fprintf(stderr, "Error: write failed on init\n");
//            closefs(fp);
//            exit(1);
//        }

        fseek(fp, (block) * BLOCKSIZE, SEEK_SET);

        char zerobuf[BLOCKSIZE + 1] = {0};
        if (fwrite(zerobuf, BLOCKSIZE, 1, fp) < 1) {
            fprintf(stderr, "Error: write failed on init\n");
            closefs(fp);
            exit(1);
        }

        new_block = fnodes[block].nextblock;
        fnodes[block].nextblock = -1;
        block = new_block;
        fseek(fp, sizeof(fentry) * MAXFILES, SEEK_SET);
        if (fwrite(fnodes, BLOCKSIZE, 1, fp) < 1) {
            fprintf(stderr, "Error: write failed on init\n");
            closefs(fp);
            exit(1);
        }
    }
//    fseek(fp, 0, SEEK_SET);


    closefs(fp);
}

void readfile(char *fsname, char *filename, int start, int length) {
    FILE *fp;
    fp = openfs(fsname, "r+");
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];

    if (fread(files, sizeof(fentry), MAXFILES, fp) == 0) {
        fprintf(stdout, "Hello");
        fprintf(stderr, "Error: Could not read file entries\n");
        closefs(fp);
        exit(1);
    }

    if (fread(fnodes, sizeof(fnode), MAXBLOCKS, fp) == 0) {
        fprintf(stdout, "Hello");
        fprintf(stderr, "Error: Could not read file entries\n");
        closefs(fp);
        exit(1);
    }

    int block = -1;
    int count = 0;
    for (int i = 0; i < MAXFILES; i ++) {
        if (strncmp(filename, files[i].name, 11) == 0) {
            block = files[i].firstblock;
            if (start > files[i].size) {
                fprintf(stderr, "Error: Starting position is greater than file size\n");
                closefs(fp);
                exit(1);
            }
            if (length > files[i].size) {
                length = files[i].size;
//                length = length - start;
            }
            count ++;
            break;
        }
    }

    if (count == 0) {
        fprintf(stderr, "Error: File does not exist\n");
        closefs(fp);
        exit(1);
    }
    while (start >= BLOCKSIZE) {
        start -= BLOCKSIZE;
        length -= BLOCKSIZE;
        block = fnodes[block].nextblock;
    }

    while (length > 0) {

        fseek(fp, (block) * BLOCKSIZE + start, SEEK_SET);

        char var[BLOCKSIZE + 1];

        if (length > BLOCKSIZE) {
//            fprintf(stdout, "Hello");
            if (fread(var, sizeof(char), BLOCKSIZE - start, fp) == 0) {
                fprintf(stderr, "Error: Could not read file entries\n");
                closefs(fp);
                exit(1);
            }
		var[length - start] = '\0';
            fprintf(stdout, "%s", var);
        }

        else {
//            fprintf(stdout, "Hello");
            if (fread(var, sizeof(char), length - start, fp) == 0) {
                fprintf(stderr, "Error: Could not read file entries\n");
                closefs(fp);
                exit(1);
           }
		var[length - start] = '\0';
            fprintf(stdout, "%s", var);

        }
        for (int j = 0; j < BLOCKSIZE + 1; j ++) {
            var[j] = '\0';
        }
        length -= BLOCKSIZE;
        block = fnodes[block].nextblock;
        if (start != 0) {
            start = 0;
        }
    }

    closefs(fp);
}


void writefile(char *fsname, char *filename, int start, int length) {
    FILE *fp;
    fp = openfs(fsname, "r+");
    fentry files[MAXFILES];
    fnode fnodes[MAXBLOCKS];

    if (fread(files, sizeof(fentry), MAXFILES, fp) == 0) {
        fprintf(stderr, "Error: Could not read file entries\n");
        closefs(fp);
        exit(1);
    }

    if (fread(fnodes, sizeof(fnode), MAXBLOCKS, fp) == 0) {
        fprintf(stderr, "Error: Could not read file entries\n");
        closefs(fp);
        exit(1);
    }

    int check = 0;
    int block = -1;
    int write_length = -1;
    for (int i = 0; i < MAXFILES; i ++) {
        if (strncmp(filename, files[i].name, 11) == 0) {
            block = files[i].firstblock;
            int block_track = files[i].firstblock;
            if (start > files[i].size) {
                fprintf(stderr, "Error: Starting position is greater than file size\n");
                closefs(fp);
                exit(1);
            }
            if (block == -1 && length > space_finder(fnodes)) {
                fprintf(stderr, "Error: There is not enough space for your file\n");
                closefs(fp);
                exit(1);
            }
            int count = 0;
//            fprintf(stdout, "hello");
            if (block_track != -1) {
                while (fnodes[block_track].nextblock != -1) {
                    count ++;
                    block_track = fnodes[block_track].nextblock;
                }
            }

            if (block != -1 && length > space_finder(fnodes) + count * BLOCKSIZE) {
                fprintf(stderr, "Error: There is not enough space for your file\n");
                closefs(fp);
                exit(1);
            }
            if (length + start >= files[i].size) {
                files[i].size = length + start;
            }
            if (block == -1) {
                files[i].firstblock = block_finder(fnodes);
                block = files[i].firstblock;
            }
            check = 1;
            write_length = files[i].size - start;
            break;
        }
    }

    if (check == 0) {
        fprintf(stderr, "Error: File does not exist\n");
        closefs(fp);
        exit(1);
    }

    while (start >= BLOCKSIZE) {
        start -= BLOCKSIZE;
        length -= BLOCKSIZE;
        block = fnodes[block].nextblock;
    }

    char to_write[length + 1];
    if (fread(to_write, length + 1, 1,stdin) == 0) {
        fprintf(stderr, "Error: Could not write from stdin\n");
        closefs(fp);
        exit(1);
    }

    int s = 0;
    int before = 0;
    while (length > 0) {
//        if (block == -1) {
//            block = block_finder(fnodes);
//        }
        fseek(fp, (block) * BLOCKSIZE + start, SEEK_SET);
//        if (start != 0) {
//            start = 0;
//        }
        char in_write[BLOCKSIZE + 1];

        if (length > BLOCKSIZE) {
            for (int j = before; j < before + BLOCKSIZE; j++) {
                in_write[(j - before) % BLOCKSIZE] = to_write[j];
            }
            before = (s + 1) * (BLOCKSIZE) - start;
            if (fwrite(in_write, BLOCKSIZE - start, 1, fp) < 1) {
                fprintf(stderr, "Error: Write failed on init\n");
                closefs(fp);
                exit(1);
            }

        }
        else {
            for (int j = before; j < length + before; j++) {
                in_write[(j - before) % BLOCKSIZE] = to_write[j];
            }
            before = s * (BLOCKSIZE) - start;
            if (fwrite(in_write, length - start, 1, fp) < 1) {
                fprintf(stderr, "Error: Write failed on init\n");
                closefs(fp);
                exit(1);
            }
            fseek(fp, (block) * BLOCKSIZE + length, SEEK_SET);
            if (write_length <= 0) {
                char zerobuf[BLOCKSIZE] = {0};
                if (fwrite(zerobuf, BLOCKSIZE - length, 1, fp) < 1) {
                    fprintf(stderr, "Error: write failed on init\n");
                    closefs(fp);
                    exit(1);
                }
            }
        }

        length = length - (BLOCKSIZE - start);
        write_length = length - (BLOCKSIZE - start);

        if (start != 0) {
            start = 0;
        }
        fnodes[block].blockindex = block;
        if (length > 0 && fnodes[block].nextblock == -1) {
            fnodes[block].nextblock = block_finder(fnodes);
        }
        block = fnodes[block].nextblock;
        s ++;

    }

    fseek(fp, 0, SEEK_SET);
    if(fwrite(files, sizeof(fentry), MAXFILES, fp) < MAXFILES) {
        fprintf(stderr, "Error: Write failed on init\n");
        closefs(fp);
        exit(1);
    }

    fseek(fp, sizeof(fentry) * MAXFILES, SEEK_SET);
    if(fwrite(fnodes, sizeof(fnode), MAXBLOCKS, fp) < MAXBLOCKS) {
        fprintf(stderr, "Error: Write failed on init\n");
        closefs(fp);
        exit(1);
    }

    closefs(fp);

}

// Signatures omitted; design as you wish.
