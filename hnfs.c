#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <curl/curl.h>
#include <pthread.h>
#include <assert.h>

#include "hnfs/post.h"
#include "hnfs/decls.h"

static hnfs_post_collection_t post_collection = {
  .mutex = PTHREAD_MUTEX_INITIALIZER,
  .update_time = 0
};

const char content_template[] = "<html><head><meta http-equiv=\"refresh\""
  "content=\"0;URL='%s'\" /></head></html>\n";

/* 
 * incredibly simply function to check if the given path is
 * the root path
 */
static int is_root(const char *path)
{
  return strcmp(path, "/") == 0;
}

/* 
 * Get the depth of a path. 0 Means it is the root path,
 * 1 means a path like "/thing"
 * 2 means a path like "/thing/two"
 */
static int path_depth(const char *path)
{
  if (is_root(path)) {
    return 0;
  }

  int depth = 0;
  for ( ;*path != '\0'; path++) {
    if (*path == '/') {
      depth++;
    }
  }

  if (depth == 0) {
    return -ENOENT;
  }

  return depth;
}

/* 
 * Get the index of the base path. So if the given path was
 * "/storyname/url" we would return the index for storyname
 * in the post_collection
 */
static int get_level_one_path_index(const char *path)
{
  /*
   * Assume this function is only called when path_depth(path) == 1
   * or path_depth(path) == 2
   * so we don't make that check again
   */
  path++;
  char * end_lvl_one_ptr = strchr(path, '/');
  size_t level_one_size;
  if (end_lvl_one_ptr) {
    level_one_size = (end_lvl_one_ptr - path);
  } else {
    level_one_size = strlen(path);
  }
  assert(level_one_size <= HNFS_POST_STRING_SIZE);
  for (int i = 0; i < HNFS_NUM_POSTS; i++) {
    if (strncmp(path, post_collection.posts[i].title, level_one_size) == 0) {
      return i;
    }
  }

  /* entry not found */
  return -ENOENT;
}

static int
read_str(const char *str, char *buf, size_t size, off_t offset)
{
  size_t str_len = strlen(str);
  if (offset >= ((long long)str_len)) {
    return 0;
  }
  str += offset;
  str_len -= offset;
  int ret = size;
  if (str_len < size) {
    ret = str_len;
  }

  memcpy(buf, str, ret);

  return ret;
}

static int
read_str_with_newline(const char *str,
                               char *buf,
                               size_t size,
                               off_t offset)
{
  int ret = read_str(str, buf, size, offset);
  if (ret < ((int) size)) {
    *(buf + ret) = '\n';
    ret++;
  }

  return ret;
}

static int
l2_getattr_url(hnfs_post_t *post, struct stat *stbuf)
{
  /* add 1 for the newline at the end */
  stbuf->st_size = strlen(post->url) + 1;

  return 0;
}

static int
l2_read_url(hnfs_post_t *post,
            char *buf,
            size_t size,
            off_t offset,
            struct fuse_file_info *fi)
{
  (void) fi;

  return read_str_with_newline(post->url,
                               buf,
                               size,
                               offset);
}

static int
l2_getattr_content(hnfs_post_t *post, struct stat *stbuf)
{
  hnfs_post_fetch_content(post);
  if (post->content) {
    stbuf->st_size = strlen(post->content);
  } else {
    /* 
     * if we couldn't download the content for any reason just say the
     * size is 0
     */
     stbuf->st_size = 0;
  }

  return 0;
}

static int
l2_read_content(hnfs_post_t *post,
                char *buf,
                size_t size,
                off_t offset,
                struct fuse_file_info *fi)
{
  (void) fi;

  hnfs_post_fetch_content(post);
  if (!post->content) {
    return 0;
  }

  return read_str(post->content,
                  buf,
                  size,
                  offset);
}

