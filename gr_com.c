/*
 * @(#)$Id: gr_com.c,v 1.2 2001/05/06 03:45:16 twitham Rel $
 *
 * Copyright (C) 1996 - 2001 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements pieces common to multiple gr_ user interfaces
 *
 */

#include <stdio.h>
#include <sys/stat.h>
#include "oscope.h"
#include "display.h"

/* get a file name for load (0) or save (1) */
void
LoadSaveFile(int save)
{
  char *s;
  struct stat buff;

  if ((s = GetFile(save ? "Save:" : "Load:", NULL, NULL, NULL)) == NULL) return;
  if (!save) {
    loadfile(s);
    return;
  }
  if (!stat(s, &buff)) {
    sprintf(error, "Overwrite existing file %s?", s);
    if (!(GetYesNo(error))) return;
  }
  savefile(s);
  return;
}

void
ExternCommand()
{
  char *s;

  if ((s = GetString("External command and args:",
		     ch[scope.select].command)) != NULL)
    startcommand(s);
}
