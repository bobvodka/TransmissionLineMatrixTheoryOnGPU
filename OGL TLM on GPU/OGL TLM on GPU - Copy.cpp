// OGL TLM on GPU
//
// A program to demo and bench mark the implementation of the TLM Theory on a GPU
// using OpenGL as the renderer and for GPGPU operations
#define NOMINMAX
#include "stdafx.h"
#include <oglwfw/WindowMgr.hpp>
#include "Camera/PerspectiveWorldCamera.hpp"
#include "Camera/PolarOrbitView.hpp"
#include "Maths/mtxlib.h"

#include "TLM Modules/TLMCommon.h"
#include "TLM Modules/ITLMRenderer.h"
#include "TLM Modules/TLMFullShader.h"
#include "TLM Modules/TLMCPUGPUHybrid.h"
#include "TLM Modules/TLMCPUOnly.h"
#include "TLM Modules/TLMMPCPUOnly.h"
#include "TLM Modules/TLMMPCPUGPUHybrid.h"

#include "Mesh/IMesh.hpp"
#include "Mesh/PlaneMesh.hpp"

#include <fstream>
#include <boost/format.hpp>	// for output stream data formatting
#include <limits>


//#include "libFrameEncoder.hpp"

struct timeDetails 
{
	int framenumber;
	LONGLONG updateTime;
	LONGLONG driveTime;
	LONGLONG renderTime;
	LONGLONG totalTime;
};

int _tmain(int argc, _TCHAR* argv[])
{
	OpenGLWFW::WindowManager WinMgr;
	try
	{
		if(!WinMgr.FindCompatibleOGLMode())
			return -1;
		if(!WinMgr.FindCompatibleDisplayMode(800,600))
			return -2;

	//	WinMgr.SetFullScreen(OpenGLWFW::winprops::fullscreen);
		WinMgr.CreateWin();
		WinMgr.Show();
	}
	catch (std::bad_exception &) 
	{
		return -3;
	}

	Resurrection::CameraSystem::PerspectiveWorldCamera camera(60.0f,800.0f/600.0f,1.0f,100.0f);
	vector3 pos(-4.0f, -1.0f, -4.0f);
	vector3 target(0.0f, 0.0f, 0.0f);
	float angle = 0.0f;
	Resurrection::CameraSystem::PolarOrbitView view(target,pos,DegToRad(angle - 270));
	camera.setView(&view);

	// Standard OpenGL setup stuff
	glClearColor(0.0f, 0.0f, 0.2f, 1.0f);		// This Will Clear The Background Color To kinda blue
	glClearDepth(1.0);							// Enables Clearing Of The Depth Buffer
	glDepthFunc(GL_LEQUAL);						// The Type Of Depth Test To Do
	glEnable(GL_DEPTH_TEST);					// Turn Depth Testing On
	glShadeModel(GL_SMOOTH);					// Enables Smooth Color Shading
	glDisable(GL_BLEND);
	glDisable(GL_CULL_FACE);
	glFrontFace(GL_CCW);
	glCullFace(GL_BACK);
	

	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);					// Full Brightness.  50% Alpha

	TLM::ITLMRenderer * currentmode;
	// Test system needs to automate the generation of the data for each module type at each size with each mesh
	// Size : 40, 50, 100, 128, 256, 512, 1024
	// GPU based one needs to be tested at both 16bit and 32bit per pixel.

	int meshsize = 512;
	Mesh::PlaneMesh mesh(meshsize);
//	TLM::TLMFullShader module(GL_RGBA32F_ARB,mesh);
//	TLM::TLMFullShader module(GL_RGBA16F_ARB,mesh);
//	TLM::TLMCPUGPUHybrid module(mesh);
//	TLM::TLMCPUOnly module(mesh);
//	TLM::TLMMPCPUOnly module(mesh);
	TLM::TLMMPCPUGPUHybrid module(mesh);
	module.CreateResources(meshsize);
	currentmode = &module;

//	currentmode->GenerateTLMData();
#undef TLM_RECORD
#ifdef TLM_RECORD
//	FrameEncoder::TheoraEncoder *encoder = new FrameEncoder::TheoraEncoder("tlm-singlepoint-scaled.ogm",800,600);
//	FrameEncoder::TheoraEncoder *encoder = new FrameEncoder::TheoraEncoder("tlm-dualpoints-noscale.ogm",800,600);
//	FrameEncoder::TheoraEncoder *encoder = new FrameEncoder::TheoraEncoder("tlm-dualpoints-scaled.ogm",800,600);	// didn't record o.O
//	FrameEncoder::TheoraEncoder *encoder = new FrameEncoder::TheoraEncoder("tlm-singlepoint-scaled-40by40.ogm",800,600);	
	FrameEncoder::TheoraEncoder *encoder = new FrameEncoder::TheoraEncoder("tlm-singlepoint-scaled-100by100.ogm",800,600);
	encoder->beginEncoding();
