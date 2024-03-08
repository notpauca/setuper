#include <stdio.h>
#include <stdlib.h>
#include <hfsuser.h>
#include <stdbool.h>
#include <stddef.h>
#include <errno.h>
#include <limits.h>
#include <inttypes.h>
#include <fuse.h>
#include <ublio.h>
#include <utf8proc.h>

#ifdef __cplusplus
extern "C" {
#endif

struct hfsfuse_config {
	struct hfs_volume_config volume_config;
	char* device;
	int noallow_other;
	int force;
};

static void hfsfuse_destroy(void* vol); 

struct hf_file {
	hfs_cnid_t cnid;
	hfs_extent_descriptor_t* extents;
	uint16_t nextents;
	uint8_t fork;
	bool is_empty;
};

static int hfsfuse_open(const char* path, struct fuse_file_info* info);

static int hfsfuse_release(const char* path, struct fuse_file_info* info);

static int hfsfuse_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* info); 

static int hfsfuse_readlink(const char* path, char* buf, size_t size); 

static int hfsfuse_getattr(const char* path, struct stat* st); 

static int hfsfuse_fgetattr(const char* path, struct stat* st, struct fuse_file_info* info); 

struct hf_dir {
	hfs_cnid_t cnid;
	hfs_catalog_keyed_record_t* keys;
	hfs_unistr255_t* paths;
	uint32_t npaths;
};

static int hfsfuse_opendir(const char* path, struct fuse_file_info* info); 

static int hfsfuse_releasedir(const char* path, struct fuse_file_info* info); 

static int hfsfuse_readdir2(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* info); 

static int hfsfuse_statfs(const char* path, struct statvfs* st); 

static int hfsfuse_getxtimes(const char* path, struct timespec* bkuptime, struct timespec* crtime); 

#define attrname(name) name

#define declare_attr(name, buf, bufsize, accum) accum += strlen(name)+1; if(bufsize >= (size_t)accum) {strcpy(buf,name); buf += strlen(name)+1;	}

static int hfsfuse_listxattr(const char* path, char* attr, size_t size); 

#define define_attr(attr, name, size, attrsize, block) if(!strcmp(attr, name)) { if(size) { if(size < (size_t)attrsize) return -ERANGE; else block} return attrsize;}

static int hfsfuse_getxattr_darwin(const char* path, const char* attr, char* value, size_t size, u_int32_t unused); 

int MountHFS(char* Filename, char* Mountpoint, char* Mountname); 
#ifdef __cplusplus
}
#endif