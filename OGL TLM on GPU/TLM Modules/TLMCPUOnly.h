/************************************************************************/
/* TLM Rendered via CPU only				                            */
/*																		*/
/*	Used CPU to generate all the information required to render a TLM	*/
/*	visualisation														*/
/************************************************************************/
#ifndef OGLTLM_TLMCPUONLY_H
#define OGLTLM_TLMCPUONLY_H

#include "../Maths/mtxlib.h"
#include "ITLMRenderer.h"
#include "../GLSL Support/GLSLProgram.hpp"
#include "../GLSL Support/GLSLShader.hpp"

#include "..\Mesh\IMesh.hpp"

namespace TLM
{
	class TLMCPUOnly : public ITLMRenderer
	{
	public:
		TLMCPUOnly(Mesh::IMesh &sourceMesh);
		~TLMCPUOnly();

		// Functions as required by interface
		virtual int CreateResources(const int vertsperedge);
		virtual int DestoryResources();
		virtual int GetVRAMUsage();
		virtual std::string GetFriendlyName( );
		virtual void GenerateTLMData();
		virtual void DriveTLMData(float drivingValue);
		virtual int RenderTLM(const vector3& pCamera, const vector3& pLight );

		//DEbug
		virtual void VisualiseTextures();
	protected:
	private:

		// size of the render targets
		int vertsperedge_;
		int numVerts_;
		int numIndices_;

		// Vertex arrays
		GLuint vertexBuffer_;			// XYZ vertex buffer for final drawing
		// normal data buffer
		GLuint normalBuffer_;
		// index data buffer
		GLuint indexBuffer_;

		// TLM Simulation information
		float * Ie;
		float * Iw;
		float * In;
		float * Is;
		float * Se;
		float * Sw;
		float * Sn;
		float * Ss;
		float * Mv;
		vector3 * finalMesh_;
		

		// Shaders
		// Final screen rendering
		GLSLProgram *final_;
		GLSLShader *finalVS_;
		GLSLShader *finalFS_;

		// Source Mesh
		Mesh::IMesh &sourceMesh_;
	};
}

#endif