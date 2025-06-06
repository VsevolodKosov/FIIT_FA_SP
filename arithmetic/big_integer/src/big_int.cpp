#include "../include/big_int.h"

#include <algorithm>
#include <cmath>
#include <sstream>
#include <string>

unsigned long long BASE = 1ULL << (8 * sizeof(unsigned int));


bool is_zero(const std::vector<unsigned int, pp_allocator<unsigned int>> &digits) {
    return digits.size() == 1 && digits[0] == 0;
}

void optimise(std::vector<unsigned int, pp_allocator<unsigned int>> &digits) {
    while (digits.size() > 1 && digits.back() == 0) {
        digits.pop_back();
    }
}

std::string big_int::to_string() const {
    if (is_zero(_digits)) return "0";

    std::string res;
    big_int tmp = *this;
    tmp._sign = true;
    while (tmp) {
        auto val = tmp % 10;
        res += static_cast<char>('0' + val._digits[0]);
        tmp /= 10;
    }

    if (!_sign) {
        res += '-';
    }

    std::reverse(res.begin(), res.end());
    return res;
}

big_int::multiplication_rule big_int::decide_mult(size_t rhs) const noexcept {
    return rhs > 64 ? big_int::multiplication_rule::Karatsuba : big_int::multiplication_rule::trivial;
}

big_int::division_rule big_int::decide_div(size_t rhs) const noexcept {
    return big_int::division_rule::trivial;
}

std::strong_ordering big_int::operator<=>(const big_int &other) const noexcept {
    if (_sign != other._sign) return _sign ? std::strong_ordering::greater : std::strong_ordering::less;

    bool is_pos = _sign;
    if (_digits.size() != other._digits.size()) {
        return is_pos ? _digits.size() <=> other._digits.size() : other._digits.size() <=> _digits.size();
    }

    for (int i = static_cast<int>(_digits.size()) - 1; i >= 0; --i) {
        if (_digits[i] != other._digits[i]) {
            return is_pos ? _digits[i] <=> other._digits[i] : other._digits[i] <=> _digits[i];
        }
    }

    return std::strong_ordering::equal;
}

bool big_int::operator==(const big_int &other) const noexcept {
    return std::strong_ordering::equal == (*this <=> other);
}

big_int operator""_bi(unsigned long long n) {
    return {n};
}

big_int::operator bool() const noexcept {
    return !is_zero(_digits);
}

big_int &big_int::operator++() & {
    *this += big_int(1, _digits.get_allocator());
    return *this;
}

big_int big_int::operator++(int) {
    auto tmp = *this;
    ++(*this);
    return tmp;
}

big_int &big_int::operator--() & {
    *this -= big_int(1, _digits.get_allocator());
    return *this;
}

big_int big_int::operator--(int) {
    auto tmp = *this;
    --(*this);
    return tmp;
}

big_int &big_int::operator+=(const big_int &other) & {
    return plus_assign(other, 0);
}

big_int &big_int::operator-=(const big_int &other) & {
    return minus_assign(other, 0);
}

big_int &big_int::operator*=(const big_int &other) & {
    return multiply_assign(other, decide_mult((other._digits.size())));
}

big_int &big_int::operator/=(const big_int &other) & {
    return divide_assign(other, decide_div(other._digits.size()));
}

big_int &big_int::operator%=(const big_int &other) & {
    return modulo_assign(other, decide_div(other._digits.size()));
}

big_int big_int::operator+(const big_int &other) const {
    big_int tmp = *this;
    return tmp += other;
}

big_int big_int::operator-(const big_int &other) const {
    big_int tmp = *this;
    return tmp -= other;
}

big_int big_int::operator*(const big_int &other) const {
    big_int tmp = *this;
    return tmp *= other;
}

big_int big_int::operator/(const big_int &other) const {
    big_int tmp = *this;
    return tmp /= other;
}

big_int big_int::operator%(const big_int &other) const {
    big_int tmp = *this;
    return tmp %= other;
}

