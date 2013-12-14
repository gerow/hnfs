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

static hnfs_post_collection_t post_collection = {
  .mutex = PTHREAD_MUTEX_INITIALIZER
};

static int hnfs_getattr(const char *path, struct stat *stbuf)
{
  fprintf(stderr, "getattr called for %s\n", path);
  int res = 0;
  int found_post_entry = 0;

  hnfs_post_update(&post_collection);

  memset(stbuf, 0, sizeof (struct stat));
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
  } else {
    /* hacky, fix... */
    /* TODO: */
    path++;
    pthread_mutex_lock(&post_collection.mutex);
    for (int i = 0; i < HNFS_NUM_POSTS; i++) {
      if (strcmp(path, post_collection.posts[i].title) == 0) {
        found_post_entry = 1;
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        break;
      }
    }
    pthread_mutex_unlock(&post_collection.mutex);
    if (!found_post_entry) {
      return -ENOENT;
    }
  }

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
    hnfs_post_update(&post_collection);
    pthread_mutex_lock(&post_collection.mutex);
    for (int i = 0; i < HNFS_NUM_POSTS; i++) {
      fprintf(stderr, "adding post %s\n", post_collection.posts[i].title);
      filler(buf, post_collection.posts[i].title, NULL, 0);
    }
    pthread_mutex_unlock(&post_collection.mutex);
    return 0;
  } else {
    /* otherwise it's probably trying to find entries for a specific post */
    int chr_loc = 0;
    /* it should be 0 since the first char in a path is / */
    if (chr_loc != 0) {
      return -ENOENT;
    }
    /* get a new pointer to the part of the string after the slash */
    const char *dirname = path + chr_loc + 1;
    fprintf(stderr, "trying to find %s\n", dirname);
    pthread_mutex_lock(&post_collection.mutex);
    int found_post_entry = 0;
    for (int i = 0; i < HNFS_NUM_POSTS; i++) {
      if (strcmp(dirname, post_collection.posts[i].title) == 0) {
        /* great, we found the entry! Now just fill it with a link
         * to the current dir, previous dir, and a url, and comments file */
        filler(buf, ".", NULL, 0);
        filler(buf, "..", NULL, 0);
        filler(buf, "url", NULL, 0);
        filler(buf, "comments", NULL, 0);
        found_post_entry = 1;
        break;
      }
    }
    pthread_mutex_unlock(&post_collection.mutex);
    if (found_post_entry) {
      return 0;
    }
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
  (void) path;
  (void) buf;
  (void) size;
  (void) offset;
  (void) fi;
  /*
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
  */

  return 0;
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
  curl_global_init(CURL_GLOBAL_ALL);
  return fuse_main(argc, argv, &hnfs_oper, NULL);
}
