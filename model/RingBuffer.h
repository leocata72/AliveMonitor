/**
 * @file RingBuffer.h
 * @brief Ring buffer generico a capacità fissa, senza allocazioni a regime.
 *
 * La memoria viene allocata UNA SOLA VOLTA nel costruttore; push() e
 * l'accesso indicizzato non allocano mai. Quando il buffer è pieno, il
 * campione più vecchio viene sovrascritto.
 *
 * NOTA sulla concorrenza: questa classe NON è thread-safe di per sé.
 * La sincronizzazione è responsabilità del proprietario (vedi
 * AnalogDataBuffer, che protegge i sei ring buffer con un unico mutex,
 * garantendo snapshot coerenti tra i canali).
 */
#pragma once

#include <cstddef>
#include <vector>

namespace am {

template <typename T>
class RingBuffer {
public:
    /// Alloca subito tutta la capacità richiesta (nessuna allocazione futura).
    explicit RingBuffer(std::size_t capacity)
        : storage_(capacity)
        , capacity_(capacity)
    {
    }

    /// Inserisce un elemento; se pieno sovrascrive il più vecchio. O(1).
    void push(const T& value) noexcept
    {
        if (count_ < capacity_) {
            storage_[physicalIndex(count_)] = value;
            ++count_;
        } else {
            storage_[head_] = value;
            head_ = (head_ + 1) % capacity_;
        }
    }

    /// Elemento i-esimo in ordine cronologico (0 = più vecchio). O(1).
    [[nodiscard]] const T& operator[](std::size_t i) const noexcept
    {
        return storage_[physicalIndex(i)];
    }

    [[nodiscard]] std::size_t size() const noexcept { return count_; }
    [[nodiscard]] std::size_t capacity() const noexcept { return capacity_; }
    [[nodiscard]] bool empty() const noexcept { return count_ == 0; }

    /// L'elemento più recente. Precondizione: !empty().
    [[nodiscard]] const T& back() const noexcept
    {
        return storage_[physicalIndex(count_ - 1)];
    }

    /// Svuota logicamente il buffer (nessuna deallocazione).
    void clear() noexcept
    {
        head_ = 0;
        count_ = 0;
    }

private:
    [[nodiscard]] std::size_t physicalIndex(std::size_t logical) const noexcept
    {
        return (head_ + logical) % capacity_;
    }

    std::vector<T> storage_;   ///< Memoria pre-allocata (dimensione fissa).
    std::size_t capacity_;
    std::size_t head_ = 0;     ///< Indice fisico dell'elemento più vecchio.
    std::size_t count_ = 0;    ///< Numero di elementi validi.
};

} // namespace am
