#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <check.h>

#include "masktest.h"
#include "hashtabletest.h"
#include "logfile.h"

static Suite*
make_suite()
{
    Suite   *s;
    TCase   *tc;

    s  = suite_create("memlink");
    tc = tcase_create("zhaowei");

    suite_add_tcase(s, tc);

    tcase_add_test(tc, test_mask);
    tcase_add_test(tc, test_hashtable);

    return s;
}


int main()
{
    SRunner *srun;
    Suite   *s;

    //logfile_create("stdout", 3);

    s = make_suite();
    srun = srunner_create(s);

    srunner_set_fork_status(srun, CK_FORK);
    srunner_run_all(srun, CK_NORMAL);
    srunner_free(srun);

    return 0;
}


