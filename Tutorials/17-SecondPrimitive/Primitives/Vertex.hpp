#pragma once
#include <Externals/GLM/glm/glm.hpp>

namespace Primitives
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec2 texcoord;
	};
};
