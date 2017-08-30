// This file is a part of OpenRTE
// Version: 1.0
// Author: TheoryWrong
// Thanks to: CTurt, Zecoxao, wskeu, BadChoicez, wildcard, Z80 and all people contribuing to ps4 scene
// Also thanks to: Dev_Shootz, MsKx, Marbella, JimmyModding, AZN, MarentDev (Modder/Tester)
// This file is under GNU licence

#ifndef OPENRTE_H
#define OPENRTE_H

#define CTL_KERN 1
#define KERN_PROC 14
#define KERN_PROC_PID 1

#define DEBUG_PORT 15000 // Debug port
#define DEBUG_IP "192.168.1.2" // IP of your Computer here (socat udp-recv:15000 stdout) for debug

enum Command
{
    COMMAND_GET_PROCESS = 0x01,
    COMMAND_GET_PAGE = 0x02,
    COMMAND_GET_MEMORY = 0x03,
    COMMAND_SET_MEMORY = 0x04,
    COMMAND_EXIT = 0x05,
    COMMAND_SEND_NOTIFY = 0x06,
    COMMAND_LOAD_MODULE = 0x07,
    COMMAND_DUMP_MODULE = 0x08,
    COMMAND_GET_IDPS = 0x09,
    COMMAND_GET_PSID = 0x10,
    COMMAND_SHUTDOWN = 0x11,
    COMMAND_REBOOT = 0x12,
    COMMAND_SUSPEND = 0x13,
    COMMAND_MODULE_INFO = 0x14,
    COMMAND_RESOLVE_FUNC = 0x15,
    COMMAND_TEST = 0x16,
    COMMAND_BEEP = 0x17,
    COMMAND_REDLED = 0x18,
    COMMAND_WHITELED = 0x19,
    COMMAND_ORANGELED = 0x20,
    COMMAND_BLUELED = 0x21
};

typedef struct {
	int process_id;
	char process_name[50];
} Process;

typedef struct {
	uint64_t start;
	uint64_t stop;
	int protection;
} Page;

typedef struct {
	uint32_t id;
	char name[256];
	uint64_t base;
	int size;
} Module;

typedef struct {
	int process_id;
	uint64_t offset;
	int size;
} GetMemoryRequest;

typedef struct {
	int process_id;
	uint64_t offset;
	int size;
	char data[0];
} SetMemoryRequest;

typedef struct {
	int process_id;
} GetPageRequest;

typedef struct {
	char name[256];
} ModuleRequest;

typedef struct {
	char message[256];
} NotifyRequest;

typedef struct {
	int id;
} ModuleIDRequest;

typedef struct {
	int id;
	char func_name[256];
} ResolveRequest;

typedef int EGLBoolean;
typedef int32_t EGLint;
typedef void *EGLDisplay;
typedef void *EGLConfig;
typedef void *EGLSurface;
typedef void *EGLContext;
typedef int GLsizei;
typedef unsigned int GLenum;
typedef int GLint;
typedef void GLvoid;
typedef float GLclampf;
typedef unsigned int GLbitfield;
typedef unsigned int GLuint;

typedef void *NativeDisplayType;
#define EGL_DEFAULT_DISPLAY ((NativeDisplayType)0)

#define EGL_READ                          0x305A
#define EGL_DRAW                          0x3059

/* PixelFormat */
#define GL_ALPHA                          0x1906
#define GL_RGB                            0x1907
#define GL_RGBA                           0x1908
#define GL_LUMINANCE                      0x1909
#define GL_LUMINANCE_ALPHA                0x190A

/* PixelType */
/*      GL_UNSIGNED_BYTE */
#define GL_UNSIGNED_SHORT_4_4_4_4         0x8033
#define GL_UNSIGNED_SHORT_5_5_5_1         0x8034
#define GL_UNSIGNED_SHORT_5_6_5           0x8363

/* DataType */
#define GL_BYTE                           0x1400
#define GL_UNSIGNED_BYTE                  0x1401
#define GL_SHORT                          0x1402
#define GL_UNSIGNED_SHORT                 0x1403
#define GL_FLOAT                          0x1406
#define GL_FIXED                          0x140C

#define GL_FRAMEBUFFER_BINDING            0x8CA6
#define GL_DRAW_FRAMEBUFFER_BINDING       0x8CA6
#define GL_RENDERBUFFER_BINDING           0x8CA7
#define GL_READ_FRAMEBUFFER               0x8CA8
#define GL_DRAW_FRAMEBUFFER               0x8CA9
#define GL_READ_FRAMEBUFFER_BINDING       0x8CAA

#define GL_TRIANGLES                      0x0004

/* ClearBufferMask */
#define GL_DEPTH_BUFFER_BIT               0x00000100
#define GL_STENCIL_BUFFER_BIT             0x00000400
#define GL_COLOR_BUFFER_BIT               0x00004000

#define GL_FRAMEBUFFER                    0x8D40

int (*sceSysUtilSendSystemNotificationWithText)(int messageType, int userID, char* message);

int (*sceScreenShotCapture)(void);
int (*sceScreenShotDisable)(void);

int (*scePadInit)(void);
int (*scePadOpen)(int userID, int, int, void *);
int (*scePadClose)(int handle);
int (*scePadSetLightBar)(int handle, void *color);

int (*scePadGetControllerInformation)(void);
int (*scePadGetDeviceInfo)(void);
int (*scePadSetLightBarBlinking)(int handle, void* unknown_struct);
int (*scePadSetVibration)(int handle, void* vibration_struct);
int (*scePadSetLightBarBaseBrightness)(int handle, int intensity);

int (*sceKernelGetIdPs)(void* ret);
int (*sceKernelGetOpenPsIdForSystem)(void* ret);
int (*sceKernelReboot)(void);
int (*sceKernelGetAppInfo)(void);

int (*sceVdecCoreMapMemoryBlock)(void);
int (*sceVdecCoreGetDecodeOutput)(void);
int (*sceVdecCoreQueryFrameBufferInfo)(void* param, void* returned_data);

int (*sceApplicationSystemReboot)(void);
int (*sceApplicationSystemShutdown2)(void);
int (*sceApplicationSystemSuspend)(void);
int (*_sceApplicationGetAppId)(int pid, int* app_id);
int (*sceApplicationSetControllerFocus)(int app_id);
int (*sceApplicationSetApplicationFocus)(int app_id);
int (*sceApplicationSuspend)(int app_id);
int (*sceApplicationExitSpawn)(int app_id, char* path, void* args, int lenghtArgs);

EGLDisplay (*eglGetCurrentDisplay)(void);
EGLSurface (*eglGetCurrentSurface)(EGLint readdraw);
void (*glReadPixels)(GLint x, GLint y, GLsizei width, GLsizei height, GLenum format, GLenum type, GLvoid * data);
void (*glGetIntegerv)(GLenum pname, GLint * data);
void (*glDrawArrays)(GLenum mode, GLint first, GLsizei count);
void (*glClearColor)(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha);
void (*glClearDepthf)(GLclampf depth);
void (*glClear)(GLbitfield mask);
EGLBoolean (*eglSwapBuffers)(EGLDisplay display, EGLSurface surface);
void (*glBindFramebuffer)(GLenum target, GLuint framebuffer);
EGLDisplay (*eglGetDisplay)(NativeDisplayType native_display);

int sysutil_module, syscore_module, screenshot_module, pad_module, kernel_module, vdec_module, pigletv_module;

void execute_action(int client, void* data, int data_size);

#endif