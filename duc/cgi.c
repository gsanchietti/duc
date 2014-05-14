
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>

#include "cmd.h"
#include "duc.h"

int size = 600;
int depth = 4;

/*
 * Simple parser for CGI parameter line. Does not excape CGI strings.
 */

struct param {
	char *key;
	char *val;
	struct param *next;
};

struct param *param_list = NULL;


static void cgi_parse(void)
{
	char *qs = getenv("QUERY_STRING");
	if(qs == NULL) return;

	char *p = qs;

	for(;;) {

		char *pe = strchr(p, '=');
		if(!pe) break;
		char *pn = strchr(pe, '&');
		if(!pn) pn = pe + strlen(pe);

		char *key = p;
		int keylen = pe-p;
		char *val = pe+1;
		int vallen = pn-pe-1;

		struct param *param = malloc(sizeof(struct param));
		assert(param);

		param->key = malloc(keylen+1);
		assert(param->key);
		strncpy(param->key, key, keylen);

		param->val = malloc(vallen+1);
		assert(param->val);
		strncpy(param->val, val, vallen);
		
		param->next = param_list;
		param_list = param;

		if(*pn == 0) break;
		p = pn+1;
	}
}


static char *cgi_get(const char *key)
{
	struct param *param = param_list;

	while(param) {
		if(strcmp(param->key, key) == 0) {
			return param->val;
		}
		param = param->next;
	}

	return NULL;
}



static char *find_xy(duc *duc, int x, int y)
{
	static char path_out[PATH_MAX];

	char *path = cgi_get("path");
	if(!path) return NULL;

        duc_dir *dir = duc_opendir(duc, path);
        if(dir == NULL) {
                fprintf(stderr, "%s\n", duc_strerror(duc));
                return NULL;
        }

	char path_found[PATH_MAX];
	int found = duc_graph_xy_to_path(dir, size, depth, x, y, path_found, sizeof(path_found));
	if(found) {
		snprintf(path_out, sizeof(path_out), "%s%s", path, path_found);
		fprintf(stderr, "found %s", path_out);
		return path_out;
	}
	
	duc_closedir(dir);

	return NULL;
}



static void do_index(duc *duc)
{

	printf("Content-Type: text/html\n");
	printf("\n");
	
	char *path = cgi_get("path");
	char *script = getenv("SCRIPT_NAME");
	if(!script) return;

	char *qs = getenv("QUERY_STRING");
	int x, y;
	if(qs) {
		char *p1 = strchr(qs, '?');
		if(p1) {
			char *p2 = strchr(p1, ',');
			if(p2) {
				x = atoi(p1+1);
				y = atoi(p2+1);
				path = find_xy(duc, x, y);
			}
		}
	}

	if(path == NULL) path = "/home/ico";
	
	struct duc_index_report report;
	int i = 0;

	printf("<table>");
	while( duc_list(duc, i, &report) == 0) {

		char ts_date[32];
		char ts_time[32];
		struct tm *tm = localtime(&report.time_start);
		strftime(ts_date, sizeof ts_date, "%Y-%m-%d",tm);
		strftime(ts_time, sizeof ts_time, "%H:%M:%S",tm);

		char url[PATH_MAX];
		snprintf(url, sizeof url, "%s?cmd=index&path=%s", script, report.path);

		printf("<tr>");
		printf("<td>%s</td>", ts_date);
		printf("<td>%s</td>", ts_time);
		printf("<td><a href='%s'>%s</a></td>", url, report.path);
		printf("</tr>\n");

		i++;
	}
	printf("</table>");


	printf("<center>");
	printf("<b>%s</b><br>", path);
	printf("<a href='%s?cmd=index&path=%s&'>", script, path);
	printf("<img src='%s?cmd=image&path=%s' ismap='ismap'>\n", script, path);
	printf("</a>");
	fflush(stdout);
}


void do_image(duc *duc)
{
	printf("Content-Type: image/png\n");
	printf("\n");

	char *path = cgi_get("path");
	if(!path) return;

        duc_dir *dir = duc_opendir(duc, path);
        if(dir == NULL) {
                fprintf(stderr, "%s\n", duc_strerror(duc));
                return;
        }

	duc_graph(dir, size, depth, stdout);

	duc_closedir(dir);
	duc_close(duc);
}



static int cgi_main(int argc, char **argv)
{
	cgi_parse();

	char *cmd = cgi_get("cmd");
	if(cmd == NULL) cmd = "index";
	
	duc *duc = duc_new();
	if(duc == NULL) {
                fprintf(stderr, "Error creating duc context\n");
		return -1;
        }

        int r = duc_open(duc, "/home/ico/.duc.db", DUC_OPEN_RO);
        if(r != DUC_OK) {
                fprintf(stderr, "%s\n", duc_strerror(duc));
		return -1;
        }

	if(strcmp(cmd, "index") == 0) do_index(duc);
	if(strcmp(cmd, "image") == 0) do_image(duc);
	
	duc_close(duc);
	duc_del(duc);

	return 0;
}


struct cmd cmd_cgi = {
	.name = "cgi",
	.description = "CGI interface",
	.usage = "",
	.help = "",
	.main = cgi_main
};


/*
 * End
 */
