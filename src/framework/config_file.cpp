#include <fstream>
#include <cstring>
#include <stdexcept>

#include "config_file.h"
#include "log/log.h"

#include <boost/config/warning_disable.hpp>
#include <boost/spirit/include/qi.hpp>
#include <boost/spirit/include/phoenix_core.hpp>
#include <boost/spirit/include/phoenix_operator.hpp>
#include <boost/spirit/include/phoenix_fusion.hpp>
#include <boost/fusion/adapted.hpp>

//////////////////////////////////////////////////////////////////////////

namespace qi = boost::spirit::qi;
namespace ascii = boost::spirit::ascii;
using boost::spirit::hold_any;
using boost::spirit::any_cast;

//////////////////////////////////////////////////////////////////////////

struct ConfVar {
    std::string name;
    hold_any    data;
};

BOOST_FUSION_ADAPT_STRUCT(
        ConfVar,
        (std::string, name)
        (hold_any,    data)
)

//////////////////////////////////////////////////////////////////////////

namespace framework
{

//////////////////////////////////////////////////////////////////////////

template <typename Iterator>
struct config_parser : qi::grammar<Iterator, ConfVar(), ascii::space_type>
{
    config_parser() : config_parser::base_type(start)
    {
        using qi::int_;
        using qi::double_;
        using qi::bool_;
        using qi::lexeme;
        using qi::char_;
        using qi::alpha;
        using qi::alnum;
        //using boost::phoenix::ref;
        using qi::_1;
        using qi::_val;
        //using ascii::space;
        //using qi::lit;
        using boost::phoenix::at_c;

        qi::real_parser<double, qi::strict_real_policies<double> > strict_double;
        varname = lexeme[+(alpha | char_("_")) >> *(alnum | char_("_"))];
        quoted_string = lexeme['"' >> +(char_ - '"') >> '"'];

        start =
            varname [at_c<0>(_val) = _1]
            >> '=' >>
            (
                quoted_string [at_c<1>(_val) = _1] |
                bool_         [at_c<1>(_val) = _1] |
                strict_double [at_c<1>(_val) = _1] |
                int_          [at_c<1>(_val) = _1]
            ) >> -('#' >> *char_);
    }

    qi::rule<Iterator, std::string(), ascii::space_type> varname;
    qi::rule<Iterator, std::string(), ascii::space_type> quoted_string;
    qi::rule<Iterator, ConfVar(), ascii::space_type> start;
};

//////////////////////////////////////////////////////////////////////////

ConfigFile::ConfigFile(const std::string& filename)
{
    std::ifstream is(filename.c_str());
    if (!is) {
        throw std::runtime_error("file not found: " + filename);
    }

    typedef std::string::const_iterator Iterator;
    config_parser<Iterator> g;

    bool error = false;
    std::string line;
    unsigned int line_number = 0;
    while (std::getline(is, line)) {
        ++line_number;
        // check empty/comment line
        const size_t line_beg = line.find_first_not_of(" \t");
        if (line_beg == std::string::npos || line[line_beg] == '#')
            continue;

        ConfVar var;
        Iterator it = line.begin();
        Iterator end = line.end();
        bool r = qi::phrase_parse(it, end, g, ascii::space, var);
        if (r && it == end) {
            m_vars.emplace(std::move(var.name), std::move(var.data));
        } else {
            LOG_ERROR(logtag::ConfigFile, "Error parsing file ",
                    filename, ":", line_number, " \"", line,
                    "\" (remaining: ", std::string(it, end), ')');
            error = true;
            break;
        }

    }
    if (error) {
        throw std::runtime_error("error parsing file " + filename);
    }
}

//////////////////////////////////////////////////////////////////////////

bool ConfigFile::has(const std::string& name) const
{
    return m_vars.end() != m_vars.find(name);
}

//////////////////////////////////////////////////////////////////////////

} // namespace framework
