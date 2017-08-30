// This file is a part of OpenRTE
// Version: 1.0
// Author: TheoryWrong
// Thanks to: CTurt, Zecoxao, wskeu, BadChoicez, wildcard, Z80 and all people contribuing to ps4 scene
// Also thanks to: Dev_Shootz, MsKx, Marbella, JimmyModding, AZN, MarentDev (Modder/Tester)
// This file is under GNU licence

#include <errno.h>
#define _KERNEL
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/sysctl.h>
#include <sys/socket.h>
#include <string.h>
#include <ps4/socket.h>
#include <ps4/stream.h>
#include <ps4/kernel.h>
#include <sys/syscall.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#include <sys/types.h>
#include <sys/ptrace.h>
#include <sys/mman.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <debugnet.h>
#include <pthread.h>
#include <sce/kernel.h>
#include <sce/types/kernel.h>

#define PTRACE_ATTACH		0x10

#include "server.h"
#include "jailbreak.h"
#include "kmemory.h"
#include "openrte.h"

// Get memory page information from remote process
// From https://github.com/freebsd/freebsd/blob/9e0a154b0fd5fa9010238ac9497ec59f84167c92/lib/libutil/kinfo_getvmmap.c
struct kinfo_vmentry *kinfo_getvmmap(pid_t pid, int *cntp)
{
	int mib[4];
	int error;
	int cnt;
	size_t len;
	char *buf, *bp, *eb;
	struct kinfo_vmentry *kiv, *kp, *kv;

	*cntp = 0;
	len = 0;
	mib[0] = CTL_KERN;
	mib[1] = KERN_PROC;
	mib[2] = KERN_PROC_VMMAP;
	mib[3] = pid;

	error = sysctl(mib, 4, NULL, &len, NULL, 0);
	if (error)
		return (NULL);
	len = len * 4 / 3;
	buf = malloc(len);
	if (buf == NULL)
		return (NULL);
	error = sysctl(mib, 4, buf, &len, NULL, 0);
	if (error) {
		free(buf);
		return (NULL);
	}
	/* Pass 1: count items */
	cnt = 0;
	bp = buf;
	eb = buf + len;
	while (bp < eb) {
		kv = (struct kinfo_vmentry *)(uintptr_t)bp;
		if (kv->kve_structsize == 0)
			break;
		bp += kv->kve_structsize;
		cnt++;
	}

	kiv = calloc(cnt, sizeof(*kiv));
	if (kiv == NULL) {
		free(buf);
		return (NULL);
	}
	bp = buf;
	eb = buf + len;
	kp = kiv;
	/* Pass 2: unpack */
	while (bp < eb) {
		kv = (struct kinfo_vmentry *)(uintptr_t)bp;
		if (kv->kve_structsize == 0)
			break;
		/* Copy/expand into pre-zeroed buffer */
		memcpy(kp, kv, kv->kve_structsize);
		/* Advance to next packed record */
		bp += kv->kve_structsize;
		/* Set field size to fixed length, advance */
		kp->kve_structsize = sizeof(*kp);
		kp++;
	}
	free(buf);
	*cntp = cnt;
	return (kiv);	/* Caller must free() return value */
}

// Get all process
void* get_processes(int* process_nbr) {
	int mib[4];
    size_t len;
    int proc_num = 0;

    char *dump = malloc(PAGE_SIZE);

    void* data = malloc(1);

    int i = 1;
    for (i = 1; i < 100; i += 1) {
        size_t mlen = 4;
        sysctlnametomib("kern.proc.pid", mib, &mlen);
        mib[3] = i;
        len = PAGE_SIZE;

        int ret = sysctl(mib, 4, dump, &len, NULL, 0);
        if(ret != -1 && len > 0) {
        	data = realloc(data, sizeof(Process) * (proc_num + 1));
            struct kinfo_proc *proc = (struct kinfo_proc *)dump;

            Process* proc_c = (Process*)(data + (sizeof(Process) * proc_num));

            strcpy(proc_c->process_name, proc->ki_comm);
            proc_c->process_id = i;

            proc_num++;
        }
    }
	free(dump);

	*process_nbr = proc_num;
	return data;
}

// Get all pages
void* get_pages(int process_id, int* page_nbr) {
	int p_nbr;
	struct kinfo_vmentry *kmi;
	kmi = kinfo_getvmmap(process_id, &p_nbr);

	void* data = malloc(1);
	for (int i = 0; i<p_nbr; i++) {
		data = realloc(data, sizeof(Page) * (i + 1));
		Page* page_c = (Page*)(data + (sizeof(Page) * i));
		page_c->start = kmi[i].kve_start;
		page_c->stop = kmi[i].kve_end;
		page_c->protection = kmi[i].kve_protection;
	}

	free(kmi);
	*page_nbr = p_nbr;
	return data;
}

