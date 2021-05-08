#pragma once
#include "Cube.hpp"
#include <Externals/GLM/glm/gtc/constants.hpp>

void Primitives::Cube::Init(float size, bool uvHorizontalFlip, bool uvVerticalFlip, float uTileFactor, float vTileFactor)
{
    float uCoordMin = uvHorizontalFlip ? uTileFactor : 0.0f;
    float uCoordMax = uvHorizontalFlip ? 0.0f : uTileFactor;
    float vCoordMin = uvVerticalFlip ? vTileFactor : 0.0f;
    float vCoordMax = uvVerticalFlip ? 0.0f : vTileFactor;

    // A cube has six faces, each one pointing in a different direction.
    const glm::vec3 normals[] =
    {
        glm::vec3(0, 0, 1),
        glm::vec3(0, 0, -1),
        glm::vec3(1, 0, 0),
        glm::vec3(-1, 0, 0),
        glm::vec3(0, 1, 0),
        glm::vec3(0, -1, 0)
    };

    glm::vec2 texCoord[] =
    {
         glm::vec2(uCoordMax, vCoordMax),  glm::vec2(uCoordMin, vCoordMax),  glm::vec2(uCoordMin, vCoordMin),  glm::vec2(uCoordMax, vCoordMin),
         glm::vec2(uCoordMin, vCoordMin),  glm::vec2(uCoordMax, vCoordMin),  glm::vec2(uCoordMax, vCoordMax),  glm::vec2(uCoordMin, vCoordMax),
         glm::vec2(uCoordMax, vCoordMin),  glm::vec2(uCoordMax, vCoordMax),  glm::vec2(uCoordMin, vCoordMax),  glm::vec2(uCoordMin, vCoordMin),
         glm::vec2(uCoordMax, vCoordMin),  glm::vec2(uCoordMax, vCoordMax),  glm::vec2(uCoordMin, vCoordMax),  glm::vec2(uCoordMin, vCoordMin),
         glm::vec2(uCoordMin, vCoordMax),  glm::vec2(uCoordMin, vCoordMin),  glm::vec2(uCoordMax, vCoordMin),  glm::vec2(uCoordMax, vCoordMax),
         glm::vec2(uCoordMax, vCoordMin),  glm::vec2(uCoordMax, vCoordMax),  glm::vec2(uCoordMin, vCoordMax),  glm::vec2(uCoordMin, vCoordMin)
    };

    glm::vec3 tangents[] =
    {
         glm::vec3(1, 0, 0),
         glm::vec3(-1, 0, 0),
         glm::vec3(0, 0, -1),
         glm::vec3(0, 0, 1),
         glm::vec3(1, 0, 0),
         glm::vec3(1, 0, 0)
    };

    // Create each face in turn.
    for (int i = 0, j = 0; i < sizeof(normals)/sizeof(normals[0]); i++, j += 4)
    {
        glm::vec3 normal = normals[i];
        glm::vec3 tangent = tangents[i];

        // Get two vectors perpendicular to the face normal and to each other.
        glm::vec3 side1(normal.y, normal.z, normal.x);
        glm::vec3 side2 = glm::cross(normal, side1);

        int vertexCount = static_cast<int>(mVertices.size());
        // Six indices (two triangles) per face.
        // this.AddIndex(this.VerticesCount + 0);
        mIndices.push_back((vertexCount + 0));

        // this.AddIndex(this.VerticesCount + 1);
        mIndices.push_back((vertexCount + 1));

        // this.AddIndex(this.VerticesCount + 3);
        mIndices.push_back((vertexCount + 3));

        // this.AddIndex(this.VerticesCount + 1);
        mIndices.push_back((vertexCount + 1));

        // this.AddIndex(this.VerticesCount + 2);
        mIndices.push_back((vertexCount + 2));

        // this.AddIndex(this.VerticesCount + 3);
        mIndices.push_back((vertexCount + 3));

        // 0   3
        // 1   2
        float sideOverTwo = size * 0.5f;

        // Four vertices per face.
        // this.AddVertex((normal - side1 - side2) * sideOverTwo, normal, tangent, texCoord[j]);
        Vertex vertex;
        vertex.position = (normal - side1 - side2) * sideOverTwo;
        vertex.normal = normal;
        vertex.tangent = tangent;
        vertex.texcoord = texCoord[j];
        mVertices.push_back(vertex);

        vertex.position = (normal - side1 + side2) * sideOverTwo;
        vertex.normal = normal;
        vertex.tangent = tangent;
        vertex.texcoord = texCoord[j+1];
        mVertices.push_back(vertex);

        vertex.position = (normal + side1 + side2) * sideOverTwo;
        vertex.normal = normal;
        vertex.tangent = tangent;
        vertex.texcoord = texCoord[j + 2];
        mVertices.push_back(vertex);

        vertex.position = (normal + side1 - side2) * sideOverTwo;
        vertex.normal = normal;
        vertex.tangent = tangent;
        vertex.texcoord = texCoord[j + 3];
        mVertices.push_back(vertex);
    }

    CalculateTangentSpace();
}

