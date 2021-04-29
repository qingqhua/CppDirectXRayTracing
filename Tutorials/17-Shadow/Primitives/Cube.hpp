#pragma once
#include <vector>
#include "Vertex.hpp"

namespace Primitives
{
	class Cube
	{
	public:
		Cube() = default;
		~Cube() = default;

		void Init(float size, bool uvHorizontalFlip = false, bool uvVerticalFlip = false, float uTileFactor = 1, float vTileFactor = 1);

		std::vector <Vertex> GetVertices();
		std::vector <uint16_t> GetIndices();
	
	private:
		void CalculateTangentSpace();
		std::vector <glm::vec3> GetPositions();
		std::vector<uint16_t> mIndices;
		std::vector<Vertex> mVertices;
	};

};
