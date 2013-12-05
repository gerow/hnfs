#include "hnfs/post.h"

size_t
hnfs_posts_libcurl_writer(void *buffer, size_t size, size_t nmemb, void *userp)
{
  struct hnfs_post **posts = userp;
}
