#ifndef HNFS_DECLS_H_
#define HNFS_DECLS_H_

#define HNFS_NUM_POSTS 30
/* string size does NOT include null character, so if you want this
 * to be a null terminated string then you'll need +1 more space */
#define HNFS_POST_STRING_SIZE 255
/* wait 2 minutes between upadtes */
#define HNFS_TIME_BETWEEN_UPDATES (60 * 2)

#endif /* HNFS_DECLS_H_ */
