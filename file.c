/*
 * @(#)$Id: file.c,v 1.17 2000/04/09 04:30:14 twitham Exp $
 *
 * Copyright (C) 1996 - 2000 Tim Witham <twitham@quiknet.com>
 *
 * (see the files README and COPYING for more details)
 *
 * This file implements the file and command-line I/O
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "oscope.h"		/* program defaults */
#include "display.h"		/* display routines */
#include "func.h"		/* signal math functions */
#include "proscope.h"		/* probescope */

/* force num to stay within the range lo - hi */
int
limit(int num, int lo, int hi)
{
  if (num < lo)
    num = lo;
  if (num > hi)
    num = hi;
  return(num);
}

/* parse command-line or file opt character, using given optarg string */
void
handle_opt(int opt, char *optarg)
{
  char *p, *q;
  Channel *s;

  switch (opt) {
  case 'r':			/* sample rate */
  case 'R':
    scope.rate = limit(strtol(optarg, NULL, 0), 8000, 44100);
    break;
  case 's':			/* scale (zoom) */
  case 'S':
    scope.scale = limit(strtol(p = optarg, NULL, 0), 1, 100);
    if ((q = strchr(p, '/')) != NULL)
      scope.div = limit(strtol(++q, NULL, 0), 1, 100);
    break;
  case 't':			/* trigger */
  case 'T':
      scope.trig = limit(strtol(p = optarg, NULL, 0) + 128, 0, 255);
      if ((q = strchr(p, ':')) != NULL) {
	scope.trige = limit(strtol(++q, NULL, 0), 0, 2);
	p = q;
      }
      if ((q = strchr(p, ':')) != NULL) {
	if (*(++q) == 'x' || *q == 'X')
	  scope.trigch = 0;
	else if (*q == 'y' || *q == 'Y')
	  scope.trigch = 1;
	else
	  scope.trigch = limit(strtol(q, NULL, 0) - 1, 0, 1);
      }
    break;
  case 'c':			/* graticule color */
  case 'C':
    scope.color = limit(strtol(optarg, NULL, 0), 0, 15);
    break;
  case 'm':			/* video mode */
  case 'M':
    scope.size = limit(strtol(optarg, NULL, 0), 0, 3);
    break;
  case 'd':			/* dma divisor */
  case 'D':
    scope.dma = limit(strtol(optarg, NULL, 0), 1, 4);
    break;
  case 'f':			/* font name */
  case 'F':
    strcpy(fontname, optarg);
    break;
  case 'p':			/* plotting mode */
  case 'P':
    scope.mode = limit(strtol(optarg, NULL, 0), 0, 3);
    break;
  case 'g':			/* graticule on/off */
  case 'G':
    scope.grat = limit(strtol(optarg, NULL, 0), 0, 2);
    break;
  case 'b':			/* behind/front */
  case 'B':
    scope.behind = !DEF_B;
    break;
  case 'v':			/* verbose display */
  case 'V':
    scope.verbose = !DEF_V;
    break;
  case 'x':			/* sound card */
  case 'X':
  case 'y':
  case 'Y':
    snd = DEF_X;
    break;
  case 'z':			/* ProbeScope */
  case 'Z':
    ps.found = DEF_Z;
    break;
  case 'a':			/* Active (selected) channel */
  case 'A':
    scope.select = limit(strtol(optarg, NULL, 0) - 1, 0, CHANNELS - 1);
    break;
  case 'h':			/* help */
  case 'H':
    usage(0);
    break;
  case ':':			/* unknown option */
  case '?':
    usage(1);
    break;
  default:			/* channel settings */
    if (opt >= '1' && opt <= '0' + CHANNELS) {
      s = &ch[opt - '1'];
      p = optarg;
      s->show = 1;
      if (*p == '+') {
	s->show = 0;
	p++;
      }
      s->pos = limit(-strtol(p, NULL, 0), -1280, 1280);
      if ((q = strchr(p, '.')) != NULL) {
	s->bits = limit(strtol(++q, NULL, 0), 0, 16);
	p = q;
      }
      if ((q = strchr(p, ':')) != NULL) {
	s->mult = limit(strtol(++q, NULL, 0), 1, 100);
	p = q;
      }
      if ((q = strchr(p, '/')) != NULL) {
	s->div = limit(strtol(++q, NULL, 0), 1, 100);
	p = q;
      }
      if ((q = strchr(p, ':')) != NULL) {
	if (*++q >= '0' && *q <= '9') {
	  if (*q > '0') {
	    s->func = limit(strtol(q, NULL, 0) - 1 + FUNC0,
			    FUNC0, funccount - 1);
	    s->mem = 0;
	  }
	} else if (*q >= 'a' && *q <= 'z'
		   && (*(q + 1) == '\0' || *(q + 1) == '\n')) {
	  s->signal = &mem[*q - 'a'];
	  s->func = *q >= 'x' ? *q - 'x' : FUNCMEM;
	  s->mem = *q;
	} else {
	  s->func = FUNCEXT;
	  strcpy(s->command, q);
	  if ((p = strchr(s->command, '\n')) != NULL)
	    *p = '\0';
	  s->mem = EXTSTART;
	}
      }
    }
    break;
  }
}

