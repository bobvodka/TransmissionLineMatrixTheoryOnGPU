/************************************************************************/
/* TLM Rendered via a fully shader R2VB method                          */
/*																		*/
/*	Used R2VB to generate all the information required to render a TLM	*/
/*	visualisation														*/
/************************************************************************/
#ifndef OGLTLM_TLMFULLSHADER_H
#define OGLTLM_TLMFULLSHADER_H

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
	class TLMFullShader : public ITLMRenderer
	{
	public:
		TLMFullShader(GLuint internalformat, Mesh::IMesh &sourceMesh);
		~TLMFullShader();

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

		// Our Render target for performing the TLM simulation
		Resurrection::RenderToTexture::FrameBufferObject rendertarget_;

		// The format of the texture we'll be using
		GLuint internalformat_;

		// Textures required
		GLuint positionMap_;			// Holds vertex height information
		GLuint energyMap_[2];		// Holds energy at a given point (two buffers for ping-pong rendering)
		GLuint normalMap_;			// Holds per vertex normals
		GLuint drivingMap_;			// Driving data for the simulation
		GLfloat *drivingTexture_;	// The memory required for the driving data texture
		
		// Vars used to swap between energyMap buffers
		GLuint energySource_;			// Texture id for source data
		GLuint energySink_;				// Texture id for sink data
		GLenum energyFBOSource_;		// FBO Render-target for Source data
		GLenum energyFBOSink_;			// FBO Render-target for sink data

		// Vertex arrays
		GLuint vertexMap_;			// XYZ buffer for original vertex information
		GLuint orignalnormalMap_;	// Buffer for original normal information
		// PBO/VBO normal data
		GLuint normalBuffer_;
		// PBO/VBO height data
		GLuint positionBuffer_;
		// Vertex streams from shaders for normal and height information
		GLint normalStream_;
		GLint heightStream_;
		// Vertex streams for full 3D position generation
		GLuint orignalPosStream_;
		GLuint orignalNormalStream_;
		// Index buffer for final drawing
		GLuint indexBuffer_;

		// Shaders
		// Firstly a general vertex shader for GPGPU stuff
		GLSLShader *gpgpuVS_;
		// Next pass 1 : Energy transfer
		GLSLProgram *pass1_;
		GLSLShader *energyTransfer_;
		// Next pass 2 : Generate height map
		GLSLProgram *pass2_;
		GLSLShader *heightGeneration_;
		// Pass 3 : Generate energy for next iteration
		GLSLProgram *pass3_;
		GLSLShader *energyGeneration_;
		// Pass 4 : Generate normals from height information
		GLSLProgram *pass4_;
		GLSLShader *normalGeneration_;
		// Pass 5 : Final screen rendering
		GLSLProgram *pass5_;
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
