/************************************************************************/
/* Implementation of the TLM Renderer via shaders for all calcs         */
/************************************************************************/

#include "ITLMRenderer.h"
#include "TLMFullShader.h"
#include <GLee.h>
#include <algorithm>
#include "../fbo/framebufferobject.hpp"
#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>

namespace TLM
{
	TLMFullShader::TLMFullShader(GLuint internalformat, Mesh::IMesh &sourceMesh) : camera_(0,1,1,0,-1,1), internalformat_(internalformat), vertsperedge_(0), 
																					numVerts_(0), numIndices_(0),sourceMesh_(sourceMesh)
	{
		camera_.setView(&view);	// Setup the orthographic camera for the R2T operation
		camera_.updateViewMatrix();
	}

	TLMFullShader::~TLMFullShader()
	{
		DestoryResources();
	}

	namespace 
	{
		void SetupRenderTarget(const int size, const int internalformat, const int type = GL_RGBA, void * data = NULL)
		{
			glTexImage2D(GL_TEXTURE_2D,0, internalformat, size, size, 0, type, GL_FLOAT, data);
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

		void CheckProgramLogs(GLSLProgram *&program)
		{
			std::string log;
			program->validate();
			program->GetProgramLog(log);
			std::cout << log << std::endl;
		}
	}

