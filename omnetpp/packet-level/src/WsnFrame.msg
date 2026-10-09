import inet.common.packet.chunk.Chunk;

namespace pktwsn;

enum WsnFrameType
{
    WSN_HELLO = 1;      // neighbour discovery, once at deployment (v8)
    WSN_BEACON = 2;     // BS -> all: average residual energy (v8, every set-up)
    WSN_ADV = 3;        // CH advertisement (broadcast, whole field)
    WSN_JOIN = 4;       // member -> CH
    WSN_SCHED = 5;      // CH -> its members: TDMA schedule (broadcast)
    WSN_HANDOVER = 6;   // new CH -> members of the old CH (v8, I2)
    WSN_DATA = 7;       // member -> CH, or member -> BS (direct)
    WSN_AGG = 8;        // CH -> next CH or BS: fused packet
}

// Payload of every frame. Its length is the thesis packet size:
// control 200 bits, data 2000 bits (+16-bit energy field for v8).
class WsnFrame extends inet::FieldsChunk
{
    int type @enum(WsnFrameType);
    int src;          // node index, -1 = BS
    int round;
    int sources;      // readings carried (DATA = 1, AGG = fused count)
    double energy;    // sender's residual energy (piggy-backed)
    double value;     // BEACON: average energy; HANDOVER: old CH index
}
