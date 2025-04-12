#ifndef GF2_H_
#define GF2_H_

#include <Eigen/Core>

class GF2 {
public:
    // Конструкторы
    GF2() : value_{0} {}

    GF2(bool b) : value_(b ? 1 : 0) {}

    GF2(int i) : value_(i ? 1 : 0) {
        if (i > 1 || i < 0)
            throw std::range_error{"GF2 value must be 0 or 1"};
    }

    GF2(size_t i) : value_(i ? 1 : 0) {
        if (i > 1)
            throw std::range_error{"GF2 value must be 0 or 1"};
    }

    // Операции над элементами поля GF(2)
    GF2 &operator+=(const GF2 &other) {
        value_ ^= other.value_;
        return *this;
    }

    friend GF2 operator+(GF2 lhs, const GF2 &rhs) {
        lhs += rhs;
        return lhs;
    }

    GF2 &operator*=(const GF2 &other) {
        value_ &= other.value_;
        return *this;
    }

    friend GF2 operator*(GF2 lhs, const GF2 &rhs) {
        lhs *= rhs;
        return lhs;
    }

    // Операции сравнения
    bool operator==(const GF2 &other) const { return value_ == other.value_; }

    bool operator!=(const GF2 &other) const { return value_ != other.value_; }

    bool operator==(const int &other) const { return value_ == other; }

    bool operator!=(const int &other) const { return value_ != other; }

    // Приведение типов
    operator bool() const { return value_ != 0; }

    operator int() const { return value_ != 0; }

    operator size_t() const { return value_ != 0; }

    operator char() const { return value_ != 0 ? 1 : 0; }

    operator double() const { return value_ != 0 ? 1.0 : 0.0; }

    friend std::ostream &operator<<(std::ostream &stream, GF2 const &element) {
        stream << static_cast<int>(element);
        return stream;
    }

    friend std::istream &operator>>(std::istream &stream, GF2 &element) {
        int elemValue{0};
        if (!stream >> elemValue) {
            throw std::ios_base::failure{"Invalid value for GF2"};
        }
        element = elemValue;
        return stream;
    }

private:
    unsigned char value_;
};

namespace Eigen {

    template<>
    struct NumTraits<GF2> : NumTraits<unsigned char> {
        typedef GF2 Real;
        typedef GF2 NonInteger;
        typedef GF2 Nested;

        enum {
            IsComplex = 0,
            IsInteger = 1,
            IsSigned = 0,
            RequireInitialization = 1,
            ReadCost = 1,
            AddCost = 1,
            MulCost = 1
        };

        static inline GF2 epsilon() { return GF2(false); }

        static inline GF2 dummy_precision() { return GF2(false); }
    };

}  // namespace Eigen

#endif  // GF2_H_