	int TLMFullShader::CreateResources(const int vertsperedge)
	{
		vertsperedge_ = vertsperedge;	// save the size of the buffer we'll need
		numVerts_ = vertsperedge*vertsperedge;
		numIndices_ = 6*(vertsperedge-1)*(vertsperedge-1);

		// Setup our render target
		// This will be used to hold all of our textures for rendering to
		rendertarget_.initialize(vertsperedge_,vertsperedge_);

		// Textures required for the TLM simulation

		// This can be any format (32bit or 16bit)
		glGenTextures(1,&positionMap_);
		glBindTexture(GL_TEXTURE_2D, positionMap_);
		SetupRenderTarget(vertsperedge,internalformat_);

		// This can be any format (32bit or 16bit)
		glGenTextures(2,energyMap_);
		glBindTexture(GL_TEXTURE_2D, energyMap_[0]);
		SetupRenderTarget(vertsperedge,internalformat_);
		glBindTexture(GL_TEXTURE_2D, energyMap_[1]);
		SetupRenderTarget(vertsperedge,internalformat_);

		// Due to vertex format limitations this is always 32bit per channel (o rly? no, not rly...)
		glGenTextures(1,&normalMap_);
		glBindTexture(GL_TEXTURE_2D, normalMap_);
		SetupRenderTarget(vertsperedge,internalformat_);

		// This can be any format (32bit or 16bit)
		glGenTextures(1,&drivingMap_);
		glBindTexture(GL_TEXTURE_2D, drivingMap_);
		if(internalformat_ == GL_RGBA32F_ARB)
			SetupRenderTarget(vertsperedge,GL_ALPHA32F_ARB, GL_ALPHA);
		else if(internalformat_ == GL_RGBA16F_ARB)
			SetupRenderTarget(vertsperedge,GL_ALPHA16F_ARB, GL_ALPHA);
		else
			SetupRenderTarget(vertsperedge,internalformat_);

		// Now we need to setup the vars for ping-pong rendering to the energy map
		energySource_ = energyMap_[0];
		energySink_ = energyMap_[1];
		energyFBOSource_ = GL_COLOR_ATTACHMENT1_EXT;
		energyFBOSink_ = GL_COLOR_ATTACHMENT2_EXT;

		// Next we'll need to load shaders here
		try
		{			
			// First the general GPGPU shader
			gpgpuVS_ = new GLSLShader("shaders/full shader/gpgpu.vs");
			// Next we load each pass in turn
			energyTransfer_ = new GLSLShader("shaders/full shader/energytransfer.fs",GL_FRAGMENT_SHADER);
			std::cout << "Energy Transfer logs..." << std::endl;
			CheckShaderLogs(gpgpuVS_);
			CheckShaderLogs(energyTransfer_);
			pass1_ = new GLSLProgram();
			pass1_->attach(gpgpuVS_);
			pass1_->attach(energyTransfer_);
			pass1_->link();
			CheckProgramLogs(pass1_);

			heightGeneration_ = new GLSLShader("shaders/full shader/heightgen.fs",GL_FRAGMENT_SHADER);
			pass2_ = new GLSLProgram();
			pass2_->attach(gpgpuVS_);
			pass2_->attach(heightGeneration_);
			pass2_->link();
			std::cout << "Height Generation logs..." << std::endl;
			CheckShaderLogs(heightGeneration_);
			CheckProgramLogs(pass2_);

			energyGeneration_ = new GLSLShader("shaders/full shader/energygen.fs",GL_FRAGMENT_SHADER);
			pass3_ = new GLSLProgram();
			pass3_->attach(gpgpuVS_);
			pass3_->attach(energyGeneration_);
			pass3_->link();
			std::cout << "Energy Generation logs..." << std::endl;
			CheckShaderLogs(energyGeneration_);
			CheckProgramLogs(pass3_);

			normalGeneration_ = new GLSLShader("shaders/full shader/normalgen.fs",GL_FRAGMENT_SHADER);
			pass4_ = new GLSLProgram();
			pass4_->attach(gpgpuVS_);
			pass4_->attach(normalGeneration_);
			pass4_->link();
			std::cout << "Normal Generation logs..." << std::endl;
			CheckShaderLogs(normalGeneration_);
			CheckProgramLogs(pass4_);
	
			finalVS_ = new GLSLShader("shaders/full shader/final.vs");
			finalFS_ = new GLSLShader("shaders/full shader/final.fs",GL_FRAGMENT_SHADER);
			pass5_ = new GLSLProgram();
			pass5_->attach(finalVS_);
			pass5_->attach(finalFS_);
			pass5_->link();
			std::cout << "Final render logs..." << std::endl;
			CheckShaderLogs(finalVS_);
			CheckShaderLogs(finalFS_);
			CheckProgramLogs(pass5_);

			// Extract attrib locations for the two data streams we need to send in
			// for the final render
			normalStream_ = pass5_->getAttributeLocation("normalData");
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
		SetupRenderTarget(vertsperedge_,internalformat_,GL_RGBA,tmp);
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
		SetupRenderTarget(vertsperedge_,internalformat_,GL_RGBA,tmp);

		delete[] tmp;

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

		// Finally create enough memory to be used for the driving texture
		// and zero it out
		drivingTexture_ = new GLfloat[numVerts_];
		std::fill_n(drivingTexture_,(numVerts_),0.0f);

		return 0;
	}

	int TLMFullShader::DestoryResources() 
	{
		// Detach from render target
		rendertarget_.activate();
		rendertarget_.detachRenderTarget(positionMap_,0);
		rendertarget_.detachRenderTarget(energyMap_[0],1);
		rendertarget_.detachRenderTarget(energyMap_[1],2);

		rendertarget_.detachRenderTarget(normalMap_,4);
		rendertarget_.detachRenderTarget(drivingMap_,5);
		rendertarget_.deactivate();

		// Delete textures
		glDeleteTextures(1,&positionMap_);
		glDeleteTextures(2,energyMap_);
//		glDeleteTextures(1,&heightMapSource_);
		glDeleteTextures(1,&normalMap_);
		glDeleteTextures(1,&drivingMap_);
		glDeleteTextures(1,&vertexMap_);
		glDeleteTextures(1,&orignalnormalMap_);

		// Delete VBOs
		glDeleteBuffers(1,&positionBuffer_);
		glDeleteBuffers(1,&normalBuffer_);

		// Delete driving texture memory
		delete [] drivingTexture_; drivingTexture_ = 0;

		// Clean up all the shaders

		delete finalFS_; finalFS_ = 0;
		delete finalVS_; finalVS_ = 0;
		delete gpgpuVS_; gpgpuVS_ = 0;
		delete energyTransfer_; energyTransfer_ = 0;
		delete heightGeneration_; heightGeneration_ = 0;
		delete normalGeneration_; normalGeneration_ = 0;
		delete energyGeneration_; energyGeneration_ = 0;

		delete pass1_; pass1_ = 0;
		delete pass2_; pass2_ = 0;
		delete pass3_; pass3_ = 0;
		delete pass4_; pass4_ = 0;
		delete pass5_; pass5_ = 0;

		return 0;
	}
	int TLMFullShader::GetVRAMUsage()
	{
		return 0;
	}
	std::string TLMFullShader::GetFriendlyName( )
	{
		return std::string("Full shader driven method" );
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
	
	void TLMFullShader::GenerateTLMData()
	{
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

		// Setup for pass 1
		// Generates new energy map
		// Bind energy source texture
		glActiveTexture(GL_TEXTURE0);		
		glBindTexture(GL_TEXTURE_2D, energySource_);			// Setup the energy source map
		rendertarget_.attachRenderTarget(energySink_);			// Set destination to energy sink buffer
		pass1_->use();											// Set to energy transfer shader
		pass1_->sendUniform("energySource",0);					// setup the sampler for the energy source map
		pass1_->sendUniform("step",1.0f/float(vertsperedge_));	// Set offset between texels
		// Draw quad here
		DrawQuad(0.0f, 0.0f, 1.0f, 1.0f);						// Perform transfer
 		rendertarget_.detachRenderTarget(energySink_);

		// Setup for pass 2
		// Generates positional information
		rendertarget_.attachRenderTarget(positionMap_);
		glActiveTexture(GL_TEXTURE0);	
		glBindTexture(GL_TEXTURE_2D, energySink_);				// Set the correct source for the data
		glActiveTexture(GL_TEXTURE1);	
		glBindTexture(GL_TEXTURE_2D, vertexMap_);			// Pass in the original positional information via a texture
		glActiveTexture(GL_TEXTURE2);	
		glBindTexture(GL_TEXTURE_2D, orignalnormalMap_);		// Pass in the original normal information via a texture

		pass2_->use();											// Set to use height map generation shader
		pass2_->sendUniform("energySource",0);					// setup the sampler for the energy source map
		pass2_->sendUniform("orignalPos",1);
		pass2_->sendUniform("orignalNormal",2);
		pass2_->sendUniform("scale",0.01f);						// Scale factor for the output
		
		// Draw quad here
		DrawQuad(0.0f, 0.0f, 1.0f, 1.0f);
		
		// now we need to copy the height map to an VBO for later rendering
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB, positionBuffer_);
		glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
		glReadPixels(0,0,vertsperedge_,vertsperedge_,GL_BGRA, GL_FLOAT,NULL);	// copy to VBO
		glBindBuffer(GL_PIXEL_PACK_BUFFER_ARB,0);
		rendertarget_.detachRenderTarget(positionMap_);
		
		// Setup for pass 3
		// Generate energy for next iteration
		rendertarget_.attachRenderTarget(energySource_);		// Setup the energy destination buffer
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D,drivingMap_);				// Bind the driving texture
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D,energySink_);
		pass3_->use();
		pass3_->sendUniform("energySource",0);					// setup the sampler for the energy source map
		pass3_->sendUniform("drivingMap",1);						// setup the sampler for the driving map
		// Draw quad here
		DrawQuad(0.0f, 0.0f, 1.0f, 1.0f);
		rendertarget_.detachRenderTarget(energySource_);

