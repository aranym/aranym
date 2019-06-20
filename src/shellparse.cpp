/*
 * shellparse.cpp - command line parsing for shell
 *
 * Copyright (c) 2019 ARAnyM developer team (see AUTHORS)
 *				 2019 Thorsten Otto
 *
 * This file is part of the ARAnyM project which builds a new and powerful
 * TOS/FreeMiNT compatible virtual machine running on almost any hardware.
 *
 * ARAnyM is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * ARAnyM is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ARAnyM; if not, see <http://www.gnu.org/licenses/>.
 */

#include "sysdeps.h"
#include <stdlib.h>
#include <string.h>
#include "shellparse.h"

#ifndef FALSE
# define FALSE 0
# define TRUE  1
#endif


/* parse a string into a list of arguments. Words are defined to be
 * (1) any sequence of non-blank characters
 * (2) any sequence of characters starting with a ', ", or ` and ending
 *     with the same character. These quotes are stripped off.
 *
 * Returns a single memory block that can be passed to free() to
 * deallocate both the argument vector and the strings.
 */

struct arg {
	struct arg *next;
	char name[1];
};

static int appendarg(struct arg **list, const char *name)
{
	struct arg *newarg;
	
	newarg = (struct arg *)malloc(sizeof(struct arg) + strlen(name));
	if (newarg == NULL)
		return FALSE;
	while (*list)
		list = &(*list)->next;
	strcpy(newarg->name, name);
	newarg->next = NULL;
	*list = newarg;
	return TRUE;
}


char **shell_parse(const char *cmd, int *pargc)
{
	char quote;
	char c;
	int argc;
	const char *p;
	int inarg;
	char **argv;
	char *cmdbuf;
	struct arg *arglist = NULL;
	struct arg *arg, *next;
	size_t len;
	int ret;
	size_t argalloc, arglen;

#define is_space(c) ((c) == ' ' || (c) == '\t' || (c) == '\n')

	if (cmd == NULL)
		return NULL;
	p = cmd;
	while (is_space(*p))
		p++;
	
	argalloc = strlen(p);
	cmdbuf = (char *)malloc(argalloc);
	if (cmdbuf == NULL)
		return NULL;
	
	quote = '\0';
	p = cmd;
	inarg = FALSE;
	arglen = 0;
#define append_c(c) \
	if (arglen >= argalloc) \
	{ \
		argalloc += argalloc; \
		cmdbuf = (char *)realloc(cmdbuf, argalloc); \
		if (cmdbuf == NULL) \
			break; \
	} \
	cmdbuf[arglen++] = c
#define isquote(c) ((c) == '\'' || (c) == '"' || (c) == '`')

	for (;;)
	{
		c = *p;
		if (c == quote)
		{
			if (c == '\0')
			{
				if (inarg)
				{
					append_c('\0');
					ret = appendarg(&arglist, cmdbuf);
					if (ret == FALSE)
						break;
				}
				break;
			}
			quote = '\0';
			p++;
		} else if (quote == '\0' && isquote(c))
		{
			quote = c;
			if (!inarg)
			{
				arglen = 0;
			}
			inarg = TRUE;
			p++;
		} else if (c == '\0' || (quote == '\0' && is_space(c)))
		{
			if (inarg)
			{
				append_c('\0');
				ret = appendarg(&arglist, cmdbuf);
				if (ret == FALSE)
					break;
			}
			inarg = FALSE;
			if (c == '\0')
				break;
			p++;
		} else
		{
			if (!inarg)
			{
				arglen = 0;
			}
			p++;
			if (c == '\\')
			{
				if (isquote(*p) || *p == '\\' || *p == '$' || *p == '*' || *p == '?' || *p == '[' || *p == ']')
					c = *p++;
			} else if (quote == '\0' && (c == '*' || c == '?' || c == '['))
			{
				/* dowild = TRUE; */
			} else
			{
			}
			append_c(c);
			inarg = TRUE;
		}
	}
	free(cmdbuf);
#undef append_c
#undef isquote
#undef is_space

	argc = 0;
	len = 0;
	for (arg = arglist; arg != NULL; arg = arg->next)
	{
		argc++;
		len += strlen(arg->name) + 1;
	}
	
	argv = (char **)malloc((argc + 1) * sizeof(char *) + len);
	if (argv != NULL)
	{
		char *q = (char *)(argv + argc + 1);
		argc = 0;

		for (arg = arglist; arg != NULL; arg = arg->next)
		{
			argv[argc++] = q;
			p = arg->name;
			while ((*q++ = *p++) != '\0')
				;
		}
		
		argv[argc] = NULL;
	}
	
	for (arg = arglist; arg != NULL; arg = next)
	{
		next = arg->next;
		free(arg);
	}
	
	if (pargc)
		*pargc = argc;
	return argv;
}


#ifdef MAIN
int main(int argc, char **argv)
{
	int i;
	
	if (argc > 1)
	{
		argv = shell_parse(argv[1], &argc);
		for (i = 0; i < argc; i++)
		{
			printf("%d: '%s'\n", i, argv[i]);
		}
	}
	
	return 0;
}
#endif
