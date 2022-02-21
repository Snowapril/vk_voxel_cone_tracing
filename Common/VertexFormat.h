// Author : Jihong Shin (snowapril)

#if !defined(COMMON_VERTEX_FORMAT)
#define COMMON_VERTEX_FORMAT

namespace vfs
{
    enum class VertexFormat : unsigned int
    {
        None        = 0,
        Position3   = 1 << 0,
        Normal3     = 1 << 1,
        TexCoord2   = 1 << 2,
        TexCoord3   = 1 << 3,
        Color4      = 1 << 4,
        Tangent4    = 1 << 5,
        Last        = 1 << 6,
        Position3Normal3    = Position3 | Normal3,
        Position3TexCoord2  = Position3 | TexCoord2,
        Position3TexCoord3  = Position3 | TexCoord3,
        Position3Color4     = Position3 | Color4,
        Position3Normal3TexCoord2       = Position3Normal3 | TexCoord2,
        Position3Normal3Color4          = Position3Normal3 | Color4,
        Position3TexCoord2Color4        = Position3TexCoord2 | Color4,
        Position3Normal3TexCoord2Color4 = Position3Normal3TexCoord2 | Color4,
        Position3Normal3Tangent4        = Position3Normal3 | Tangent4,
        Position3Normal3TexCoord2Tangent4       = Position3Normal3TexCoord2 | Tangent4,
        Position3Normal3TexCoord2Color4Tangent4 = Position3Normal3TexCoord2Color4 | Tangent4,
    };

    inline VertexFormat operator|(VertexFormat a, VertexFormat b)
    {
        return static_cast<VertexFormat>(static_cast<unsigned int>(a) | 
                                         static_cast<unsigned int>(b));
    }

    inline VertexFormat operator&(VertexFormat a, VertexFormat b)
    {
        return static_cast<VertexFormat>(static_cast<unsigned int>(a) &
                                         static_cast<unsigned int>(b));
    }

    class VertexHelper
    {
    public:
        VertexHelper() = delete;

    public:
        static unsigned int GetNumBytes (VertexFormat vertexFormat);
        static unsigned int GetNumFloats(VertexFormat vertexFormat);
    };
}

#endif