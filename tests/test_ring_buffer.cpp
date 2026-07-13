/**
 * @file test_ring_buffer.cpp
 * @brief Unit test per model/RingBuffer.h.
 */
#include "TestFramework.h"
#include "model/RingBuffer.h"

using am::RingBuffer;

TEST_CASE("RingBuffer: vuoto alla costruzione")
{
    RingBuffer<int> rb(4);
    CHECK(rb.empty());
    CHECK_EQ(rb.size(), std::size_t{ 0 });
    CHECK_EQ(rb.capacity(), std::size_t{ 4 });
}

TEST_CASE("RingBuffer: push sotto capacita' mantiene l'ordine cronologico")
{
    RingBuffer<int> rb(4);
    rb.push(10);
    rb.push(20);
    rb.push(30);
    CHECK_EQ(rb.size(), std::size_t{ 3 });
    CHECK_EQ(rb[0], 10);  // piu' vecchio
    CHECK_EQ(rb[1], 20);
    CHECK_EQ(rb[2], 30);  // piu' recente
    CHECK_EQ(rb.back(), 30);
}

TEST_CASE("RingBuffer: oltre capacita' sovrascrive il piu' vecchio (wraparound)")
{
    RingBuffer<int> rb(3);
    rb.push(1);
    rb.push(2);
    rb.push(3);
    rb.push(4);  // sovrascrive 1
    rb.push(5);  // sovrascrive 2
    CHECK_EQ(rb.size(), std::size_t{ 3 });  // non cresce oltre la capacita'
    CHECK_EQ(rb[0], 3);  // ora il piu' vecchio
    CHECK_EQ(rb[1], 4);
    CHECK_EQ(rb[2], 5);
    CHECK_EQ(rb.back(), 5);
}

TEST_CASE("RingBuffer: clear() svuota logicamente senza deallocare")
{
    RingBuffer<int> rb(3);
    rb.push(1);
    rb.push(2);
    rb.clear();
    CHECK(rb.empty());
    CHECK_EQ(rb.size(), std::size_t{ 0 });
    CHECK_EQ(rb.capacity(), std::size_t{ 3 });  // capacita' invariata
    rb.push(99);
    CHECK_EQ(rb[0], 99);  // riparte pulito dopo clear()
}

TEST_CASE("RingBuffer: capacity 0 viene clampata a 1 (Bug 9, niente UB)")
{
    RingBuffer<int> rb(0);
    CHECK_EQ(rb.capacity(), std::size_t{ 1 });  // clampata, non 0
    rb.push(7);
    rb.push(8);  // sovrascrive 7, capacita' reale 1
    CHECK_EQ(rb.size(), std::size_t{ 1 });
    CHECK_EQ(rb.back(), 8);
}

TEST_CASE("RingBuffer: molte iterazioni di wraparound restano coerenti")
{
    RingBuffer<int> rb(5);
    for (int i = 0; i < 100; ++i) {
        rb.push(i);
    }
    CHECK_EQ(rb.size(), std::size_t{ 5 });
    // Gli ultimi 5 valori spinti: 95..99, in ordine cronologico.
    for (int i = 0; i < 5; ++i) {
        CHECK_EQ(rb[static_cast<std::size_t>(i)], 95 + i);
    }
    CHECK_EQ(rb.back(), 99);
}
