#include "WsnRuntime.h"

#include <omnetpp.h>

namespace wsn {

std::map<std::string, ProtocolInfo>& Registry()
{
    static std::map<std::string, ProtocolInfo> registry;
    return registry;
}

Registrar::Registrar(const char* name, const char* source, RunFn fn, std::vector<std::string> defines)
{
    Registry()[name] = ProtocolInfo{name, source, fn, std::move(defines)};
}

void Fatal(const std::string& message)
{
    throw omnetpp::cRuntimeError("%s", message.c_str());
}

} // namespace wsn
