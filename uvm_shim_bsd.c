#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/ioccom.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define NV_ERR_NOT_SUPPORTED 0x56

struct NvUvmInitParams
{
  uint64_t flags __attribute__((aligned(8)));
  uint32_t status;
};

/* Linux legacy raw ioctl codes */
#define NV_UVM_INITIALIZE_RAW   0x30000001
#define NV_UVM_DEINITIALIZE_RAW 0x30000002

/* FreeBSD dynamic BSD-encoded ioctl codes */
#define NV_UVM_INITIALIZE_BSD   _IOWR('0', 1, struct NvUvmInitParams)
#define NV_UVM_DEINITIALIZE_BSD _IO('0', 2)

static inline int is_uvm_initialize(unsigned long request) {
  if (request == NV_UVM_INITIALIZE_RAW || request == NV_UVM_INITIALIZE_BSD)
    return 1;

  unsigned char group = (request >> 8) & 0xFF;
  unsigned char cmd   = request & 0xFF;
  return ((group == '0' || group == 'u' || group == 0x30) && cmd == 1);
}

static inline int is_uvm_deinitialize(unsigned long request) {
  if (request == NV_UVM_DEINITIALIZE_RAW || request == NV_UVM_DEINITIALIZE_BSD)
    return 1;

  unsigned char group = (request >> 8) & 0xFF;
  unsigned char cmd   = request & 0xFF;
  return ((group == '0' || group == 'u' || group == 0x30) && cmd == 2);
}

// ============================================================================
// ioctl Interception
// ============================================================================

int (*libc_ioctl)(int fd, unsigned long request, ...) = NULL;

int ioctl(int fd, unsigned long request, ...) {
  if (!libc_ioctl)
    libc_ioctl = dlsym(RTLD_NEXT, "ioctl");

  va_list _args_;
  va_start(_args_, request);
  void* data = va_arg(_args_, void*);
  va_end(_args_);

  if (is_uvm_initialize(request)) {
    if (data != NULL) {
      struct NvUvmInitParams* params = (struct NvUvmInitParams*)data;
      /* Return not supported to allow safe explicit allocation fallbacks */
      params->status = NV_ERR_NOT_SUPPORTED; 
    }
    return 0;
  }

  if (is_uvm_deinitialize(request)) {
    return 0;
  }

  return libc_ioctl(fd, request, data);
}

// ============================================================================
// Path Tracking Overrides for 590+ Driver Environments
// ============================================================================

static int is_nvidia_uvm(const char* path) {
  return path && strcmp("/dev/nvidia-uvm", path) == 0;
}

static int is_proc_task_comm(const char* path) {
  if (!path) return 0;
  if (strncmp(path, "/proc/self/task/", 16) != 0) return 0;
  char* tail = strchr(path + 16, '/');
  return (tail != NULL && strcmp(tail, "/comm") == 0);
}

// open() interceptor
int (*libc_open)(const char* path, int flags, ...) = NULL;

int open(const char* path, int flags, ...) {
  if (!libc_open) 
    libc_open = dlsym(RTLD_NEXT, "open");

  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list _args_;
    va_start(_args_, flags);
    mode = (mode_t)va_arg(_args_, int);
    va_end(_args_);
  }

  if (is_nvidia_uvm(path) || is_proc_task_comm(path))
    return libc_open("/dev/null", flags, mode);

  return libc_open(path, flags, mode);
}

// openat() interceptor
int (*libc_openat)(int dirfd, const char* path, int flags, ...) = NULL;

int openat(int dirfd, const char* path, int flags, ...) {
  if (!libc_openat) 
    libc_openat = dlsym(RTLD_NEXT, "openat");

  mode_t mode = 0;
  if (flags & O_CREAT) {
    va_list _args_;
    va_start(_args_, flags);
    mode = (mode_t)va_arg(_args_, int);
    va_end(_args_);
  }

  if (is_nvidia_uvm(path) || is_proc_task_comm(path))
    return libc_openat(dirfd, "/dev/null", flags, mode);

  return libc_openat(dirfd, path, flags, mode);
}

// fopen() interceptor
FILE* (*libc_fopen)(const char* path, const char* mode) = NULL;

FILE* fopen(const char* path, const char* mode) {
  if (!libc_fopen)
    libc_fopen = dlsym(RTLD_NEXT, "fopen");

  if (is_nvidia_uvm(path) || is_proc_task_comm(path))
    return libc_fopen("/dev/null", mode);

  return libc_fopen(path, mode);
}

