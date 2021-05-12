#pragma once
#include "Sphere.hpp"
#include <stdexcept>
#include <Externals/GLM/glm/gtc/constants.hpp>

void Primitives::Sphere::Init(float diameter, int tessellation, bool uvHorizontalFlip, bool uvVerticalFlip)
{
    if (tessellation < 3)
    {
        throw std::invalid_argument("tessellation smaller than 3 is invalid.");
    }

    int verticalSegments = tessellation;
    int horizontalSegments = tessellation * 2;
    float uIncrement = 1.0f / horizontalSegments;
    float vIncrement = 1.0f / verticalSegments;
    float radius = diameter / 2.0f;

    uIncrement *= uvHorizontalFlip ? 1.0f : -1.0f;
    vIncrement *= uvVerticalFlip ? 1.0f : -1.0f;

    float u = uvHorizontalFlip ? 0.0f : 1.0f;
    float v = uvVerticalFlip ? 0.0f : 1.0f;

    // Start with a single vertex at the bottom of the sphere.
    for (int i = 0; i < horizontalSegments; i++)
    {
        u += uIncrement;

        // this.AddVertex(new Vector3(0,-1,0) * radius, new Vector3(0,-1,0), new Vector2(u, v));
        Vertex vertex;
        vertex.position = glm::vec3(0, -1, 0) * radius;
        vertex.normal = glm::vec3(0, -1, 0);
        vertex.tangent = glm::vec3(0, 0, 0);
        vertex.texcoord = glm::vec2(u, v);

        mVertices.push_back(vertex);
    }
    
    // Create rings of vertices at progressively higher latitudes.
    v = uvVerticalFlip ? 0.0f : 1.0f;
    for (int i = 0; i < verticalSegments - 1; i++)
    {
        float latitude = (((i + 1) * glm::pi <float>()) / verticalSegments) - glm::pi<float>() / 2.0f;
        u = uvHorizontalFlip ? 0.0f : 1.0f;
        v += vIncrement;
        float dy = glm::sin(latitude);
        float dxz = glm::cos(latitude);

        // Create a single ring of vertices at this latitude.
        for (int j = 0; j <= horizontalSegments; j++)
        {
            float longitude = j * glm::pi <float>() * 2 / horizontalSegments;

            float dx = glm::sin(longitude) * dxz;
            float dz = glm::cos(longitude) * dxz;

            glm::vec3 normal(dx, dy, dz);

            glm::vec2 texCoord(u, v);
            u += uIncrement;

            Vertex vertex;
            vertex.position = normal * radius;
            vertex.normal = normal;
            vertex.tangent = glm::vec3(0, 0, 0);
            vertex.texcoord = texCoord;

            mVertices.push_back(vertex);
        }
    }

    // Finish with a single vertex at the top of the sphere.
    v = uvVerticalFlip ? 1.0f : 0.0f;
    u = uvHorizontalFlip ? 0.0f : 1.0f;
    for (int i = 0; i < horizontalSegments; i++)
    {
        u += uIncrement;

        // this.AddVertex(new Vector3(0,1,0) * radius, new Vector3(0,1,0), new Vector2(u, v));
        Vertex vertex;
        vertex.position = glm::vec3(0, 1, 0) * radius;
        vertex.normal = glm::vec3(0, 1, 0);
        vertex.tangent = glm::vec3(0, 0, 0);
        vertex.texcoord = glm::vec2(u, v);

        mVertices.push_back(vertex);
    }

    // Create a fan connecting the bottom vertex to the bottom latitude ring.
    for (int i = 0; i < horizontalSegments; i++)
    {
        // this.AddIndex(i);
        mIndices.push_back(i);

        // this.AddIndex(1 + i + horizontalSegments);
        mIndices.push_back(1 + i + horizontalSegments);

        // this.AddIndex(i + horizontalSegments);
        mIndices.push_back(i + horizontalSegments);
    }

    // Fill the sphere body with triangles joining each pair of latitude rings.
    for (int i = 0; i < verticalSegments - 2; i++)
    {
        for (int j = 0; j < horizontalSegments; j++)
        {
            int nextI = i + 1;
            int nextJ = j + 1;
            int num = horizontalSegments + 1;

            int i1 = horizontalSegments + (i * num) + j;
            int i2 = horizontalSegments + (i * num) + nextJ;
            int i3 = horizontalSegments + (nextI * num) + j;
            int i4 = i3 + 1;

            // this.AddIndex(i1);
            mIndices.push_back(i1);

            // this.AddIndex(i2);
            mIndices.push_back(i2);

            // this.AddIndex(i3);
            mIndices.push_back(i3);

            // this.AddIndex(i2);
            mIndices.push_back(i2);

            // this.AddIndex(i4);
            mIndices.push_back(i4);

            // this.AddIndex(i3);
            mIndices.push_back(i3);
        }
    }

    // Create a fan connecting the top vertex to the top latitude ring.
    for (int i = 0; i < horizontalSegments; i++)
    {
        // this.AddIndex(this.VerticesCount - 1 - i);
        mIndices.push_back(static_cast<uint16_t>(mVertices.size() - 1 - i));

        // this.AddIndex(this.VerticesCount - horizontalSegments - 2 - i);
        mIndices.push_back(static_cast<uint16_t>(mVertices.size() - horizontalSegments - 2 - i));

        // this.AddIndex(this.VerticesCount - horizontalSegments - 1 - i);
        mIndices.push_back(static_cast<uint16_t>(mVertices.size() - horizontalSegments - 1 - i));
    }

    CalculateTangentSpace();
}

void Primitives::Sphere::Transform(glm::mat4 transform)
{
    for (int i = 0; i < mVertices.size(); i++)
    {
        glm::vec4 res = transform * glm::vec4(mVertices[i].position, 1.0f);
        mVertices[i].position = glm::vec3(res.x, res.y, res.z);
    }
}

std::vector<Primitives::Vertex> Primitives::Sphere::GetVertices()
{
    return mVertices;
}

std::vector<uint16_t> Primitives::Sphere::GetIndices()
{
    return mIndices;
}

void Primitives::Sphere::CalculateTangentSpace()
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