static int
l2_getattr_redirect(hnfs_post_t *post, struct stat *stbuf)
{
  /* 
   * -3 is to remove the %s and \0 from the template. The +1 is
   * for a newline at the end
   */
  stbuf->st_size = sizeof(content_template) -
                   3 +
                   strlen(post->url) +
                   1;

  return 0;
}

static int
l2_read_redirect(hnfs_post_t *post,
                 char *buf,
                 size_t size,
                 off_t offset,
                 struct fuse_file_info *fi)
{
  (void) fi;

  char output[sizeof(content_template) +  sizeof(post_collection.posts[0].url)];
  sprintf(output, content_template, post->url);
  return read_str(output, buf, size, offset);
}

static int
l2_getattr_user(hnfs_post_t *post, struct stat *stbuf)
{
  stbuf->st_size = strlen(post->user) + 1;

  return 0;
}

static int
l2_read_user(hnfs_post_t *post,
             char *buf,
             size_t size,
             off_t offset,
             struct fuse_file_info *fi)
{
  (void) fi;

  return read_str_with_newline(post->user,
                               buf,
                               size,
                               offset);
}

static int
read_redirect(const char *str, char *buf, size_t size, off_t offset)
{
  char output[sizeof(content_template) +  sizeof(post_collection.posts[0].url)];
  sprintf(output, content_template, str);
  fprintf(stderr, "CONTENT IS %s\n", output);
  return read_str(output, buf, size, offset);
}

typedef enum {
  HNFS_SECOND_LEVEL_URL,
  HNFS_SECOND_LEVEL_CONTENT,
  HNFS_SECOND_LEVEL_REDIRECT,
  HNFS_SECOND_LEVEL_USER,
} hnfs_second_level_type_t;

typedef struct strmap {
  const char *name;
  int val;
  int (*getattr)(hnfs_post_t *post, struct stat *stbuf);
  int (*read)(hnfs_post_t *post,
              char *buf,
              size_t size,
              off_t offset,
              struct fuse_file_info *fi);
} strmap_t;

strmap_t l2_map[] = {
  {
    "url",
    HNFS_SECOND_LEVEL_URL,
    l2_getattr_url,
    l2_read_url
  },
  {
    "content.html",
    HNFS_SECOND_LEVEL_CONTENT,
    l2_getattr_content,
    l2_read_content
  },
  {
    "redirect.html",
    HNFS_SECOND_LEVEL_REDIRECT,
    l2_getattr_redirect,
    l2_read_redirect
  },
  {
    "user",
    HNFS_SECOND_LEVEL_USER,
    l2_getattr_user,
    l2_read_user
  }
};

const static unsigned int l2_types = (sizeof(l2_map) /
                                      sizeof(l2_map[0]));

/*
 * Get the second level of the path. If we have something like
 * "/storyname/url" we would return the number for the url type, which
 * would be 0 in this case.
 */
static int get_level_two_path_type(const char *path)
{
  /* skip past the first / */
  path++;
  /* skip the second / */
  path = strchr(path, '/');
  path++;
  /* extract the level two name */
  char * end_lvl_two_ptr = strchr(path, '/');
  size_t level_two_size;
  if (end_lvl_two_ptr) {
    level_two_size = (end_lvl_two_ptr - path);
  } else {
    level_two_size = strlen(path);
  }

  assert(level_two_size <= HNFS_POST_STRING_SIZE);

  for (unsigned int i = 0;
       i < l2_types;
       i++) {
    if (strncmp(path, l2_map[i].name, level_two_size) == 0) {
      return l2_map[i].val;
    }
  }

  return -1;
}

