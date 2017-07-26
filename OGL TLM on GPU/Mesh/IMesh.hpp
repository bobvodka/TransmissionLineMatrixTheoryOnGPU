/************************************************************************/
/* Base interface for the Mesh drawing class                            */
/*																		*/
/*	Provides interface to build a stateless mesh and retrieve const 	*/
/*	pointers to it's internal data (position and normal)				*/
/************************************************************************/
#ifndef TLM_IMESH_HPP
#define TLM_IMESH_HPP
#include "..\Maths\mtxlib.h"
namespace Mesh
{
	class IMesh
	{
	public:
		IMesh(int size, float scale = 1.0f);
		virtual ~IMesh();

		vector3 const * getVertexData() const;
		vector3 const * getNormalData() const;
		int const * getIndexData() const;
		int getNumIndices() const;

		void createResources(int size);
		void destoryResources();

		virtual void generateMesh() = 0;
		
	protected:
		int size_;
		int numIndices_;
		int scale_;
		vector3 * vertex_;
		vector3 * normal_;
		int * index_;
	private:
	};
}
#endif