#ifndef FRAME_BUFFER_OBJECT_H
#define FRAME_BUFFER_OBJECT_H

#include <GLee.h>

namespace Resurrection
{
	namespace RenderToTexture
	{
		class FrameBufferObject
		{
		private:
			GLuint stencilBufferID_,
				depthBufferID_,
				frameBufferID_,
				height_,
				width_;

			bool   sharedStencilBuffer_,
				sharedDepthBuffer_;

		public:
			FrameBufferObject();
			~FrameBufferObject();

			bool initialize(int width, int height_,
				GLuint   sharedStencilBufferID = 0,
				GLuint   sharedDepthBufferID   = 0,
				bool     depthOnly             = false);
			void attachRenderTarget(GLuint texture, int index = 0, int newTarget = GL_TEXTURE_2D);
			void detachRenderTarget(GLuint texture, int index = 0, int newTarget = GL_TEXTURE_2D);
			void genAttachDepthRenderBuffer();

			static const void deactivate();
			void   activate();

			const GLuint getStencilBufferID() const;
			const GLuint getDepthBufferID()   const;
			const GLuint getFrameBufferID()   const;
			const GLuint getHeight()          const;
			const GLuint getWidth()           const;

			static  const bool  checkStatus();
		};

	}
}
#endif