void Primitives::Cube::Transform(glm::mat4 transform)
{
    for (int i=0;i<mVertices.size();i++)
    {
        glm::vec4 res = transform * glm::vec4(mVertices[i].position, 1.0f);
        mVertices[i].position = glm::vec3(res.x, res.y, res.z);
    }
}

std::vector<Primitives::Vertex> Primitives::Cube::GetVertices()
{
    return mVertices;
}

std::vector<glm::vec3> Primitives::Cube::GetPositions()
{
    std::vector<glm::vec3> positions;
    for (auto& v : mVertices)
    {
        positions.push_back(v.position);
    }
    return positions;
}

std::vector<uint16_t> Primitives::Cube::GetIndices()
{
    return mIndices;
}

void Primitives::Cube::CalculateTangentSpace()
{
    int vertexCount = static_cast<int>(mVertices.size());
    int triangleCount = static_cast<int> (mIndices.size()) / 3;
    //todo: check if correct
    std::vector<glm::vec3> tan1,tan2;
    tan1.resize(vertexCount * 2);
    tan2.resize(vertexCount * 2);
    for (auto& t2 : tan2)
        t2 += vertexCount;

    Vertex a1, a2, a3;
    glm::vec3 v1, v2, v3;
    glm::vec2 w1, w2, w3;

    for (int a = 0; a < triangleCount; a++)
    {
        uint16_t i1 = mIndices[(a * 3) + 0];
        uint16_t i2 = mIndices[(a * 3) + 1];
        uint16_t i3 = mIndices[(a * 3) + 2];

        a1 = mVertices[i1];
        a2 = mVertices[i2];
        a3 = mVertices[i3];

        v1 = a1.position;
        v2 = a2.position;
        v3 = a3.position;

        w1 = a1.texcoord;
        w2 = a2.texcoord;
        w3 = a3.texcoord;

        float x1 = v2.x - v1.x;
        float x2 = v3.x - v1.x;
        float y1 = v2.y - v1.y;
        float y2 = v3.y - v1.y;
        float z1 = v2.z - v1.z;
        float z2 = v3.z - v1.z;

        float s1 = w2.x - w1.x;
        float s2 = w3.x - w1.x;
        float t1 = w2.y - w1.y;
        float t2 = w3.y - w1.y;

        float r = 1.0F / ((s1 * t2) - (s2 * t1));
        glm::vec3 sdir(((t2 * x1) - (t1 * x2)) * r, ((t2 * y1) - (t1 * y2)) * r, ((t2 * z1) - (t1 * z2)) * r);
        glm::vec3 tdir(((s1 * x2) - (s2 * x1)) * r, ((s1 * y2) - (s2 * y1)) * r, ((s1 * z2) - (s2 * z1)) * r);

        tan1[i1] += sdir;
        tan1[i2] += sdir;
        tan1[i3] += sdir;

        tan2[i1] += tdir;
        tan2[i2] += tdir;
        tan2[i3] += tdir;
    }

    for (int a = 0; a < vertexCount; a++)
    {
        Vertex vertex = mVertices[a];

        glm::vec3 n = vertex.normal;
        glm::vec3 t = tan1[a];

        // Gram-Schmidt orthogonalize
        vertex.tangent = t - (n * glm::dot(n, t));
        vertex.tangent = glm::normalize(vertex.tangent);

        mVertices[a] = vertex;
    }
}
