#include <type_traits>
#include <ostream>

#include "vars.h"
#include "config_file.h"
#include "log/log.h"

framework::Variables vars;

namespace framework
{

#define VAR_ARG(...) __VA_ARGS__

//////////////////////////////////////////////////////////////////////////

#define DEF_VAR(var_name, var_type, default_value) var_name{default_value},
Variables::Variables() :
#include "vars.def"
this_is_very_important{0}
{

}
#undef DEF_VAR

//////////////////////////////////////////////////////////////////////////

namespace {

// helper struct/function, to prevent assignment to const variables
template <bool is_const, typename T>
struct AssignmentHelper
{
    static bool assign(T& dst, T&& src) {
        dst = std::move(src);
        return true;
    }
};

template <typename T>
struct AssignmentHelper<true, T>
{
    static bool assign(T&, T&&) {
        return false;
    }
};

template <typename T>
bool assign(T& dst, typename std::remove_const<T>::type&& src)
{
    return AssignmentHelper<std::is_const<T>::value, T>::assign(dst, std::move(src));
}

} // namespace

//////////////////////////////////////////////////////////////////////////

//#define CONFIG_VAR(name, type, default) name = config_file.get<type>(#name, name);
#define DEF_VAR(var_name, var_type, default_value)                                                       \
    if (config_file.has(#var_name)) {                                                                       \
        if (!assign(var_name, config_file.get<std::remove_const<var_type>::type>(#var_name))) {             \
            LOG_WARNING("Warning: assignment to read-only variable '" #var_name "' failed!");               \
        }                                                                                                   \
    }
void Variables::load(const ConfigFile& config_file)
{
#include "vars.def"
}
#undef DEF_VAR

//////////////////////////////////////////////////////////////////////////

void Variables::dump(std::ostream& output) const
{
#define DEF_VAR(var_name, var_type, default_value) \
    output << (#var_name " ") << var_name << std::endl;
#include "vars.def"
#undef DEF_VAR
}

//////////////////////////////////////////////////////////////////////////

} // namespace framework
