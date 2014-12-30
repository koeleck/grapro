#include "shadersource.h"

#include <cassert>
#include <fstream>
#include <sstream>
#include <utility>
//#include <regex>
#include <boost/regex.hpp>

#include <boost/tokenizer.hpp>
#include <boost/filesystem.hpp>
using namespace boost::filesystem;

#include "log/log.h"
#include "framework/vars.h"

/////////////////////////////////////////////////////////////////////

namespace
{
//
// local functions and stuff
//

/////////////////////////////////////////////////////////////////////

enum RegexResult
{
    None = 0x00,
    Include = 0x01,
    Local = 0x02,
    System = 0x04,
    Error = 0x08
};

constexpr unsigned int MAX_DEPTH = 12;

/////////////////////////////////////////////////////////////////////

//std::regex includeRegex{"^[[:space:]]*#[[:space:]]*include[[:space:]]+"
//        "([\"<])([^[:space:]]+)([\">])[[:space:]]*$",
//        std::regex_constants::ECMAScript | std::regex_constants::optimize};
boost::regex includeRegex("^[[:space:]]*#[[:space:]]*include[[:space:]]+"
        "([\"<])([^[:space:]]+)([\">])[[:space:]]*$");

/////////////////////////////////////////////////////////////////////

// Check, if line matches regexpr.
std::pair<RegexResult, std::string> checkLine(const std::string& line)
{
    boost::smatch match;
    const bool success = boost::regex_match(line, match, includeRegex);
    if (!success) {
        return std::make_pair(RegexResult::None, std::string());
    }
    assert(match.size() == 4);

    const char pre = match[1].str()[0];
    const char post = match[3].str()[0];

    // check "<...>" brackets
    if ((pre == '<' && post != '>') ||
        (pre == '\"' && post != '\"'))
    {
        return std::make_pair(RegexResult::Error, "Ill-formed include statement");
    }

    int res = RegexResult::Include;
    if (pre == '<') {
        res |= RegexResult::System;
    } else {
        res |= RegexResult::Local;
    }
    return std::make_pair(static_cast<RegexResult>(res), match[2].str());
}

/////////////////////////////////////////////////////////////////////

bool readFile(std::ostringstream& out, std::vector<std::string>& filenames,
        const path& file, unsigned int depth,
        const std::vector<std::string>* defines)
{
    if (depth > MAX_DEPTH) {
        LOG_ERROR(logtag::OpenGL, "ShaderSource reached maximum recursion depth. "
            "(Maybe circular dependency?)");
        return false;
    }

    std::ifstream is(file.c_str());
    if (!is) {
        LOG_ERROR("File not found: ", file.native());
        return false;
    }
    filenames.push_back(file.native());

    // Adjust line and source-string-number
    if (depth > 0) {
        out << "#line 1 " << depth << '\n';
    }

    // read file
    unsigned int lineNumber = 0;
    std::string line;
    while (std::getline(is, line)) {
        ++lineNumber;

        // for defines:
        if (depth == 0 && lineNumber == 2) {
            if (defines != nullptr) {
                for (const auto& s : *defines)
                    out << "#define " << s << '\n';
            }
            out << "#line 2 0\n";
        }

        // check for '#include ...'
        auto res = checkLine(line);
        if (res.first & RegexResult::Include) {
            path nextFile;
            if (res.first & RegexResult::Local) {
                nextFile = file.parent_path();
                nextFile /= res.second;
                try {
                    nextFile = canonical(nextFile);
                } catch (filesystem_error& err) {
                    // try root path instead
                    nextFile = vars.shader_dir;
                    nextFile /= res.second;
                }
            } else {
                nextFile = vars.shader_dir;
                nextFile /= res.second;
            }
            try {
                nextFile = canonical(nextFile);
            } catch (filesystem_error& err) {
                LOG_ERROR(err.what());
                return false;
            }
            // replace include-statement with comment
            out << "// include " << nextFile.c_str() << '\n';
            if (false == readFile(out, filenames, nextFile, depth + 1, nullptr)) {
                return false;
            }
            // place comment at end of include statement
            out << "// end of file " << nextFile.c_str() << '\n';
            // adjust line number
            out << "#line " << lineNumber + 1 << ' ' << depth << '\n';
        } else if (res.first & RegexResult::Error) {
            LOG_ERROR(res.second);
            return false;
        } else {
            out << line << '\n';
        }
    }

    return true;
}

/////////////////////////////////////////////////////////////////////

} // local namespace

/////////////////////////////////////////////////////////////////////

