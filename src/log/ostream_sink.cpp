#include <sstream>

#include "entry.h"
#include "ostream_sink.h"


namespace logging
{

///////////////////////////////////////////////////////////////////////////

bool OStreamSink::filter(const Entry&)
{
    return true;
}

///////////////////////////////////////////////////////////////////////////

std::string OStreamSink::format(const Entry& e)
{
    std::ostringstream s;
    s << severity2str(e.severity);

    const std::vector<Tag>& tags = e.tags;
    if (!tags.empty()) {
        s << " [";
        for (size_t i = 0; i < tags.size() - 1; ++i)
            s << tag2str(tags[i]) << '|';
        s << tag2str(tags.back()) << ']';
    }

    s << ": " << e.msg << '\n';

    return s.str();
}

///////////////////////////////////////////////////////////////////////////

OStreamSink::OStreamSink(std::ostream& out)
  : m_out{out}
{
}

///////////////////////////////////////////////////////////////////////////

bool OStreamSink::addEntry(const Entry& entry)
noexcept
{
    if (filter(entry)) {
        m_out << format(entry);
        m_out.flush();
        return true;
    }
    return false;
}

///////////////////////////////////////////////////////////////////////////

void OStreamSink::flush()
noexcept
{
    m_out.flush();
}

///////////////////////////////////////////////////////////////////////////

} // namespace logging