big_int big_int::operator~() const {
    big_int result(*this);
    for (auto &digit: result._digits) {
        digit = ~digit;
    }

    optimise(result._digits);
    return result;
}

big_int big_int::operator&(const big_int &other) const {
    big_int tmp = *this;
    return tmp &= other;
}

big_int big_int::operator|(const big_int &other) const {
    big_int tmp = *this;
    return tmp |= other;
}

big_int big_int::operator^(const big_int &other) const {
    big_int tmp = *this;
    return tmp ^= other;
}

big_int big_int::operator<<(size_t shift) const {
    big_int tmp = *this;
    return tmp <<= shift;
}

big_int big_int::operator>>(size_t shift) const {
    big_int tmp = *this;
    return tmp >>= shift;
}

big_int &big_int::operator&=(const big_int &other) & {
    size_t max_size = std::max(_digits.size(), other._digits.size());
    _digits.resize(max_size, 0);
    for (size_t i = 0; i < max_size; ++i) {
        _digits[i] &= (i < other._digits.size()) ? other._digits[i] : 0;
    }

    optimise(_digits);
    return *this;
}

big_int &big_int::operator|=(const big_int &other) & {
    size_t max_size = std::max(_digits.size(), other._digits.size());
    _digits.resize(max_size, 0);
    for (size_t i = 0; i < max_size; ++i) {
        _digits[i] |= (i < other._digits.size()) ? other._digits[i] : 0;
    }

    optimise(_digits);
    return *this;
}

big_int &big_int::operator^=(const big_int &other) & {
    size_t max_size = std::max(_digits.size(), other._digits.size());
    _digits.resize(max_size, 0);
    for (size_t i = 0; i < max_size; ++i) {
        _digits[i] ^= (i < other._digits.size()) ? other._digits[i] : 0;
    }

    optimise(_digits);
    return *this;
}

big_int &big_int::operator<<=(size_t shift) & {
    if (shift == 0 || (_digits.size() == 1 && _digits[0] == 0)) {
        return *this;
    }

    size_t full_shift = shift / BASE;
    size_t bit_shift = shift % BASE;

    if (full_shift > 0) {
        _digits.insert(_digits.begin(), full_shift, 0);
    }

    if (bit_shift > 0) {
        uint32_t carry = 0;
        for (size_t i = 0; i < _digits.size(); ++i) {
            uint64_t current = (static_cast<uint64_t>(_digits[i]) << bit_shift) | carry;
            _digits[i] = static_cast<uint32_t>(current & BASE);
            carry = static_cast<uint32_t>(current >> BASE);
        }
        if (carry != 0) {
            _digits.push_back(carry);
        }
    }

    optimise(_digits);
    return *this;
}

big_int &big_int::operator>>=(size_t shift) & {
    if (shift == 0 || (_digits.size() == 1 && _digits[0] == 0)) {
        return *this;
    }

    size_t full_shift = shift / BASE;
    size_t bit_shift = shift % BASE;

    if (full_shift >= _digits.size()) {
        _digits = {0};
        _sign = true;
        return *this;
    }

    _digits.erase(_digits.begin(), _digits.begin() + static_cast<std::vector<unsigned int>::difference_type>(full_shift));

    if (bit_shift > 0) {
        uint32_t carry = 0;
        for (size_t i = _digits.size(); i-- > 0;) {
            uint64_t current = (static_cast<uint64_t>(carry) << BASE) | _digits[i];
            _digits[i] = static_cast<uint32_t>(current >> bit_shift);
            carry = static_cast<uint32_t>(current & ((1ULL << bit_shift) - 1));
        }
    }

    optimise(_digits);
    if (is_zero(_digits)) {
        _sign = true;
    }

    return *this;
}

std::ostream &operator<<(std::ostream &stream, const big_int &value) {
    stream << value.to_string();
    return stream;
}

