#ifndef HNFS_POST_H_
#define HNFS_POST_H_

#include <stdlib.h>
#include <pthread.h>
#include <time.h>

#include "hnfs/decls.h"

typedef struct {
  char title[HNFS_POST_STRING_SIZE + 1];
  char url[HNFS_POST_STRING_SIZE + 1];
  char user[HNFS_POST_STRING_SIZE + 1];
  char id[HNFS_POST_STRING_SIZE + 1];
  char *content;
  time_t content_update_time;
} hnfs_post_t;

typedef struct {
  hnfs_post_t posts[HNFS_NUM_POSTS];
  pthread_mutex_t mutex;
  time_t update_time;
} hnfs_post_collection_t;

int
hnfs_post_update(hnfs_post_collection_t *collection);

int
hnfs_post_fetch_content(hnfs_post_t *post);

#endif /* HNFS_POST_H_ */
