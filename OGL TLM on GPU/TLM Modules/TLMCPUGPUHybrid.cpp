/************************************************************************/
/* Implementation of the TLM Renderer via CPU for all calcs		        */
/************************************************************************/

#include "ITLMRenderer.h"
#include "TLMCPUGPUHybrid.h"
#include <GLee.h>
#include <algorithm>
#include "../fbo/framebufferobject.hpp"
#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>

#include "..\Mesh\IMesh.hpp"

namespace TLM
{
	TLMCPUGPUHybrid::TLMCPUGPUHybrid(GLuint internalformat,Mesh::IMesh &sourceMesh) : camera_(0,1,1,0,-1,1), internalformat_(internalformat),vertsperedge_(0), 
																						numVerts_(0), numIndices_(0),sourceMesh_(sourceMesh)
	{
		camera_.setView(&view);	// Setup the orthographic camera for the R2T operation
		camera_.updateViewMatrix();
	}

	TLMCPUGPUHybrid::~TLMCPUGPUHybrid()
	{
		DestoryResources();
	}

	namespace 
	{
		void SetupRenderTarget(const int size, const int internalformat, void * data = NULL)
		{
			glTexImage2D(GL_TEXTURE_2D,0, internalformat, size, size, 0, GL_RGBA, GL_FLOAT, data);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		}
		
		void CheckShaderLogs(GLSLShader *shader)
		{
			std::string log;
			shader->getShaderLog(log);
			std::cout << log << std::endl;
		}

		void CheckProgramLogs(GLSLProgram *program)
		{
			std::string log;
			program->validate();
			program->GetProgramLog(log);
			std::cout << log << std::endl;
		}
	}

