/************************************************************************/
/* Base interface implementation for the Mesh drawing class             */
/*																		*/
/*	Provides interface to build a stateless mesh and retrieve const 	*/
/*	pointers to it's internal data (position and normal)				*/
/************************************************************************/
#include "IMesh.hpp"
#include "..\Maths\mtxlib.h"

namespace Mesh
{
	IMesh::IMesh(int size,float scale) : size_(size), scale_(scale)
	{
		vertex_ = 0;
		normal_ = 0;
	}

	IMesh::~IMesh()
	{
		destoryResources();
	}

	void IMesh::createResources(int size)
	{
		destoryResources();

		size_ = size;

		vertex_ = new vector3[size_*size_];
		normal_ = new vector3[size_*size_];
		numIndices_ = 6*(size_-1)*(size_-1);
		index_ = new int[numIndices_];
	}

	void IMesh::destoryResources()
	{
		delete[] vertex_; vertex_ = 0;
		delete[] normal_; normal_ = 0;
	}

	vector3 const * IMesh::getVertexData() const
	{
		return vertex_;
	}

	vector3 const * IMesh::getNormalData() const
	{
		return normal_;
	}

	int const * IMesh::getIndexData() const
	{
		return index_;
	}

	int IMesh::getNumIndices() const
	{
		return numIndices_;
	}
}