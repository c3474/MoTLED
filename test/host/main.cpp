// Host test runner: runs every TEST_CASE registered by unity.h and reports.
#include <stdio.h>
#include "unity.h"

int main()
{
    int failed = 0;
    for (const HostTest &t : host_tests()) {
        host_test_failed() = false;
        t.fn();
        printf("%s %s\n", host_test_failed() ? "FAIL" : "PASS", t.name);
        if (host_test_failed()) {
            failed++;
        }
    }
    int total = (int)host_tests().size();
    printf("%d tests, %d passed, %d failed\n", total, total - failed, failed);
    return failed ? 1 : 0;
}
