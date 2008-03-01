/*
 *	aprsc
 *
 *	(c) Heikki Hannikainen, OH7LZB <hessu@hes.iki.fi>
 *
 *    This program is free software; you can redistribute it and/or modify
 *    it under the terms of the GNU General Public License as published by
 *    the Free Software Foundation; either version 2 of the License, or
 *    (at your option) any later version.
 *
 *    This program is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *    GNU General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public License
 *    along with this program; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *	
 */

/*
 *	config.c: configuration parsing, based on Tomi's code
 */

#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "config.h"
#include "hmalloc.h"
#include "hlog.h"
#include "cfgfile.h"

char def_cfgfile[] = "aprsc.conf";

char *cfgfile = def_cfgfile;
char *pidfile = NULL;
char *new_rundir = NULL;
char *rundir = NULL;
char *new_logdir = NULL;
char *logdir = NULL;	/* access logs go here */

char def_logname[] = "aprsc";
char *logname = def_logname;	/* syslog entries use this program name */

char *mycall = NULL;
char *myemail = NULL;
char *myadmin = NULL;
char *myhostname = "undefined-hostname";
char *new_mycall = NULL;
char *new_myemail = NULL;
char *new_myadmin = NULL;

int fork_a_daemon = 0;	/* fork a daemon */

int dump_splay = 0;	/* print splay tree information */

int workers_configured = 2;	/* number of workers to run */

int expiry_interval = 10;
int stats_interval = 1 * 60;

int lastposition_storetime = 24*60*60;	/* how long the last position packet of each station is stored */
int dupefilter_storetime = 24*60*60;	/* how long to store information required for dupe filtering */

int upstream_timeout = 5*60;		/* after N seconds of no input from an upstream, disconnect */
int client_timeout = 60*60;		/* after N seconds of no input from a client, disconnect */

int ibuf_size = 1500;			/* size of input buffer for clients */
int obuf_size = 32*1024;		/* size of output buffer for clients */

int verbose = 0;

/* address:port pairs being listened */
struct listen_config_t *listen_config = NULL, *listen_config_new = NULL;

int do_listen(struct listen_config_t **lq, int argc, char **argv);
int do_interval(time_t *dest, int argc, char **argv);

/*
 *	Configuration file commands
 */

#define _CFUNC_ (int (*)(void *dest, int argc, char **argv))

static struct cfgcmd cfg_cmds[] = {
	{ "rundir",		_CFUNC_ do_string,		&new_rundir		},
	{ "logdir",		_CFUNC_ do_string,		&new_logdir		},
	{ "mycall",		_CFUNC_ do_string,		&new_mycall		},
	{ "myemail",		_CFUNC_ do_string,		&new_myemail		},
	{ "myhostname",		_CFUNC_ do_string,		&myhostname		},
	{ "myadmin",		_CFUNC_ do_string,		&new_myadmin		},
	{ "workerthreads",	_CFUNC_ do_int,			&workers_configured	},
	{ "statsinterval",	_CFUNC_ do_interval,		&stats_interval		},
	{ "expiryinterval",	_CFUNC_ do_interval,		&expiry_interval	},
	{ "lastpositioncache",	_CFUNC_ do_interval,		&lastposition_storetime	},
	{ "dupefiltercache",	_CFUNC_ do_interval,		&dupefilter_storetime	},
	{ "upstreamtimeout",	_CFUNC_ do_interval,		&upstream_timeout	},
	{ "clienttimeout",	_CFUNC_ do_interval,		&client_timeout		},
	{ "listen",		_CFUNC_ do_listen,		&listen_config_new	},
	{ NULL,			NULL,				NULL			}
};

/*
 *	Free a listen config tree
 */

void free_listen_config(struct listen_config_t **lc)
{
	struct listen_config_t *this;
	int i;

	while (*lc) {
		this = *lc;
		*lc = this->next;
		hfree(this->name);
		hfree(this->host);
		for (i = 0; i < (sizeof(this->filters)/sizeof(this->filters[0])); ++i)
			if (this->filters[i])
				hfree(this->filters[i]);
		hfree(this);
	}
}

/*
 *	parse an interval specification
 */
 
time_t parse_interval(char *origs)
{
	time_t t = 0;
	int i;
	char *s, *np, *p, c;
	
	np = p = s = hstrdup(origs);
	
	while (*p) {
		if (!isdigit((int)*p)) {
			c = tolower(*p);
			*p = '\0';
			i = atoi(np);
			if (c == 's')
				t += i;
			else if (c == 'm')
				t += 60 * i;
			else if (c == 'h')
				t += 60 * 60 * i;
			else if (c == 'd')
				t += 24 * 60 * 60 * i;
			np = p + 1;
		}
		p++;
	}
	
	if (*np)
		t += atoi(np);
		
	hfree(s);
	return t;
}


/*
 *	Parse an interval configuration entry
 */

int do_interval(time_t *dest, int argc, char **argv)
{
	if (argc < 2)
		return -1;
		
	*dest = parse_interval(argv[1]);
	return 0;
}

