#include <GLee.h>
#include <stdexcept>
#include <iostream>
#include <boost/lexical_cast.hpp>
#include "framebufferobject.hpp" 


namespace Resurrection
{
	namespace RenderToTexture
	{

		FrameBufferObject::FrameBufferObject()
		{
			sharedStencilBuffer_ = false;
			sharedDepthBuffer_   = false;
			stencilBufferID_     =     0;
			frameBufferID_       =     0;
			depthBufferID_       =     0;
			height_              =     0;
			width_               =     0;
		}

		bool  FrameBufferObject::initialize(int width, int height, 
                                    GLuint   sharedStencilBufferID,
                                    GLuint   sharedDepthBufferID,
                                    bool     depthOnly)
		{
			if(frameBufferID_)
				return false;
 
			sharedStencilBuffer_ = (sharedStencilBufferID!= 0);
			sharedDepthBuffer_   = (sharedDepthBufferID  != 0);

			height_              = height;
			width_               = width;

			glGenFramebuffersEXT(1, &frameBufferID_);
			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frameBufferID_);

	
/*
            			glGenRenderbuffersEXT(1, &depthBufferID_);
            			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthBufferID_);
            			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, width_, height_);
            			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
                        GL_RENDERBUFFER_EXT, depthBufferID_);*/
            

			glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
			return true;
		}

		/***************************************************************************************/
		/*                                                                                     */
		/*This function will report the status of the currently bound FBO                      */
		/*                                                                                     */
		/***************************************************************************************/
		const bool FrameBufferObject::checkStatus()                            
		{     

			GLenum status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);  

			//Our FBO is perfect, return true
			if(status == GL_FRAMEBUFFER_COMPLETE_EXT)
			return true;


			switch(status) 
			{                                          
			case GL_FRAMEBUFFER_INCOMPLETE_FORMATS_EXT: 
			  throw std::logic_error("FBO has one or several image attachments with different internal formats");

			case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT: 
			  throw std::logic_error("FBO has one or several image attachments with different dimensions");

			case GL_FRAMEBUFFER_INCOMPLETE_DUPLICATE_ATTACHMENT_EXT: 
			  throw std::logic_error("FBO has a duplicate image attachment");

			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT: 
			  throw std::logic_error("FBO missing an image attachment");

			case GL_FRAMEBUFFER_UNSUPPORTED_EXT: 
			  throw std::logic_error("FBO format unsupported");

			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
				throw std::logic_error("FBO Incomplete attachment error");

			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
				throw std::logic_error("FBO has an imcomplete draw buffer");
			}
			throw std::logic_error("Unknown FBO error number " + boost::lexical_cast<std::string>(status));  

			return false;
		}

		namespace
		{
			int clamp(int val, int bottom, int top)
			{
				if(val < bottom)
					return bottom;
				if(val > top)
					return top;
				
				return val;
			}
		}

		void FrameBufferObject::attachRenderTarget(GLuint texture, int index, int newTarget)
		{
				index = clamp(index, 0, 15);
				//std::cout << "Attaching texture " << texture << "to bind index " << index << std::endl;
			if(frameBufferID_)
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + index,
										  newTarget, texture, 0);
			else
			   throw std::logic_error("Invalid FrameBufferObject index");
		}

		void FrameBufferObject::detachRenderTarget(GLuint texture, int index, int newTarget)
		{
			index = clamp(index, 0, 15);

			if(frameBufferID_)
				glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT + index,
										  newTarget, 0, 0);
			else
				throw std::logic_error("Invalid FrameBufferObject index");
		}

		void FrameBufferObject::activate()
		{
			if(frameBufferID_)
				glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, frameBufferID_);
			else
				throw std::logic_error("Invalid FrameBufferObject index");
		}

		void const FrameBufferObject::deactivate()
		{
		  glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
		}

		void FrameBufferObject::genAttachDepthRenderBuffer()
		{
			activate();
			glGenRenderbuffersEXT(1, &depthBufferID_);
			glBindRenderbufferEXT(GL_RENDERBUFFER_EXT, depthBufferID_);
			glRenderbufferStorageEXT(GL_RENDERBUFFER_EXT, GL_DEPTH_COMPONENT24, width_, height_);
			glFramebufferRenderbufferEXT(GL_FRAMEBUFFER_EXT, GL_DEPTH_ATTACHMENT_EXT,
				GL_RENDERBUFFER_EXT, depthBufferID_);
			deactivate();
		}

		const GLuint FrameBufferObject::getStencilBufferID() const { return stencilBufferID_; }
		const GLuint FrameBufferObject::getDepthBufferID()   const { return depthBufferID_;   }
		const GLuint FrameBufferObject::getFrameBufferID()   const { return frameBufferID_;   }
		const GLuint FrameBufferObject::getHeight()          const { return height_;          }
		const GLuint FrameBufferObject::getWidth()           const { return width_;           }

		FrameBufferObject::~FrameBufferObject()
		{
			if(stencilBufferID_ && !sharedStencilBuffer_)
				glDeleteRenderbuffersEXT(1, &stencilBufferID_);

			if(depthBufferID_ && !sharedDepthBuffer_)
				glDeleteRenderbuffersEXT(1, &depthBufferID_);

			if(frameBufferID_)
				glDeleteFramebuffersEXT(1, &frameBufferID_);

			sharedStencilBuffer_ = false;
			sharedDepthBuffer_   = false;
			stencilBufferID_     =     0;
			frameBufferID_       =     0;
			depthBufferID_       =     0;
			height_              =     0;
			width_               =     0;
		}
	}
}