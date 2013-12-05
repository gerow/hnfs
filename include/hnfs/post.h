#ifndef HNFS_POST_H_
#define HNFS_POST_H_

#include <stdlib.h>

typedef struct hnfs_post {
  char *title;
  char *url;
  char *user;
  char *id;
} hnfs_post_t;

size_t
hnfs_posts_libcurl_writer(void *buffer, size_t size, size_t nmemb, void *userp);

#endif /* HNFS_POST_H_ */
