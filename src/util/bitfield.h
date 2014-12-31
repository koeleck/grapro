#ifndef UTIL_BITFIELD_H
#define UTIL_BITFIELD_H

#include <type_traits>
#include <initializer_list>

namespace util
{

template <typename T>
class bitfield
{
    // Make sure, only enums are used.
    static_assert(std::is_enum<T>::value, "bitfield requires an enum type");

public:
    typedef typename std::underlying_type<T>::type type;

private:
    static constexpr type setBits(typename std::initializer_list<T>::iterator el, typename std::initializer_list<T>::iterator end)
    {
        return (el == end) ? 0 : (static_cast<type>(*el) | setBits(el + 1, end));
    }
    type   m_value;

public:

    constexpr bitfield() : m_value{0} {}

    constexpr bitfield(const T flag) : m_value{static_cast<type>(flag)} {}

    constexpr bitfield(std::initializer_list<T> flags) : m_value{setBits(flags.begin(), flags.end())} {}

    void clear() noexcept
    {
        m_value = 0;
    }

    explicit operator type() const noexcept
    {
        return m_value;
    }

    explicit operator bool() const noexcept
    {
        return m_value != 0;
    }

    type operator()() const noexcept
    {
        return m_value;
    }

    bitfield operator~() const noexcept
    {
        return ~m_value;
    }

    bitfield& operator=(const T flag) noexcept
    {
        m_value = static_cast<type>(flag);
        return *this;
    }

    bitfield& operator|=(const T flag) noexcept
    {
        m_value |= static_cast<type>(flag);
        return *this;
    }

    bitfield& operator&=(const T flag) noexcept
    {
        m_value &= static_cast<type>(flag);
        return *this;
    }

    bitfield& operator^=(const T flag) noexcept
    {
        m_value ^= static_cast<type>(flag);
        return *this;
    }

    bool operator!() const noexcept
    {
        return m_value == 0;
    }

};

template <typename T>
bitfield<T> operator&(const bitfield<T>& bf0, const bitfield<T>& bf1) noexcept
{
    return bf0() & bf1();
}

template <typename T>
bitfield<T> operator|(const bitfield<T>& bf0, const bitfield<T>& bf1) noexcept
{
    return bf0() | bf1();
}

template <typename T>
bitfield<T> operator^(const bitfield<T>& bf0, const bitfield<T>& bf1) noexcept
{
    return bf0() ^ bf1();
}

template <typename T>
bool operator==(const bitfield<T>& bf0, const bitfield<T>& bf1) noexcept
{
    return bf0() == bf1();
}

} // namespace util

#endif // UTIL_BITFIELD_H

