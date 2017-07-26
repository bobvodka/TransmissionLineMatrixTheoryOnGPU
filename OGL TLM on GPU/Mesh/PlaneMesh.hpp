/************************************************************************/
/*  Plane implementation of the Base interface                          */
/*																		*/
/************************************************************************/
#ifndef TLM_PLANEMEHS_HPP
#define TLM_PLANEMEHS_HPP

#include "IMesh.hpp"
namespace Mesh
{
	class PlaneMesh : public IMesh
	{
	public:
		PlaneMesh(int size, float scale = 1.0f);
		void generateMesh();
	protected:
	private:
	};
}
#endif 