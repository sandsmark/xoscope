/*
 * @(#)$Id: file.h,v 1.1 1996/03/10 01:36:45 twitham Rel $
 *
 * Copyright (C) 1996 Tim Witham <twitham@pcocd2.intel.com>
 *
 * (see the files README and COPYING for more details)
 *
 * file variables and routines available outside of file.c
 *
 */

void
handle_opt(char opt, char *optarg);

void
writefile(char *filename);

void
readfile(char *filename);