		// Setup for pass 4
		// Generates normals
		rendertarget_.attachRenderTarget(normalMap_);			// Set rendering for normal map
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, positionMap_);
		pass4_->use();
		pass4_->sendUniform("step",1.0f/float(vertsperedge_));
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
		
	int TLMFullShader::RenderTLM(const vector3& pCamera, const vector3& pLight )
	{
		
		// Now we setup for the final rendering pass
		pass5_->use();
		
		glBindBuffer(GL_ARRAY_BUFFER, positionBuffer_);
		glVertexPointer(4,GL_FLOAT,sizeof(float)*4,NULL);				// XYZW (sources from PBO copy)
	
		
		if(normalStream_ > -1)
		{
			glBindBuffer(GL_ARRAY_BUFFER, normalBuffer_);
			glEnableVertexAttribArray(normalStream_);
			glVertexAttribPointer(normalStream_,4,GL_FLOAT,GL_FALSE,0,NULL );

		}
		// Enable the data streams
		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_TEXTURE_COORD_ARRAY);

		// Rendering using glDrawElements command or the like
		// Vertex index array already bound to the element array bind point from init
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,indexBuffer_);
		glDrawElements(GL_TRIANGLES,numIndices_,GL_UNSIGNED_INT,NULL);

		// Disable the streams
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_TEXTURE_COORD_ARRAY);
		if(normalStream_ > -1) 
		{
			glDisableVertexAttribArray(normalStream_);
		}
		glBindBufferARB(GL_ARRAY_BUFFER, 0);

		pass5_->disable();
		
		return 0;
	}

	void TLMFullShader::DriveTLMData(float drivingValue)
	{
		glBindTexture(GL_TEXTURE_2D, drivingMap_);		// Setup driving map texture

		int row  = (vertsperedge_ ) * (vertsperedge_/2);
		int col = (vertsperedge_/4);


		drivingTexture_[row + col] = drivingValue;

		col = (vertsperedge_/4) * 3;

		drivingTexture_[row + col] = drivingValue;

		glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, vertsperedge_, vertsperedge_, GL_ALPHA, GL_FLOAT, drivingTexture_);

	}

	void TLMFullShader::VisualiseTextures()
	{
		// Debug function to visualise texture data
		camera_.setMatricies();
		glEnable(GL_TEXTURE_2D);
		
		glBindTexture(GL_TEXTURE_2D, positionMap_);
		DrawQuad(0.0f,0.0f,0.2f,0.2f);
		
		glBindTexture(GL_TEXTURE_2D, energyMap_[0]);
		DrawQuad(0.25f,0.0f,0.2f,0.2f);

		glBindTexture(GL_TEXTURE_2D, energyMap_[1]);
		DrawQuad(0.5f,0.0f,0.2f,0.2f);

		glBindTexture(GL_TEXTURE_2D, vertexMap_);
		DrawQuad(0.0f,0.8f,0.2f,0.2f);

		glBindTexture(GL_TEXTURE_2D, orignalnormalMap_);
		DrawQuad(0.25f,0.8f,0.2f,0.2f);
		
		glDisable(GL_TEXTURE_2D);

	}

}