std::istream &operator>>(std::istream &stream, big_int &value) {
    std::string val;
    stream >> val;
    value = big_int(val, 10, value._digits.get_allocator());
    return stream;
}


big_int::big_int(const std::vector<unsigned int, pp_allocator<unsigned int>> &digits, bool sign) : _digits(digits), _sign(sign) {
    if (_digits.empty()) {
        _digits.push_back(0);
    }
    optimise(_digits);
}

big_int::big_int(std::vector<unsigned int, pp_allocator<unsigned int>> &&digits, bool sign) noexcept : _digits(std::move(digits)), _sign(sign) {
    if (_digits.empty()) {
        _digits.push_back(0);
    }
    optimise(_digits);
}


big_int::big_int(const std::string &num, unsigned int radix, pp_allocator<unsigned int> allocator) : _sign(true), _digits(allocator) {
    if (num.empty()) {
        _digits.push_back(0);
        return;
    }

    std::string num1 = num;
    bool is_neg = false;
    if (num1[0] == '-') {
        is_neg = true;
        num1 = num1.substr(1);
    } else if (num1[0] == '+') {
        num1 = num1.substr(1);
    }

    while (num1.size() > 1 && num1[0] == '0') {
        num1 = num1.substr(1);
    }

    if (num1.empty()) {
        _digits.push_back(0);
        return;
    }

    _digits.push_back(0);
    for (char c: num1) {
        if (!std::isdigit(c)) {
            throw std::invalid_argument("Invalid character in number string");
        }

        unsigned int digit = c - '0';
        *this *= 10;
        *this += big_int(static_cast<long long>(digit), allocator);
    }

    _sign = !is_neg;
    if (is_zero(_digits)) {
        _sign = true;
    }
}

big_int::big_int(pp_allocator<unsigned int> allocator) : _digits(allocator), _sign(true) {
    _digits.push_back(0);
}


big_int &big_int::plus_assign(const big_int &other, size_t shift) & {
    if (is_zero(other._digits)) return *this;

    if (_sign == other._sign) {
        size_t max_size = std::max(_digits.size(), other._digits.size() + shift);
        _digits.resize(max_size, 0);

        uint32_t carry = 0;
        for (size_t i = 0; i < max_size; ++i) {
            uint32_t a = (i < _digits.size()) ? _digits[i] : 0;
            uint32_t b = (i >= shift && (i - shift) < other._digits.size()) ? other._digits[i - shift] : 0;


            uint16_t a_lo = a & 0xFFFF, a_hi = a >> 16;
            uint16_t b_lo = b & 0xFFFF, b_hi = b >> 16;


            uint32_t lo = static_cast<uint32_t>(a_lo) + b_lo + (carry & 0xFFFF);
            carry = lo >> 16;

            uint32_t hi = static_cast<uint32_t>(a_hi) + b_hi + carry;
            carry = hi >> 16;

            _digits[i] = (hi << 16) | (lo & 0xFFFF);
        }

        if (carry > 0) {
            _digits.push_back(carry);
        }
    } else {
        big_int temp(other);
        temp._sign = _sign;
        minus_assign(temp, shift);
    }

    optimise(_digits);
    if (is_zero(_digits)) _sign = true;

    return *this;
}

