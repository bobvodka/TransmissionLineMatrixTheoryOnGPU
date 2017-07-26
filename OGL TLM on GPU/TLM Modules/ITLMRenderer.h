/************************************************************************/
/* Interface for all TLM Renderer class									*/
/* All classes are based on this for uniformity of rendering			*/
/* Each class will be responsible for loading it's own resources as		*/
/* required and releasing them upon request.							*/
/************************************************************************/

#ifndef OGLTLM_ITLMRENDERER_H
#define OGLTLM_ITLMRENDERER_H

#include <string>
#include "../Maths/mtxlib.h"

namespace TLM
{
	class ITLMRenderer
	{
	public:
		// Will be called when device is create or reset
		virtual int CreateResources(const int vertsperedge) = 0;

		// Will be called when device is lost or destroyed
		virtual int DestoryResources() = 0;

		// Should return VRAM usage in bytes
		virtual int GetVRAMUsage() = 0;

		// Get the name of this rendering module
		virtual std::string GetFriendlyName( ) = 0;

		// Generate the next step in the TLM equation
		virtual void GenerateTLMData() = 0;

		// Drive Data updates the drive data source
		virtual void DriveTLMData(float drivingValue) = 0;

		// Perform the final rendering
		virtual int RenderTLM(const vector3& pCamera, const vector3& pLight ) = 0;
		
		//DEbug
		virtual void VisualiseTextures() = 0;
	};
}


#endif