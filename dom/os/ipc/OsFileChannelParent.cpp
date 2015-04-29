/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <sys/time.h>
#include "mozilla/ipc/BackgroundParent.h"
#include "mozilla/unused.h"
#include "OsFileChannelParent.h"

namespace mozilla {

using namespace ipc;
using mozilla::ipc::FileDescriptor;

namespace dom {
namespace os {

OsFileChannelParent::OsFileChannelParent()
{
  AssertIsOnBackgroundThread();
}

OsFileChannelParent::~OsFileChannelParent()
{
  AssertIsOnBackgroundThread();
}

/**
 * If the return value of this function is false, aPath will be free'd
 * before returning!
 * If the return value is true, you need to free() yourself.
 */
bool
OsFileChannelParent::VerifyRights(char* aPath)
{
  if (!aPath) {
    return false; // @todo: NS_ERROR_OUT_OF_MEMORY
  }

  // use path while calling dirname, keep aPath to free later
  char* path = aPath;

  // do a real_path call on path
  char* real_path;
  while ((real_path = realpath(path, NULL)) == NULL) {
    // if it's NULL that means that we have an issue
    // when ENOENT we check parent directory (and up and up, etc.)
    // when something else, break and return false
    if (errno != ENOENT) {
      printf("VerifyRights for '%s' failed with %d (%s)\n",
             aPath, errno, strerror(errno));
      free(aPath);
      return false;
    }

    // dirname changes the pointer you feed into it so we need to copy it first
    char* old_path = (char*)malloc(strlen(path) + 1);
    if (!old_path) {
      free(aPath);
      return false; //@todo: NS_ERROR_OUT_OF_MEMORY
    }
    strcpy(old_path, path);

    path = dirname(path);

    if (strcmp(path, old_path) == 0) { // ended at invalid root point
      free(old_path);
      free(aPath);
      return false;
    }

    free(old_path);
  }

  // not null? then time to check... real_path is the path we need to check

  free(real_path);
  return true;
}

bool
OsFileChannelParent::RecvOpen(const nsString& aPath,
                              const int& aAccess,
                              const int& aPermission,
                              FileDescriptorResponse* aFd)
{
  AssertIsOnBackgroundThread();

  auto path = ToNewCString(aPath);
  if (!VerifyRights(path)) {
    *aFd = *(new FileDescriptorResponse(FileDescriptor(), EACCES));
    return true;
  }

  int fd;
  if (aPermission == 0) { // @todo make method overload?
    fd = open(path, aAccess);
  }
  else {
    fd = open(path, aAccess, aPermission);
  }
  *aFd = *(new FileDescriptorResponse(FileDescriptor(fd), fd == -1 ? errno : 0));

  free(path);

  return true;
}

bool
OsFileChannelParent::RecvStat(const nsString& aPath, StatWrapper* aRetval)
{
  AssertIsOnBackgroundThread();

  struct stat sb;

  auto path = ToNewCString(aPath);
  if (!VerifyRights(path)) {
    *aRetval = *(new StatWrapper(sb, EACCES));
    return true;
  }

  char* real_path = realpath(path, NULL);
  if (real_path == NULL) {
    *aRetval = *(new StatWrapper(sb, errno));
    free(path);
    return true;
  }

  int error = 0;
  if (stat(real_path, &sb) == -1) {
    error = errno;
  }

  *aRetval = *(new StatWrapper(sb, error));

  free(path);
  free(real_path);

  return true;
}

bool
OsFileChannelParent::RecvLstat(const nsString& aPath, StatWrapper* aRetval)
{
  AssertIsOnBackgroundThread();

  struct stat sb;

  auto path = ToNewCString(aPath);
  if (!VerifyRights(path)) {
    *aRetval = *(new StatWrapper(sb, EACCES));
    return true;
  }

  char* real_path = realpath(path, NULL);
  if (real_path == NULL) {
    *aRetval = *(new StatWrapper(sb, errno));
    free(path);
    return true;
  }

  int error = 0;
  if (lstat(real_path, &sb) == -1) {
    error = errno;
  }

  *aRetval = *(new StatWrapper(sb, error));

  free(real_path);

  return true;
}

bool
OsFileChannelParent::RecvUnlink(const nsString& aPath, int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto path = ToNewCString(aPath);
  if (!VerifyRights(path)) {
    *aRetval = EACCES;
    return true;
  }

  if (unlink(path) == -1) {
    *aRetval = errno;
  }
  else {
    *aRetval = 0;
  }

  free(path);

  return true;
}

bool
OsFileChannelParent::RecvChmod(const nsString& aPath,
                               const int& aPermission,
                               int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto path = ToNewCString(aPath);
  if (!VerifyRights(path)) {
    *aRetval = EACCES;
    return true;
  }

  if (chmod(path, aPermission) == -1) {
    *aRetval = errno;
  }
  else {
    *aRetval = 0;
  }

  free(path);

  return true;
}

bool
OsFileChannelParent::RecvUtimes(const nsString& aPath,
                                const double& aActime,
                                const double& aModtime,
                                int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto path = ToNewCString(aPath);
  if (!VerifyRights(path)) {
    *aRetval = EACCES;
    return true;
  }

  struct timeval tv[2];
  tv[0] = {
    .tv_sec = ((time_t)floor(aActime)) / 1000,
    .tv_usec = (suseconds_t)(((long)floor(aActime)) % 1000) * 1000
  };
  tv[1] = {
    .tv_sec = ((time_t)floor(aModtime)) / 1000,
    .tv_usec = (suseconds_t)(((long)floor(aModtime)) % 1000) * 1000
  };

  if (utimes(path, tv) == -1) {
    *aRetval = errno;
  }
  else {
    *aRetval = 0;
  }

  free(path);

  return true;
}

bool
OsFileChannelParent::RecvLutimes(const nsString& aPath,
                                 const double& aActime,
                                 const double& aModtime,
                                 int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto path = ToNewCString(aPath);
  if (!VerifyRights(path)) {
    *aRetval = EACCES;
    return true;
  }

  struct timeval tv[2];
  tv[0] = {
    .tv_sec = ((time_t)floor(aActime)) / 1000,
    .tv_usec = (suseconds_t)(((long)floor(aActime)) % 1000) * 1000
  };
  tv[1] = {
    .tv_sec = ((time_t)floor(aModtime)) / 1000,
    .tv_usec = (suseconds_t)(((long)floor(aModtime)) % 1000) * 1000
  };

  if (lutimes(path, tv) == -1) {
    *aRetval = errno;
  }
  else {
    *aRetval = 0;
  }

  free(path);

  return true;
}

bool
OsFileChannelParent::RecvTruncate(const nsString& aPath,
                               const int& aLength,
                               int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto path = ToNewCString(aPath);
  if (!VerifyRights(path)) {
    *aRetval = EACCES;
    return true;
  }

  if (truncate(path, aLength) == -1) {
    *aRetval = errno;
  }
  else {
    *aRetval = 0;
  }

  free(path);

  return true;
}

bool
OsFileChannelParent::RecvMkdir(const nsString& aPath,
                               const int& aMode,
                               int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto path = ToNewCString(aPath);
  if (!VerifyRights(path)) {
    *aRetval = EACCES;
    return true;
  }

  if (mkdir(path, aMode) == -1) {
    *aRetval = errno;
  }
  else {
    *aRetval = 0;
  }

  free(path);

  return true;
}

bool
OsFileChannelParent::RecvRmdir(const nsString& aPath, int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto path = ToNewCString(aPath);
  if (!VerifyRights(path)) {
    *aRetval = EACCES;
    return true;
  }

  if (rmdir(path) == -1) {
    *aRetval = errno;
  }
  else {
    *aRetval = 0;
  }

  free(path);

  return true;
}

bool
OsFileChannelParent::RecvRename(const nsString& aOldPath,
                                const nsString& aNewPath, int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto oldPath = ToNewCString(aOldPath);
  auto newPath = ToNewCString(aNewPath);
  if (!VerifyRights(oldPath) || !VerifyRights(newPath)) {
    *aRetval = EACCES;
    return true;
  }

  if (rename(oldPath, newPath) == -1) {
    *aRetval = errno;
  }
  else {
    *aRetval = 0;
  }

  free(oldPath);
  free(newPath);

  return true;
}

bool
OsFileChannelParent::RecvReaddir(const nsString& aPath,
                                 ReaddirResponse* aRetval)
{
  nsTArray<nsString> files;

  auto path = ToNewCString(aPath);
  if (!VerifyRights(path)) {
    *aRetval = *(new ReaddirResponse(files, EACCES));
    return true;
  }

  DIR* d = opendir(path);
  if (!d) {
    *aRetval = *(new ReaddirResponse(files, errno));
    return true;
  }

  struct dirent* dir;
  while ((dir = readdir(d)) != NULL) {
    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
      continue;
    }

    files.AppendElement(NS_ConvertASCIItoUTF16(dir->d_name));
  }

  closedir(d);

  *aRetval = *(new ReaddirResponse(files, 0));

  free(path);

  return true;
}

void
OsFileChannelParent::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnBackgroundThread();
}

}
} // dom namespace
} // mozilla namespace
