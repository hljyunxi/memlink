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


static void sig_handler(const int sig) {
    printf("SIGINT handled.\n");
    exit(EXIT_SUCCESS);
}

int signal_install()
{
    struct sigaction sigact;
    //int    ret;

    sigact.sa_handler = sig_handler;
    sigemptyset(&sigact.sa_mask);
    sigact.sa_flags = 0;

    sigaction(SIGINT, &sigact, NULL);

    sigact.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &sigact, NULL);

    return 0;
}


void master(char *pgname) 
{
    logfile_create("stdout", 3);
    DINFO("logfile ok!\n");
    runtime_create_master(pgname);
    DINFO("master runtime ok!\n");

    mainserver_loop(g_runtime->server);
}

void slave(char *pgname) 
{
    slave_runtime_create(pgname);
    DINFO("slave runtime ok!\n");
    mainserver_loop(g_runtime->server);
}

int main(int argc, char *argv[])
{
    int ret;
    struct rlimit rlim;
	char *conffile;

	if (argc == 2) {
		conffile = argv[1];
	}else{
		conffile = "etc/memlink.conf";
	}
	
	DINFO("config file: %s\n", conffile);
    myconfig_create(conffile);
    DINFO("config ok!\n");
    
    if (g_cf->max_core) {
        struct rlimit rlim_new;
        /*
         * First try raising to infinity; if that fails, try bringing
         * the soft limit to the hard.
         */
        if (getrlimit(RLIMIT_CORE, &rlim) == 0) {
            rlim_new.rlim_cur = rlim_new.rlim_max = RLIM_INFINITY;
            if (setrlimit(RLIMIT_CORE, &rlim_new)!= 0) {
                /* failed. try raising just to the old max */
                rlim_new.rlim_cur = rlim_new.rlim_max = rlim.rlim_max;
                (void)setrlimit(RLIMIT_CORE, &rlim_new);
            }
        }
        /*
         * getrlimit again to see what we ended up with. Only fail if
         * the soft limit ends up 0, because then no core files will be
         * created at all.
         */

        if ((getrlimit(RLIMIT_CORE, &rlim) != 0) || rlim.rlim_cur == 0) {
            DERROR("failed to ensure corefile creation\n");
            MEMLINK_EXIT;
        }
    }
    

    if (getrlimit(RLIMIT_NOFILE, &rlim) != 0) { 
        DERROR("failed to getrlimit number of files\n");
        MEMLINK_EXIT;
    } else {
        int maxfiles = g_cf->max_conn;
        if (rlim.rlim_cur < maxfiles)
            rlim.rlim_cur = maxfiles;
        if (rlim.rlim_max < rlim.rlim_cur)
            rlim.rlim_max = rlim.rlim_cur;
        if (setrlimit(RLIMIT_NOFILE, &rlim) != 0) { 
            DERROR("failed to set rlimit for open files. Try running as root or requesting smaller maxconns value.\n");
            MEMLINK_EXIT;
        }    
    }

    signal_install();
    DINFO("signal ok!\n");

    if (g_cf->is_daemon) {
        ret = daemonize(g_cf->max_core, 0);
        if (ret == -1) {
            DERROR("daemon error! %s\n", strerror(errno));
            MEMLINK_EXIT;
        }
    }

    if (g_cf->role == 1) {
        master(argv[0]);
    }else{
        slave(argv[0]);
    }

    return 0;
}


