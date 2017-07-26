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
#include "TLM Modules/TLMCPUOnlyMTTLM.h"

#include "Mesh/IMesh.hpp"
#include "Mesh/PlaneMesh.hpp"

#include <fstream>
#include <boost/format.hpp>	// for output stream data formatting
#include <boost/lexical_cast.hpp>
#include <limits>
#include <string>


#include "libFrameEncoder.hpp"

struct timeDetails 
{
	int framenumber;
	LONGLONG updateTime;
	LONGLONG driveTime;
	LONGLONG renderTime;
	LONGLONG totalTime;
};

// Declare the test function
void Test(const size_t MAX_TEST_ITOR, const size_t DELAY, float frequency, int scale, int meshsize, TLM::ITLMRenderer &currentmode, const std::string &filename, OpenGLWFW::WindowManager const &WinMgr);

// Main program function
int _tmain(int argc, _TCHAR* argv[])
{
	OpenGLWFW::WindowManager WinMgr;
	try
	{
		WinMgr.Init();
		if(!WinMgr.FindCompatibleOGLMode())
			return -1;
		if(!WinMgr.FindCompatibleDisplayMode(800,600))
			return -2;

//		WinMgr.SetFullScreen(OpenGLWFW::winprops::fullscreen);
		WinMgr.CreateWin();
		WinMgr.Show();
	}
	catch (std::bad_exception &) 
	{
		return -3;
	}

	OpenGLWFW::opengldetails opengldetails;
	WinMgr.GetOpenGLDetails(opengldetails);

	{
		LARGE_INTEGER freq;
		QueryPerformanceFrequency(&freq);
		std::ofstream displaydata("details.txt");
		displaydata << "Information about your graphics card" << std::endl;
		displaydata << "----------------------------------------------------" << std::endl;
		displaydata << "Vendor\t\t\t\t\t:\t" << opengldetails.vendor << std::endl;
		displaydata << "Version\t\t\t\t\t:\t" << opengldetails.version << std::endl;
		displaydata << "Renderer\t\t\t\t\t:\t" << opengldetails.renderer << std::endl;
		displaydata << "\nPerformance counter frequency (counts per second): \t" << freq.QuadPart << std::endl;
	}


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

	// Constants for all tests
	const float FREQUENCY = 1.0f;
	const int SCALE = 100;
	const size_t MAX_TEST_ITOR = 1000;
	const int NUM_TESTS = 7; // only 7 of the sizes work
	const int MESHSIZES[] = { 40, 50, 100, 128, 256, 512, 1024, 2048, 4096};
	const size_t ITERATIONDELAY = 10;	// Wait 10 iterations before collecting results


	Mesh::PlaneMesh mesh(0);

	// CPU only tests
	{
		TLM::TLMCPUOnly module(mesh);
		for(int i = 0; i < NUM_TESTS; i++)
		{		
			mesh.createResources(MESHSIZES[i]);
			mesh.generateMesh();
			module.CreateResources(MESHSIZES[i]);
			std::string filename(module.GetFriendlyName() + " " + boost::lexical_cast<std::string>(MESHSIZES[i]) + ".txt");
			WinMgr.SetWindowTitle("Testing : " + module.GetFriendlyName() + " @ " + boost::lexical_cast<std::string>(MESHSIZES[i]));
			Test(MAX_TEST_ITOR,ITERATIONDELAY, FREQUENCY,SCALE,MESHSIZES[i],module,filename,WinMgr);
			module.DestoryResources();
		}
	}

	// CPUGPU Hybrid tests @ 32bit per channel
	{
		TLM::TLMCPUGPUHybrid module(GL_RGBA32F_ARB,mesh);
		for(int i = 0; i < NUM_TESTS; i++)
		{	
			mesh.createResources(MESHSIZES[i]);
			mesh.generateMesh();
			module.CreateResources(MESHSIZES[i]);
			std::string filename(module.GetFriendlyName() + " " + boost::lexical_cast<std::string>(MESHSIZES[i]) + " 32bpc.txt");
			WinMgr.SetWindowTitle("Testing @ 32bpc : " + module.GetFriendlyName() + " @ " + boost::lexical_cast<std::string>(MESHSIZES[i]));
			Test(MAX_TEST_ITOR,ITERATIONDELAY,FREQUENCY,SCALE,MESHSIZES[i],module,filename,WinMgr);
			module.DestoryResources();
		}
	}
	// CPUGPU Hybrid tests @ 16bit per channel
	{
		TLM::TLMCPUGPUHybrid module(GL_RGBA16F_ARB,mesh);
		for(int i = 0; i < NUM_TESTS; i++)
		{	
			mesh.createResources(MESHSIZES[i]);
			mesh.generateMesh();
			module.CreateResources(MESHSIZES[i]);
			std::string filename(module.GetFriendlyName() + " " + boost::lexical_cast<std::string>(MESHSIZES[i]) + " 16bpc.txt");
			WinMgr.SetWindowTitle("Testing @ 16bpc : " + module.GetFriendlyName() + " @ " + boost::lexical_cast<std::string>(MESHSIZES[i]));
			Test(MAX_TEST_ITOR,ITERATIONDELAY,FREQUENCY,SCALE,MESHSIZES[i],module,filename,WinMgr);
			module.DestoryResources();
		}
	}
	
	// CPU Only MT tests
	{
		TLM::TLMMPCPUOnly module(mesh);

		for(int i = 0; i < NUM_TESTS; i++)
		{	
			mesh.createResources(MESHSIZES[i]);
			mesh.generateMesh();
			module.CreateResources(MESHSIZES[i]);
			std::string filename(module.GetFriendlyName() + " " + boost::lexical_cast<std::string>(MESHSIZES[i]) + ".txt");
			WinMgr.SetWindowTitle("Testing : " + module.GetFriendlyName() + " @ " + boost::lexical_cast<std::string>(MESHSIZES[i]));
			Test(MAX_TEST_ITOR,ITERATIONDELAY,FREQUENCY,SCALE,MESHSIZES[i],module,filename,WinMgr);
			module.DestoryResources();
		}
	}

	// CPU only with MT TLM tests
	{
		TLM::TLMCPUOnlyMTTLM module(mesh);
		for(int i = 0; i < NUM_TESTS; i++)
		{		
			mesh.createResources(MESHSIZES[i]);
			mesh.generateMesh();
			module.CreateResources(MESHSIZES[i]);
			std::string filename(module.GetFriendlyName() + " " + boost::lexical_cast<std::string>(MESHSIZES[i]) + ".txt");
			WinMgr.SetWindowTitle("Testing : " + module.GetFriendlyName() + " @ " + boost::lexical_cast<std::string>(MESHSIZES[i]));
			Test(MAX_TEST_ITOR,ITERATIONDELAY, FREQUENCY,SCALE,MESHSIZES[i],module,filename,WinMgr);
			module.DestoryResources();
		}
	}

	// CPU/GPU MT tests @ 32bpc
	{
		TLM::TLMMPCPUGPUHybrid module(GL_RGBA32F_ARB,mesh);

		for(int i = 0; i < NUM_TESTS; i++)
		{	
			mesh.createResources(MESHSIZES[i]);
			mesh.generateMesh();
			module.CreateResources(MESHSIZES[i]);
			std::string filename(module.GetFriendlyName() + " " + boost::lexical_cast<std::string>(MESHSIZES[i]) + " 32bpc.txt");
			WinMgr.SetWindowTitle("Testing @ 32bpc : " + module.GetFriendlyName() + " @ " + boost::lexical_cast<std::string>(MESHSIZES[i]));
			Test(MAX_TEST_ITOR,ITERATIONDELAY,FREQUENCY,SCALE,MESHSIZES[i],module,filename,WinMgr);
			module.DestoryResources();
		}
	}

	// CPU/GPU MT tests @ 16bpc
	{
		TLM::TLMMPCPUGPUHybrid module(GL_RGBA16F_ARB,mesh);

		for(int i = 0; i < NUM_TESTS; i++)
		{	
			mesh.createResources(MESHSIZES[i]);
			mesh.generateMesh();
			module.CreateResources(MESHSIZES[i]);
			std::string filename(module.GetFriendlyName() + " " + boost::lexical_cast<std::string>(MESHSIZES[i]) + " 16bpc.txt");
			WinMgr.SetWindowTitle("Testing @ 16bpc : " + module.GetFriendlyName() + " @ " + boost::lexical_cast<std::string>(MESHSIZES[i]));
			Test(MAX_TEST_ITOR,ITERATIONDELAY,FREQUENCY,SCALE,MESHSIZES[i],module,filename,WinMgr);
			module.DestoryResources();
		}
	}

	// GPU only tests @ 32bit per channel
	// We need at least GL2.0, PBO and FP textures to let this method work
	if(GLEE_ARB_pixel_buffer_object && GLEE_ARB_texture_float && GLEE_VERSION_2_0)
	{
		TLM::TLMFullShader module(GL_RGBA32F_ARB,mesh);
		for(int i = 0; i < NUM_TESTS; i++)
		{	
			mesh.createResources(MESHSIZES[i]);
			mesh.generateMesh();
			module.CreateResources(MESHSIZES[i]);
			std::string filename(module.GetFriendlyName() + " " + boost::lexical_cast<std::string>(MESHSIZES[i]) + " 32bpc");
			WinMgr.SetWindowTitle("Testing @ 32bpc : " + module.GetFriendlyName() + " @ " + boost::lexical_cast<std::string>(MESHSIZES[i]));
			Test(MAX_TEST_ITOR,ITERATIONDELAY,FREQUENCY,SCALE,MESHSIZES[i],module,filename,WinMgr);
			module.DestoryResources();
		}
	}

	// GPU only tests @ 16bit per channel
	// We need at least GL2.0, PBO and FP textures to let this method work
	if(GLEE_ARB_pixel_buffer_object && GLEE_ARB_texture_float && GLEE_VERSION_2_0)
	{
		TLM::TLMFullShader module(GL_RGBA16F_ARB,mesh);
		for(int i = 0; i < NUM_TESTS; i++)
		{	
			mesh.createResources(MESHSIZES[i]);
			mesh.generateMesh();
			module.CreateResources(MESHSIZES[i]);
			std::string filename(module.GetFriendlyName() + " " + boost::lexical_cast<std::string>(MESHSIZES[i]) + " 16bpc.txt");
			WinMgr.SetWindowTitle("Testing @ 16bpc : " + module.GetFriendlyName() + " @ " + boost::lexical_cast<std::string>(MESHSIZES[i]));
			Test(MAX_TEST_ITOR,ITERATIONDELAY,FREQUENCY,SCALE,MESHSIZES[i],module,filename,WinMgr);
			module.DestoryResources();
		}
	}
	
	return 0;
}

