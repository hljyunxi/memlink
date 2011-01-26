#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <sys/resource.h>
#include <unistd.h>
#include "myconfig.h"
#include "logfile.h"
#include "server.h"
#include "daemon.h"
#include "common.h"

char *VERSION = "memlink-0.2.5";

static void 
sig_handler(const int sig) {
    DFATALERR("====== SIGNAL %d handled ======\n", sig);
    exit(EXIT_SUCCESS);
}

static void 
sig_handler_segv(const int sig) {
    DFATALERR("====== SIGSEGV handled ======\n");
	abort();
}

int 
signal_install()
{
    struct sigaction sigact;

    sigact.sa_handler = sig_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;

    sigaction(SIGINT, &sigact, NULL);
    
	sigact.sa_handler = sig_handler_segv;
    sigaction(SIGSEGV, &sigact, NULL);

    sigact.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sigact, NULL);

    return 0;
}

void 
master(char *pgname) 
{
    logfile_create(g_cf->log_name, g_cf->log_level);
    DINFO("logfile ok!\n");
    runtime_create_master(pgname);
    DINFO("master runtime ok!\n");

    mainserver_loop(g_runtime->server);
}

void 
slave(char *pgname) 
{
    logfile_create(g_cf->log_name, g_cf->log_level);
    runtime_create_slave(pgname);
    DINFO("slave runtime ok!\n");

    mainserver_loop(g_runtime->server);
}

void 
usage()
{
	fprintf(stderr, "usage: memlink [config file path]\n");
}

int main(int argc, char *argv[])
{
    int ret;
    struct rlimit rlim;
	char *conffile;

	if (argc == 2) {
		conffile = argv[1];
	}else{
		//conffile = "etc/memlink.conf";
		usage();
		return 0;
	}
	
	DINFO("config file: %s\n", conffile);
    myconfig_create(conffile);
    
    if (g_cf->max_core) {
        struct rlimit rlim_new;
        if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
            rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
            if (setrlimit(RLIMIT_CORE, &rlim_new)!= 0) {
                /* failed. try raising just to the old max */
                rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
                (void)setrlimit(RLIMIT_CORE, &rlim_new);
            }
        }
        if ((getrlimit(RLIMIT_CORE, &rlim) != 0) || rlim.rlim_cur == 0) {
            DFATALERR("failed to ensure corefile creation\n");
            MEMLINK_EXIT;
        }
    }

    if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) { 
        DFATALERR("failed to getrlimit number of files\n");
        MEMLINK_EXIT;
    } else {
        int maxfiles = g_cf->max_conn + 20;
        if (rlim.rlim_cur < maxfiles)
            rlim.rlim_cur = maxfiles;
        if (rlim.rlim_max < rlim.rlim_cur)
            rlim.rlim_max = rlim.rlim_cur;
        if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) { 
            DFATALERR("failed to set rlimit for open files. Try running as root or requesting smaller maxconns value.\n");
            MEMLINK_EXIT;
        }    
    }

    signal_install();

    if (g_cf->is_daemon) {
        ret = daemonize(g_cf->max_core, 0);
        if (ret == -1) {
            DFATALERR("daemon error! %s\n", strerror(errno));
            MEMLINK_EXIT;
        }
    }

    if (g_cf->role == ROLE_MASTER) {
        master(argv[0]);
    }else{
        slave(argv[0]);
    }

    return 0;
}


