/************************************************************************/
/* Implementation of the TLM Renderer via CPU for all calcs		        */
/************************************************************************/

#include "ITLMRenderer.h"
#include "TLMCPUOnly.h"
#include <GLee.h>
#include <algorithm>
#include "../fbo/framebufferobject.hpp"
#define _USE_MATH_DEFINES
#include <math.h>

#include <iostream>

#include "..\Mesh\IMesh.hpp"

namespace TLM
{
	TLMCPUOnly::TLMCPUOnly(Mesh::IMesh &sourceMesh) : vertsperedge_(0), numVerts_(0), numIndices_(0),sourceMesh_(sourceMesh)
	{
	}

	TLMCPUOnly::~TLMCPUOnly()
	{
		DestoryResources();
	}

	namespace 
	{

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

	int TLMCPUOnly::CreateResources(const int vertsperedge)
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

		Mv = new float[numVerts_];
		finalMesh_ = new vector3[numVerts_];

		// And zero it out
		std::fill_n(Ie,(numVerts_),0.0f);
		std::fill_n(Iw,(numVerts_),0.0f);
		std::fill_n(In,(numVerts_),0.0f);
		std::fill_n(Is,(numVerts_),0.0f);

		std::fill_n(Se,(numVerts_),0.0f);
		std::fill_n(Sw,(numVerts_),0.0f);
		std::fill_n(Sn,(numVerts_),0.0f);
		std::fill_n(Ss,(numVerts_),0.0f);

		std::fill_n(Mv,(numVerts_),0.0f);
		std::fill_n(finalMesh_, (numVerts_), vector3(0.0f,0.0f,0.0f));

		// Next we'll need to load shaders here
		try
		{			
			finalVS_ = new GLSLShader("shaders/cpu only shader/final.vs");
			finalFS_ = new GLSLShader("shaders/cpu only shader/final.fs",GL_FRAGMENT_SHADER);
			final_ = new GLSLProgram();
			final_->attach(finalVS_);
			final_->attach(finalFS_);
			final_->link();
			std::cout << "Final render logs..." << std::endl;
			CheckShaderLogs(finalVS_);
			CheckShaderLogs(finalFS_);
			CheckProgramLogs(final_);

		}
		catch (std::exception &e)
		{
			std::cout << e.what() << std::endl;
		}

