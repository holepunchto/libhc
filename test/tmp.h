#ifndef HC_TEST_TMP_H
#define HC_TEST_TMP_H

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>

#include <uv.h>

// Refuse paths that aren't under uv_os_tmpdir. Defence against accidentally
// calling rmdir with the wrong path (e.g. "/" or a real user directory).
// Returns 1 if `path` is safe to operate on, 0 otherwise.
static int
hc__test_path_under_tmpdir (const char *path) {
  if (path == NULL) return 0;

  char base[1024];
  size_t base_len = sizeof(base);
  if (uv_os_tmpdir(base, &base_len) != 0) return 0;

  // base_len from uv_os_tmpdir excludes the NUL terminator.
  if (strncmp(path, base, base_len) != 0) return 0;
  if (path[base_len] != '/') return 0;          // exact base or "<base>foo"
  if (path[base_len + 1] == '\0') return 0;     // just "<base>/"

  return 1;
}

// Create a unique tmp directory under uv_os_tmpdir using the given prefix.
// The resulting path is written to `out` (must be at least 2048 bytes).
// Returns 0 on success or a libuv error code on failure.
static int
hc_test_mkdtemp (char *out, size_t out_size, const char *prefix) {
  char base[1024];
  size_t base_len = sizeof(base);
  if (uv_os_tmpdir(base, &base_len) != 0) return UV_ENOENT;

  char tpl[2048];
  int n = snprintf(tpl, sizeof(tpl), "%s/%s-XXXXXX", base, prefix);
  if (n < 0 || (size_t) n >= sizeof(tpl)) return UV_ENAMETOOLONG;

  uv_fs_t req;
  int rc = uv_fs_mkdtemp(uv_default_loop(), &req, tpl, NULL);
  if (rc < 0) {
    uv_fs_req_cleanup(&req);
    return rc;
  }

  size_t path_len = strlen(req.path);
  if (path_len >= out_size) {
    uv_fs_req_cleanup(&req);
    return UV_ENAMETOOLONG;
  }
  memcpy(out, req.path, path_len + 1);
  uv_fs_req_cleanup(&req);
  return 0;
}

// Recursively remove a directory and all its contents. Returns 0 on success
// or a libuv error code on failure. Intended for test teardown only.
//
// Safety guarantees:
//   * Refuses any path not strictly under uv_os_tmpdir.
//   * Uses lstat to determine entry type, so symlinks are unlinked rather
//     than followed.
//   * Detects path-join truncation and aborts the entry rather than
//     operating on a wrong path.
static int
hc_test_rmdir (const char *path) {
  if (!hc__test_path_under_tmpdir(path)) return UV_EINVAL;

  uv_loop_t *loop = uv_default_loop();

  uv_fs_t st;
  if (uv_fs_lstat(loop, &st, path, NULL) < 0) {
    uv_fs_req_cleanup(&st);
    return UV_ENOENT;
  }
  uint64_t mode = st.statbuf.st_mode;
  uv_fs_req_cleanup(&st);

  if ((mode & S_IFMT) != S_IFDIR) {
    // Not a directory (file, symlink, anything else): unlink. unlink does
    // not follow symlinks, so the target of a symlink is never touched.
    uv_fs_t r;
    int rc = uv_fs_unlink(loop, &r, path, NULL);
    uv_fs_req_cleanup(&r);
    return rc;
  }

  uv_fs_t scan;
  int n = uv_fs_scandir(loop, &scan, path, 0, NULL);
  if (n < 0) {
    uv_fs_req_cleanup(&scan);
    return n;
  }

  int err = 0;
  uv_dirent_t ent;
  while (uv_fs_scandir_next(&scan, &ent) != UV_EOF) {
    char child[4096];
    int written = snprintf(child, sizeof(child), "%s/%s", path, ent.name);
    if (written < 0 || (size_t) written >= sizeof(child)) {
      // Truncation — refuse to operate on a wrong path.
      err = UV_ENAMETOOLONG;
      continue;
    }
    int rc = hc_test_rmdir(child);
    if (rc < 0) err = rc;
  }
  uv_fs_req_cleanup(&scan);

  uv_fs_t r;
  int rc = uv_fs_rmdir(loop, &r, path, NULL);
  uv_fs_req_cleanup(&r);
  return err ? err : rc;
}

#endif // HC_TEST_TMP_H
