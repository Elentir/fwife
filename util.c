/*
 *  util.c for Fwife
 * 
 *  Copyright (c) 2005-2007 by Miklos Vajna <vmiklos@frugalware.org>
 *  Copyright (c) 2006 by Alex Smith <alex@alex-smith.me.uk>
 *  Copyright (c) 2008 by Albar Boris <boris.a@cegetel.net>
 * 
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, 
 *  USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <libintl.h>
#include <unistd.h>

#include "setup.h"
#include "util.h"

#define VERSIONFILE "/etc/frugalware-release"

data_t *data_new(void)
{
	data_t *data=NULL;

	data = (data_t*)malloc(sizeof(data_t));
	if(data==NULL)
		return(NULL);
	data->name=NULL;
	data->data=NULL;
	return(data);
}

void *data_get(GList *config, char *title)
{
	int i;
	data_t *data;

	for (i=0; i<g_list_length(config); i++)
	{
		data = g_list_nth_data(config, i);
		if(!strcmp(title, data->name))
			return data->data;
	}
	return(NULL);
}

void data_put(GList **config, char *name, void *data)
{
	int i;
	data_t *dp;

	for (i=0; i<g_list_length(*config); i++)
	{
		dp = g_list_nth_data(*config, i);
		if(!strcmp(name, dp->name))
		{
			dp->data=data;
			return;
		}
	}

	dp = data_new();
	dp->name = name;
	dp->data = data;
	(*config) = g_list_append((*config), dp);
}

int copyfile(char *src, char *dest)
{
	FILE *in, *out;
	size_t len;
	char buf[4097];

	in = fopen(src, "r");
	if(in == NULL)
		return(1);

	out = fopen(dest, "w");
	if(out == NULL)
		return(1);

	while((len = fread(buf, 1, 4096, in)))
		fwrite(buf, 1, len, out);

	fclose(in);
	fclose(out);
	return(0);
}

int exit_perform(void)
{
	fw_system_interactive("/sbin/reboot");
	exit(1);
}

int makepath(char *path)
{
	char *orig, *str, *ptr;
	char full[PATH_MAX] = "";
	mode_t oldmask;

	oldmask = umask(0000);

	orig = strdup(path);
	str = orig;
	while((ptr = strsep(&str, "/")))
		if(strlen(ptr))
		{
			struct stat buf;

			strcat(full, "/");
			strcat(full, ptr);
			if(stat(full, &buf))
				if(mkdir(full, 0755))
				{
					free(orig);
					umask(oldmask);
					return(1);
				}
		}
	free(orig);
	umask(oldmask);
	return(0);
}

/* does the same thing as 'rm -rf' */
int rmrf(char *path)
{
	int errflag = 0;
	struct dirent *dp;
	DIR *dirp;
	char name[PATH_MAX];

	if(!unlink(path))
		return(0);
	else
	{
		if(errno == ENOENT)
			return(0);
		else if(errno == EPERM)
		{
			/* fallthrough */
		}
		else if(errno == EISDIR)
		{
			/* fallthrough */
		}
		else if(errno == ENOTDIR)
			return(1);
		else
			/* not a directory */
			return(1);

		if((dirp = opendir(path)) == (DIR *)-1)
			return(1);
		for(dp = readdir(dirp); dp != NULL; dp = readdir(dirp))
			if(dp->d_ino)
			{
				sprintf(name, "%s/%s", path, dp->d_name);
				if(strcmp(dp->d_name, "..") && strcmp(dp->d_name, "."))
					errflag += rmrf(name);
			}
		closedir(dirp);
		if(rmdir(path))
			errflag++;
		return(errflag);
	}
	return(0);
}

int umount_if_needed(char *sourcedir)
{
	FILE *fp;
	char line[PATH_MAX];
	char *dev=NULL;
	char *realdir;
	int i;

	realdir = g_strdup_printf("%s/%s", TARGETDIR, sourcedir);

	if ((fp = fopen("/proc/mounts", "r")) == NULL)
	{
		perror(_("Could not open output file for writing"));
		return(1);
	}
	while(!feof(fp))
	{
		if(fgets(line, 256, fp) == NULL)
			break;
		if(strstr(line, realdir))
		{
			for(i=0;i<strlen(line);i++)
				if(line[i]==' ')
					line[i]='\0';
			dev = strdup(line);
		}
	}
	fclose(fp);
	if(dev != NULL)
		return (umount2(dev,0));
	return(0);
}

