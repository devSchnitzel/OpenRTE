// This file is a part of OpenRTE
// Version: 1.0
// Author: TheoryWrong
// Thanks to: CTurt, Zecoxao, wskeu, BadChoicez, wildcard, Z80 and all people contribuing to ps4 scene
// Also thanks to: Dev_Shootz, MsKx, Marbella, JimmyModding, AZN, MarentDev (Modder/Tester)
// This file is under GNU licence

#ifndef KMEMORYH
#define KMEMORYH

#include <sys/sysent.h>

typedef int (*RunnableInt)();
typedef void (*generic_int)(int);
typedef void (*generic)();

struct ReadMemArgs {
    int pid;
    uint64_t offset;
    void *buff;
    int len;
};

struct WriteMemArgs{
	int pid;
	uint64_t offset;
	char *buff;
	int len;
};

struct KlogMemArgs{
	void* buff;
};

int sys_read_process_mem(struct thread *td, void *uap);
int sys_write_process_mem(struct thread *td, void *uap);
int sys_beep(struct thread *td, void *uap);
int sys_redled(struct thread *td, void *uap);
int sys_orangeled(struct thread *td, void *uap);
int sys_blueled(struct thread *td, void *uap);
int sys_whiteled(struct thread *td, void *uap);

#endif