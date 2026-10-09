#include <omnetpp.h>

using namespace omnetpp;

namespace wsn {

class BaseStation : public cSimpleModule
{
  protected:
    void handleMessage(cMessage* msg) override { delete msg; }
};

Define_Module(BaseStation);

} // namespace wsn