// Get all module list
void* get_modules(int* module_nbr) {
	SceKernelModule modules[1024];
	size_t moduleCount = 0;

	sceKernelGetModuleList(modules, 1024, &moduleCount);
	debugNetPrintf(DEBUG, "Nbr of module: %i\n", moduleCount);

	Module *moduleInfo = malloc(moduleCount * sizeof(Module));

	SceKernelModuleInfo mInfo;
	for(int i = 0; i < moduleCount; i++) {
		mInfo.size = sizeof(SceKernelModuleInfo);
		sceKernelGetModuleInfo(modules[i], &mInfo);
		debugNetPrintf(DEBUG, "Module name: %s\n", mInfo.name);

		moduleInfo[i].id = modules[i];
		strcpy(moduleInfo[i].name, mInfo.name);

		uint64_t base = (uint64_t)mInfo.segmentInfo[0].address;
		debugNetPrintf(DEBUG, "Module base: %p\n", mInfo.segmentInfo[0].address);

		moduleInfo[i].base = (uint64_t)mInfo.segmentInfo[0].address;
		moduleInfo[i].size = 0;
		for (int y = 0; y<mInfo.segmentCount; y++) {
			moduleInfo[i].size = moduleInfo[i].size + mInfo.segmentInfo[y].size;
		}
	}

	*module_nbr = moduleCount;
	return moduleInfo;
}

// Get module info with this id
void* get_module_by_id(int id) {
	int module_nbr;
	Module* all_module = get_modules(&module_nbr);
	Module* n_module = malloc(sizeof(Module));
	for (int i = 0; i < module_nbr; i++) {
		if (all_module[i].id == id) {
			int module_pad = sizeof(Module) * i;
			memcpy(n_module, (void*)((char*)all_module + module_pad), sizeof(Module));
			break;
		}
	}
	free(all_module);
	return (void*)n_module;
}

// Send an notification system
void* notify(char* message) {
	// Thanks to BadChoiceZ
	char buffer[512];
	sprintf(buffer, "%s\n\n\n\n\n\n\n", message);
	sceSysUtilSendSystemNotificationWithText(36, 0x10000000, buffer);
}

// Read memory of other process
void read_process_form_sys(int pid, uint64_t base, void *buff, int size) {
    int64_t ret;
    char *mem = malloc(sizeof(struct ReadMemArgs));
    memset(mem, 0, sizeof(struct ReadMemArgs));
    struct ReadMemArgs *args = (struct ReadMemArgs *)mem;
    args->pid = pid;
    args->offset = base;
    args->buff = buff;
    args->len = size;

    ps4KernelExecute((void*)sys_read_process_mem, args, &ret, NULL);
    free(mem);
}

// Write memory of other process
void write_process_form_sys(int pid, uint64_t base, void *buff, int size) {
    int64_t ret;
    char *mem = malloc(sizeof(struct WriteMemArgs));
    memset(mem, 0, sizeof(struct WriteMemArgs));
    struct WriteMemArgs *args = (struct WriteMemArgs *)mem;
    args->pid = pid;
    args->offset = base;
    args->buff = buff;
    args->len = size;

	debugNetPrintf(DEBUG, "Write Data: (%i bytes)\n", args->len);
    ps4KernelExecute((void*)sys_write_process_mem, args, &ret, NULL);

    debugNetPrintf(DEBUG, "sys_write_process_mem() -> %08x (%i)\n", ret, ret);
    free(mem);
}

int div_up(int n, int d) {
    return n / d + (((n < 0) ^ (d > 0)) && (n % d));
}

