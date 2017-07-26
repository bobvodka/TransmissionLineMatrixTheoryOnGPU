/************************************************************************/
/*  Plane implementation of the Base interface                          */
/*																		*/
/************************************************************************/
#include "PlaneMesh.hpp"
#include <GLee.h>
#define _USE_MATH_DEFINES
#include <math.h>

namespace Mesh
{
	PlaneMesh::PlaneMesh(int size, float scale) : IMesh(size,scale)
	{

	}

	void PlaneMesh::generateMesh()
	{
		// Generate mesh data
		vector3 * vtmp = vertex_;
		vector3 * ntmp = normal_;
		for(int y = 0; y<size_; y++)
		{
			for(int x = 0; x<size_; x++)
			{
				// X Y Z
				*vtmp++ = vector3((float(x)/float(size_- scale_) - 0.5f) * float(M_PI), 0.0, (float(y)/float(size_- scale_) - 0.5f) * float(M_PI));
				// normal
				*ntmp++ = vector3(0.0f, 1.0f, 0.0f);
			}
		}

		// Generate index data
		int * itmp = index_;
		for(int y = 1; y < size_; y++)
		{
			for (int x = 1; x < size_; x++)
			{
				*itmp++ = static_cast<GLint>((y-1)*size_ + (x-1));
				*itmp++ = static_cast<GLint>((y-0)*size_ + (x-1));
				*itmp++ = static_cast<GLint>((y-1)*size_ + (x-0));

				*itmp++ = static_cast<GLint>((y-1)*size_ + (x-0));
				*itmp++ = static_cast<GLint>((y-0)*size_ + (x-1));
				*itmp++ = static_cast<GLint>((y-0)*size_ + (x-0));
			}
		}
	}
}