int fw_system(char* cmd)
{
	char *ptr, line[PATH_MAX];
	FILE *pp;
	LOG("running external command: '%s'", cmd);
	ptr = g_strdup_printf("%s 2>&1", cmd);
	pp = popen(ptr, "r");
	if(!pp)
	{
		LOG("call to popen falied (%s)", strerror(errno));
		return(-1);
	}
	while(!feof(pp))
	{
		if(fgets(line, PATH_MAX, pp) == NULL)
			break;
		line[strlen(line)-1]='\0';
		LOG("> %s", line);
	}
	int ret = pclose(pp);
	FREE(ptr);
	LOG("external command returned with exit code '%d'", ret);
	return (ret);
}

int fw_system_interactive(char* cmd)
{
	char *ptr;
	int ret;

	LOG("running interactive external command: '%s'", cmd);
	ptr = g_strdup_printf("%s 2>&1", cmd);
	ret = system(ptr);
	FREE(ptr);
	LOG("interactive external command returned with exit code '%d'", ret);
	return (ret);
}
char *drop_version(char *str)
{
	char *ptr;
	int i;

	for(i=0;i<2;i++)
		if((ptr = strrchr(str, '-')))
			*ptr = '\0';
	return(str);
}

GList *g_list_strremove(GList *list, char *str)
{
	int i;

	for(i=0;i<g_list_length(list);i++)
		if(!strcmp(g_list_nth_data(list, i), str))
			return(g_list_remove(list, g_list_nth_data(list, i)));
	return(NULL);
}

char *g_list_display(GList *list, char *sep)
{
	int i, len=0;
	char *ret;

	for (i=0; i<g_list_length(list); i++)
	{
		drop_version((char*)g_list_nth_data(list, i));
		len += strlen((char*)g_list_nth_data(list, i));
		len += strlen(sep)+1;
	}
	if(len==0)
		return(NULL);
	MALLOC(ret, len);
	*ret='\0';
	for (i=0; i<g_list_length(list); i++)
	{
		strcat(ret, (char*)g_list_nth_data(list, i));
		strcat(ret, sep);
	}
	return(ret);
}

int msg(char *str)
{
	printf("\e[01;36m::\e[0m \e[01m%s\e[0m\n", str);
	return(0);
}

int disable_cache(char *path)
{
	DIR *dir;
	struct dirent *ent;
	char *filename;
	char *targetname;

	dir = opendir(path);
	if (!dir)
		return(1);
	while ((ent = readdir(dir)) != NULL)
	{
		if(!strcmp(ent->d_name, ".") || !strcmp(ent->d_name, ".."))
			continue;
		filename = g_strdup_printf("%s/%s", path, ent->d_name);
		targetname = g_strdup_printf("%s/var/cache/pacman-g2/pkg/%s", TARGETDIR, ent->d_name);
		symlink(filename, targetname);
		FREE(filename);
		FREE(targetname);
	}
	closedir(dir);
	return(0);
}

void signal_handler(int signum)
{
	if (signum == SIGSEGV)
	{
		printf("error : segfault");
		exit_perform();
	} else if (signum == SIGINT) {
		signal(SIGINT, signal_handler);
	} else {
		exit_perform();
	}
}

int setup_log(char *file, int line, char *fmt, ...)
{
	static FILE *ldp = NULL, *lfp = NULL;
	va_list args;
	char str[PATH_MAX], *ptr;
	struct stat buf;

	va_start(args, fmt);
	vsnprintf(str, PATH_MAX, fmt, args);
	va_end(args);

	if(!ldp)
	{
		ldp = fopen(LOGDEV, "w");
		if(!ldp)
			return(-1);
	}
	ptr = g_strdup_printf("%s/%s", TARGETDIR, LOGFILE);
	if(stat(LOGFILE, &buf) && !stat(ptr, &buf))
	{
		fclose(lfp);
		lfp = NULL;
	}
	else
	{
		FREE(ptr);
		ptr = strdup(LOGFILE);
	}
	if(!lfp)
	{
		lfp = fopen(ptr, "a");
		if(!lfp)
			return(-1);
	}
	fprintf(ldp, "%s:%d: %s\n", file, line, str);
	fflush(ldp);
	fprintf(lfp, "%s:%d: %s\n", file, line, str);
	fflush(lfp);
	return(0);
}

void cb_log(unsigned short level, char *msg)
{
	LOG("[libpacman, level %d] %s", level, msg);
}

char *fsize(int length)
{
	if(length/1953125 == 0)
		return g_strdup_printf("%d MB", length/1907);
	else
		return g_strdup_printf("%d GB", length/1953125);
}

int cmp_str(gconstpointer a, gconstpointer b)
{
	return(strcmp(a, b));
}
