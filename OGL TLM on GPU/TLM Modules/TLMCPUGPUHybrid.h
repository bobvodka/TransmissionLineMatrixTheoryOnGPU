/************************************************************************/
/* TLM Rendered via CPU & GPU				                            */
/*																		*/
/*	Used CPU to generate the TLM and the GPU is used to deform the mesh	*/
/*	for visualisation													*/
/************************************************************************/
#ifndef OGLTLM_TLMCPUGPUHYBRID_H
#define OGLTLM_TLMCPUGPUHYBRID_H

#include "../Maths/mtxlib.h"
#include "ITLMRenderer.h"
#include "../fbo/framebufferobject.hpp"
#include "../GLSL Support/GLSLProgram.hpp"
#include "../GLSL Support/GLSLShader.hpp"
#include "../Camera/DefaultView.hpp"
#include "../Camera/OrthographicWorldCamera.hpp"

#include "..\Mesh\IMesh.hpp"

namespace TLM
{
	class TLMCPUGPUHybrid : public ITLMRenderer
	{
	public:
		TLMCPUGPUHybrid(GLuint internalformat, Mesh::IMesh &sourceMesh);
		~TLMCPUGPUHybrid();

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

		// TLM Simulation information
		float * Ie;
		float * Iw;
		float * In;
		float * Is;
		float * Se;
		float * Sw;
		float * Sn;
		float * Ss;
		vector4 * Mv;

		// Our Render target for performing the TLM simulation
		Resurrection::RenderToTexture::FrameBufferObject rendertarget_;

		// The format of the texture we'll be using
		GLuint internalformat_;

		// Textures required
		GLuint positionMap_;			// Holds vertex height information
		GLuint normalMap_;				// Holds per vertex normals
		GLuint heightMap_;			// Height buffer data

		// Vertex arrays
		GLuint vertexMap_;			// XYZ buffer for original vertex information
		GLuint orignalnormalMap_;	// Buffer for original normal information
		
		// Vertex arrays
		GLuint positionBuffer_;			// XYZ vertex buffer for final drawing
		// normal data buffer
		GLuint normalBuffer_;
		// index data buffer
		GLuint indexBuffer_;

		// Stream for height data into vertex shader
		GLint normalStream_;

		// Shaders
		// Firstly a general vertex shader for GPGPU stuff
		GLSLShader *gpgpuVS_;
		// Next pass 1 : Generate height map
		GLSLProgram *pass1_;
		GLSLShader *meshGeneration_;
		// Pass 2 : Generate normals from height information
		GLSLProgram *pass2_;
		GLSLShader *normalGeneration_;
		// Final screen rendering
		GLSLProgram *final_;
		GLSLShader *finalVS_;
		GLSLShader *finalFS_;

		// Local camera details for setting up an ortho-view
		Resurrection::CameraSystem::OrthographicWorldCamera camera_;
		Resurrection::CameraSystem::DefaultView view;

		// Source Mesh
		Mesh::IMesh &sourceMesh_;
	};
}

#endif