	int TLMCPUGPUHybrid::CreateResources(const int vertsperedge)
	{
		vertsperedge_ = vertsperedge;	// save the size of the buffer we'll need
		numVerts_ = vertsperedge*vertsperedge;
		numIndices_ = 6*(vertsperedge-1)*(vertsperedge-1);

		// Reserve the memory we need for each step of the simulation
		Ie = new float[numVerts_];
		Iw = new float[numVerts_];
		In = new float[numVerts_];
		Is = new float[numVerts_];

		Se = new float[numVerts_];
		Sw = new float[numVerts_];
		Sn = new float[numVerts_];
		Ss = new float[numVerts_];

		Mv = new vector4[numVerts_];

		// And zero it out
		std::fill_n(Ie,(numVerts_),0.0f);
		std::fill_n(Iw,(numVerts_),0.0f);
		std::fill_n(In,(numVerts_),0.0f);
		std::fill_n(Is,(numVerts_),0.0f);

		std::fill_n(Se,(numVerts_),0.0f);
		std::fill_n(Sw,(numVerts_),0.0f);
		std::fill_n(Sn,(numVerts_),0.0f);
		std::fill_n(Ss,(numVerts_),0.0f);

		std::fill_n(Mv,(numVerts_),vector4());

		// Setup our render target
		// This will be used to hold all of our textures for rendering to
		rendertarget_.initialize(vertsperedge_,vertsperedge_);

		// This can be any format (32bit or 16bit)
		glGenTextures(1,&positionMap_);
		glBindTexture(GL_TEXTURE_2D, positionMap_);
		SetupRenderTarget(vertsperedge,internalformat_);

		// Due to vertex format limitations this is always 32bit per channel (o rly? no, apprently not rly)
		glGenTextures(1,&normalMap_);
		glBindTexture(GL_TEXTURE_2D, normalMap_);
		SetupRenderTarget(vertsperedge,internalformat_);

		// Used to hold post TLM height information
		glGenTextures(1,&heightMap_);
		glBindTexture(GL_TEXTURE_2D, heightMap_);
		SetupRenderTarget(vertsperedge,internalformat_);


		// Next we'll need to load shaders here
		try
		{			

			// First the general GPGPU shader
			gpgpuVS_ = new GLSLShader("shaders/cpu gpu shader/gpgpu.vs");
			// Next we load each pass in turn
			meshGeneration_ = new GLSLShader("shaders/cpu gpu shader/meshGeneration.fs",GL_FRAGMENT_SHADER);
			std::cout << "Mesh Generation logs..." << std::endl;
			CheckShaderLogs(gpgpuVS_);
			CheckShaderLogs(meshGeneration_);
			pass1_ = new GLSLProgram();
			pass1_->attach(gpgpuVS_);
			pass1_->attach(meshGeneration_);
			pass1_->link();
			CheckProgramLogs(pass1_);
			
			normalGeneration_ = new GLSLShader("shaders/cpu gpu shader/normalgen.fs",GL_FRAGMENT_SHADER);
			pass2_ = new GLSLProgram();
			pass2_->attach(gpgpuVS_);
			pass2_->attach(normalGeneration_);
			pass2_->link();
			std::cout << "Normal Generation logs..." << std::endl;
			CheckShaderLogs(normalGeneration_);
			CheckProgramLogs(pass2_);


			finalVS_ = new GLSLShader("shaders/cpu gpu shader/final.vs");
			finalFS_ = new GLSLShader("shaders/cpu gpu shader/final.fs",GL_FRAGMENT_SHADER);
			final_ = new GLSLProgram();
			final_->attach(finalVS_);
			final_->attach(finalFS_);
			final_->link();
			std::cout << "Final render logs..." << std::endl;
			CheckShaderLogs(finalVS_);
			CheckShaderLogs(finalFS_);
			CheckProgramLogs(final_);

			normalStream_ = final_->getAttributeLocation("normalData");
			std::cout << "Normal stream assigned to attrib slot " << normalStream_ << std::endl;

		}
		catch (std::exception &e)
		{
			std::cout << e.what() << std::endl;
		}


		// temp memory for data (we need to convert from vec3 to vec4 in effect)
		float * tmp = new float[numVerts_ * 4];

		// Generate a texture to hold the original vertex position data
		glGenTextures(1,&vertexMap_);
		glBindTexture(GL_TEXTURE_2D, vertexMap_);
		const vector3 * src = sourceMesh_.getVertexData();
		for(int i = 0; i < numVerts_ * 4; i+=4)
		{
			tmp[i] = src->x;
			tmp[i + 1] = src->y;
			tmp[i + 2] = src->z;
			tmp[i + 3] = 0.0f;
			src++;
		}
		SetupRenderTarget(vertsperedge_,internalformat_,tmp);

		// Generate a texture to hold the original normal data
		glGenTextures(1,&orignalnormalMap_);
		glBindTexture(GL_TEXTURE_2D, orignalnormalMap_);
		src = sourceMesh_.getNormalData();
		for(int i = 0; i < numVerts_ * 4; i+=4)
		{
			tmp[i] = src->x;
			tmp[i + 1] = src->y;
			tmp[i + 2] = src->z;
			tmp[i + 3] = 0.0f;
			src++;
		}
		SetupRenderTarget(vertsperedge_,internalformat_,tmp);

		delete[] tmp;

		// Finally allocate some buffers for the vertex data
		// Next generate an index buffer for final drawing
		glGenBuffers(1,&indexBuffer_);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,indexBuffer_);
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,numIndices_*(sizeof(GL_INT)), sourceMesh_.getIndexData(), GL_STATIC_DRAW_ARB);

		// Finally we need to reserve some space for the VBOs for normal and height data
		glGenBuffers(1, &normalBuffer_);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, normalBuffer_);
		glBufferData(GL_PIXEL_PACK_BUFFER_ARB, numVerts_*(sizeof(float)*4),NULL, GL_STREAM_DRAW);

		// Same details but this time for height data
		glGenBuffers(1, &positionBuffer_);
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, positionBuffer_);
		glBufferData(GL_PIXEL_PACK_BUFFER_ARB, numVerts_*(sizeof(float)*4),NULL, GL_STREAM_DRAW);

		return 0;
	}

	int TLMCPUGPUHybrid::DestoryResources() 
	{

		delete[] Ie; Ie = 0;
		delete[] Iw; Iw = 0;
		delete[] In; In = 0;
		delete[] Is; Is = 0;
		delete[] Se; Se = 0;
		delete[] Sw; Sw = 0;
		delete[] Sn; Sn = 0;
		delete[] Ss; Ss = 0;
		

		delete[] Mv; Mv = 0;


		glDeleteBuffers(1,&positionBuffer_);
		glDeleteBuffers(1,&normalBuffer_);
		glDeleteBuffers(1,&indexBuffer_);


		// Detach from render target
		rendertarget_.activate();
		rendertarget_.detachRenderTarget(positionMap_,0);
		rendertarget_.detachRenderTarget(normalMap_,1);
		rendertarget_.deactivate();

		// Delete textures
		glDeleteTextures(1,&positionMap_);
		glDeleteTextures(1,&normalMap_);
		glDeleteTextures(1,&heightMap_);
		glDeleteTextures(1,&vertexMap_);
		glDeleteTextures(1,&orignalnormalMap_);

		delete gpgpuVS_; gpgpuVS_ = 0;
		delete meshGeneration_; meshGeneration_ = 0;
		delete pass1_; pass1_ = 0;
		delete normalGeneration_; normalGeneration_ = 0;
		delete pass2_; pass2_ = 0;
		delete finalFS_; finalFS_ = 0;
		delete finalVS_; finalVS_ = 0;
		delete final_; final_ = 0;

		return 0;
	}
	int TLMCPUGPUHybrid::GetVRAMUsage()
	{
		return 0;
	}
	std::string TLMCPUGPUHybrid::GetFriendlyName( )
	{
		return std::string("CPU GPU Hybrid method" );
	}

	namespace
	{
		void DrawQuad(float x, float y, float w, float h)
		{
			glBegin(GL_QUADS);
			{
				glTexCoord2f(0.0f, 1.0f);	glVertex2f(x, y);	// top left corner
				glTexCoord2f(0.0f, 0.0f);	glVertex2f(x, y + h);
				glTexCoord2f(1.0f, 0.0f);	glVertex2f(x + w, y + h);
				glTexCoord2f(1.0f, 1.0f);	glVertex2f(x + w, y);
			}
			glEnd();
		}
	}

	void TLMCPUGPUHybrid::GenerateTLMData()
	{
		// Scatter
		for( int row = 0; row < vertsperedge_ -1; ++row)
		{
			for( int col = 0; col < vertsperedge_ -1; ++col)
			{
				int pos = (row * vertsperedge_) + col;

				Sn[pos] = 0.5f * (-In[pos] + Is[pos] + Ie[pos] + Iw[pos]);
				Ss[pos] = 0.5f * ( In[pos] - Is[pos] + Ie[pos] + Iw[pos]);
				Se[pos] = 0.5f * ( In[pos] + Is[pos] - Ie[pos] + Iw[pos]);
				Sw[pos] = 0.5f * ( In[pos] + Is[pos] + Ie[pos] - Iw[pos]);
			}
		}

		// Gather
		for( int row = 0; row < vertsperedge_; ++row)
		{
			for( int col = 0; col < vertsperedge_; ++col)
			{
				int pos = (row * vertsperedge_) + col;
				
				// This check is needed to make sure we don't try to access out of bounds
				// data later in the loop but also to ensure the edge data is zero'd to
				// prevent rendering artifacts
				if(col == 0 || row == 0 || col == vertsperedge_ -1 || row == vertsperedge_ - 1)
				{
					Mv[pos] = vector4(0.0,0.0,0.0,0.0);
					continue;
				}
				
				Mv[pos].x = Ie[pos] = Sw[pos + 1];
				Mv[pos].y = Iw[pos] = Se[pos - 1];
				Mv[pos].z = In[pos] = Ss[pos - vertsperedge_];
				Mv[pos].w = Is[pos] = Sn[pos + vertsperedge_];
			}
		}
		// Update the height information on the GPU
		glActiveTexture(GL_TEXTURE0);	
		glBindTexture(GL_TEXTURE_2D, heightMap_);
		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vertsperedge_, vertsperedge_,  GL_RGBA, GL_FLOAT, Mv);

		// Prepare for GPU based section
		glPushAttrib(GL_COLOR_BUFFER_BIT | GL_VIEWPORT_BIT);		// Save the clear colour and viewport

		// Setup the view for orthographic projection.
		camera_.setMatricies();
		// Switch to render target
		rendertarget_.activate();

		glClearColor(0.0f,0.0f,0.0f,0.0f);
		glViewport(0,0,vertsperedge_,vertsperedge_);
		glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
		glClampColorARB(GL_CLAMP_READ_COLOR_ARB, GL_FALSE);
		glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);


		// Setup the RT output
		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT);	

		// Generate mesh
		rendertarget_.attachRenderTarget(positionMap_);
		glActiveTexture(GL_TEXTURE1);	
		glBindTexture(GL_TEXTURE_2D, vertexMap_);			// Pass in the original positional information via a texture
		glActiveTexture(GL_TEXTURE2);	
		glBindTexture(GL_TEXTURE_2D, orignalnormalMap_);		// Pass in the original normal information via a texture

		pass1_->use();											// Set to use height map generation shader
		pass1_->sendUniform("energySource",0);					// setup the sampler for the energy source map
		pass1_->sendUniform("orignalPos",1);
		pass1_->sendUniform("orignalNormal",2);
		pass1_->sendUniform("scale",0.01f);						// Scale factor for the output

		// Draw quad here
		DrawQuad(0.0f, 0.0f, 1.0f, 1.0f);

		// now we need to copy the height map to an VBO for later rendering
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, positionBuffer_);
		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glReadPixels(0,0,vertsperedge_,vertsperedge_,GL_BGRA, GL_FLOAT,NULL);	// copy to VBO
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,0);
		rendertarget_.detachRenderTarget(positionMap_);

		// Generate normals
		rendertarget_.attachRenderTarget(normalMap_);			// Set rendering for normal map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, positionMap_);
		pass2_->use();
		pass2_->sendUniform("step",1.0f/float(vertsperedge_));
		// Draw quad here
		DrawQuad(0.0f, 0.0f, 1.0f, 1.0f);

		// Now we need to copy the normal map to a VBO for later rendering
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, normalBuffer_);
		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glReadPixels(0,0,vertsperedge_,vertsperedge_,GL_BGRA, GL_FLOAT,NULL);	// copy to VBO
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,0);
		rendertarget_.detachRenderTarget(normalMap_);

		rendertarget_.deactivate();	// switch back to normal rendering
		glPopAttrib();	// restore clear colour and view port

		glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_TRUE);
		glClampColorARB(GL_CLAMP_READ_COLOR_ARB, GL_TRUE);
		glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_TRUE);
		glFinish();


	}

	int TLMCPUGPUHybrid::RenderTLM(const vector3& pCamera, const vector3& pLight )
	{

		// Now we setup for the final rendering pass
		final_->use();

		// Draw final mesh data
		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer_);
		glVertexPointer(4,GL_FLOAT,sizeof(float)*4,NULL);				// XYZW (sources from PBO copy)
		
		if(normalStream_ > -1)
		{
			glBindBuffer(GL_ARRAY_BUFFER, normalBuffer_);
			glEnableVertexAttribArray(normalStream_);
			glVertexAttribPointer(normalStream_,4,GL_FLOAT,GL_FALSE,0,NULL );

		}

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);

			glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
			glDrawElements(GL_TRIANGLES,numIndices_,GL_UNSIGNED_INT,NULL);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);

		if(normalStream_ > -1) 
		{
			glDisableVertexAttribArray(normalStream_);
		}

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
		glBindBuffer(GL_ARRAY_BUFFER,0);		

		final_->disable();

		return 0;
	}

	void TLMCPUGPUHybrid::DriveTLMData(float drivingValue)
	{
		// This function is used to update the TLM driving map
		int row  = (vertsperedge_ ) * (vertsperedge_/2);
		int col = (vertsperedge_/2);

		Ie[row  + col] -= drivingValue;
		Iw[row  + col] -= drivingValue;
		In[row  + col] -= drivingValue;
		Is[row  + col] -= drivingValue;
	}

	void TLMCPUGPUHybrid::VisualiseTextures()
	{


	}

}