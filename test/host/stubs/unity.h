// Tiny stand-in for ESP-IDF's Unity so the pure-logic tests build on a PC.
// Provides TEST_CASE (auto-registers the test) and the TEST_ASSERT_* macros we use.
#pragma once

#include <stdio.h>
#include <vector>

struct HostTest {
    const char *name;
    void (*fn)();
};

// All registered tests, and whether the currently running one has failed.
inline std::vector<HostTest> &host_tests()
{
    static std::vector<HostTest> tests;
    return tests;
}
inline bool &host_test_failed()
{
    static bool failed = false;
    return failed;
}

struct HostTestRegistrar {
    HostTestRegistrar(const char *name, void (*fn)()) { host_tests().push_back({name, fn}); }
};

#define HOST_CAT2(a, b) a##b
#define HOST_CAT(a, b) HOST_CAT2(a, b)

// TEST_CASE("name", "[tags]") { ... }  (tags are ignored on the host)
// __LINE__ makes the names unique, since each test starts on its own line.
#define TEST_CASE(name, tags)                                                        \
    static void HOST_CAT(host_test_fn_, __LINE__)();                                 \
    static HostTestRegistrar HOST_CAT(host_test_reg_, __LINE__)(                     \
        name, HOST_CAT(host_test_fn_, __LINE__));                                    \
    static void HOST_CAT(host_test_fn_, __LINE__)()

// Report a failed check and mark the current test failed (the run continues).
#define HOST_FAIL(expected, actual)                                                  \
    do {                                                                             \
        printf("    %s:%d: expected %ld, got %ld\n", __FILE__, __LINE__,             \
               (long)(expected), (long)(actual));                                    \
        host_test_failed() = true;                                                   \
    } while (0)

#define TEST_ASSERT_EQUAL_UINT8(expected, actual)                                    \
    do {                                                                             \
        unsigned e_ = (unsigned)(expected), a_ = (unsigned)(actual);                 \
        if (e_ != a_) HOST_FAIL(e_, a_);                                             \
    } while (0)
