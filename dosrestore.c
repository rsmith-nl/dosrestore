/* file: dosrestore.c
 * vim:fileencoding=utf-8:fdm=marker:ft=c
 *
 * This file is part of dosrestore, a program to unpack old DOS backups.
 * Use 'indent -kr -i8' to indent this code properly.
 *
 * Copyright © 2018 R.F. Smith <rsmith@xs4all.nl>.
 * SPDX-License-Identifier: MIT
 * Created: 2003-06-22T12:18:31+0200
 * Last modified: 2025-04-21T16:23:46+0200
 */

/* If you don't want debugging or assertions, #define NDEBUG */
#define NDBG

#ifndef NULL
#define NULL (void*)0
#endif

#include "version.h"
#include <assert.h>
#include <errno.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>

const char id[9] = {0x8B, 0x42, 0x41, 0x43, 0x4B, 0x55, 0x50, 0x20, 0x20};
const char *bfname[2]= {"BACKUP", "backup"};
const char *cfname[2]= {"CONTROL", "control"};

char conbuf[1024];
char dirname[PATH_MAX];
char restfilename[PATH_MAX];
unsigned totalsize = 0;
FILE *control = NULL, *backup = NULL;
unsigned char *restbuf = NULL;
unsigned restsize = 0;

/* Open control file #num */
int open_control(char *dir, int num);

/* Open backup file #num */
int open_backup(char *dir, int num);

/* Read size bytes from the control file into conbuf. */
int read_con_buf(size_t size);

/* Read and process a file record from control. */
int process_file_rec(void);

/* Read and process a directory record from control. */
int process_dir_rec(void);

/* Read an unsigned integer from conbuf at offset. */
unsigned buf2uint(unsigned offset);


int main(int argc, char *argv[])
{
  int disknum = 1;
  int enter;
  int c;
  if (argc != 2) {
    fprintf(stderr, "%s version %s\n", PACKAGE, VERSION);
    fprintf(stderr, "Usage: %s /path/to/backup\n\n", PACKAGE);
    fprintf(stderr, "This utility is in the public domain.");
    return 0;
  }
  while (disknum) {
    if (open_control(argv[1], disknum)) {
      fprintf(stderr, "Opening the control file failed.\n");
      return 1;
    }
    /* Check for a valid control file */
    if (read_con_buf(0x8B)) {
      fprintf(stderr, "Reading control header failed.\n");
      fclose(control);
      return 2;
    }
    if (strncmp(conbuf, id, 9) != 0) {
      fprintf(stderr,
              "File is not a backup control file.\n");
      fclose(control);
      return 3;
    }
    /* Open the backup file */
    if (open_backup(argv[1], disknum)) {
      fprintf(stderr, "Opening backup file failed.\n");
      fclose(control);
      return 4;
    }
    /* Scan the rest of the control file */
    while ((c=fgetc(control)) != EOF) {
      if (c == 0x46) {
        if (process_dir_rec()) {
          fclose(backup);
          fclose(control);
          return 5;
        }
      } else if (c == 0x22) {
        if (process_file_rec()) {
          fclose(backup);
          fclose(control);
          return 6;
        }
      } else {
        fprintf(stderr, "Unrecognized structure\n");
        fclose(backup);
        fclose(control);
        return 7;
      }
    }
    fclose(control);
    fclose(backup);
    if (restsize == 0) {
      disknum = 0;
    } else {
      disknum++;
      printf("Mount backup disk #%d at %s, "\
             "and press ENTER", disknum, argv[1]);
      do {
        enter = getchar();
      } while (enter != (int)'\n');
    }
  }
  return 0;
}

int open_control(char *dir, int num)
{
  char cname[PATH_MAX];
  int i;
  assert(num > 0);
  for (i=0; i<2; i++) {
    snprintf(cname, PATH_MAX, "%s/%s.%03d",
             dir, cfname[i], num);
    control = fopen(cname, "rb");
    if (control) {
      break;
    }
  }
  if (control == NULL) {
    return 1;
  }
  return 0;
}

int open_backup(char *dir, int num)
{
  char bname[PATH_MAX];
  int i;
  assert(num > 0);
  for (i=0; i<2; i++) {
    snprintf(bname, PATH_MAX, "%s/%s.%03d",
             dir, bfname[i], num);
    backup = fopen(bname, "rb");
    if (backup) {
      break;
    }
  }
  if (backup == NULL) {
    return 1;
  }
  return 0;
}

int read_con_buf(size_t size)
{
  int rv;
  assert(size <1024);
  memset(conbuf, 0, 1024);
  rv =  fread(conbuf, 1, size, control);
  if (rv != size) {
    return 1;
  }
  return 0;
}

int process_file_rec(void)
{
  unsigned size = 0, heresize = 0, sz;
  int toread;
  char fname[13];
  char completename[PATH_MAX];
  FILE *outfile;
  char buf[1024];
  int rv;
  if (read_con_buf(33)) {
    fprintf(stderr, "File structure too short\n");
    return 1;
  }
  memset(fname, 0, 13);
  strncpy(fname, conbuf, 12);
  snprintf(completename, PATH_MAX, "%s/%s", dirname, fname);
  size = buf2uint(16);
  heresize = buf2uint(26);
  toread = heresize;
  outfile = fopen(completename, "ab");
  if (outfile == NULL) {
    perror("Error opening output file:");
    return 2;
  }
  if (heresize < size) {
    if (restbuf) {
      fwrite(restbuf, 1, restsize, outfile);
      free(restbuf);
      restbuf = NULL;
      restsize = 0;
      if (ferror(outfile)) {
        perror("writing saved part");
        return 3;
      }
    } else {
      restsize = heresize;
      restbuf = malloc(heresize);
      if (restbuf == NULL) {
        perror("memory allocation failed");
        fclose(outfile);
        return 4;
      }
      fread(restbuf, 1, heresize, backup);
      return 0;
    }
  }
  printf("file : %s, %u bytes\n", completename, size);
  do {
    if (toread < 1024) {
      sz = toread;
    } else {
      sz = 1024;
    }
    rv = fread(buf, 1, sz, backup);
    if (ferror(backup)) {
      fprintf(stderr, "Error reading backup file\n");
      fclose(outfile);
      return 5;
    }
    toread -= rv;
    fwrite(buf, 1, rv, outfile);
    if (ferror(outfile)) {
      fclose(outfile);
      perror("Error writing output file:");
      return 6;
    }
  } while (toread > 0);
  fclose(outfile);
  return 0;
}

int process_dir_rec(void)
{
  int rc;
  char *pc = NULL;
  if (read_con_buf(69)) {
    fprintf(stderr, "Directory structure too short\n");
    return 1;
  }
  pc = conbuf;
  while (*pc != 0) {
    if (*pc == '\\') {
      *pc = '/';
    }
    pc++;
  }
  memset(dirname, 0, NAME_MAX);
  strncpy(dirname, conbuf, NAME_MAX-1);
  rc = mkdir(dirname, S_IRWXU|S_IRWXG|S_IRWXO);
  if (rc && errno != EEXIST) {
    perror("process_dir_rec");
    return 2;
  }
  printf("directory name %s\n", dirname);
  return 0;
}

unsigned buf2uint(unsigned offset)
{
  unsigned rv = 0;
  unsigned n, lim = offset - sizeof(unsigned);
  assert(offset >= sizeof(unsigned));
  for (n=offset; n > lim; n--) {
    rv = (rv<<8) + (unsigned char)conbuf[n];
  }
  return rv;
}

/* EOF dosrestore.c */