// Get the good error message
void get_error(char* name) {
	switch (errno) {
		case EPERM:
		{
			debugNetPrintf(ERROR, "%s: Permission denied\n", name);
			break;
		}
		case EBUSY:
		{
			debugNetPrintf(ERROR, "%s: Process already traced\n", name);
			break;
		}
		case EINVAL:
		{
			debugNetPrintf(ERROR, "%s: Can't trace itself\n", name);
			break;
		}
		case ESRCH:
		{
			debugNetPrintf(ERROR, "%s: Process not found\n", name);
			break;
		}
		case ECONNRESET:
		{
			debugNetPrintf(ERROR, "%s: ECONNRESET (Connexion reset by peer)\n", name);
			break;
		}
		case EAGAIN:
		{
			debugNetPrintf(ERROR, "%s: EAGAIN (Timeout)\n", name);
			break;
		}
		case ENOTCONN:
		{
			debugNetPrintf(ERROR, "%s: ENOTCONN (Not connected)\n", name);
			break;
		}
		case EBADF:
		{
			debugNetPrintf(ERROR, "%s: EBADF (Invalid descriptor)\n", name);
			break;
		}
		case EINTR:
		{
			debugNetPrintf(ERROR, "%s: EINTR (Accept was interupted)\n", name);
			break;
		}
		case ENFILE:
		{
			debugNetPrintf(ERROR, "%s: ENFILE (Descriptor table is full)\n", name);
			break;
		}
		case ENOTSOCK:
		{
			debugNetPrintf(ERROR, "%s: ENOTSOCK (The descriptor is a file, not a socket)\n", name);
			break;
		}
		default:
		{
			debugNetPrintf(ERROR, "%s: Unknown error ! (0x%08x)\n", name, errno);
			break;
		}
	}
}

