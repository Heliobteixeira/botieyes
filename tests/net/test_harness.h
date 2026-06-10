#ifndef BOTIEYES_TEST_HARNESS_H
#define BOTIEYES_TEST_HARNESS_H

// Minimal host-native test harness for the BotiEyes network layer.
// No external dependencies — compiles with any C++11 compiler.

#include <cstdio>
#include <cstdlib>

namespace botest {

inline int& failures() {
    static int n = 0;
    return n;
}

inline int& checks() {
    static int n = 0;
    return n;
}

} // namespace botest

#define CHECK(cond)                                                        \
    do {                                                                   \
        ++botest::checks();                                                \
        if (!(cond)) {                                                     \
            ++botest::failures();                                          \
            std::printf("  FAIL: %s (line %d): %s\n", __func__, __LINE__,  \
                        #cond);                                            \
        }                                                                  \
    } while (0)

#define CHECK_EQ(a, b)                                                     \
    do {                                                                   \
        ++botest::checks();                                                \
        if (!((a) == (b))) {                                               \
            ++botest::failures();                                          \
            std::printf("  FAIL: %s (line %d): %s == %s (got %ld vs %ld)\n",\
                        __func__, __LINE__, #a, #b, (long)(a), (long)(b)); \
        }                                                                  \
    } while (0)

#define RUN(test)                                                          \
    do {                                                                   \
        std::printf("RUN  %s\n", #test);                                   \
        test();                                                            \
    } while (0)

#define TEST_MAIN_END()                                                    \
    do {                                                                   \
        std::printf("\n%d checks, %d failures\n", botest::checks(),        \
                    botest::failures());                                   \
        return botest::failures() == 0 ? 0 : 1;                            \
    } while (0)

#endif // BOTIEYES_TEST_HARNESS_H