static int hnfs_getattr(const char *path, struct stat *stbuf)
{
  fprintf(stderr, "getattr called for %s\n", path);
  int res = 0;

  hnfs_post_update(&post_collection);

  int depth = path_depth(path);
  if (depth < 0) {
    return -ENOENT;
  }

  memset(stbuf, 0, sizeof (struct stat));
  if (depth == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
  } else {
    /* find the index for the entry */
    pthread_mutex_lock(&post_collection.mutex);
    int post_entry = get_level_one_path_index(path);
    if (post_entry < 0) {
      pthread_mutex_unlock(&post_collection.mutex);
      return -ENOENT;
    }
    /* great, now if it's a level one type just set all its properties
     * as that of a directory */
    if (depth == 1) {
      stbuf->st_mode = S_IFDIR | 0755;
      stbuf->st_nlink = 2;
    }
    else if (depth == 2) {
      int l2_path_type = get_level_two_path_type(path);
      if (l2_path_type < 0) {
        pthread_mutex_unlock(&post_collection.mutex);
        return -ENOENT;
      }
      /* read only regular file */
      stbuf->st_mode = S_IFREG | 0444;
      stbuf->st_nlink = 1;

      for (unsigned int i = 0; i < l2_types; i++) {
        if (l2_path_type == l2_map[i].val) {
          l2_map[i].getattr(&post_collection.posts[post_entry],
                            stbuf);
        }
      }
    }
    pthread_mutex_unlock(&post_collection.mutex);
  }

  return res;
}

static int hnfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
                        off_t offset, struct fuse_file_info *fi)
{
  (void) offset;
  (void) fi;
  
  fprintf(stderr, "readdir called for %s\n", path);

  int depth = path_depth(path);

  if (depth < 0) {
    return -ENOENT;
  }

  if (depth == 0) {
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
  } else if (depth == 1) {
    /* otherwise it's probably trying to find entries for a specific post */
    pthread_mutex_lock(&post_collection.mutex);
    int post_index = get_level_one_path_index(path);
    if (post_index < 0) {
      pthread_mutex_unlock(&post_collection.mutex);
      return -ENOENT;
    }

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    for (unsigned int i = 0;
         i < l2_types;
         i++) {
      filler(buf, l2_map[i].name, NULL, 0);
    }

    pthread_mutex_unlock(&post_collection.mutex);

    return 0;
  }

  return -ENOENT;
}

static int hnfs_open(const char *path, struct fuse_file_info *fi)
{
  fprintf(stderr, "open called for %s\n", path);

  /* make sure this is a valid entry */
  int depth = path_depth(path);
  if (depth < 0) {
    return -ENOENT;
  }

  if (depth != 0) {
    pthread_mutex_lock(&post_collection.mutex);
    int post_index = get_level_one_path_index(path);
    pthread_mutex_unlock(&post_collection.mutex);
    if (post_index < 0) {
      return -ENOENT;
    }
    if (depth == 2) {
      int l2_type = get_level_two_path_type(path);
      if (l2_type < 0) {
        return -ENOENT;
      }
    }
  }

  /* if not opened readonly disallow */
  if ((fi->flags & 3) != O_RDONLY)
    return -EACCES;

  return 0;
}



static int hnfs_read(const char *path,
                     char *buf,
                     size_t size,
                     off_t offset,
                     struct fuse_file_info *fi)
{
  (void) path;
  (void) buf;
  (void) size;
  (void) offset;
  (void) fi;

  int ret = 0;

  int depth = path_depth(path);
  if (depth != 2) {
    return -ENOENT;
  }
  pthread_mutex_lock(&post_collection.mutex);

  int post_index = get_level_one_path_index(path);
  if (post_index < 0) {
    ret = -ENOENT;
    goto cleanup_mutex;
  }
  int path_type = get_level_two_path_type(path);
  if (path_type < 0) {
    ret = -ENOENT;
    goto cleanup_mutex;
  }

  for (unsigned int i = 0; i < l2_types; i++) {
    if (path_type == l2_map[i].val) {
      ret = l2_map[i].read(&post_collection.posts[post_index],
                           buf,
                           size,
                           offset,
                           fi);
    }
  }
cleanup_mutex:
  pthread_mutex_unlock(&post_collection.mutex);

  return ret;
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
