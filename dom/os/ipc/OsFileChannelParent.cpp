/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

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

bool
OsFileChannelParent::VerifyRights(const char* aPath)
{
  // first make a normal char* out of aPath
  int aPathLen = strlen(aPath);
  char* org_path = (char*)malloc(aPathLen + 1);
  if (!org_path) {
    return false; // @todo: NS_ERROR_OUT_OF_MEMORY
  }
  for (int ix = 0; ix < aPathLen; ix++) {
    org_path[ix] = aPath[ix];
  }
  org_path[aPathLen] = '\0';

  // use path while calling dirname, keep org_path to free later
  char* path = org_path;

  // do a real_path call on path
  char* real_path;
  while ((real_path = realpath(path, NULL)) == NULL) {
    // if it's NULL that means that we have an issue
    // when ENOENT we check parent directory (and up and up, etc.)
    // when something else, break and return false
    if (errno != ENOENT) {
      printf("VerifyRights for '%s' failed with %d (%s)\n",
             org_path, errno, strerror(errno));
      free(org_path);
      return false;
    }

    // dirname changes the pointer you feed into it so we need to copy it first
    char* old_path = (char*)malloc(strlen(path) + 1);
    if (!old_path) {
      free(org_path);
      return false; //@todo: NS_ERROR_OUT_OF_MEMORY
    }
    strcpy(old_path, path);

    path = dirname(path);

    if (strcmp(path, old_path) == 0) { // ended at invalid root point
      free(old_path);
      free(org_path);
      return false;
    }

    free(old_path);
  }

  // not null? then success! real_path is the path we need to check

  free(real_path);
  free(org_path);
  return true;
}

bool
OsFileChannelParent::RecvOpen(const nsString& aPath,
                              const int& aAccess,
                              const int& aPermission,
                              FileDescriptorResponse* aFd)
{
  AssertIsOnBackgroundThread();

  auto path = NS_LossyConvertUTF16toASCII(aPath).get();
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

  return true;
}

bool
OsFileChannelParent::RecvStat(const nsString& aPath, StatWrapper* aRetval)
{
  AssertIsOnBackgroundThread();

  struct stat sb;

  auto path = NS_LossyConvertUTF16toASCII(aPath).get();
  if (!VerifyRights(path)) {
    *aRetval = *(new StatWrapper(sb, EACCES));
    return true;
  }

  char* real_path = realpath(path, NULL);
  if (real_path == NULL) {
    *aRetval = *(new StatWrapper(sb, errno));
    return true;
  }

  int error = 0;
  if (stat(real_path, &sb) == -1) {
    error = errno;
  }

  *aRetval = *(new StatWrapper(sb, error));

  free(real_path);

  return true;
}

bool
OsFileChannelParent::RecvLstat(const nsString& aPath, StatWrapper* aRetval)
{
  AssertIsOnBackgroundThread();

  struct stat sb;

  auto path = NS_LossyConvertUTF16toASCII(aPath).get();
  if (!VerifyRights(path)) {
    *aRetval = *(new StatWrapper(sb, EACCES));
    return true;
  }

  char* real_path = realpath(path, NULL);
  if (real_path == NULL) {
    *aRetval = *(new StatWrapper(sb, errno));
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

  auto path = NS_LossyConvertUTF16toASCII(aPath).get();
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

  return true;
}

bool
OsFileChannelParent::RecvChmod(const nsString& aPath,
                               const int& aPermission,
                               int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto path = NS_LossyConvertUTF16toASCII(aPath).get();
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

  return true;
}

bool
OsFileChannelParent::RecvUtimes(const nsString& aPath,
                                const double& aActime,
                                const double& aModtime,
                                int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto path = NS_LossyConvertUTF16toASCII(aPath).get();
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

  return true;
}

bool
OsFileChannelParent::RecvLutimes(const nsString& aPath,
                                 const double& aActime,
                                 const double& aModtime,
                                 int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto path = NS_LossyConvertUTF16toASCII(aPath).get();
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

  return true;
}

bool
OsFileChannelParent::RecvTruncate(const nsString& aPath,
                               const int& aLength,
                               int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto path = NS_LossyConvertUTF16toASCII(aPath).get();
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

  return true;
}

bool
OsFileChannelParent::RecvMkdir(const nsString& aPath,
                               const int& aMode,
                               int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto path = NS_LossyConvertUTF16toASCII(aPath).get();
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

  return true;
}

bool
OsFileChannelParent::RecvRmdir(const nsString& aPath, int* aRetval)
{
  AssertIsOnBackgroundThread();

  auto path = NS_LossyConvertUTF16toASCII(aPath).get();
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
