#include "HFSOperations.h"

static struct fuse_operations hfsfuse_ops = {
	.destroy     = hfsfuse_destroy,
	.open        = hfsfuse_open,
	.opendir     = hfsfuse_opendir,
	.read        = hfsfuse_read,
	.readdir     = hfsfuse_readdir2,
	.release     = hfsfuse_release,
	.releasedir  = hfsfuse_releasedir,
	.statfs      = hfsfuse_statfs,
	.getattr     = hfsfuse_getattr,
	.readlink    = hfsfuse_readlink,
	.fgetattr    = hfsfuse_fgetattr,
	.listxattr   = hfsfuse_listxattr,
	.getxattr    = hfsfuse_getxattr_darwin,
	.getxtimes   = hfsfuse_getxtimes, 
#if FUSE_VERSION >= 29
	.flag_nopath = 1, 
#endif
#if FUSE_VERSION >= 28
	.flag_nullpath_ok = 1
#endif
};

static void hfsfuse_destroy(void* vol) {
	hfslib_close_volume((hfs_volume*)vol, NULL);
}

static int hfsfuse_open(const char* path, struct fuse_file_info* info) {
	hfs_volume* vol = (hfs_volume*)fuse_get_context()->private_data;
	hfs_catalog_keyed_record_t rec; hfs_catalog_key_t key; uint8_t fork;
	int ret = hfs_lookup(vol,path,&rec,&key,&fork);
	if(ret) {return ret;}

	struct hf_file* f = malloc(sizeof(*f));
	if(!f) {return -ENOMEM;}

	f->cnid = rec.file.cnid;
	f->fork = fork;
	f->is_empty = (fork == HFS_RSRCFORK ? rec.file.rsrc_fork : rec.file.data_fork).logical_size == 0;
	f->nextents = hfslib_get_file_extents(vol,f->cnid,fork,&f->extents,NULL);
	info->fh = (uint64_t)f;
	info->keep_cache = 1;
	return 0;
}

static int hfsfuse_release(const char* path, struct fuse_file_info* info) {
	struct hf_file* f = (struct hf_file*)info->fh;
	free(f->extents);
	free(f);
	return 0;
}

static int hfsfuse_read(const char* path, char* buf, size_t size, off_t offset, struct fuse_file_info* info) {
	hfs_volume* vol = (hfs_volume*)fuse_get_context()->private_data;
	struct hf_file* f = (struct hf_file*)info->fh;
	uint64_t bytes;
	if(f->is_empty)  // empty files have no extents, which to hfslib_readd_with_extents() is an error
		return 0;  // so skip it and just return 0 for EOF
	int ret = hfslib_readd_with_extents(vol,buf,&bytes,size,offset,f->extents,f->nextents,NULL);
	if(ret < 0)
		return ret;
	return bytes;
}

static int hfsfuse_readlink(const char* path, char* buf, size_t size) {
	struct fuse_file_info info;
	if(hfsfuse_open(path,&info)) return -errno;
	int bytes = hfsfuse_read(path,buf,size,0,&info);
	hfsfuse_release(NULL,&info);
	if(bytes < 0) return -errno;
	return 0;
}

static int hfsfuse_getattr(const char* path, struct stat* st) {
	hfs_volume* vol = (hfs_volume*)fuse_get_context()->private_data;
	hfs_catalog_keyed_record_t rec; hfs_catalog_key_t key; uint8_t fork;
	int ret = hfs_lookup(vol,path,&rec,&key,&fork);
	if(ret) {return ret;}
	hfs_stat(vol, &rec,st,fork);
	return 0;
}

static int hfsfuse_fgetattr(const char* path, struct stat* st, struct fuse_file_info* info) {
	hfs_volume* vol = (hfs_volume*)fuse_get_context()->private_data;
	struct hf_file* f = (struct hf_file*)info->fh;
	hfs_catalog_keyed_record_t rec; hfs_catalog_key_t key;
	int ret = hfslib_find_catalog_record_with_cnid(vol,f->cnid,&rec,&key,NULL);
	if(ret < 0) return ret;
	else if(ret > 0) return -ENOENT;
	hfs_stat(vol,&rec,st,f->fork);
	return 0;
}

