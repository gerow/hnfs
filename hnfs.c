#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <pthread.h>

#include "hnfs/post.h"
#include "hnfs/decls.h"

static const char *hello_const_str = "Hello World!\n";
static const char *hello_path = "/hello";

static char hello_str[255];

static hnfs_post_collection_t post_collection = {
  .mutex = PTHREAD_MUTEX_INITIALIZER
};

static int hnfs_getattr(const char *path, struct stat *stbuf)
{
  fprintf(stderr, "getattr called for %s\n", path);
  int res = 0;

  hnfs_post_update(&post_collection);

  memset(stbuf, 0, sizeof (struct stat));
  /*if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
  } else if (strcmp(path, hello_path) == 0) {
    stbuf->st_mode = S_IFREG | 0444;
    stbuf->st_nlink = 1;
    stbuf->st_size = strlen(hello_str);
  } else {
    //res = -ENOENT;
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
  }*/

  stbuf->st_mode = S_IFDIR | 0755;
  stbuf->st_nlink = 2;

  return res;
}

static int hnfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi)
{
  (void) offset;
  (void) fi;
  
  fprintf(stderr, "readdir called for %s\n", path);

  if (strcmp(path, "/") == 0) {
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    for (int i = 0; i < HNFS_NUM_POSTS; i++) {
      fprintf(stderr, "adding post %s\n", post_collection.posts[i].title);
      filler(buf, post_collection.posts[i].title, NULL, 0);
    }
    return 0;
  } else {
    return -ENOENT;
  }

  return 0;
}

static int hnfs_open(const char *path, struct fuse_file_info *fi)
{
  fprintf(stderr, "open called for %s\n", path);

  //if (strcmp(path, hello_path) != 0)
  //  return -ENOENT;

  if ((fi->flags & 3) != O_RDONLY)
    return -EACCES;

  return 0;
}

static int hnfs_read(const char *path, char *buf, size_t size, off_t offset,
                      struct fuse_file_info *fi)
{
  fprintf(stderr, "read called for %s\n", path);


  size_t len;
  (void) fi;
  if (strcmp(path, hello_path) != 0)
    return -ENOENT;
  len = strlen(hello_str);
  if (offset < len) {
    if (offset + size > len)
      size = len - offset;
    memcpy(buf, hello_str + offset, size);
  } else {
    size = 0;
  }

  return size;
}

static struct fuse_operations hnfs_oper = {
  .getattr = hnfs_getattr,
  .readdir = hnfs_readdir,
  .open = hnfs_open,
  .read = hnfs_read,
};

int
main(int argc, char **argv)
{
  strcpy(hello_str, hello_const_str);

  /*for (int i = 0; i < HNFS_NUM_POSTS; i++) {
    posts[i].title = "test title";
  }*/
  
  curl_global_init(CURL_GLOBAL_ALL);
  return fuse_main(argc, argv, &hnfs_oper, NULL);
}
