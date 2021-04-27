#pragma once
#include <vector>
#include "Vertex.hpp"

namespace Primitives
{
	class Sphere
	{
	public:
		Sphere() = delete;
		Sphere(float diameter, int tessellation, bool uvHorizontalFlip = false, bool uvVerticalFlip = false);
		~Sphere() = default;

		std::vector <Vertex> GetVertices();
		std::vector <uint32_t> GetIndices();
	
	private:
		void CalculateTangentSpace();
		std::vector<uint32_t> mIndices;
		std::vector <Vertex> mVertices;
	};

};