big_int &big_int::minus_assign(const big_int &other, size_t shift) & {
    if (is_zero(other._digits)) return *this;

    if (_sign != other._sign) {
        big_int temp(other);
        temp._sign = _sign;
        return plus_assign(temp, shift);
    }

    big_int abs_this(*this);
    abs_this._sign = true;
    big_int abs_other(other);
    abs_other._sign = true;
    abs_other <<= shift;

    bool result_sign = _sign;
    if ((abs_this <=> abs_other) == std::strong_ordering::less) {
        result_sign = !result_sign;
        std::swap(abs_this._digits, abs_other._digits);
    }

    size_t max_size = std::max(abs_this._digits.size(), abs_other._digits.size());
    std::vector<unsigned int, pp_allocator<unsigned int>> result(max_size, 0, _digits.get_allocator());

    int32_t borrow = 0;
    for (size_t i = 0; i < max_size; ++i) {
        uint32_t a = (i < abs_this._digits.size()) ? abs_this._digits[i] : 0;
        uint32_t b = (i < abs_other._digits.size()) ? abs_other._digits[i] : 0;


        uint16_t a_lo = a & 0xFFFF, a_hi = a >> 16;
        uint16_t b_lo = b & 0xFFFF, b_hi = b >> 16;


        int32_t lo = static_cast<int32_t>(a_lo) - b_lo - (borrow & 0xFFFF);
        borrow = 0;
        if (lo < 0) {
            lo += 1 << 16;
            borrow = 1;
        }

        int32_t hi = static_cast<int32_t>(a_hi) - b_hi - borrow;
        borrow = 0;
        if (hi < 0) {
            hi += 1 << 16;
            borrow = 1;
        }

        result[i] = (hi << 16) | (lo & 0xFFFF);
    }

    _digits = std::move(result);
    _sign = result_sign;
    optimise(_digits);

    if (is_zero(_digits)) {
        _sign = true;
    }

    return *this;
}

big_int &big_int::multiply_assign(const big_int &other, big_int::multiplication_rule rule) & {
    if (is_zero(_digits) || is_zero(other._digits)) {
        _digits = {0};
        _sign = true;
        return *this;
    }

    size_t n = _digits.size();
    size_t m = other._digits.size();

    std::vector<unsigned int, pp_allocator<unsigned int>> result(n + m, 0, _digits.get_allocator());

    for (size_t i = 0; i < n; ++i) {
        uint32_t a = _digits[i];
        uint16_t a_lo = a & 0xFFFF;
        uint16_t a_hi = a >> 16;

        uint32_t carry = 0;
        for (size_t j = 0; j < m || carry; ++j) {
            uint64_t cur = result[i + j];
            uint32_t b = (j < m) ? other._digits[j] : 0;
            uint16_t b_lo = b & 0xFFFF;
            uint16_t b_hi = b >> 16;

            uint64_t part1 = static_cast<uint64_t>(a_lo) * b_lo;
            uint64_t part2 = static_cast<uint64_t>(a_lo) * b_hi;
            uint64_t part3 = static_cast<uint64_t>(a_hi) * b_lo;
            uint64_t part4 = static_cast<uint64_t>(a_hi) * b_hi;

            cur += part1 + ((part2 + part3) << 16) + (part4 << 32) + carry;
            result[i + j] = static_cast<uint32_t>(cur % BASE);
            carry = static_cast<uint32_t>(cur / BASE);
        }
    }

    _digits = std::move(result);
    _sign = (_sign == other._sign);
    optimise(_digits);
    return *this;
}

