#include "../include/fraction.h"
#include <cmath>
#include <sstream>

big_int gcd(big_int a, big_int b) {
    if (a < 0) a = 0_bi - a;
    if (b < 0) b = 0_bi - b;

    while (b != 0) {
        big_int tmp = b;
        b = a % b;
        a = tmp;
    }

    return a;
}

void fraction::optimise() {
    if (_denominator == 0) throw std::invalid_argument("Denominator cannot be zero");
    if (_numerator == 0) {
        _denominator = 1;
        return;
    }

    big_int divisor = gcd(_numerator, _denominator);
    _numerator /= divisor;
    _denominator /= divisor;
    if (_denominator < 0) {
        _numerator = 0_bi - _numerator;
        _denominator = 0_bi - _denominator;
    }
}

fraction::fraction(const pp_allocator<big_int::value_type> allocator)
        : _numerator(0, allocator), _denominator(1, allocator) {}

fraction& fraction::operator+=(fraction const& other) & {
    _numerator = _numerator * other._denominator + _denominator * other._numerator;
    _denominator = _denominator * other._denominator;
    optimise();
    return *this;
}

fraction fraction::operator+(fraction const& other) const {
    fraction result = *this;
    result += other;
    return result;
}

fraction& fraction::operator-=(fraction const& other) & {
    _numerator = _numerator * other._denominator - _denominator * other._numerator;
    _denominator = _denominator * other._denominator;
    optimise();
    return *this;
}

fraction fraction::operator-(fraction const& other) const {
    fraction result = *this;
    result -= other;
    return result;
}

fraction& fraction::operator*=(fraction const& other) & {
    _numerator *= other._numerator;
    _denominator *= other._denominator;
    optimise();
    return *this;
}

fraction fraction::operator*(fraction const& other) const {
    fraction result = *this;
    result *= other;
    return result;
}

fraction& fraction::operator/=(fraction const& other) & {
    if (other._numerator == 0) throw std::invalid_argument("Division by zero");
    _numerator *= other._denominator;
    _denominator *= other._numerator;
    optimise();
    return *this;
}

fraction fraction::operator/(fraction const& other) const {
    fraction result = *this;
    result /= other;
    return result;
}

fraction fraction::operator-() const {
    fraction result = *this;
    result._numerator = 0_bi - result._numerator;
    return result;
}

bool fraction::operator==(fraction const& other) const noexcept {
    return _numerator == other._numerator && _denominator == other._denominator;
}

std::partial_ordering fraction::operator<=>(const fraction& other) const noexcept {
    big_int l_val = _numerator * other._denominator;
    big_int r_val = _denominator * other._numerator;
    if (l_val < r_val) return std::partial_ordering::less;
    if (l_val > r_val) return std::partial_ordering::greater;
    return std::partial_ordering::equivalent;
}

std::ostream &operator<<(std::ostream &stream, fraction const &obj) {
    return stream << obj.to_string();
}

std::istream& operator>>(std::istream& stream, fraction& obj) {
    std::string input;
    stream >> input;

    size_t slash_pos = input.find('/');
    std::string num_str, denom_str;

    if (slash_pos != std::string::npos) {
        num_str = input.substr(0, slash_pos);
        denom_str = input.substr(slash_pos + 1);
        if (denom_str.empty()) {
            throw std::invalid_argument("Invalid fraction format");
        }
    } else {
        num_str = input;
        denom_str = "1";
    }

    auto is_valid_number = [](const std::string& s) {
        if (s.empty()) return false;
        size_t start = 0;
        if (s[0] == '+' || s[0] == '-') {
            start = 1;
            if (s.length() == 1) return false;
        }
        for (size_t i = start; i < s.length(); i++) {
            if (!isdigit(s[i])) return false;
        }
        return true;
    };

    if (!is_valid_number(num_str) || !is_valid_number(denom_str)) {
        throw std::invalid_argument("Invalid fraction format");
    }

    try {
        big_int numerator(num_str, 10);
        big_int denominator(denom_str, 10);
        if (denominator == 0) {
            throw std::invalid_argument("Denominator cannot be zero");
        }
        obj = fraction(numerator, denominator);
    } catch (const std::exception& e) {
        throw std::invalid_argument(std::string("Error creating fraction: ") + e.what());
    }

    return stream;
}

std::string fraction::to_string() const {
    std::stringstream ss;
    ss << _numerator << "/" << _denominator;
    return ss.str();
}

fraction fraction::sin(fraction const& epsilon) const {
    auto x = *this;
    fraction result(0, 1);
    fraction term = x;
    int n = 1;
    fraction prev_result;

    do {
        prev_result = result;
        result += term;
        term = term * (-x * x) / fraction((2*n)*(2*n + 1), 1);
        n++;
    } while ((result - prev_result > epsilon) || (prev_result - result > epsilon));

    return result;
}

fraction fraction::cos(fraction const& epsilon) const {
    auto x = *this;
    fraction result(1, 1);
    fraction term(1, 1);
    int n = 1;
    fraction prev_result;

    do {
        prev_result = result;
        term = term * (-x * x) / fraction((2*n - 1)*(2*n), 1);
        result += term;
        n++;
    } while ((result - prev_result > epsilon) || (prev_result - result > epsilon));

    return result;
}

fraction fraction::tg(fraction const& epsilon) const {
    auto c = cos(epsilon * epsilon);
    if (c._numerator == 0) throw std::domain_error("Tangent undefined");
    return sin(epsilon * epsilon) / c;
}