// Get action from client and execute this
void execute_action(int client, void* data, int data_size) {
	debugNetPrintf(INFO, "Command: %x\n", ((char*)data)[0]);

	// Find the command
	switch(((char*)data)[0]) {
		// Work
		case COMMAND_GET_PROCESS:
		{
			int proc_num = 0;
			void* proc = get_processes(&proc_num);
			for (int i = 0; i < proc_num; i++) {
				debugNetPrintf(DEBUG, "Proc: %s (%i)\n", ((Process*)proc)[i].process_name, ((Process*)proc)[i].process_id);
			}

			send_all(client, proc, (proc_num * sizeof(Process)));

			free(proc);
			break;
		}

		// Work
		case COMMAND_GET_MEMORY:
		{
			GetMemoryRequest* gmr = (GetMemoryRequest*)(data+0x1);
			debugNetPrintf(DEBUG, "Get Memory Request:\n");
			debugNetPrintf(DEBUG, "Process ID: %i\n", gmr->process_id);
			debugNetPrintf(DEBUG, "Offset: 0x%016llX\n", gmr->offset);
			debugNetPrintf(DEBUG, "Size: %i bytes\n", gmr->size);

			if (gmr->size <= 0) {
				debugNetPrintf(ERROR, "Invalid size !\n");
				break;
			}

			debugNetPrintf(DEBUG, "Calculating size cut ...\n");

			int packet_nbr = div_up(gmr->size, TRANSFERT_RATE);
			int end_size = gmr->size - ((packet_nbr - 1) * TRANSFERT_RATE);

			debugNetPrintf(DEBUG, "Send %i packets of %i bytes (end packet: %i bytes)\n", packet_nbr, TRANSFERT_RATE, end_size);

			int page_nbr;
			Page* pages = get_pages(gmr->process_id, &page_nbr);

			uint64_t offset = pages[0].start + gmr->offset;

			debugNetPrintf(DEBUG, "Process %i have %i page of memory\n", gmr->process_id, page_nbr);
			debugNetPrintf(DEBUG, "Start address: 0x%016llX\n", pages[0].start);

			free(pages);

			// Ont envoi du header
			Header h;
			h.magic = 1337;
			h.size = gmr->size;

			debugNetPrintf(DEBUG, "Envoi du header ...\n");
			debugNetPrintf(DEBUG, "Taille du header: %i bytes\n", sizeof(Header));
			debugNetPrintf(DEBUG, "Taille des donnée: %i bytes\n", h.size);

			send(client, &h, sizeof(Header), 0);

			uint64_t offset_count = offset;
			for (int i = 1; i<(packet_nbr+1); i++) {
				if (packet_nbr == i) {
					debugNetPrintf(DEBUG, "[%i/%i] Read to: 0x%016llX (%i bytes) (ending)\n", i, packet_nbr, offset_count, end_size);

					void* buff = malloc(end_size);
					memset(buff, 0, end_size);
					read_process_form_sys(gmr->process_id, offset_count, buff, end_size);
					debugNetPrintf(DEBUG, "Read complede sending last data ...\n");
					send(client, buff, end_size, 0);
					free(buff);
				} else {
					debugNetPrintf(DEBUG, "[%i/%i] Read to: 0x%016llX (%i bytes)\n", i, packet_nbr, offset_count, 1024);

					void* buff = malloc(TRANSFERT_RATE);
					memset(buff, 0, TRANSFERT_RATE);
					read_process_form_sys(gmr->process_id, offset_count, buff, TRANSFERT_RATE);
					debugNetPrintf(DEBUG, "Read complede, sending data ...\n");
					send(client, buff, TRANSFERT_RATE, 0);
					free(buff);
					offset_count = offset_count + TRANSFERT_RATE;
				}
			}

			debugNetPrintf(DEBUG, "Done !\n");

			break;
		}

		// Work
		case COMMAND_GET_PAGE:
		{
			GetPageRequest* pr = (GetPageRequest*)(data+0x1);
			
			debugNetPrintf(DEBUG, "Get Page Request:\n");
			debugNetPrintf(DEBUG, "Process ID: %i\n", pr->process_id);

			int page_nbr;
			Page* pages = (Page*)get_pages(pr->process_id, &page_nbr);

			int i = 0;
			for (i = 0;i<page_nbr; i++) {
				debugNetPrintf(DEBUG, "[-][%i] [Start: %016llX] [Stop: %016llX] [Size: %016llX]\n", i, pages[i].start, pages[i].stop, (pages[i].stop - pages[i].start));
			}

			debugNetPrintf(DEBUG, "%i pages detected\n", page_nbr);

			send_all(client, pages, (page_nbr * sizeof(Page)));

			free(pages);
			debugNetPrintf(DEBUG, "Done !\n");

			break;
		}

		// Work
		case COMMAND_SET_MEMORY:
		{
			SetMemoryRequest* smr = (SetMemoryRequest*)(data+0x1);
			char* set_data = malloc(smr->size);
			memcpy(set_data, data+0x1+sizeof(SetMemoryRequest), smr->size);

			debugNetPrintf(DEBUG, "Set Memory Request:\n");
			debugNetPrintf(DEBUG, "Process ID: %i\n", smr->process_id);
			debugNetPrintf(DEBUG, "Offset: 0x%016llX\n", smr->offset);
			debugNetPrintf(DEBUG, "Size: %i bytes\n", smr->size);

			debugNetPrintf(DEBUG, "Checking size integrity ...\n");
			if (smr->size <= 0) {
				debugNetPrintf(ERROR, "Invalid size !\n");
				break;
			}


			int page_nbr;
			Page* pages = get_pages(smr->process_id, &page_nbr);

			uint64_t offset = pages[0].start + smr->offset;

			debugNetPrintf(DEBUG, "Process %i have %i page of memory\n", smr->process_id, page_nbr);
			debugNetPrintf(DEBUG, "Start address: 0x%016llX\n", pages[0].start);

			free(pages);

			debugNetPrintf(DEBUG, "Write to: 0x%016llX (%i bytes)\n", offset, smr->size);

			write_process_form_sys(smr->process_id, offset, set_data, smr->size);
			free(set_data);

			send(client, "\x01", 1, 0);
			debugNetPrintf(DEBUG, "Write memory complede !\n");

			break;
		}

		// Work
		case COMMAND_SEND_NOTIFY:
		{
			NotifyRequest* nr = (NotifyRequest*)(data+0x1);
			notify(nr->message);
			debugNetPrintf(DEBUG, "Notification with message %s send !\n", nr->message);
			break;
		}

		// Work
		case COMMAND_EXIT:
		{
			exit(0);
			break;
		}

		// Work
		case COMMAND_LOAD_MODULE:
		{
			ModuleRequest* lr = (ModuleRequest*)(data+0x1);

			debugNetPrintf(INFO, "Load module requested\n");
			debugNetPrintf(INFO, "Module name: %s\n", lr->name);

			int result = sceKernelLoadStartModule(lr->name, 0, NULL, 0, NULL, NULL);

			send_all(client, &result, sizeof(int));
			debugNetPrintf(INFO, "Load complede !\n");
			break;
		}

		// Work
		case COMMAND_DUMP_MODULE:
		{
			ModuleRequest* dr = (ModuleRequest*)(data+0x1);

			debugNetPrintf(INFO, "Dump module requested\n");
			debugNetPrintf(INFO, "Module name: %s\n", dr->name);

			SceKernelModule modules[1024];
			size_t moduleCount = 0;
			
			sceKernelGetModuleList(modules, 1024, &moduleCount);
			debugNetPrintf(INFO, "Nbr of module: %i\n", moduleCount);

			SceKernelModuleInfo mInfo;
			for (int i = 0; i < moduleCount; ++i) {
				mInfo.size = sizeof(SceKernelModuleInfo);
				sceKernelGetModuleInfo(modules[i], &mInfo);
				debugNetPrintf(INFO, "Module name: %s\n", mInfo.name);
				debugNetPrintf(INFO, "Segment Number: %i\n", mInfo.segmentCount);

				int total_size = 0;
				for (int k = 0; k < mInfo.segmentCount; k++) {
					if (k == 0) {
						debugNetPrintf(INFO, "Segment 0 adress: %p (0x%08x)\n", mInfo.segmentInfo[k].address, *(uint64_t*)mInfo.segmentInfo[k].address);
					}
					
					debugNetPrintf(INFO, "Segment %i: %i bytes\n", k, mInfo.segmentInfo[k].size);
					total_size = total_size + mInfo.segmentInfo[k].size;
				}

				debugNetPrintf(INFO, "Module size: %i bytes\n", total_size);

				if (!strcmp(mInfo.name, dr->name)) {
					debugNetPrintf(INFO, "Module detected, dumping ...\n");

					send_all(client, mInfo.segmentInfo[0].address, total_size);
					debugNetPrintf(INFO, "Done !\n");
					break;
				}
			}

			debugNetPrintf(INFO, "Dump complede !\n");
			break;
		}

		// Work
		case COMMAND_GET_IDPS:
		{
			void* idps = malloc(64);
			memset(idps, 0, 64);
			sceKernelGetIdPs(idps);

			/*
			int mib[4];
			int mlen, len;
			sysctlnametomib("machdep.idps", mib, &mlen);

	        int ret = sysctl(mib, 4, idps, &len, NULL, 0);
	        debugNetPrintf(INFO, "IDPS R=%i\n", ret);
	        debugNetPrintf(INFO, "IDPS Len=%i\n", len); // retourne que des 0 :'(
	        */

			send_all(client, idps, 64);
			free(idps);

			debugNetPrintf(INFO, "IDPS Dumped\n");
			break;
		}

		// Work
		case COMMAND_GET_PSID:
		{
			void* psid = malloc(16);
			memset(psid, 0, 16);
			sceKernelGetOpenPsIdForSystem(psid);

			send_all(client, psid, 16);
			free(psid);

			debugNetPrintf(INFO, "PSID Dumped\n");
			break;
		}

		// Work
		case COMMAND_MODULE_INFO:
		{
			debugNetPrintf(INFO, "Get module info requested\n");

			ModuleIDRequest* mir = (ModuleIDRequest*)(data+0x1);

			debugNetPrintf(INFO, "Module id requested: %i\n", mir->id);

			Module* module = get_module_by_id(mir->id);

			debugNetPrintf(INFO, "Module name: %s\n", module->name);
			debugNetPrintf(INFO, "Module base: %08x\n", module->base);
			debugNetPrintf(INFO, "Module size: %i\n", module->size);

			send_all(client, module, sizeof(Module));
			debugNetPrintf(INFO, "Module info sended\n");
			break;
		}

		// Work
		case COMMAND_RESOLVE_FUNC:
		{
			debugNetPrintf(INFO, "Resolving function ...\n");
			ResolveRequest* rr = (ResolveRequest*)(data+0x1);
			void* function_end;
			sceKernelDlsym(rr->id, rr->func_name, &function_end);

			uint64_t function_addr = (uint64_t)function_end;

			debugNetPrintf(INFO, "Module id: %i\n", rr->id);
			debugNetPrintf(INFO, "Function name: %s\n", rr->func_name);
			debugNetPrintf(INFO, "Function address: %p\n", function_end);

			send_all(client, &function_addr, sizeof(uint64_t));
			debugNetPrintf(INFO, "Sended data: %i byte\n", sizeof(uint64_t));
			debugNetPrintf(INFO, "End sended\n");

			debugNetPrintf(INFO, "Function resolved\n");
			break;
		}

		case COMMAND_TEST:
		{
			debugNetPrintf(DEBUG, "TEST: Execution de sceApplicationExitSpawn\n");

			int current_app_id, current_pid;
			current_pid = getpid();

			debugNetPrintf(DEBUG, "Current PID: %i\n", current_pid);
			_sceApplicationGetAppId(current_pid, &current_app_id);
			debugNetPrintf(DEBUG, "Current AppID: %i\n", current_app_id);

			debugNetPrintf(DEBUG, "Executing ...\n");
			int ret = sceApplicationExitSpawn(current_app_id, "/app0/default_mp.elf", NULL, 0);
			debugNetPrintf(DEBUG, "Done ! (0x%x)\n", ret);


			/*
			debugNetPrintf(INFO, "Test: Webkit display manipulation\n");
			
			EGLDisplay display = eglGetCurrentDisplay();
			EGLSurface surface = eglGetCurrentSurface(EGL_READ);

			void* data = malloc(1000);
			memset(data, 0, 1000);

			debugNetPrintf(INFO, "Display (p): %p\n", display);
			debugNetPrintf(INFO, "Display (08x): %08x\n", display);
			debugNetPrintf(INFO, "Surface (p): %p\n", surface);
			debugNetPrintf(INFO, "Surface (08x): %08x\n", surface);

			GLint drawFboId = 0, readFboId = 0;
			glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &drawFboId);
			glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &readFboId);

			debugNetPrintf(INFO, "Framebuffer (DRAW): %i\n", drawFboId);
			debugNetPrintf(INFO, "Framebuffer (READ): %i\n", readFboId);

			debugNetPrintf(INFO, "Getting pixels ...\n");
			glReadPixels(0, 0, 10, 10, GL_RGB, GL_UNSIGNED_BYTE, data);
			debugNetPrintf(INFO, "Pixels saved in buffer\n");

			send(client, data, 1000, 0);
			send(client, "\x0d\x0a", 2, 0);

			while (1) {
				glClearColor(0.0f, 0.0f, 0.3f, 1.0f);
				glClearDepthf(1.0f);
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
				eglSwapBuffers(display, surface);

				debugNetPrintf(INFO, "Screen flip\n");
			}

			debugNetPrintf(INFO, "Buffer sended\n");
			*/

			break;
		}

		case COMMAND_BEEP:
		{
			uint64_t ret;
    		ps4KernelExecute((void*)sys_beep, NULL, &ret, NULL);
    		debugNetPrintf(INFO, "Beep send\n");
    		break;
		}

		case COMMAND_REDLED:
		{
			uint64_t ret;
    		ps4KernelExecute((void*)sys_redled, NULL, &ret, NULL);
    		debugNetPrintf(INFO, "Led set to: RED\n");
    		break;
		}

		case COMMAND_WHITELED:
		{
			uint64_t ret;
    		ps4KernelExecute((void*)sys_whiteled, NULL, &ret, NULL);
    		debugNetPrintf(INFO, "Led set to: WHITE\n");
    		break;
		}

		case COMMAND_BLUELED:
		{
			uint64_t ret;
    		ps4KernelExecute((void*)sys_blueled, NULL, &ret, NULL);
    		debugNetPrintf(INFO, "Led set to: BLUE\n");
    		break;
		}

		case COMMAND_ORANGELED:
		{
			uint64_t ret;
    		ps4KernelExecute((void*)sys_orangeled, NULL, &ret, NULL);
    		debugNetPrintf(INFO, "Led set to: ORANGE\n");
    		break;
		}
		
		default:
		{
			debugNetPrintf(INFO, "This command doesn't exist\n");
			break;
		}
	}

	free(data);
}