big_int &big_int::divide_assign(const big_int &other, big_int::division_rule rule) & {
    if (is_zero(_digits)) return *this;
    if (is_zero(other._digits)) throw std::logic_error("Division by zero");

    big_int abs_this(*this);
    abs_this._sign = true;
    big_int abs_other(other);
    abs_other._sign = true;
    if (abs_this < abs_other) {
        _digits.clear();
        _digits.push_back(0);
        _sign = true;
        return *this;
    }

    std::vector<unsigned int, pp_allocator<unsigned int>> quotient(_digits.size(), 0, _digits.get_allocator());
    big_int remain(_digits.get_allocator());
    remain._digits.clear();
    remain._digits.push_back(0);
    for (int i = static_cast<int>(_digits.size()) - 1; i >= 0; i--) {
        remain._digits.insert(remain._digits.begin(), _digits[i]);
        while (remain._digits.size() > 1 && remain._digits.back() == 0) {
            remain._digits.pop_back();
        }

        if (remain._digits.empty()) {
            remain._sign = true;
        }

        unsigned long long left = 0, q = 0, right = BASE;
        while (left <= right) {
            unsigned long long mid = left + (right - left) / 2;
            big_int temp = abs_other * big_int(static_cast<long long>(mid), _digits.get_allocator());
            if (remain >= temp) {
                q = mid;
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }

        if (q > 0) {
            big_int temp = abs_other * big_int(static_cast<long long>(q), _digits.get_allocator());
            remain -= temp;
        }
        quotient[i] = static_cast<unsigned int>(q);
    }

    _sign = (_sign == other._sign);
    _digits = std::move(quotient);
    optimise(_digits);
    return *this;
}

big_int &big_int::modulo_assign(const big_int &other, big_int::division_rule rule) & {
    if (is_zero(_digits)) return *this;
    if (is_zero(other._digits)) throw std::logic_error("Division by zero");

    big_int abs_this(*this);
    abs_this._sign = true;
    big_int abs_other(other);
    abs_other._sign = true;
    if (abs_this < abs_other) {
        _sign = true;
        return *this;
    }

    big_int remain(_digits.get_allocator());
    remain._digits.clear();
    remain._digits.push_back(0);
    for (int i = static_cast<int>(_digits.size()) - 1; i >= 0; --i) {
        remain._digits.insert(remain._digits.begin(), _digits[i]);
        while (remain._digits.size() > 1 && remain._digits.back() == 0) {
            remain._digits.pop_back();
        }

        if (remain._digits.empty()) {
            remain._sign = true;
        }

        unsigned long long left = 0, right = BASE;
        unsigned long long q = 0;
        while (left <= right) {
            unsigned long long mid = left + (right - left) / 2;
            big_int temp = abs_other * big_int(static_cast<long long>(mid), _digits.get_allocator());
            if (remain >= temp) {
                q = mid;
                left = mid + 1;
            } else {
                right = mid - 1;
            }
        }

        if (q > 0) {
            big_int temp = abs_other * big_int(static_cast<long long>(q), _digits.get_allocator());
            remain -= temp;
        }
    }

    _digits = std::move(remain._digits);
    _sign = true;
    optimise(_digits);
    return *this;
}

big_int multiply_karatsuba(const big_int &a, const big_int &b) {
    if (a._digits.size() < 32 || b._digits.size() < 32) {
        big_int result = a;
        result.multiply_assign(b, big_int::multiplication_rule::trivial);
        return result;
    }

    size_t max_size = std::max(a._digits.size(), b._digits.size()) / 2;

    big_int low1(a._digits.get_allocator()), high1(a._digits.get_allocator());
    big_int low2(b._digits.get_allocator()), high2(b._digits.get_allocator());

    low1._digits.assign(a._digits.begin(),
                        a._digits.begin() + static_cast<long long>(std::min(max_size, a._digits.size())));

    if (a._digits.size() > max_size) {
        high1._digits.assign(
                a._digits.begin() + static_cast<long long>(max_size),
                a._digits.end()
        );
    }

    low2._digits.assign(b._digits.begin(),
                        b._digits.begin() + static_cast<long long>(std::min(max_size, b._digits.size())));

    if (b._digits.size() > max_size) {
        high2._digits.assign(
                b._digits.begin() + static_cast<long long>(max_size),
                b._digits.end()
        );
    }

    low1._sign = high1._sign = low2._sign = high2._sign = true;

    big_int z0 = multiply_karatsuba(low1, low2);
    big_int z2 = multiply_karatsuba(high1, high2);

    big_int sum1 = low1;
    sum1.plus_assign(high1);
    big_int sum2 = low2;
    sum2.plus_assign(high2);

    big_int z1 = multiply_karatsuba(sum1, sum2);
    z1.minus_assign(z0);
    z1.minus_assign(z2);

    big_int result = z0;
    result.plus_assign(z1, max_size * sizeof(unsigned int));
    result.plus_assign(z2, max_size * 2 * sizeof(unsigned int));

    result._sign = (a._sign == b._sign);
    optimise(result._digits);
    return result;
}