/*
 *	Parse a Listen directive
 *
 *	listen <label> <typelabel> tcp <hostname> <portnum> [<filter> [..<more_filters>]]
 *
 */

int do_listen(struct listen_config_t **lq, int argc, char **argv)
{
	int i, port;
	struct listen_config_t *l;

	if (argc < 6)
		return -1;
	
	if (strcasecmp(argv[3], "tcp") == 0) {
		// well, do nothing for now.
	} else {
		fprintf(stderr, "Listen: Unsupported protocol '%s'\n", argv[3]);
		return -2;
	}
	
	port = atoi(argv[5]);
	if (port < 1 || port > 65535) {
		fprintf(stderr, "Listen: unsupported port number '%s'\n", argv[5]);
		return -2;
	}
	
	l = hmalloc(sizeof(*l));
	l->name = hstrdup(argv[1]);
	l->host = hstrdup(argv[4]);
	l->port = port;
	for (i = 0; i < (sizeof(l->filters)/sizeof(l->filters[0])); ++i) {
		l->filters[i] = NULL;
		if (argc - 6 > i) {
			l->filters[i] = hstrdup(argv[i+6]);
		}
	}
	
	/* put in the list */
	l->next = *lq;
	if (l->next)
		l->next->prevp = &l->next;
	*lq = l;
	
	return 0;
}

/*
 *	Validate an APRS-IS callsign
 */

int valid_aprsis_call(char *s)
{
	if (strlen(s) > 12)
		return 0;
	if (strlen(s) < 3)
		return 0;
	
	return 1;
}

/*
 *	upcase
 */
 
char *strupr(char *s)
{
	char *p;
	
	for (p = s; (*p); p++)
		*p = toupper(*p);
		
	return s;
}

/*
 *	Read configuration files, should add checks for this program's
 *	specific needs and obvious misconfigurations!
 */

int read_config(void)
{
	int failed = 0;
	char *s;
	
	if (read_cfgfile(cfgfile, cfg_cmds))
		return -1;
	
	/* these parameters will only be used when reading the configuration
	 * for the first time.
	 */
	if (!rundir) {
		if (new_rundir) {
			rundir = new_rundir;
			new_rundir = NULL;
		} else {
			hlog(LOG_CRIT, "Config: rundir not defined.");
			failed = 1;
		}
	}
	if (!logdir) {
		if (new_logdir) {
			logdir = new_logdir;
			new_logdir = NULL;
		} else {
			hlog(LOG_CRIT, "Config: logdir not defined.");
			failed = 1;
		}
	}
	
	/* mycall is only applied when running for the first time. */
	if (mycall) {
		if (new_mycall && strcasecmp(new_mycall, mycall)) {
			hlog(LOG_WARNING, "Config: Not changing mycall while running.");
		}
		hfree(new_mycall);
	} else {
		if (!new_mycall) {
			hlog(LOG_CRIT, "Config: mycall is not defined.");
			failed = 1;
		} else if (!valid_aprsis_call(new_mycall)) {
			hlog(LOG_CRIT, "Config: mycall '%s' is not valid.", new_mycall);
			failed = 1;
		} else {
			strupr(new_mycall);
			mycall = new_mycall;
			new_mycall = NULL;
		}
	}
	
	if (new_myadmin) {
		if (myadmin)
			hfree(myadmin);
		myadmin = new_myadmin;
		new_myadmin = NULL;
	} else {
		hlog(LOG_WARNING, "Config: myadmin is not defined.");
		failed = 1;
	}
	
	if (new_myemail) {
		if (myemail)
			hfree(myemail);
		myemail = new_myemail;
		new_myemail = NULL;
	} else {
		hlog(LOG_WARNING, "Config: myemail is not defined.");
		failed = 1;
	}
	
	if (workers_configured < 1) {
		hlog(LOG_WARNING, "Configured less than 1 worker threads. Using 1.");
	} else if (workers_configured > 32) {
		hlog(LOG_WARNING, "Configured more than 32 worker threads. Using 32.");
		workers_configured = 32;
	}
	
	/* put in the new listening config */
	free_listen_config(&listen_config);
	listen_config = listen_config_new;
	if (listen_config)
		listen_config->prevp = &listen_config;
	listen_config_new = NULL;
	
	if (failed)
		return -1;
	
	if (!pidfile) {
		s = hmalloc(strlen(logdir) + 10 + 3);
		sprintf(s, "%s/%s.pid", logdir, logname);
		
		pidfile = s;
	}
	
	return 0;
}

/*
 *	Free configuration variables
 */

void free_config(void)
{
	if (logdir)
		hfree(logdir);
	logdir = NULL;
	if (rundir)
		hfree(rundir);
	rundir = NULL;
	if (pidfile)
		hfree(pidfile);
	pidfile = NULL;
	if (cfgfile != def_cfgfile)
		hfree(cfgfile);
	cfgfile = NULL;
	if (logname != def_logname)
		hfree(logname);
	hfree(mycall);
	hfree(myemail);
	hfree(myadmin);
	mycall = myemail = myadmin = NULL;
	logname = NULL;
}

