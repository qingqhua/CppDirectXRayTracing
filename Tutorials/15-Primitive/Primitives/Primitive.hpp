#pragma once
#include <vector>
#include "Vertex.hpp"

namespace Primitives
{
	class Primitive
	{
	public:
		Primitive() {};

		virtual std::vector <Vertex> GetVertices() = 0;
		virtual std::vector <uint32_t> GetIndices() = 0;
	

	private:
		void CalculateTangentSpace(std::vector<uint32_t> mIndices, std::vector <Vertex> vertices) {
            int vertexCount = static_cast<int>(mVertices.size());
            int triangleCount = static_cast<int> (mIndices.size()) / 3;
            //todo: check if correct
            std::vector<glm::vec3> tan1, tan2;
            tan1.resize(vertexCount * 2);
            tan2.resize(vertexCount * 2);
            for (auto& t2 : tan2)
                t2 += vertexCount;

            Vertex a1, a2, a3;
            glm::vec3 v1, v2, v3;
            glm::vec2 w1, w2, w3;

            for (int a = 0; a < triangleCount; a++)
            {
                uint32_t i1 = mIndices[(a * 3) + 0];
                uint32_t i2 = mIndices[(a * 3) + 1];
                uint32_t i3 = mIndices[(a * 3) + 2];

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

	};

};
