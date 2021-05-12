#pragma once
#include "Quad.hpp"

#include <Externals/GLM/glm/gtc/constants.hpp>


void Primitives::Quad::Init(float size, bool uvHorizontalFlip, bool uvVerticalFlip, float uTileFactor, float vTileFactor)
{
    // Indexed Quad
    {
        Vertex vertex;
        vertex.position = glm::vec3(-size, 0, -size);
        vertex.normal = glm::vec3(0, 1, 0);
        vertex.tangent = glm::vec3(0, 0, 0);
        vertex.texcoord = glm::vec2(0, 0);
        mVertices.push_back(vertex);
    }

    {
        Vertex vertex1;
        vertex1.position = glm::vec3(size, 0, -size);
        vertex1.normal = glm::vec3(0, 1, 0);
        vertex1.tangent = glm::vec3(0, 0, 0);
        vertex1.texcoord = glm::vec2(0, 0);
        mVertices.push_back(vertex1);
    }

    {
        Vertex vertex2;
        vertex2.position = glm::vec3(size, 0, size);
        vertex2.normal = glm::vec3(0, 1, 0);
        vertex2.tangent = glm::vec3(0, 0, 0);
        vertex2.texcoord = glm::vec2(0, 0);
        mVertices.push_back(vertex2);
    }

    {
        Vertex vertex3;
        vertex3.position = glm::vec3(-size, 0, size);
        vertex3.normal = glm::vec3(0, 1, 0);
        vertex3.tangent = glm::vec3(0, 0, 0);
        vertex3.texcoord = glm::vec2(0, 0);
        mVertices.push_back(vertex3);
    }

    mIndices.push_back(0);
    mIndices.push_back(1);
    mIndices.push_back(2);
    mIndices.push_back(0);
    mIndices.push_back(2);
    mIndices.push_back(3);
}

std::vector<Primitives::Vertex> Primitives::Quad::GetVertices()
{
    return mVertices;
}

std::vector<glm::vec3> Primitives::Quad::GetPositions()
{
    std::vector<glm::vec3> positions;
    for (auto& v : mVertices)
    {
        positions.push_back(v.position);
    }
    return positions;
}

std::vector<uint16_t> Primitives::Quad::GetIndices()
{
    return mIndices;
}

void Primitives::Quad::CalculateTangentSpace()
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
