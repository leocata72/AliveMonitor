/**
 * @file main.cpp
 * @brief Entry point della suite di unit test. Esegue tutti i TEST_CASE
 *        autoregistrati (vedi TestFramework.h) e ritorna il numero di
 *        asserzioni fallite come exit code (0 = tutto ok).
 */
#include "TestFramework.h"

int main()
{
    return ::am::test::runAll();
}
