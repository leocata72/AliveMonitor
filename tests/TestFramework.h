/**
 * @file TestFramework.h
 * @brief Micro-framework di unit test header-only, senza dipendenze esterne
 *        (coerente con la filosofia "zero dipendenze" del progetto: niente
 *        Catch2/GoogleTest da scaricare o vendorizzare).
 *
 * Uso:
 *   TEST_CASE("descrizione") {
 *       CHECK(espressione_booleana);
 *       CHECK_EQ(valore_ottenuto, valore_atteso);
 *   }
 *
 * Ogni TEST_CASE si autoregistra in un registro globale al caricamento del
 * programma (costruttori statici); RunAllTests() nel main() li esegue tutti,
 * stampa un riepilogo e ritorna il numero di CHECK falliti (0 = successo,
 * comodo come exit code per CI/ctest).
 */
#pragma once

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace am::test {

struct TestCase {
    std::string name;
    std::function<void()> fn;
};

inline std::vector<TestCase>& registry()
{
    static std::vector<TestCase> tests;
    return tests;
}

struct Registrar {
    Registrar(const std::string& name, std::function<void()> fn)
    {
        registry().push_back({ name, std::move(fn) });
    }
};

// Contatori dell'esecuzione corrente (un solo processo, nessuna concorrenza).
inline int& failCount() { static int n = 0; return n; }
inline int& checkCount() { static int n = 0; return n; }
inline std::string& currentTestName() { static std::string s; return s; }

inline void reportFailure(const std::string& exprText, const std::string& file, int line)
{
    ++failCount();
    std::cerr << "  [FAIL] " << currentTestName() << " (" << file << ":" << line << "): "
              << exprText << '\n';
}

inline void reportFailureEq(const std::string& exprText, const std::string& got,
                            const std::string& expected, const std::string& file, int line)
{
    ++failCount();
    std::cerr << "  [FAIL] " << currentTestName() << " (" << file << ":" << line << "): "
              << exprText << " -- ottenuto=" << got << " atteso=" << expected << '\n';
}

template <typename T>
std::string toStr(const T& v)
{
    std::ostringstream oss;
    oss << v;
    return oss.str();
}

/// Confronto in virgola mobile con tolleranza assoluta (i CHECK_EQ su double
/// userebbero altrimenti l'uguaglianza esatta, fragile per calcoli derivati).
inline bool approxEqual(double a, double b, double eps = 1e-9)
{
    return std::fabs(a - b) <= eps;
}

inline int runAll()
{
    int total = 0;
    for (const auto& t : registry()) {
        currentTestName() = t.name;
        std::cout << "RUN  " << t.name << '\n';
        const int failBefore = failCount();
        t.fn();
        ++total;
        if (failCount() == failBefore) {
            std::cout << "OK   " << t.name << '\n';
        }
    }
    std::cout << "\n" << total << " test case, " << checkCount() << " asserzioni, "
              << failCount() << " fallite.\n";
    return failCount();
}

} // namespace am::test

#define AM_TEST_CONCAT_(a, b) a##b
#define AM_TEST_CONCAT(a, b) AM_TEST_CONCAT_(a, b)

#define TEST_CASE(name)                                                                \
    static void AM_TEST_CONCAT(am_test_fn_, __LINE__)();                               \
    static ::am::test::Registrar AM_TEST_CONCAT(am_test_reg_, __LINE__)(               \
        name, AM_TEST_CONCAT(am_test_fn_, __LINE__));                                  \
    static void AM_TEST_CONCAT(am_test_fn_, __LINE__)()

#define CHECK(expr)                                                                    \
    do {                                                                               \
        ++::am::test::checkCount();                                                    \
        if (!(expr)) {                                                                 \
            ::am::test::reportFailure(#expr, __FILE__, __LINE__);                      \
        }                                                                              \
    } while (0)

#define CHECK_EQ(got, expected)                                                        \
    do {                                                                               \
        ++::am::test::checkCount();                                                    \
        auto am_got_ = (got);                                                          \
        auto am_expected_ = (expected);                                                \
        if (!(am_got_ == am_expected_)) {                                              \
            ::am::test::reportFailureEq(#got " == " #expected,                         \
                                        ::am::test::toStr(am_got_),                     \
                                        ::am::test::toStr(am_expected_),                \
                                        __FILE__, __LINE__);                            \
        }                                                                              \
    } while (0)

#define CHECK_NEAR(got, expected, eps)                                                 \
    do {                                                                               \
        ++::am::test::checkCount();                                                    \
        const double am_got_ = static_cast<double>(got);                               \
        const double am_expected_ = static_cast<double>(expected);                     \
        if (!::am::test::approxEqual(am_got_, am_expected_, (eps))) {                  \
            ::am::test::reportFailureEq(#got " ~= " #expected,                         \
                                        ::am::test::toStr(am_got_),                     \
                                        ::am::test::toStr(am_expected_),                \
                                        __FILE__, __LINE__);                            \
        }                                                                              \
    } while (0)