#endif
	size_t iter = 0;
	float frequency = 0.5f;
	int scale = 100;

	LARGE_INTEGER timestart;
	LARGE_INTEGER timeend;
	
	const size_t MAX_TEST_ITOR = 1000;

	timeDetails working;
	std::vector<timeDetails> details(MAX_TEST_ITOR);

	while (WinMgr.Dispatch() && iter < MAX_TEST_ITOR)
	{
		working.framenumber = iter;
		
		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		QueryPerformanceCounter(&timestart);
		currentmode->DriveTLMData(TLM::generateDrivingValue(iter,frequency,scale));
		QueryPerformanceCounter(&timeend);
		working.driveTime = timeend.QuadPart - timestart.QuadPart;
		QueryPerformanceCounter(&timestart);
		currentmode->GenerateTLMData();
		QueryPerformanceCounter(&timeend);
		working.updateTime = timeend.QuadPart - timestart.QuadPart;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		view.setAngle(DegToRad(angle - 270));
		camera.updateViewMatrix();
		camera.setMatricies();
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		QueryPerformanceCounter(&timestart);
		currentmode->RenderTLM(vector3(0.0f,0.0f,0.0f), vector3(0.0f,0.0f,0.0f));		
		QueryPerformanceCounter(&timeend);
		working.renderTime = timeend.QuadPart - timestart.QuadPart;
		
		// Store details
		details[iter] = working;

		angle += 0.3f;
		if(angle > 360.0f)
			angle = 0.0f;

	//	currentmode->VisualiseTextures();

#ifdef TLM_RECORD		
		unsigned char * buffer = encoder->requestBuffer();
		glReadPixels(0,0,800,600,GL_RGB,GL_UNSIGNED_BYTE,buffer);
		encoder->processBuffer(buffer);
		encoder->Process();
#endif	
		
		WinMgr.SwapBuffers();
		iter++;
	}
#ifdef TLM_RECORD
	encoder->endEncoding();
	delete encoder;
#endif
	currentmode->DestoryResources();

	// Work out details
	timeDetails average;
	average.driveTime = 0;
	average.renderTime = 0;
	average.updateTime = 0;

	// Collect all details to build average
	for(int i = 0; i < std::min(MAX_TEST_ITOR,details.size()); i++)
	{
		average.driveTime += details[i].driveTime;
		average.renderTime += details[i].renderTime;
		average.updateTime += details[i].updateTime;
	}
	average.driveTime /= std::min(MAX_TEST_ITOR,details.size());
	average.renderTime /= std::min(MAX_TEST_ITOR,details.size());
	average.updateTime /= std::min(MAX_TEST_ITOR,details.size());

	timeDetails min;
	min.driveTime = std::numeric_limits<LONGLONG>::max();
	min.renderTime = std::numeric_limits<LONGLONG>::max();
	min.updateTime = std::numeric_limits<LONGLONG>::max();

	// Collect all details to build min times
	for(int i = 0; i < std::min(MAX_TEST_ITOR,details.size()); i++)
	{
		min.driveTime = std::min(details[i].driveTime, min.driveTime);
		min.renderTime = std::min(details[i].renderTime, min.renderTime);
		min.updateTime = std::min(details[i].updateTime, min.updateTime);
	}

	timeDetails max;
	max.driveTime = 0;
	max.renderTime = 0;
	max.updateTime = 0;

	// Collect all details to build max times
	for(int i = 0; i < std::min(MAX_TEST_ITOR,details.size()); i++)
	{
		max.driveTime = std::max(details[i].driveTime, max.driveTime);
		max.renderTime = std::max(details[i].renderTime, max.renderTime);
		max.updateTime = std::max(details[i].updateTime, max.updateTime);
	}
	
	std::ofstream results("results.txt");

	results << boost::format("Name : %1%\nSize : %2%\n") % currentmode->GetFriendlyName() % meshsize; 
	results << boost::format("\t\tMax\tMin\tAverage\n");
	results << boost::format("Drive Time\t%1%\t%2%\t%3%\n") % max.driveTime % min.driveTime % average.driveTime;
	results << boost::format("Render Time\t%1%\t%2%\t%3%\n") % max.renderTime % min.renderTime % average.renderTime;
	results << boost::format("Update Time\t%1%\t%2%\t%3%\n") % max.updateTime % min.updateTime % average.updateTime;
	results << boost::format("Dumping full results in CSV format (drive,render,update)...\n\n");	
	for(int i = 0; i < std::min(MAX_TEST_ITOR,details.size()); i++)
	{
		results << boost::format("%1%,%2%,%3%\n") % details[i].driveTime % details[i].renderTime % details[i].updateTime;
	}

	results << boost::format("--------------------------------------------------------\n\n");

	return 0;
}