// Load needed module
void loadModule() {
	debugNetPrintf(INFO, "Load needed module ...\n");
	
	sysutil_module = sceKernelLoadStartModule("libSceSysUtil.sprx", 0, NULL, 0, NULL, NULL);
	screenshot_module = sceKernelLoadStartModule("libSceScreenShot.sprx", 0, NULL, 0, NULL, NULL);
	pad_module = sceKernelLoadStartModule("libScePad.sprx", 0, NULL, 0, NULL, NULL);
	kernel_module = sceKernelLoadStartModule("libkernel.sprx", 0, NULL, 0, NULL, NULL);
	vdec_module = sceKernelLoadStartModule("libSceVdecCore.sprx", 0, NULL, 0, NULL, NULL);
	syscore_module = sceKernelLoadStartModule("libSceSysCore.sprx", 0, NULL, 0, NULL, NULL);
	pigletv_module = sceKernelLoadStartModule("libScePigletv2VSH.sprx", 0, NULL, 0, NULL, NULL);

	debugNetPrintf(INFO, "libSceSysUtil.sprx: %i\n", sysutil_module);
	debugNetPrintf(INFO, "libSceScreenShot.sprx: %i\n", screenshot_module);
	debugNetPrintf(INFO, "libScePad.sprx: %i\n", pad_module);
	debugNetPrintf(INFO, "libkernel.sprx: %i\n", kernel_module);
	debugNetPrintf(INFO, "libSceVdecCore.sprx: %i\n", vdec_module);
	debugNetPrintf(INFO, "libSceSysCore.sprx: %i\n", syscore_module);
	debugNetPrintf(INFO, "libScePigletv2VSH.sprx: %i\n", pigletv_module);
}