/* write scope settings and memory buffers to given filename */
void
writefile(char *filename)
{
  FILE *file;
  int i, j, k = 0, chan[26], roloc[256];
  Channel *p;

  if ((file = fopen(filename, "w")) == NULL) {
    sprintf(error, "%s: can't write %s", progname, filename);
    message(error, KEY_FG);
    perror(error);
    return;
  }
  fprintf(file, "# %s, version %s, %dx%d
#
# -a %d
# -r %d
# -s %d/%d
# -t %d:%d:%c
# -c %d
# -d %d
# -m %d
# -p %d
# -g %d
%s%s%s%s",
	  progname, version, h_points, v_points,
	  scope.select + 1,
	  scope.rate,
	  scope.scale, scope.div,
	  scope.trig - 128, scope.trige, scope.trigch + 'x',
	  scope.color,
	  scope.dma,
	  scope.size,
	  scope.mode,
	  scope.grat,
	  scope.behind ? "# -b\n" : "",
	  scope.verbose ? "# -v\n" : "",
	  snd ? "" : "# -x\n",
	  ps.found ? "" : "# -z\n");
  for (i = 0 ; i < CHANNELS ; i++) {
    p = &ch[i];
    fprintf(file, "# -%d %s%d.%d:%d/%d:", i + 1, p->show ? "" : "+",
	    -p->pos, p->bits, p->mult, p->div);
    if (p->func <= FUNCMEM)
      fprintf(file, "%c", (p->mem >= 'a' && p->mem <= 'z') ? p->mem : '0');
    else if (p->func == FUNCEXT)
      fprintf(file, "%s", p->command);
    else
      fprintf(file, "%d", i > 1 ? (p->func - FUNC0 + 1) : 0);
    fprintf(file, "\n");
  }
  for (i = 0 ; i < 23 ; i++) {
    if (mem[i].color != 0)
      chan[k++] = i;
  }
  if (k) {
    for (i = 0 ; i < 16 ; i++) { /* reverse color mapper */
      roloc[color[i]] = i;
    }
    for (i = 0 ; i < k ; i++) {
      fprintf(file, "%s%c(%02d)", i ? "\t" : "# ",
	      chan[i] + 'a', roloc[mem[chan[i]].color]);
    }
    for (i = 0 ; i < k ; i++) {
      fprintf(file, "%s%d", i ? "\t" : "\n#:", mem[chan[i]].rate);
    }
    fprintf(file, "\n");
    for (j = 0 ; j < h_points ; j++) {
      for (i = 0 ; i < k ; i++) {
	fprintf(file, "%s%d", i ? "\t" : "", mem[chan[i]].data[j]);
      }
      fprintf(file, "\n");
    }
  }
  fclose(file);
  sprintf(error, "wrote %s", filename);
  message(error, TEXT_FG);
}

/* read scope settings and memory buffers from given filename */
void
readfile(char *filename)
{
  FILE *file;
  char c, *p, *q, buff[256];
  int i = 0, j = 0, k, valid = 0, chan[26] =
  {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
   -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1};

  if ((file = fopen(filename, "r")) == NULL) {
    sprintf(error, "%s: can't read %s", progname, filename);
    message(error, KEY_FG);
    perror(error);
    return;
  }
  init_scope();			/* reset everything */
  init_channels();
  init_math();
  init_screen();
  while (fgets(buff, 256, file)) {
    if (buff[0] == '#') {
      if (!strncmp("# -", buff, 3)) {
	valid = 1;
	handle_opt(buff[3], &buff[5]);
      } else if (valid && buff[3] == '(' && buff[6] == ')') {
	j = 0;
	q = buff + 2;
	while (j < 26 && (sscanf(q, "%c(%d) ", &c, &k) == 2)) {
	  if (c >= 'a' && c <= 'z' && k >= 0 && k <= 255) {
	    chan[j++] = c - 'a';
	    mem[c - 'a'].color = color[k];
	  }
	  q += 6;
	}
      } else if (!strncmp("#:", buff, 2)) {
	j = 0;
	q = buff + 2;
	while (q && j < 26 && (sscanf(q, "%d ", &k) == 1)) {
	  mem[chan[j++]].rate = k;
	  q = strchr(++q, '\t');
	}
      }
    } else if (valid &&
	       ((buff[0] >= '0' && buff[0] <= '9') || buff[0] == '-')) {
      j = 0;
      p = buff;
      q = p;
      while (j < 26 && (!j || ((p = strchr(q, '\t')) != NULL))) {
	if (p == NULL)
	  p = buff;
	if (chan[j] > -1 && i < MAXWID)
	  mem[chan[j]].data[i] = strtol(p++, NULL, 0);
	j++;
	q = p;
      }
      i++;
    }
  }
  fclose(file);
  if (valid) {			/* recall any memories to channels */
    j = scope.select;
    for (i = 0 ; i < CHANNELS ; i++) {
      c = ch[i].mem;
      scope.select = i;
      if (c >= 'a' && c <= 'z')
	recall(c);
    }
    scope.select = j;
    clear();
    sprintf(error, "loaded %s", filename);
    message(error, TEXT_FG);
  } else {
    sprintf(error, "invalid format: %s", filename);
    message(error, KEY_FG);
  }
}
