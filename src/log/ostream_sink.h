#ifndef LOG_OSTREAM_SINK_H
#define LOG_OSTREAM_SINK_H


#include <ostream>
#include <functional>

#include "sink.h"

namespace logging
{

class OStreamSink
  : public Sink
{
public:
    OStreamSink(std::ostream& out);

    virtual bool addEntry(const Entry& entry) noexcept override;

    virtual void flush() noexcept override;

protected:
    virtual bool filter(const Entry&);
    virtual std::string format(const Entry&);

    std::ostream&   m_out;
};

} // namespace logging

#endif // LOG_OSTREAM_SINK_H
