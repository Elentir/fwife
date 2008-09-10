#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <linux/cdrom.h>
#include <dirent.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/fcntl.h>
#include <signal.h>
#include <stdarg.h>

#ifdef _
#undef _
#endif
#define _(text) gettext(text)

#define MALLOC(p, b) { if((b) > 0) \
	{ p = malloc(b); if (!(p)) \
	{ fprintf(stderr, "malloc failure: could not allocate %d bytes\n", (int)(b)); \
	exit(1); }} else p = NULL; }
#define FREE(p) { if (p) { free(p); (p) = NULL; }}

#define LOG(fmt, args...) setup_log(__FILE__, __LINE__, fmt, ##args)

typedef struct
{
	char *name;
	void *data;
} data_t;

char *get_version(void);
char *gen_backtitle(char *section);
data_t *data_new(void);
void *data_get(GList *config, char *title);
void data_put(GList **config, char *name, void *data);
int eject(char *dev, char *target);
int copyfile(char *src, char *dest);
int exit_perform(void);
int fw_system(char* cmd);
int fw_system_interactive(char* cmd);
int makepath(char *path);
int rmrf(char *path);
int umount_if_needed(char *sourcedir);
char *drop_version(char *str);
GList *g_list_strremove(GList *list, char *str);
char *g_list_display(GList *list, char *sep);
int msg(char *str);
int disable_cache(char *path);
void signal_handler(int signum);
int setup_log(char *file, int line, char *fmt, ...);
void cb_log(unsigned short level, char *msg);
char *fsize(int length);
int cmp_str(gconstpointer a, gconstpointer b);

#endif
