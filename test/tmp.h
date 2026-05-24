#ifndef HC_TEST_TMP_H
#define HC_TEST_TMP_H

#include <stdio.h>

#include <uv.h>

// Recursively remove a directory and all its contents. Returns 0 on success
// or a libuv error code on failure. Intended for test teardown — best-effort,
// not robust against symlink escapes or concurrent modification.
static int
hc_test_rmdir (const char *path) {
  uv_loop_t *loop = uv_default_loop();

  uv_fs_t scan;
  int n = uv_fs_scandir(loop, &scan, path, 0, NULL);
  if (n < 0) {
    uv_fs_req_cleanup(&scan);
    return n;
  }

  uv_dirent_t ent;
  while (uv_fs_scandir_next(&scan, &ent) != UV_EOF) {
    char child[4096];
    snprintf(child, sizeof(child), "%s/%s", path, ent.name);

    if (ent.type == UV_DIRENT_DIR) {
      hc_test_rmdir(child);
    } else {
      uv_fs_t r;
      uv_fs_unlink(loop, &r, child, NULL);
      uv_fs_req_cleanup(&r);
    }
  }
  uv_fs_req_cleanup(&scan);

  uv_fs_t r;
  int rc = uv_fs_rmdir(loop, &r, path, NULL);
  uv_fs_req_cleanup(&r);
  return rc;
}

#endif // HC_TEST_TMP_H