		// Finally allocate some buffers for the vertex data
		glGenBuffers(1,&vertexBuffer_);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,vertexBuffer_);
		// Create a buffer big enough to hold all the XYZ data
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, numVerts_*(sizeof(vector3)), NULL, GL_STATIC_DRAW_ARB);

		glGenBuffers(1,&normalBuffer_);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,normalBuffer_);
		// Create a buffer big enough to hold all the normal data
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, numVerts_*(sizeof(vector3)), NULL, GL_STATIC_DRAW_ARB);

		glGenBuffers(1,&indexBuffer_);
		glBindBufferARB(GL_ELEMENT_ARRAY_BUFFER_ARB,indexBuffer_);
		glBufferDataARB(GL_ELEMENT_ARRAY_BUFFER_ARB,numIndices_*(sizeof(GL_INT)), sourceMesh_.getIndexData(), GL_STATIC_DRAW_ARB);


		return 0;
	}

	int TLMCPUOnly::DestoryResources() 
	{
		//		OutputDebugString(L"INFO: Destroying resources for Full Shader TLM Class...\n");
		
		delete[] Ie; Ie = 0;
		delete[] Iw; Iw = 0;
		delete[] In; In = 0;
		delete[] Is; Is = 0;
		delete[] Se; Se = 0;
		delete[] Sw; Sw = 0;
		delete[] Sn; Sn = 0;
		delete[] Ss; Ss = 0;

		delete[] finalMesh_; finalMesh_ = 0;

		glDeleteBuffers(1,&vertexBuffer_);
		glDeleteBuffers(1,&normalBuffer_);
		glDeleteBuffers(1,&indexBuffer_);

		delete final_; final_ = 0;
		delete finalFS_; finalFS_ = 0;
		delete finalVS_; finalVS_ = 0;

		return 0;
	}
	int TLMCPUOnly::GetVRAMUsage()
	{
		return 0;
	}
	std::string TLMCPUOnly::GetFriendlyName( )
	{
		return std::string("CPU Only driven method" );
	}

	void TLMCPUOnly::GenerateTLMData()
	{
		// Scatter
		for( int row = 0; row < vertsperedge_ ; ++row)
		{
			for( int col = 0; col < vertsperedge_; ++col)
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
				if(col == 0 || row == 0 || col == vertsperedge_ - 1 || row == vertsperedge_ - 1)
				{
					Mv[pos] = 0.0f;
					continue;
				}

				Ie[pos] = Sw[pos + 1];
				Iw[pos] = Se[pos - 1];
				In[pos] = Ss[pos - vertsperedge_];
				Is[pos] = Sn[pos + vertsperedge_];

				//  DIVIDE BY 100 TO SCALE TO MODEL SPACE
				Mv[pos] = (Ie[pos] + Iw[pos] + In[pos] + Is[pos])/100; 
			}
		}

		// Perform mesh adjustment here
		vector3 const * srcVertData = sourceMesh_.getVertexData();
		vector3 const * srcNormData = sourceMesh_.getNormalData();

		// Move the source mesh data in the direction indicated by the normal by the TLM mag.
		// and then copy to the VBO
		// We do two writes as we later need to readback the vertex position in order to generate
		// the normal data and back reading from the gfx card is going to be sloooooow
		glBindBufferARB(GL_ARRAY_BUFFER_ARB,vertexBuffer_);
		glBufferDataARB(GL_ARRAY_BUFFER_ARB, numVerts_*(sizeof(vector3)), NULL, GL_STATIC_DRAW_ARB);
		vector3 * finalMesh = static_cast<vector3*>(glMapBuffer(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY));
		for(int i = 0; i < numVerts_; i++)
		{
			finalMesh_[i] = srcVertData[i] + (srcNormData[i] * Mv[i]);
			finalMesh[i] = finalMesh_[i];
		}
		glUnmapBuffer(GL_ARRAY_BUFFER_ARB);
		// Next we need to redevelop the normal data for the new mesh
		// Using an 8 tap method
		// Using k as the vertex to create normals for
		//
		//           h-----a-----b
		//           | \   |   / |
		//           |  \  |  /  |
		//           |   \ | /   |  
		//           g-----k-----c
		//           |   / | \   | 
		//           |  /  |  \  | 
		//           | /   |   \ |
		//           f ----e-----d
		{
			vector3 ka;
			vector3 kb;
			vector3 kc;
			vector3 kd;
			vector3 ke;
			vector3 kf;
			vector3 kg;
			vector3 kh;
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, normalBuffer_);
			glBufferDataARB(GL_ARRAY_BUFFER_ARB, numVerts_*(sizeof(vector3)), NULL, GL_STATIC_DRAW_ARB);
			vector3 * finalNormals = static_cast<vector3*>(glMapBuffer(GL_ARRAY_BUFFER_ARB, GL_WRITE_ONLY));

			for(int x = 0; x < vertsperedge_; x++)
			{
				for(int y = 0; y < vertsperedge_; y++)
				{
					// First generate vectors for each point from the point we are processing
					if(y == 0)
						ka = vector3(0.0,0.0,0.0);
					else
						ka = finalMesh_[x + ((y-1)*vertsperedge_)] - finalMesh_[x + (y*vertsperedge_)];

					if(y == 0 || x == vertsperedge_-1 )
						kb = vector3(0.0,0.0,0.0);
					else
						kb = finalMesh_[(x+1) + ((y-1)*vertsperedge_)] - finalMesh_[x + (y*vertsperedge_)];

					if(x == vertsperedge_-1)
						kc = vector3(0.0,0.0,0.0);
					else
						kc = finalMesh_[(x+1) + (y*vertsperedge_)] - finalMesh_[x + (y*vertsperedge_)];
					
					if(x == vertsperedge_-1 || y == vertsperedge_ -1 )
						kd = vector3(0.0,0.0,0.0);
					else
						kd = finalMesh_[(x+1) + ((y+1)*vertsperedge_)] - finalMesh_[x + (y*vertsperedge_)];

					if(y == vertsperedge_ - 1)
						ke = vector3(0.0,0.0,0.0);
					else
						ke = finalMesh_[x + ((y+1)*vertsperedge_)] - finalMesh_[x + (y*vertsperedge_)];

					if(x == 0 || y == vertsperedge_-1)
						kf = vector3(0.0,0.0,0.0);
					else
						kf = finalMesh_[(x-1) + ((y+1)*vertsperedge_)] - finalMesh_[x + (y*vertsperedge_)];

					if(x == 0)
						kg = vector3(0.0,0.0,0.0);
					else
						kg = finalMesh_[(x-1) + (y*vertsperedge_)] - finalMesh_[x + (y*vertsperedge_)];

					if(x == 0 || y == 0)
						kh = vector3(0.0,0.0,0.0);
					else
						kh = finalMesh_[x + (y*vertsperedge_)] - finalMesh_[x + (y*vertsperedge_)];

					// Next generate normal vectors
					vector3 normalA = CrossProduct(ka,kb);
					normalA.normalize();
					vector3 normalB = CrossProduct(kb,kc);
					normalB.normalize();
					vector3 normalC = CrossProduct(kc,kd);
					normalC.normalize();
					vector3 normalD = CrossProduct(kd,ke);
					normalD.normalize();
					vector3 normalE = CrossProduct(ke,kf);
					normalE.normalize();
					vector3 normalF = CrossProduct(kf,kg);
					normalF.normalize();
					vector3 normalG = CrossProduct(kg,kh);
					normalG.normalize();
					vector3 normalH = CrossProduct(kh,ka);
					normalH.normalize();
					
					// Create final vertex normal by averaging each point
					vector3 finalNormal = normalA + normalB + normalC + normalD + normalE + normalF + normalG + normalH;
					finalNormal /= 8;
					finalNormal.normalize();
					finalNormals[x + (y*vertsperedge_)] = finalNormal;
					
				}
			}
			glUnmapBuffer(GL_ARRAY_BUFFER_ARB);
		}

	}

	int TLMCPUOnly::RenderTLM(const vector3& pCamera, const vector3& pLight )
	{

		// Now we setup for the final rendering pass
		final_->use();

		// Draw final mesh data
		glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
		glVertexPointer(3,GL_FLOAT,sizeof(vector3),NULL);
		glBindBuffer(GL_ARRAY_BUFFER, normalBuffer_);
		glNormalPointer(GL_FLOAT,sizeof(vector3),NULL);

		glEnableClientState(GL_VERTEX_ARRAY);
		glEnableClientState(GL_NORMAL_ARRAY);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
		glDrawElements(GL_TRIANGLES,numIndices_,GL_UNSIGNED_INT,NULL);

		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_NORMAL_ARRAY);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER,0);
		glBindBuffer(GL_ARRAY_BUFFER,0);		
		final_->disable();

		return 0;
	}

	void TLMCPUOnly::DriveTLMData(float drivingValue)
	{
		// This function is used to update the TLM driving map
		int row  = (vertsperedge_ ) * (vertsperedge_/2);
		int col = (vertsperedge_/2);

		Ie[row  + col] -= drivingValue;
		Iw[row  + col] -= drivingValue;
		In[row  + col] -= drivingValue;
		Is[row  + col] -= drivingValue;
	}

	void TLMCPUOnly::VisualiseTextures()
	{
		

	}

}