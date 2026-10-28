#ifndef REVERSI_BITSET_H
#define REVERSI_BITSET_H

#include <cassert>
#include <cstdint>
#include <immintrin.h>
#include <iomanip>
#include <iostream>

#if defined(__AVX512F__) && defined(__AVX512VL__) && defined(__AVX512BW__)
  #define AVX512 1
#endif

template <uint_fast8_t N>
class bitset {
  private:
#ifdef AVX512
    static constexpr uint_fast8_t avx_word_count_ = (N + 7) >> 3;
    alignas(64) uint64_t data_[N];
#else
    static constexpr uint_fast8_t avx_word_count_ = (N + 3) >> 2;
    alignas(32) uint64_t data_[N];
#endif

  public:
    bitset() noexcept { reset(); }
    bitset(uint64_t word) noexcept {
      reset();
      data_[0] = word;
    }

    [[nodiscard]] [[using gnu: always_inline, hot]] bitset
    operator&(const bitset& rhs) const noexcept {
      bitset res;

#if AVX512
      const __m512i *src = reinterpret_cast<const __m512i*>(data_),
                    *rhs_src = reinterpret_cast<const __m512i*>(rhs.data_);
      __m512i* dest = reinterpret_cast<__m512i*>(res.data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        dest[i] = _mm512_and_si512(src[i], rhs_src[i]);
#else
      const __m256i *src = reinterpret_cast<const __m256i*>(data_),
                    *rhs_src = reinterpret_cast<const __m256i*>(rhs.data_);
      __m256i* dest = reinterpret_cast<__m256i*>(res.data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        dest[i] = _mm256_and_si256(src[i], rhs_src[i]);
#endif

      return res;
    }

    [[nodiscard]] [[gnu::always_inline]] bitset
    operator&(uint64_t word) const noexcept {
      bitset res = *this;
      res.data_[0] &= word;
      return res;
    }

    [[nodiscard]] [[using gnu: always_inline, hot]] bitset
    operator|(const bitset& rhs) const noexcept {
      bitset res;

#ifdef AVX512
      const __m512i *src = reinterpret_cast<const __m512i*>(data_),
                    *rhs_src = reinterpret_cast<const __m512i*>(rhs.data_);
      __m512i* dest = reinterpret_cast<__m512i*>(res.data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        dest[i] = _mm512_or_si512(src[i], rhs_src[i]);
#else
      const __m256i *src = reinterpret_cast<const __m256i*>(data_),
                    *rhs_src = reinterpret_cast<const __m256i*>(rhs.data_);
      __m256i* dest = reinterpret_cast<__m256i*>(res.data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        dest[i] = _mm256_or_si256(src[i], rhs_src[i]);
#endif

      return res;
    }

    [[nodiscard]] [[gnu::always_inline]] bitset
    operator|(uint64_t word) const noexcept {
      bitset res = *this;
      res.data_[0] |= word;
      return res;
    }

    [[nodiscard]] [[using gnu: always_inline, hot]] bitset
    operator~() const noexcept {
      bitset res;

#ifdef AVX512
      const __m512i* src = reinterpret_cast<const __m512i*>(data_);
      __m512i* dest = reinterpret_cast<__m512i*>(res.data_);

      const __m512i high_mask = _mm512_set1_epi32(-1);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        dest[i] = _mm512_xor_si512(src[i], high_mask);
#else
      const __m256i* src = reinterpret_cast<const __m256i*>(data_);
      __m256i* dest = reinterpret_cast<__m256i*>(res.data_);

      const __m256i high_mask = _mm256_set1_epi32(-1);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        dest[i] = _mm256_xor_si256(src[i], high_mask);
#endif

      return res;
    }

    [[using gnu: always_inline, hot]] bitset&
    operator&=(const bitset& rhs) noexcept {
#ifdef AVX512
      __m512i* dest = reinterpret_cast<__m512i*>(data_);
      const __m512i* rhs_src = reinterpret_cast<const __m512i*>(rhs.data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        dest[i] = _mm512_and_si512(dest[i], rhs_src[i]);
#else
      __m256i* dest = reinterpret_cast<__m256i*>(data_);
      const __m256i* rhs_src = reinterpret_cast<const __m256i*>(rhs.data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        dest[i] = _mm256_and_si256(dest[i], rhs_src[i]);
#endif

      return *this;
    }

    [[using gnu: always_inline, hot]] bitset&
    operator|=(const bitset& rhs) noexcept {
#ifdef AVX512
      __m512i* dest = reinterpret_cast<__m512i*>(data_);
      const __m512i* rhs_src = reinterpret_cast<const __m512i*>(rhs.data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        dest[i] = _mm512_or_si512(dest[i], rhs_src[i]);
#else
      __m256i* dest = reinterpret_cast<__m256i*>(data_);
      const __m256i* rhs_src = reinterpret_cast<const __m256i*>(rhs.data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        dest[i] = _mm256_or_si256(dest[i], rhs_src[i]);
#endif

      return *this;
    }

    [[nodiscard]] [[using gnu: always_inline, hot]] bitset
    operator^(const bitset& rhs) const noexcept {
      bitset res;

#ifdef AVX512
      const __m512i *src = reinterpret_cast<const __m512i*>(data_),
                    *rhs_src = reinterpret_cast<const __m512i*>(rhs.data_);
      __m512i* dest = reinterpret_cast<__m512i*>(res.data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        dest[i] = _mm512_xor_si512(src[i], rhs_src[i]);
#else
      const __m256i *src = reinterpret_cast<const __m256i*>(data_),
                    *rhs_src = reinterpret_cast<const __m256i*>(rhs.data_);
      __m256i* dest = reinterpret_cast<__m256i*>(res.data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        dest[i] = _mm256_xor_si256(src[i], rhs_src[i]);
#endif

      return res;
    }

    [[nodiscard]] [[using gnu: always_inline, hot]] bitset
    operator<<(uint_fast16_t s) const noexcept {
      bitset res = *this;
      res <<= s;
      return res;
    }

    [[nodiscard]] [[using gnu: always_inline, hot]] bitset
    operator>>(uint_fast16_t s) const noexcept {
      bitset res = *this;
      res >>= s;
      return res;
    }

    [[using gnu: always_inline, hot]] bitset&
    operator<<=(uint_fast16_t s) noexcept {
      assert(s < N << 6);

      const uint_fast16_t word_shift = s >> 6;
      const uint_fast8_t bit_shift = s & 63;

      if (bit_shift == 0)
        for (uint_fast16_t i = N - 1; i >= word_shift; --i) {
          data_[i] = data_[i - word_shift];
          if (i == 0) break;
        }
      else {
        const uint_fast8_t shift = 64 - bit_shift;

        for (uint_fast16_t i = N - 1; i >= word_shift; --i) {
          const uint_fast16_t j = i - word_shift;
          data_[i] = data_[j] << bit_shift;
          if (j > 0) data_[i] |= data_[j - 1] >> shift;
          if (i == 0) break;
        }
      }

      for (uint_fast8_t i = 0; i < word_shift; ++i) data_[i] = 0;

      return *this;
    }

    [[using gnu: always_inline, hot]] bitset&
    operator>>=(uint_fast16_t s) noexcept {
      assert(s < N << 6);

      const uint_fast16_t word_shift = s >> 6;
      const uint_fast8_t bit_shift = s & 63;

      if (bit_shift == 0)
        for (uint_fast8_t i = 0; i < N - word_shift; ++i)
          data_[i] = data_[i + word_shift];
      else {
        const uint_fast8_t shift = 64 - bit_shift;

        for (uint_fast8_t i = 0; i < N - word_shift; ++i) {
          uint_fast16_t j = i + word_shift;
          data_[i] = data_[j] >> bit_shift;
          if (j < N - 1) data_[i] |= data_[j + 1] << shift;
        }
      }

      for (uint_fast8_t i = N - word_shift; i < N; ++i) data_[i] = 0;

      return *this;
    }

    bool operator==(const bitset& rhs) const noexcept {
#if AVX512
      const __m512i *data = reinterpret_cast<const __m512i*>(data_),
                    *rhs_data = reinterpret_cast<const __m512i*>(rhs.data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i) {
        const __mmask8 mask = _mm512_cmpeq_epi64_mask(
          _mm512_load_si512(data + i), _mm512_load_si512(rhs_data + i));
        if (mask != 0xFF) return false;
      }
#else
      const __m256i *data = reinterpret_cast<const __m256i*>(data_),
                    *rhs_data = reinterpret_cast<const __m256i*>(rhs.data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i) {
        const __m256i avx_word = _mm256_xor_si256(
          _mm256_load_si256(data + i), _mm256_load_si256(rhs_data + i));
        if (!_mm256_testz_si256(avx_word, avx_word)) return false;
      }
#endif

      return true;
    }

    [[gnu::always_inline]] bool operator!=(const bitset& rhs) const noexcept {
      return !(*this == rhs);
    }

    [[gnu::always_inline]] void set() noexcept {
#if AVX512
      __m512i* data = reinterpret_cast<__m512i*>(data_);

      const __m512i high_mask = _mm512_set1_epi32(-1);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        _mm512_store_si512(data + i, high_mask);
#else
      __m256i* data = reinterpret_cast<__m256i*>(data_);

      const __m256i high_mask = _mm256_set1_epi32(-1);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        _mm256_store_si256(data + i, high_mask);
#endif
    }

    [[gnu::always_inline]] constexpr void set(uint_fast16_t pos) noexcept {
      assert(pos < (N << 6));
      data_[pos >> 6] |= 1ull << (pos & 63);
    }

    [[gnu::always_inline]] void reset() noexcept {
#ifdef AVX512
      __m512i* data = reinterpret_cast<__m512i*>(data_);

      const __m512i low_mask = _mm512_setzero_si512();

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        _mm512_store_si512(data + i, low_mask);
#else
      __m256i* data = reinterpret_cast<__m256i*>(data_);

      const __m256i low_mask = _mm256_setzero_si256();

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        _mm256_store_si256(data + i, low_mask);
#endif
    }

    [[gnu::always_inline]] constexpr void reset(uint_fast16_t pos) noexcept {
      assert(pos < (N << 6));
      data_[pos >> 6] &= ~(1ull << (pos & 63));
    }

    [[nodiscard]] [[gnu::always_inline]] constexpr bool
    test(uint_fast16_t pos) const noexcept {
      assert(pos < (N << 6));
      return (data_[pos >> 6] >> (pos & 63)) & 1;
    }

    [[nodiscard]] [[gnu::always_inline]] uint_fast16_t count() const noexcept {
      uint_fast16_t count = 0;

#if AVX512 && defined(__AVX512VPOPCNTDQ__)
      const __m512i* data = reinterpret_cast<const __m512i*>(data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i)
        count += _mm512_reduce_add_epi64(
          _mm512_popcnt_epi64(_mm512_load_si512(data + i)));
#else
      for (uint_fast8_t i = 0; i < N; ++i)
        count += __builtin_popcountll(data_[i]);
#endif

      return count;
    }

    [[nodiscard]] [[gnu::always_inline]] constexpr uint_fast16_t
    first() const noexcept {
      for (uint_fast8_t i = 0; i < N; ++i)
        if (data_[i]) return (i << 6) + __builtin_ctzll(data_[i]);

      assert(false);
      __builtin_unreachable();
    }

    [[nodiscard]] [[gnu::always_inline]] bool none() const noexcept {
#if AVX512
      const __m512i* data = reinterpret_cast<const __m512i*>(data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i) {
        const __m512i avx_word = _mm512_load_si512(data + i);
        if (_mm512_testn_epi64_mask(avx_word, avx_word) != 0xFF) return false;
      }
#else
      const __m256i* data = reinterpret_cast<const __m256i*>(data_);

      for (uint_fast8_t i = 0; i < avx_word_count_; ++i) {
        const __m256i avx_word = _mm256_load_si256(data + i);
        if (!_mm256_testz_si256(avx_word, avx_word)) return false;
      }
#endif

      return true;
    }

    [[using gnu: always_inline, hot]] [[nodiscard]] bool any() const noexcept {
      return !none();
    }

    [[maybe_unused]] [[nodiscard]] constexpr uint64_t
    operator[](uint_fast8_t i) const noexcept {
      return data_[i];
    }

    [[maybe_unused]] [[nodiscard]] constexpr uint64_t&
    operator[](uint_fast8_t i) noexcept {
      return data_[i];
    }

    [[maybe_unused]] friend std::ostream& operator<<(std::ostream& o,
                                                     const bitset& b) {
      o << "0x" << std::hex << b.data_[N - 1];

      for (int_fast8_t i = N - 2; i >= 0; --i)
        o << std::setfill('0') << std::setw(16) << std::hex << b.data_[i];

      return o;
    }
};

#endif