fraction fraction::ctg(fraction const& epsilon) const {
    auto s = sin(epsilon * epsilon);
    if (s._numerator == 0) throw std::domain_error("Cotangent undefined");
    return cos(epsilon * epsilon) / s;
}

fraction fraction::sec(fraction const& epsilon) const {
    auto c = cos(epsilon);
    if (c._numerator == 0) throw std::domain_error("Secant undefined");
    return fraction(1, 1) / c;
}

fraction fraction::cosec(fraction const& epsilon) const {
    auto s = sin(epsilon);
    if (s._numerator == 0) throw std::domain_error("Cosecant undefined");
    return fraction(1, 1) / s;
}

fraction fraction::arcsin(const fraction& epsilon) const {
    if (*this < fraction(-1, 1) || *this > fraction(1, 1)) {
        throw std::domain_error("Arcsin undefined for |x| > 1");
    }

    fraction x = *this;
    fraction result(0, 1);
    fraction term = x;
    big_int n = 1;
    fraction prev_result;

    do {
        prev_result = result;
        result += term;
        term = term * x * x * fraction((2_bi *n - 1)*(2_bi *n - 1), (2_bi*n)*(2_bi*n + 1));
        n += 1;
    } while ((result - prev_result > epsilon) || (prev_result - result > epsilon));

    return result;
}

fraction fraction::arccos(const fraction& epsilon) const {
    if (*this < fraction(-1, 1) || *this > fraction(1, 1)) {
        throw std::domain_error("Arccos undefined for |x| > 1");
    }
    return fraction(1, 2).arcsin(epsilon) * fraction(3, 1) - arcsin(epsilon);
}

fraction fraction::arctg(fraction const &epsilon) const {
    auto x = *this;

    if (*this > fraction(1, 1)) {
        return fraction(1, 2).arcsin(epsilon) * fraction(3, 1) -
               (fraction(1, 1) / *this).arctg(epsilon);
    }

    fraction result(0, 1);
    fraction term = *this;
    big_int n = 1;
    fraction tmp_result;

    do {
        tmp_result = result;
        result += fraction(1, n) * term;
        n += 2_bi;
        term = -term * (x * x);
    } while ((result - tmp_result > epsilon) || (tmp_result - result > epsilon));

    return result;
}


fraction fraction::arcctg(fraction const& epsilon) const {
    if (_numerator == 0) throw std::domain_error("Arccotangent undefined");
    return (fraction(1, 1) / *this).arctg(epsilon);
}

fraction fraction::arcsec(fraction const& epsilon) const {
    if (_numerator == 0) throw std::domain_error("Arcsecant undefined");
    fraction reciprocal = fraction(1, 1) / *this;
    return reciprocal.arccos(epsilon);
}

fraction fraction::arccosec(fraction const& epsilon) const {
    if (_numerator == 0) throw std::domain_error("Arccosecant undefined");
    fraction reciprocal = fraction(1, 1) / *this;
    return reciprocal.arcsin(epsilon);
}

fraction fraction::pow(size_t degree) const {
    if (degree == 0) return {1, 1};

    fraction base = *this;
    fraction result(1, 1);
    while (degree > 0) {
        if (degree & 1) result *= base;
        base *= base;
        degree >>= 1;
    }
    return result;
}

fraction fraction::root(size_t degree, fraction const& epsilon) const {
    if (degree <= 0) throw std::invalid_argument("Degree must be positive");
    if (degree == 1) return *this;
    if (_numerator < 0 && degree % 2 == 0)
        throw std::domain_error("Even root of negative number");

    fraction x = *this;
    if (x._numerator < 0) x = -x;

    fraction guess = *this / fraction(degree, 1);
    fraction prev_guess;

    do {
        prev_guess = guess;
        fraction power = guess.pow(degree - 1);
        if (power._numerator == 0)
            throw std::logic_error("Division by zero in root calculation");

        guess = (fraction(degree - 1, 1) * guess + *this / power)
                / fraction(degree, 1);
    } while ((guess - prev_guess > epsilon) || (prev_guess - guess > epsilon));

    if (_numerator < 0 && degree % 2 == 1)
        guess = -guess;

    return guess;
}

fraction fraction::log2(fraction const& epsilon) const {
    if (_numerator <= 0 || _denominator <= 0)
        throw std::domain_error("Logarithm of non-positive number");
    return ln(epsilon) / fraction(2, 1).ln(epsilon);
}

fraction fraction::ln(fraction const& epsilon) const {
    if (_numerator <= 0 || _denominator <= 0)
        throw std::domain_error("Natural logarithm of non-positive number");

    fraction y = (*this - fraction(1, 1)) / (*this + fraction(1, 1));
    fraction result(0, 1);
    fraction term = y;
    fraction prev_result;
    int n = 1;

    do {
        prev_result = result;
        result += term / fraction(n, 1);
        term = term * (y * y);
        n += 2;
    } while ((result - prev_result > epsilon) || (prev_result - result > epsilon));

    return result * fraction(2, 1);
}

fraction fraction::lg(fraction const& epsilon) const {
    if (_numerator <= 0 || _denominator <= 0)
        throw std::domain_error("Base-10 logarithm of non-positive number");
    return ln(epsilon) / fraction(10, 1).ln(epsilon);
}