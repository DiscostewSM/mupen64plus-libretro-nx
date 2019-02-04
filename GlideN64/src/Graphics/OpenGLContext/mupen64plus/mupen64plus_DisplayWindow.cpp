#include <stdio.h>
#include <cstdlib>
#include <Graphics/Context.h>
#include <Graphics/OpenGLContext/GLFunctions.h>
#include <Graphics/OpenGLContext/opengl_Utils.h>
#include <mupenplus/GLideN64_mupenplus.h>
#include <GLideN64.h>
#include <Config.h>
#include <N64.h>
#include <gSP.h>
#include <Log.h>
#include <FrameBuffer.h>
#include <GLideNUI/GLideNUI.h>
#include <DisplayWindow.h>

#include <libretro_private.h>
#include "../../../../../custom/GLideN64/mupenplus/GLideN64_mupenplus.h"

#ifdef VC
#include <bcm_host.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif
uint32_t get_retro_screen_width();
uint32_t get_retro_screen_height();
#ifdef __cplusplus
}
#endif

class DisplayWindowMupen64plus : public DisplayWindow
{
public:
	DisplayWindowMupen64plus() {}

private:
	void _setAttributes();
	void _getDisplaySize();

	bool _start() override;
	void _stop() override;
	void _swapBuffers() override;
	void _saveScreenshot() override;
	void _saveBufferContent(graphics::ObjectHandle _fbo, CachedTexture *_pTexture) override;
	bool _resizeWindow() override;
	void _changeWindow() override;
	void _readScreen(void **_pDest, long *_pWidth, long *_pHeight) override;
	void _readScreen2(void * _dest, int * _width, int * _height, int _front) override;
	graphics::ObjectHandle _getDefaultFramebuffer() override;
};

DisplayWindow & DisplayWindow::get()
{
	static DisplayWindowMupen64plus video;
	return video;
}

void DisplayWindowMupen64plus::_setAttributes()
{
	LOG(LOG_VERBOSE, "[gles2GlideN64]: _setAttributes\n");
}

bool DisplayWindowMupen64plus::_start()
{
	_setAttributes();

	m_bFullscreen = 1;
	m_screenWidth = get_retro_screen_width();
	m_screenHeight = get_retro_screen_height();
	_getDisplaySize();
	_setBufferSize();

	LOG(LOG_VERBOSE, "[gles2GlideN64]: Create setting videomode %dx%d\n", m_screenWidth, m_screenHeight);


	return true;
}

void DisplayWindowMupen64plus::_stop()
{
}

void DisplayWindowMupen64plus::_swapBuffers()
{
	libretro_swap_buffer = true;
}

void DisplayWindowMupen64plus::_saveScreenshot()
{
}

void DisplayWindowMupen64plus::_saveBufferContent(graphics::ObjectHandle /*_fbo*/, CachedTexture* /*_pTexture*/)
{
}

bool DisplayWindowMupen64plus::_resizeWindow()
{
	_setAttributes();
	m_bFullscreen = false;
	m_width = m_screenWidth = m_resizeWidth;
	m_height = m_screenHeight = m_resizeHeight;

	_setBufferSize();
	opengl::Utils::isGLError(); // reset GL error.

	return true;
}

void DisplayWindowMupen64plus::_changeWindow()
{
}

void DisplayWindowMupen64plus::_getDisplaySize()
{
#ifdef VC
	if( m_bFullscreen ) {
		// Use VC get_display_size function to get the current screen resolution
		u32 fb_width;
		u32 fb_height;
		if (graphics_get_display_size(0 /* LCD */, &fb_width, &fb_height) < 0)
			printf("ERROR: Failed to get display size\n");
		else {
			m_screenWidth = fb_width;
			m_screenHeight = fb_height;
		}
	}
#endif
}

void DisplayWindowMupen64plus::_readScreen(void **_pDest, long *_pWidth, long *_pHeight)
{
	*_pWidth = m_width;
	*_pHeight = m_height;

	*_pDest = malloc(m_height * m_width * 3);
	if (*_pDest == nullptr)
		return;

#ifndef GLESX
	GLint oldMode;
	glGetIntegerv(GL_READ_BUFFER, &oldMode);
	gfxContext.bindFramebuffer(graphics::bufferTarget::READ_FRAMEBUFFER, graphics::ObjectHandle::defaultFramebuffer);
	glReadBuffer(GL_FRONT);
	glReadPixels(0, m_heightOffset, m_width, m_height, GL_BGR_EXT, GL_UNSIGNED_BYTE, *_pDest);
	if (graphics::BufferAttachmentParam(oldMode) == graphics::bufferAttachment::COLOR_ATTACHMENT0) {
		FrameBuffer * pBuffer = frameBufferList().getCurrent();
		if (pBuffer != nullptr)
			gfxContext.bindFramebuffer(graphics::bufferTarget::READ_FRAMEBUFFER, pBuffer->m_FBO);
	}
	glReadBuffer(oldMode);
#else
	glReadPixels(0, m_heightOffset, m_width, m_height, GL_RGB, GL_UNSIGNED_BYTE, *_pDest);
#endif
}

void DisplayWindowMupen64plus::_readScreen2(void * _dest, int * _width, int * _height, int _front)
{
	if (_width == nullptr || _height == nullptr)
		return;

	*_width = m_screenWidth;
	*_height = m_screenHeight;

	if (_dest == nullptr)
		return;

	u8 *pBufferData = (u8*)malloc((*_width)*(*_height) * 4);
	u8 *pDest = (u8*)_dest;

#if !defined(OS_ANDROID) && !defined(OS_IOS)
	GLint oldMode;
	glGetIntegerv(GL_READ_BUFFER, &oldMode);
	if (_front != 0)
		glReadBuffer(GL_FRONT);
	else
		glReadBuffer(GL_BACK);
	glReadPixels(0, m_heightOffset, m_screenWidth, m_screenHeight, GL_RGBA, GL_UNSIGNED_BYTE, pBufferData);
	glReadBuffer(oldMode);
#else
	glReadPixels(0, m_heightOffset, m_screenWidth, m_screenHeight, GL_RGBA, GL_UNSIGNED_BYTE, pBufferData);
#endif

	//Convert RGBA to RGB
	for (s32 y = 0; y < *_height; ++y) {
		u8 *ptr = pBufferData + ((*_width) * 4 * y);
		for (s32 x = 0; x < *_width; ++x) {
			pDest[x * 3] = ptr[0]; // red
			pDest[x * 3 + 1] = ptr[1]; // green
			pDest[x * 3 + 2] = ptr[2]; // blue
			ptr += 4;
		}
		pDest += (*_width) * 3;
	}

	free(pBufferData);
}

graphics::ObjectHandle DisplayWindowMupen64plus::_getDefaultFramebuffer()
{
	return graphics::ObjectHandle::null;
}