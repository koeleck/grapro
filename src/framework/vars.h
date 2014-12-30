#ifndef FRAMEWORK_VARS_H
#define FRAMEWORK_VARS_H

#include <iosfwd>

namespace framework
{

class ConfigFile;

class Variables
{
public:
    Variables();

    void load(const ConfigFile& config_file);
    void dump(std::ostream& output) const;

#define VAR_ARG(...) __VA_ARGS__
#define DEF_VAR(name, type, default) type name;
#include "vars.def"
#undef DEF_VAR
#undef VAR_ARG

private:
    int this_is_very_important; // ... not
};

} // namespace framework

extern framework::Variables vars;

#endif // FRAMEWORK_VARS_H