static int hfsfuse_opendir(const char* path, struct fuse_file_info* info) {
	hfs_volume* vol = (hfs_volume*)fuse_get_context()->private_data;
	hfs_catalog_keyed_record_t rec; hfs_catalog_key_t key;
	int ret = hfs_lookup(vol,path,&rec,&key,NULL);
	if(ret) {return ret;}

	struct hf_dir* d = malloc(sizeof(*d));
	if(!d) {return -ENOMEM;}

	d->cnid = rec.folder.cnid;
	hfslib_get_directory_contents(vol,d->cnid,&d->keys,&d->paths,&d->npaths,NULL);

	hfs_catalog_keyed_record_t link;
	for(hfs_catalog_keyed_record_t* record = d->keys; record != d->keys + d->npaths; record++)
		if(record->type == HFS_REC_FILE && (
		  (record->file.user_info.file_creator == HFS_HFSPLUS_CREATOR &&
		   record->file.user_info.file_type    == HFS_HARD_LINK_FILE_TYPE &&
		   !hfslib_get_hardlink(vol, record->file.bsd.special.inode_num, &link, NULL)) ||
		  (record->file.user_info.file_creator == HFS_MACS_CREATOR &&
		   record->file.user_info.file_type    == HFS_DIR_HARD_LINK_FILE_TYPE &&
		   !hfslib_get_directory_hardlink(vol, record->file.bsd.special.inode_num, &link, NULL))))
			*record = link;

	info->fh = (uint64_t)d;
	return 0;
}

static int hfsfuse_releasedir(const char* path, struct fuse_file_info* info) {
	struct hf_dir* d = (struct hf_dir*)info->fh;
	free(d->paths);
	free(d->keys);
	free(d);
	return 0;
}

// FUSE expects readder to be implemented in one of two ways
// This is the 'alternative' implementation that supports the offset param
static int hfsfuse_readdir2(const char* path, void* buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info* info) {
	hfs_volume* vol = (hfs_volume*)fuse_get_context()->private_data;
	struct hf_dir* d = (struct hf_dir*)info->fh;
	hfs_catalog_keyed_record_t rec; hfs_catalog_key_t key; struct stat st;
	if(offset < 2) {
		hfslib_find_catalog_record_with_cnid(vol, d->cnid, &rec, &key, NULL);
		if(offset < 1) {
			hfs_stat(vol, &rec, &st, 0);
			if(filler(buf, ".", &st, 1))
				return 0;
		}

		struct stat* stp = NULL;
		if(d->cnid != HFS_CNID_ROOT_FOLDER) {
			stp = &st;
			hfslib_find_catalog_record_with_cnid(vol, key.parent_cnid, &rec, &key, NULL);
			hfs_stat(vol, &rec, stp, 0);
		}
		if(filler(buf, "..", stp, 2))
			return 0;
	}
	char pelem[HFS_NAME_MAX+1];
	int ret = 0;
	for(off_t i = max(0,offset-2); i < d->npaths; i++) {
		int err;
		if((err = hfs_pathname_to_unix(d->paths+i,pelem)) < 0) {
			ret = err;
			continue;
		}
		hfs_stat(vol,d->keys+i,&st,0);
		if(filler(buf,pelem,&st,i+3))
			break;
	}
	return min(ret,0);
}


static int hfsfuse_statfs(const char* path, struct statvfs* st) {
	hfs_volume* vol = (hfs_volume*)fuse_get_context()->private_data;
	st->f_bsize = vol->vh.block_size;
	st->f_frsize = st->f_bsize;
	st->f_blocks = vol->vh.total_blocks;
	st->f_bfree = vol->vh.free_blocks;
	st->f_bavail = st->f_bfree;
	st->f_files = UINT32_MAX - HFS_CNID_USER;
	st->f_ffree = st->f_files - vol->vh.file_count - vol->vh.folder_count;
	st->f_favail = st->f_ffree;
	st->f_flag = ST_RDONLY;
	st->f_namemax = HFS_NAME_MAX;
	return 0;
}

static int hfsfuse_getxtimes(const char* path, struct timespec* bkuptime, struct timespec* crtime) {
	hfs_volume* vol = (hfs_volume*)fuse_get_context()->private_data;
	hfs_catalog_keyed_record_t rec; hfs_catalog_key_t key; uint8_t fork; 
	int ret = hfs_lookup(vol,path,&rec,&key,&fork);
	if(ret) {return ret; }
	bkuptime->tv_sec = rec.file.date_backedup;
	bkuptime->tv_nsec = 0;
	crtime->tv_sec = rec.file.date_created;
	crtime->tv_nsec = 0;
	return 0;
}

