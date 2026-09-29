#pragma once

#include <iostream>
#include <cmath>
#include <cstdlib>
#include <string>
#include <algorithm>

#include "organ/Types.hpp"

inline int g_tests_passed = 0;
inline int g_tests_failed = 0;

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "\n    FAILED: " #cond " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            g_tests_failed++; \
            return; \
        } \
    } while (0)

#define TEST_NEAR(a, b, eps) \
    do { \
        float diff_val = std::abs(static_cast<float>(a) - static_cast<float>(b)); \
        if (diff_val > static_cast<float>(eps)) { \
            std::cerr << "\n    FAILED: " #a " (" << (a) << ") vs " #b " (" << (b) \
                      << ") diff=" << diff_val << " > " << (eps) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            g_tests_failed++; \
            return; \
        } \
    } while (0)

#define TEST_VEC3_NEAR(v1, v2, eps) \
    do { \
        float diff_vec = ((v1) - (v2)).norm(); \
        if (diff_vec > static_cast<float>(eps)) { \
            std::cerr << "\n    FAILED: Vec3 near check: (" \
                      << (v1).x << ", " << (v1).y << ", " << (v1).z << ") vs (" \
                      << (v2).x << ", " << (v2).y << ", " << (v2).z << ") diff=" \
                      << diff_vec << " > " << (eps) \
                      << " at " << __FILE__ << ":" << __LINE__ << std::endl; \
            g_tests_failed++; \
            return; \
        } \
    } while (0)

#define RUN_TEST(fn) \
    do { \
        std::cout << "[ RUN      ] " #fn << std::endl; \
        int failed_before = g_tests_failed; \
        fn(); \
        if (g_tests_failed == failed_before) { \
            std::cout << "[       OK ] " #fn << std::endl; \
            g_tests_passed++; \
        } else { \
            std::cout << "[  FAILED  ] " #fn << std::endl; \
        } \
    } while (0)
