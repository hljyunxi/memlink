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
    signal(SIGINT, sig_handler);

    if (sigignore(SIGPIPE) == -1) {
        DERROR("failed to ignore SIGPIPE\n");
        return -1;
    }
   
    return 0;
}


int main(int argc, char *argv[])
{
    int ret;
    struct rlimit rlim;

    myconfig_create("memlink.conf");
    logfile_create("stdout", 3);

    runtime_create(argv[0]);

    
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

    if (g_cf->is_daemon) {
        ret = daemonize(g_cf->max_core, 0);
        if (ret == -1) {
            DERROR("daemon error! %s\n", strerror(errno));
            MEMLINK_EXIT;
        }
    }

    mainserver_loop(g_runtime->server);

    return 0;
}