static int hfsfuse_listxattr(const char* path, char* attr, size_t size) {
	hfs_volume* vol = (hfs_volume*)fuse_get_context()->private_data;
	hfs_catalog_keyed_record_t rec; hfs_catalog_key_t key; unsigned char fork;
	int ret = hfs_lookup(vol,path,&rec,&key,&fork);
	if(ret)
		return ret;

	declare_attr("hfsfuse.record.date_created", attr, size, ret);
	if(rec.file.date_backedup)
		declare_attr("hfsfuse.record.date_backedup", attr, size, ret);

	if(rec.type == HFS_REC_FILE && rec.file.rsrc_fork.logical_size && rec.file.rsrc_fork.logical_size <= INT_MAX)
		declare_attr("com.apple.ResourceFork", attr, size, ret);

	char finderinfo[32];
	hfs_serialize_finderinfo(&rec,finderinfo);
	if(memcmp(finderinfo,(char[32]){0},32))
		declare_attr("com.apple.FinderInfo", attr, size, ret);

	return ret;
}

static int hfsfuse_getxattr(const char* path, const char* attr, char* value, size_t size) {
	hfs_volume* vol = (hfs_volume*)fuse_get_context()->private_data;
	hfs_catalog_keyed_record_t rec; hfs_catalog_key_t key;
	int ret = hfs_lookup(vol,path,&rec,&key,NULL);
	if(ret)
		return ret;

	define_attr(attr, "com.apple.FinderInfo", size, 32, {
		hfs_serialize_finderinfo(&rec, value);
	});
	if(rec.type == HFS_REC_FILE && rec.file.rsrc_fork.logical_size && rec.file.rsrc_fork.logical_size <= INT_MAX) {
		ret = rec.file.rsrc_fork.logical_size;
		define_attr(attr, "com.apple.ResourceFork", size, ret, {
			hfs_extent_descriptor_t* extents = NULL;
			uint64_t bytes;
			if(size > (size_t)ret)
				size = ret;
			uint16_t nextents = hfslib_get_file_extents(vol,rec.file.cnid,HFS_RSRCFORK,&extents,NULL);
			if((ret = hfslib_readd_with_extents(vol,value,&bytes,size,0,extents,nextents,NULL)) >= 0)
				ret = bytes;
			else ret = -EIO;
			free(extents);
		});
	}

	// define_attr(attr, "hfsfuse.record.date_created", size, 24, {
	// 	// some strftime implementations require room for the null terminator
	// 	// but we don't want this in the returned attribute value
	// 	char timebuf[25];
	// 	struct tm t;
	// 	localtime_r(&(time_t){HFSTIMETOEPOCH(rec.file.date_created)}, &t);
	// 	strftime(timebuf, 25, "%FT%T%z", &t);
	// 	memcpy(value, timebuf, 24);
	// });

	// define_attr(attr, "hfsfuse.record.date_backedup", size, 24, {
	// 	char timebuf[25];
	// 	struct tm t;
	// 	localtime_r(&(time_t){HFSTIMETOEPOCH(rec.file.date_backedup)}, &t);
	// 	strftime(timebuf, 25, "%FT%T%z", &t);
	// 	memcpy(value, timebuf, 24);
	// });

	return -ENOATTR;
}

static int hfsfuse_getxattr_darwin(const char* path, const char* attr, char* value, size_t size, u_int32_t unused) {
	return hfsfuse_getxattr(path, attr, value, size);
}


int MountHFS(char* Filename, char* Mountpoint) {
	char* arguments[] = {Filename, Mountpoint}; 
	struct fuse_args args = FUSE_ARGS_INIT(3, arguments); 

    struct hfsfuse_config cfg = {0};
	hfs_volume_config_defaults(&cfg.volume_config);

    if (fuse_opt_parse(&args, &cfg, NULL, NULL) != -1 || cfg.volume_config.rsrc_suff && strchr(cfg.volume_config.rsrc_suff,'/')) {return 1; }
    char* fsname = malloc(8 + strlen(cfg.device));
	strcpy(fsname, "fsname=");
	strcat(fsname, cfg.device);

    char* opts = NULL;
	fuse_opt_add_opt(&opts, "ro");
	if(!cfg.noallow_other)
		fuse_opt_add_opt(&opts, "allow_other");
	fuse_opt_add_opt(&opts, "use_ino");
	fuse_opt_add_opt(&opts, "subtype=hfs");
	fuse_opt_add_opt_escaped(&opts, fsname);
	fuse_opt_add_arg(&args, "-o");
	fuse_opt_add_arg(&args, opts);
	fuse_opt_add_arg(&args, "-s");
	free(fsname); 

	hfslib_init(&(hfs_callbacks){hfs_vprintf, hfs_malloc, hfs_realloc, hfs_free, hfs_open, hfs_close, hfs_read});
	
    hfs_volume vol; 
    if (hfslib_open_volume(cfg.device, 1, &vol, &(hfs_callback_args){ .openvol = &cfg.volume_config })) {return 1; }
    fuse_main(args.argc, args.argv, &hfsfuse_ops, &vol);
}
