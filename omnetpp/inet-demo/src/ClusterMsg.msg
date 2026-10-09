import inet.common.packet.chunk.Chunk;

namespace inetdemo;

enum ClusterMsgType
{
    CLUSTER_ADV = 1;       // CH announcement (broadcast)
    CLUSTER_READING = 2;   // member -> CH
    CLUSTER_FUSED = 3;     // CH -> sink
}

// Payload of every frame of the demo (40 bytes, as in the ns-3 demo)
class ClusterMsg extends inet::FieldsChunk
{
    chunkLength = inet::B(40);
    int type @enum(ClusterMsgType);
    int source;        // sensor index
    int round;
    int readings;      // fused packet: number of readings inside
}
