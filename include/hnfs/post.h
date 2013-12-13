#ifndef HNFS_POST_H_
#define HNFS_POST_H_

#include <stdlib.h>
#include <pthread.h>

#include "hnfs/decls.h"

typedef struct {
  char title[HNFS_POST_STRING_SIZE + 1];
  char url[HNFS_POST_STRING_SIZE + 1];
  char user[HNFS_POST_STRING_SIZE + 1];
  char id[HNFS_POST_STRING_SIZE + 1];
} hnfs_post_t;

typedef struct {
  hnfs_post_t posts[HNFS_NUM_POSTS];
  pthread_mutex_t mutex;
} hnfs_post_collection_t;

int
hnfs_post_update(hnfs_post_collection_t *collection);

#endif /* HNFS_POST_H_ */
