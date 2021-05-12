#pragma once
#include <vector>
#include "Vertex.hpp"

namespace Primitives
{
	class Sphere
	{
	public:
		Sphere() = default;
		~Sphere() = default;

		void Init(float diameter, int tessellation, bool uvHorizontalFlip = false, bool uvVerticalFlip = false);
		void Transform(glm::mat4 transform);
		std::vector <Vertex> GetVertices();
		std::vector <uint16_t> GetIndices();
	
	private:
		void CalculateTangentSpace();
		std::vector<uint16_t> mIndices;
		std::vector <Vertex> mVertices;
	};

};