namespace gl
{

/////////////////////////////////////////////////////////////////////

ShaderSource::ShaderSource(const std::string& filename)
  : m_valid{false},
    m_srcPtr{nullptr}
{
    readFile(filename);
}

/////////////////////////////////////////////////////////////////////

ShaderSource::ShaderSource(const std::string& filename,
        const std::string& defines)
  : m_valid{false},
    m_srcPtr{nullptr}
{
    boost::char_separator<char> sep(",");
    boost::tokenizer<boost::char_separator<char>> tokens(defines, sep);
    for (const auto& d : tokens) {
        if (std::find(m_defines.begin(), m_defines.end(), d) != m_defines.end())
            continue;
        m_defines.push_back(d);
    }

    readFile(filename);
}

/////////////////////////////////////////////////////////////////////

ShaderSource::ShaderSource(const std::string& filename,
        std::initializer_list<std::string> defines)
  : m_valid{false},
    m_srcPtr{nullptr}
{
    for (const auto& d : defines) {
        if (std::find(m_defines.begin(), m_defines.end(), d) != m_defines.end())
            continue;
        m_defines.push_back(d);
    }
    readFile(filename);
}

/////////////////////////////////////////////////////////////////////

ShaderSource::ShaderSource(const ShaderSource& other)
  : m_code{other.m_code},
    m_filenames{other.m_filenames},
    m_valid{other.m_valid},
    m_srcPtr{m_code.c_str()},
    m_defines{other.m_defines}
{
}

/////////////////////////////////////////////////////////////////////

ShaderSource::ShaderSource(ShaderSource&& other)
noexcept
  : m_code{std::move(other.m_code)},
    m_filenames{std::move(other.m_filenames)},
    m_valid{other.m_valid},
    m_srcPtr{m_code.c_str()},
    m_defines{std::move(other.m_defines)}
{
}

/////////////////////////////////////////////////////////////////////

ShaderSource& ShaderSource::operator=(const ShaderSource& other) &
{
    if (this != &other) {
        m_code = other.m_code;
        m_filenames = other.m_filenames;
        m_valid = other.m_valid;
        m_srcPtr = m_code.c_str();
        m_defines = other.m_defines;
    }
    return *this;
}

/////////////////////////////////////////////////////////////////////

ShaderSource& ShaderSource::operator=(ShaderSource&& other) &
noexcept
{
    swap(other);
    return *this;
}

/////////////////////////////////////////////////////////////////////

void ShaderSource::swap(ShaderSource& other)
noexcept
{
    m_code.swap(other.m_code);
    m_filenames.swap(other.m_filenames);
    std::swap(m_valid, other.m_valid);
    m_srcPtr = m_code.c_str();
    other.m_srcPtr = other.m_code.c_str();
    m_defines.swap(other.m_defines);
}

/////////////////////////////////////////////////////////////////////

bool ShaderSource::valid()
const noexcept
{
    return m_valid;
}


/////////////////////////////////////////////////////////////////////

bool ShaderSource::operator!()
const noexcept
{
    return !m_valid;
}

/////////////////////////////////////////////////////////////////////

const std::string& ShaderSource::source()
const noexcept
{
    return m_code;
}

/////////////////////////////////////////////////////////////////////

const char** ShaderSource::sourcePtr()
const noexcept
{
    return const_cast<const char**>(&m_srcPtr);
}

/////////////////////////////////////////////////////////////////////

int ShaderSource::numFiles()
const noexcept
{
    return static_cast<int>(m_filenames.size());
}

/////////////////////////////////////////////////////////////////////

const std::vector<std::string>& ShaderSource::filenames()
const noexcept
{
    return m_filenames;
}

/////////////////////////////////////////////////////////////////////

std::vector<std::string>& ShaderSource::filenames() noexcept
{
    return m_filenames;
}

/////////////////////////////////////////////////////////////////////

const std::vector<std::string>& ShaderSource::getDefines()
const noexcept
{
    return m_defines;
}

/////////////////////////////////////////////////////////////////////

void ShaderSource::addDefines(const std::string& defines)
{
    assert(m_valid == true);

    size_t pos = m_code.find("#line 2 0");
    if (pos == std::string::npos) {
        LOG_ERROR(logtag::OpenGL, "ShaderSource cannot find position to "
            "append defines! (", m_filenames.front(), ')');
        return;
    }

    boost::char_separator<char> sep(",");
    boost::tokenizer<boost::char_separator<char>> tokens(defines, sep);
    for (const auto& d : tokens) {
        if (std::find(m_defines.begin(), m_defines.end(), d) != m_defines.end())
            continue;
        m_defines.push_back(d);

        std::string tmp{"#define " + d + '\n'};
        m_code.insert(pos, tmp);
        pos += tmp.length();
    }

    m_srcPtr = m_code.c_str();
}

/////////////////////////////////////////////////////////////////////

void ShaderSource::addDefines(std::initializer_list<std::string> defines)
{
    assert(m_valid == true);

    size_t pos = m_code.find("#line 2 0");
    if (pos == std::string::npos) {
        LOG_ERROR(logtag::OpenGL, "ShaderSource cannot find position to "
            "insert defines! (", m_filenames.front(), ')');
        return;
    }

    for (const auto& d : defines) {
        if (std::find(m_defines.begin(), m_defines.end(), d) != m_defines.end())
            continue;
        m_defines.push_back(d);

        std::string tmp{"#define " + d + '\n'};
        m_code.insert(pos, tmp);
        pos += tmp.length();
    }

    m_srcPtr = m_code.c_str();
}

/////////////////////////////////////////////////////////////////////

void ShaderSource::readFile(const std::string& filename)
{
    path file;
    try {
        file = canonical(vars.shader_dir / filename);
    } catch (filesystem_error& err) {
        LOG_ERROR(err.what());
        return;
    }

    std::ostringstream out;
    m_valid = ::readFile(out, m_filenames, file, 0, &m_defines);

    m_code = out.str();
    m_srcPtr = m_code.c_str();
}

/////////////////////////////////////////////////////////////////////

} // namespace gl

/////////////////////////////////////////////////////////////////////

