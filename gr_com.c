/*
 * @(#)$Id: gr_com.c,v 2.0 2008/12/17 17:35:46 baccala Exp $
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

  /* XXX recall previous command that was set */
  if ((s = GetString("External command and args:",
		     COMMAND)) != NULL)
    startcommand(s);
}
