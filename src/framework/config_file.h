#ifndef FRAMEWORK_CONFIG_FILE_H
#define FRAMEWORK_CONFIG_FILE_H

#include <unordered_map>
#include <string>
#include <stdexcept>
#include <type_traits>

#include <boost/spirit/home/support/detail/hold_any.hpp>

namespace framework
{

class ConfigFile
{
public:

    ConfigFile() = default;
    ConfigFile(const std::string& filename);

    bool has(const std::string& name) const;

    template <typename T>
    T get(const std::string& name) const
    {
        T val;
        if (!assign(val, name))
            throw std::runtime_error("Assignment failed!");
        return val;
    }

    template <typename T>
    bool assign(T& v, const std::string& name) const
    {
        const hold_any* var = getVar(name);
        if (var == nullptr)
            return false;
        const auto* ptr = getValue<T>(*var);
        if (ptr == nullptr)
            return false;
        v = *ptr;
        return true;
    }

private:
    using hold_any = boost::spirit::hold_any;

    const hold_any* getVar(const std::string& name) const
    {
        auto it = m_vars.find(name);
        if (it == m_vars.end())
            return nullptr;
        return &(it->second);
    }

    template <typename T>
    typename std::enable_if<std::is_floating_point<T>::value, const double*>::type
    getValue(const hold_any& var) const
    {
        return boost::spirit::any_cast<double>(&var);
    }

    template <typename T>
    typename std::enable_if<std::is_integral<T>::value && !std::is_same<T, bool>::value, const int*>::type
    getValue(const hold_any& var) const
    {
        return boost::spirit::any_cast<int>(&var);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, bool>::value, const bool*>::type
    getValue(const hold_any& var) const
    {
        return boost::spirit::any_cast<bool>(&var);
    }

    template <typename T>
    typename std::enable_if<std::is_same<T, std::string>::value, const std::string*>::type
    getValue(const hold_any& var) const
    {
        return boost::spirit::any_cast<std::string>(&var);
    }


    std::unordered_map<std::string, hold_any>    m_vars;
};

} // namespace framework

#endif // FRAMEWORK_CONFIG_FILE_H