// Resolve all needed function
void resolveFunction() {
	debugNetPrintf(INFO, "Resolving function ...\n");

	sceKernelDlsym(sysutil_module, "sceSysUtilSendSystemNotificationWithText", &sceSysUtilSendSystemNotificationWithText);
	
	sceKernelDlsym(screenshot_module, "sceScreenShotCapture", &sceScreenShotCapture);
	sceKernelDlsym(screenshot_module, "sceScreenShotDisable", &sceScreenShotDisable);

	sceKernelDlsym(pad_module, "scePadInit", &scePadInit);
	sceKernelDlsym(pad_module, "scePadOpen", &scePadOpen);
	sceKernelDlsym(pad_module, "scePadClose", &scePadClose);
	sceKernelDlsym(pad_module, "scePadSetLightBar", &scePadSetLightBar);
	sceKernelDlsym(pad_module, "scePadGetControllerInformation", &scePadGetControllerInformation);
	sceKernelDlsym(pad_module, "scePadGetDeviceInfo", &scePadGetDeviceInfo);
	sceKernelDlsym(pad_module, "scePadSetLightBarBaseBrightness", &scePadSetLightBarBaseBrightness);
	sceKernelDlsym(pad_module, "scePadSetLightBarBlinking", &scePadSetLightBarBlinking);
	sceKernelDlsym(pad_module, "scePadSetVibration", &scePadSetVibration);

	sceKernelDlsym(kernel_module, "sceKernelGetIdPs", &sceKernelGetIdPs);
	sceKernelDlsym(kernel_module, "sceKernelGetOpenPsIdForSystem", &sceKernelGetOpenPsIdForSystem);
	sceKernelDlsym(kernel_module, "sceKernelReboot", &sceKernelReboot);
	sceKernelDlsym(kernel_module, "sceKernelGetAppInfo", &sceKernelGetAppInfo);

	sceKernelDlsym(vdec_module, "sceVdecCoreMapMemoryBlock", &sceVdecCoreMapMemoryBlock);
	sceKernelDlsym(vdec_module, "sceVdecCoreGetDecodeOutput", &sceVdecCoreGetDecodeOutput);
	sceKernelDlsym(vdec_module, "sceVdecCoreQueryFrameBufferInfo", &sceVdecCoreQueryFrameBufferInfo);

	sceKernelDlsym(syscore_module, "sceApplicationSystemReboot", &sceApplicationSystemReboot);
	sceKernelDlsym(syscore_module, "sceApplicationSystemShutdown2", &sceApplicationSystemShutdown2);
	sceKernelDlsym(syscore_module, "sceApplicationSystemSuspend", &sceApplicationSystemSuspend);
	sceKernelDlsym(syscore_module, "_sceApplicationGetAppId", &_sceApplicationGetAppId);
	sceKernelDlsym(syscore_module, "sceApplicationSetApplicationFocus", &sceApplicationSetApplicationFocus);
	sceKernelDlsym(syscore_module, "sceApplicationSetControllerFocus", &sceApplicationSetControllerFocus);
	sceKernelDlsym(syscore_module, "sceApplicationSuspend", &sceApplicationSuspend);
	sceKernelDlsym(syscore_module, "sceApplicationExitSpawn", &sceApplicationExitSpawn);

	sceKernelDlsym(pigletv_module, "eglGetCurrentDisplay", &eglGetCurrentDisplay);
	sceKernelDlsym(pigletv_module, "eglGetCurrentSurface", &eglGetCurrentSurface);
	sceKernelDlsym(pigletv_module, "glReadPixels", &glReadPixels);
	sceKernelDlsym(pigletv_module, "glGetIntegerv", &glGetIntegerv);
	sceKernelDlsym(pigletv_module, "glDrawArrays", &glDrawArrays);
	sceKernelDlsym(pigletv_module, "eglSwapBuffers", &eglSwapBuffers);
	sceKernelDlsym(pigletv_module, "glClearColor", &glClearColor);
	sceKernelDlsym(pigletv_module, "glClearDepthf", &glClearDepthf);
	sceKernelDlsym(pigletv_module, "glClear", &glClear);
	sceKernelDlsym(pigletv_module, "glBindFramebuffer", &glBindFramebuffer);
	sceKernelDlsym(pigletv_module, "eglGetDisplay", &eglGetDisplay);

	debugNetPrintf(INFO, "sceSysUtilSendSystemNotificationWithText: %p\n", sceSysUtilSendSystemNotificationWithText);
	debugNetPrintf(INFO, "sceScreenShotCapture: %p\n", sceScreenShotCapture);
	debugNetPrintf(INFO, "sceScreenShotDisable: %p\n", sceScreenShotDisable);
	debugNetPrintf(INFO, "scePadInit: %p\n", scePadInit);
	debugNetPrintf(INFO, "scePadOpen: %p\n", scePadOpen);
	debugNetPrintf(INFO, "scePadClose: %p\n", scePadClose);
	debugNetPrintf(INFO, "scePadSetLightBar: %p\n", scePadSetLightBar);
	debugNetPrintf(INFO, "scePadGetControllerInformation: %p\n", scePadGetControllerInformation);
	debugNetPrintf(INFO, "scePadGetDeviceInfo: %p\n", scePadGetDeviceInfo);
	debugNetPrintf(INFO, "scePadSetLightBarBaseBrightness: %p\n", scePadSetLightBarBaseBrightness);
	debugNetPrintf(INFO, "scePadSetLightBarBlinking: %p\n", scePadSetLightBarBlinking);
	debugNetPrintf(INFO, "scePadSetVibration: %p\n", scePadSetVibration);
	debugNetPrintf(INFO, "sceKernelGetIdPs: %p\n", sceKernelGetIdPs);
	debugNetPrintf(INFO, "sceKernelGetOpenPsIdForSystem: %p\n", sceKernelGetOpenPsIdForSystem);
	debugNetPrintf(INFO, "sceKernelReboot: %p\n", sceKernelReboot);
	debugNetPrintf(INFO, "sceKernelGetAppInfo: %p\n", sceKernelGetAppInfo);
	debugNetPrintf(INFO, "sceVdecCoreMapMemoryBlock: %p\n", sceVdecCoreMapMemoryBlock);
	debugNetPrintf(INFO, "sceVdecCoreGetDecodeOutput: %p\n", sceVdecCoreGetDecodeOutput);
	debugNetPrintf(INFO, "sceVdecCoreQueryFrameBufferInfo: %p\n", sceVdecCoreQueryFrameBufferInfo);
	debugNetPrintf(INFO, "sceApplicationSystemReboot: %p\n", sceApplicationSystemReboot);
	debugNetPrintf(INFO, "sceApplicationSystemShutdown2: %p\n", sceApplicationSystemShutdown2);
	debugNetPrintf(INFO, "sceApplicationSystemSuspend: %p\n", sceApplicationSystemSuspend);
	debugNetPrintf(INFO, "eglGetCurrentDisplay: %p\n", eglGetCurrentDisplay);
	debugNetPrintf(INFO, "eglGetCurrentSurface: %p\n", eglGetCurrentSurface);
	debugNetPrintf(INFO, "glReadPixels: %p\n", glReadPixels);
	debugNetPrintf(INFO, "glGetIntegerv: %p\n", glGetIntegerv);
	debugNetPrintf(INFO, "glDrawArrays: %p\n", glDrawArrays);
	debugNetPrintf(INFO, "glClearColor: %p\n", glClearColor);
	debugNetPrintf(INFO, "glClearDepthf: %p\n", glClearDepthf);
	debugNetPrintf(INFO, "glClear: %p\n", glClear);
	debugNetPrintf(INFO, "eglSwapBuffers: %p\n", eglSwapBuffers);
	debugNetPrintf(INFO, "glBindFramebuffer: %p\n", glBindFramebuffer);
	debugNetPrintf(INFO, "eglGetDisplay: %p\n", eglGetDisplay);
}

// Main function
int main(int argc, char **argv) {
	// Initialize debugnet
	debugNetInit(DEBUG_IP, DEBUG_PORT, DEBUG);
	debugNetPrintf(INFO, "OpenRTE - By TheoryWrong\n");

	// Check Jailbreak status
	if (getuid() != 0) {
		debugNetPrintf(INFO, "Jailbreak in progress ...\n");
		int64_t ret;
		ps4KernelExecute((void*)jailbreak, NULL, &ret, NULL);
		debugNetPrintf(INFO, "Jailbreaked !\n");
		debugNetPrintf(INFO, "Code loaded !\n");
	}
	ps4KernelCall(ps4KernelUartEnable);

	// Load needed module
	loadModule();

	// Resolve needed function
	resolveFunction();

	debugNetPrintf(INFO, "Load the OpenRTE main server ...\n");

	// Load the RTE Server
	pthread_t thread;
	pthread_create(&thread, NULL, server_t, NULL);
	pthread_join(thread, NULL);

	return EXIT_SUCCESS;
}
