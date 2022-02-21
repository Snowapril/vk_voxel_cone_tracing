// Author : Jihong Shin (snowapril)

#include <Common/pch.h>
#include <Common/VertexFormat.h>

namespace vfs
{
    unsigned int VertexHelper::GetNumBytes(VertexFormat vertexFormat)
    {
        return GetNumFloats(vertexFormat) * sizeof(float);
    }

    unsigned int VertexHelper::GetNumFloats(VertexFormat vertexFormat)
    {
        unsigned int numFloats{ 0 };
        if (static_cast<bool>(vertexFormat & VertexFormat::Position3))
        {
            numFloats += 3;
        }
        if (static_cast<bool>(vertexFormat & VertexFormat::Normal3))
        {
            numFloats += 3;
        }
        if (static_cast<bool>(vertexFormat & VertexFormat::TexCoord2))
        {
            numFloats += 2;
        }
        if (static_cast<bool>(vertexFormat & VertexFormat::TexCoord3))
        {
            numFloats += 3;
        }
        if (static_cast<bool>(vertexFormat & VertexFormat::Color4))
        {
            numFloats += 4;
        }
        if (static_cast<bool>(vertexFormat & VertexFormat::Tangent4))
        {
            numFloats += 4;
        }
        return numFloats;
    }
}