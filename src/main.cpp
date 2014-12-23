#include <iostream>

#include "log/log.h"
#include "log/ostream_sink.h"
#include "log/relay.h"

#include "import/assimporter.h"
#include "import/scene.h"

int main()
{
    logging::Relay::initialize();
    std::shared_ptr<logging::Sink> sink = std::make_shared<logging::OStreamSink>(std::cout);
    logging::Relay::get().registerSink(sink);

    auto res = import::assimport("scenes/sponza/sponza.obj");

    logging::Relay::shutdown();
}