void Test(const size_t MAX_TEST_ITOR, const size_t DELAY, float frequency, int scale, int meshsize, TLM::ITLMRenderer &currentmode, const std::string &filename, OpenGLWFW::WindowManager const &WinMgr)
{
	size_t iter = 0;
	LARGE_INTEGER timestart;
	LARGE_INTEGER timeend;

	timeDetails working;
	std::vector<timeDetails> details(MAX_TEST_ITOR);

	Resurrection::CameraSystem::PerspectiveWorldCamera camera(60.0f,800.0f/600.0f,1.0f,100.0f);
	vector3 pos(-4.0f, -1.0f, -4.0f);
	vector3 target(0.0f, 0.0f, 0.0f);
	float angle = 0.0f;
	Resurrection::CameraSystem::PolarOrbitView view(target,pos,DegToRad(angle - 270));
	camera.setView(&view);
	
	std::string vfilename = filename + ".ogm";
	FrameEncoder::TheoraEncoder *encoder = new FrameEncoder::TheoraEncoder(vfilename,800,600);

	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	LARGE_INTEGER fpsstart;
	LARGE_INTEGER fpsend;
	QueryPerformanceCounter(&fpsstart);

	while (iter < MAX_TEST_ITOR + DELAY)
	{
		working.framenumber = iter;

		glDisable(GL_CULL_FACE);
		glDisable(GL_DEPTH_TEST);
		QueryPerformanceCounter(&timestart);
		currentmode.DriveTLMData(TLM::generateDrivingValue(iter,frequency,scale));
		QueryPerformanceCounter(&timeend);
		working.driveTime = timeend.QuadPart - timestart.QuadPart;
		QueryPerformanceCounter(&timestart);
		currentmode.GenerateTLMData();
		QueryPerformanceCounter(&timeend);
		working.updateTime = timeend.QuadPart - timestart.QuadPart;

		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		view.setAngle(DegToRad(angle - 270));
		camera.updateViewMatrix();
		camera.setMatricies();
		glEnable(GL_DEPTH_TEST);
		glEnable(GL_CULL_FACE);
		QueryPerformanceCounter(&timestart);
		currentmode.RenderTLM(vector3(0.0f,0.0f,0.0f), vector3(0.0f,0.0f,0.0f));		
		QueryPerformanceCounter(&timeend);
		working.renderTime = timeend.QuadPart - timestart.QuadPart;

		// Store details
		if(iter >= DELAY)
			details[iter - DELAY] = working;

		angle += 0.3f;
		if(angle > 360.0f)
			angle = 0.0f;

		//currentmode.VisualiseTextures();
		unsigned char * buffer = encoder->requestBuffer();
		glReadPixels(0,0,800,600,GL_RGB,GL_UNSIGNED_BYTE,buffer);
		encoder->processBuffer(buffer);
		encoder->Process();

		WinMgr.SwapBuffers();
		iter++;
	}
	QueryPerformanceCounter(&fpsend);

	encoder->endEncoding();
	delete encoder;

	LARGE_INTEGER runtime;
	runtime.QuadPart = fpsend.QuadPart - fpsstart.QuadPart;
	float tmp = runtime.QuadPart / freq.QuadPart;
	float fps = float(MAX_TEST_ITOR + DELAY) / tmp;

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

	std::string ofilename = filename + ".txt";
	std::ofstream results(ofilename.c_str());

	results << boost::format("Name : %1%\nSize : %2%\n") % currentmode.GetFriendlyName() % meshsize; 
	results << boost::format("\t\tMax\tMin\tAverage\n");
	results << boost::format("Drive Time\t%1%\t%2%\t%3%\n") % max.driveTime % min.driveTime % average.driveTime;
	results << boost::format("Render Time\t%1%\t%2%\t%3%\n") % max.renderTime % min.renderTime % average.renderTime;
	results << boost::format("Update Time\t%1%\t%2%\t%3%\n") % max.updateTime % min.updateTime % average.updateTime;
	results << boost::format("FPS : %1%\n") % fps;
	results << boost::format("Dumping full results in CSV format (drive,render,update)...\n");	
	for(int i = 0; i < std::min(MAX_TEST_ITOR,details.size()); i++)
	{
		results << boost::format("%1%,%2%,%3%\n") % details[i].driveTime % details[i].renderTime % details[i].updateTime;
	}
}