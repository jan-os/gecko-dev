/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <dirent.h>
#include <errno.h>
#include <libgen.h>
#include <sys/time.h>
#include "mozIApplication.h"
#include "mozilla/ipc/BackgroundParent.h"
#include "mozilla/Preferences.h"
#include "mozilla/unused.h"
#include "nsArrayUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIAppsService.h"
#include "nsIFile.h"
#include "nsISupportsPrimitives.h"
#include "nsThreadUtils.h"
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

class GetAllowedPaths final : public nsRunnable
{
public:
  GetAllowedPaths(const int aAppId, nsTArray<nsString>& aResult)
    : mAppId(aAppId), mResult(aResult) {}

  NS_IMETHOD Run() override
  {
    MOZ_ASSERT(NS_IsMainThread());

    if (Preferences::GetBool("dom.os.security.disabled")) {
      mResult.AppendElement(NS_LITERAL_STRING("/"));
      return NS_OK;
    }

    nsCOMPtr<nsIAppsService> appsService = do_GetService(APPS_SERVICE_CONTRACTID);
    if (!appsService) {
      NS_WARNING("No apps service");
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<mozIApplication> app;
    nsresult rv = appsService->GetAppByLocalId(mAppId, getter_AddRefs(app));
    if (NS_FAILED(rv)) {
      NS_WARNING("GetAppByLocalId failed");
      return rv;
    }
    if (!app) {
      NS_WARNING("Could not get app");
      return NS_ERROR_FAILURE;
    }

    nsCOMPtr<nsIArray> osPaths;
    app->GetOsPaths(getter_AddRefs(osPaths));

    uint32_t length;
    rv = osPaths->GetLength(&length);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    for (uint32_t j = 0; j < length; ++j) {
      nsCOMPtr<nsISupportsString> iss = do_QueryElementAt(osPaths, j);
      if (!iss)
        return NS_ERROR_FAILURE;

      nsAutoString path;
      rv = iss->GetData(path);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
      }

      if (!path.Equals(NS_LITERAL_STRING("TEMPDIR"))) {
        mResult.AppendElement(path);
        continue;
      }

      // TEMPDIR resolving to real temp path
      nsAutoString tmpDirPath;
      nsCOMPtr<nsIFile> tmpDir;
      rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR,
                                  getter_AddRefs(tmpDir));
      if (NS_WARN_IF(NS_FAILED(rv))) {
        continue;
      }
      rv = tmpDir->GetPath(tmpDirPath);
      if (NS_WARN_IF(NS_FAILED(rv))) {
        continue;
      }
      mResult.AppendElement(tmpDirPath);
    }

    return NS_OK;
  }

private:
  const int mAppId;
  nsTArray<nsString>& mResult;
};

bool
OsFileChannelParent::VerifyRights(const nsACString& aPath)
{
  // use path while calling dirname
  char* path = strdup(PromiseFlatCString(aPath).get());

  // do a real_path call on path
  char real_path[PATH_MAX];
  while (realpath(path, real_path) == NULL) {
    // if it's NULL that means that we have an issue
    // when ENOENT we check parent directory (and up and up, etc.)
    // when something else, break and return false
    if (errno != ENOENT) {
      NS_WARNING("VerifyRights failed");
      free(path);
      return false;
    }

    if (strlen(path) > 1024 * 1024) { // 1MB
      free(path);
      return false;
    }

    // dirname changes the pointer you feed into it so we need to copy it first
    nsAutoArrayPtr<char> old_path(new (fallible) char[strlen(path) + 1]);
    if (!old_path) {
      NS_WARNING("Out of memory");
      free(path);
      return false;
    }
    strcpy(old_path, path);

    path = dirname(path);

    if (strcmp(path, old_path) == 0) { // ended at invalid root point
      free(path);
      return false;
    }
  }

  // not null? then time to check... real_path is the path we need to check

  NS_ConvertUTF8toUTF16 rp(real_path);
  uint32_t len = mAllowedPaths.Length();
  for (uint32_t j = 0; j < len; j++) {
    NS_ConvertUTF16toUTF8 allowedPath(mAllowedPaths[j]);
    int res = rp.Find(allowedPath.BeginReading(), false, 0, -1);
    if (res == 0) {
      return true;
    }
  }

  return false;
}

bool
OsFileChannelParent::RecvInit(const int& aAppId)
{
  AssertIsOnBackgroundThread();

  nsCOMPtr<nsIRunnable> event = new GetAllowedPaths(aAppId, mAllowedPaths);
  nsresult rv = NS_DispatchToMainThread(event, NS_DISPATCH_SYNC);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return false;
  }

  return true;
}

bool
OsFileChannelParent::RecvOpen(const nsString& aPath,
                              const int& aAccess,
                              const int& aPermission,
                              FileDescriptorResponse* aFd)
{
  AssertIsOnBackgroundThread();

  NS_ConvertUTF16toUTF8 path(aPath);
  if (!VerifyRights(path)) {
    *aFd = *(new FileDescriptorResponse(FileDescriptor(), EACCES));
    return true;
  }

  int fd;
  if (aPermission == 0) { // @todo make method overload?
    fd = open(path.get(), aAccess);
  } else {
    fd = open(path.get(), aAccess, aPermission);
  }
  *aFd = *(new FileDescriptorResponse(FileDescriptor(fd), fd == -1 ? errno : 0));

  return true;
}

bool
OsFileChannelParent::RecvStat(const nsString& aPath, StatWrapper* aRetVal)
{
  AssertIsOnBackgroundThread();

  struct stat sb;

  NS_ConvertUTF16toUTF8 path(aPath);
  if (!VerifyRights(path)) {
    *aRetVal = *(new StatWrapper(sb, EACCES));
    return true;
  }

  char* real_path = realpath(path.get(), NULL);
  if (real_path == NULL) {
    *aRetVal = *(new StatWrapper(sb, errno));
    return true;
  }

  int error = 0;
  if (stat(real_path, &sb) == -1) {
    error = errno;
  }

  *aRetVal = *(new StatWrapper(sb, error));

  free(real_path);

  return true;
}

bool
OsFileChannelParent::RecvLstat(const nsString& aPath, StatWrapper* aRetVal)
{
  AssertIsOnBackgroundThread();

  struct stat sb;

  NS_ConvertUTF16toUTF8 path(aPath);
  if (!VerifyRights(path)) {
    *aRetVal = *(new StatWrapper(sb, EACCES));
    return true;
  }

  char* real_path = realpath(path.get(), NULL);
  if (real_path == NULL) {
    *aRetVal = *(new StatWrapper(sb, errno));
    return true;
  }

  int error = 0;
  if (lstat(real_path, &sb) == -1) {
    error = errno;
  }

  *aRetVal = *(new StatWrapper(sb, error));

  free(real_path);

  return true;
}

bool
OsFileChannelParent::RecvUnlink(const nsString& aPath, int* aRetVal)
{
  AssertIsOnBackgroundThread();

  NS_ConvertUTF16toUTF8 path(aPath);
  if (!VerifyRights(path)) {
    *aRetVal = EACCES;
    return true;
  }

  if (unlink(path.get()) == -1) {
    *aRetVal = errno;
  } else {
    *aRetVal = 0;
  }

  return true;
}

bool
OsFileChannelParent::RecvChmod(const nsString& aPath,
                               const int& aPermission,
                               int* aRetVal)
{
  AssertIsOnBackgroundThread();

  NS_ConvertUTF16toUTF8 path(aPath);
  if (!VerifyRights(path)) {
    *aRetVal = EACCES;
    return true;
  }

  if (chmod(path.get(), aPermission) == -1) {
    *aRetVal = errno;
  } else {
    *aRetVal = 0;
  }

  return true;
}

bool
OsFileChannelParent::RecvUtimes(const nsString& aPath,
                                const double& aActime,
                                const double& aModtime,
                                int* aRetVal)
{
  AssertIsOnBackgroundThread();

  NS_ConvertUTF16toUTF8 path(aPath);
  if (!VerifyRights(path)) {
    *aRetVal = EACCES;
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

  if (utimes(path.get(), tv) == -1) {
    *aRetVal = errno;
  } else {
    *aRetVal = 0;
  }

  return true;
}

bool
OsFileChannelParent::RecvLutimes(const nsString& aPath,
                                 const double& aActime,
                                 const double& aModtime,
                                 int* aRetVal)
{
  AssertIsOnBackgroundThread();

  NS_ConvertUTF16toUTF8 path(aPath);
  if (!VerifyRights(path)) {
    *aRetVal = EACCES;
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

  if (lutimes(path.get(), tv) == -1) {
    *aRetVal = errno;
  } else {
    *aRetVal = 0;
  }

  return true;
}

bool
OsFileChannelParent::RecvTruncate(const nsString& aPath,
                                  const int& aLength,
                                  int* aRetVal)
{
  AssertIsOnBackgroundThread();

  NS_ConvertUTF16toUTF8 path(aPath);
  if (!VerifyRights(path)) {
    *aRetVal = EACCES;
    return true;
  }

  if (truncate(path.get(), aLength) == -1) {
    *aRetVal = errno;
  } else {
    *aRetVal = 0;
  }

  return true;
}

bool
OsFileChannelParent::RecvMkdir(const nsString& aPath,
                               const int& aMode,
                               int* aRetVal)
{
  AssertIsOnBackgroundThread();

  NS_ConvertUTF16toUTF8 path(aPath);
  if (!VerifyRights(path)) {
    *aRetVal = EACCES;
    return true;
  }

  if (mkdir(path.get(), aMode) == -1) {
    *aRetVal = errno;
  } else {
    *aRetVal = 0;
  }

  return true;
}

bool
OsFileChannelParent::RecvRmdir(const nsString& aPath, int* aRetVal)
{
  AssertIsOnBackgroundThread();

  NS_ConvertUTF16toUTF8 path(aPath);
  if (!VerifyRights(path)) {
    *aRetVal = EACCES;
    return true;
  }

  if (rmdir(path.get()) == -1) {
    *aRetVal = errno;
  } else {
    *aRetVal = 0;
  }

  return true;
}

bool
OsFileChannelParent::RecvRename(const nsString& aOldPath,
                                const nsString& aNewPath, int* aRetVal)
{
  AssertIsOnBackgroundThread();

  NS_ConvertUTF16toUTF8 oldPath(aOldPath);
  NS_ConvertUTF16toUTF8 newPath(aNewPath);
  if (!VerifyRights(oldPath) || !VerifyRights(newPath)) {
    *aRetVal = EACCES;
    return true;
  }

  if (rename(oldPath.get(), newPath.get()) == -1) {
    *aRetVal = errno;
  } else {
    *aRetVal = 0;
  }

  return true;
}

bool
OsFileChannelParent::RecvReaddir(const nsString& aPath,
                                 ReaddirResponse* aRetVal)
{
  AssertIsOnBackgroundThread();

  nsTArray<nsString> files;

  NS_ConvertUTF16toUTF8 path(aPath);
  if (!VerifyRights(path)) {
    *aRetVal = *(new ReaddirResponse(files, EACCES));
    return true;
  }

  DIR* d = opendir(path.get());
  if (!d) {
    *aRetVal = *(new ReaddirResponse(files, errno));
    return true;
  }

  struct dirent* dir;
  while ((dir = readdir(d)) != NULL) {
    if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) {
      continue;
    }

    files.AppendElement(NS_ConvertUTF8toUTF16(dir->d_name));
  }

  closedir(d);

  *aRetVal = *(new ReaddirResponse(files, 0));


  return true;
}

bool
OsFileChannelParent::RecvSymlink(const nsString& aPath1,
                                 const nsString& aPath2,
                                 int* aRetVal)
{
  AssertIsOnBackgroundThread();

  // http://pubs.opengroup.org/onlinepubs/009695399/functions/symlink.html
  // The string pointed to by path1 shall be treated only as a character
  // string and shall not be validated as a pathname.

  NS_ConvertUTF16toUTF8 path1(aPath1);
  NS_ConvertUTF16toUTF8 path2(aPath2);
  if (!VerifyRights(path2)) {
    *aRetVal = EACCES;
    return true;
  }

  if (symlink(path1.get(), path2.get()) == -1) {
    *aRetVal = errno;
  } else {
    *aRetVal = 0;
  }

  return true;
}

bool
OsFileChannelParent::RecvReadlink(const nsString& aPath,
                                 ReadlinkResponse* aRetVal)
{
  AssertIsOnBackgroundThread();

  NS_ConvertUTF16toUTF8 path(aPath);
  if (!VerifyRights(path)) {
    *aRetVal = *(new ReadlinkResponse(NS_LITERAL_STRING(""), EACCES));
    return true;
  }

  int buffer_size = 255;

  while (buffer_size < 1024 * 1024) { // 1MB
    nsAutoArrayPtr<char> buffer(new (fallible) char[buffer_size]);
    if (!buffer) {
      *aRetVal = *(new ReadlinkResponse(NS_LITERAL_STRING(""), ENOMEM));
      return true;
    }

    int rl = readlink(path.get(), buffer, buffer_size);
    if (rl == -1) {
      *aRetVal = *(new ReadlinkResponse(NS_LITERAL_STRING(""), errno));
      return true;
    }

    if (rl < buffer_size) {
      buffer[rl] = '\0';
      *aRetVal = *(new ReadlinkResponse(NS_ConvertUTF8toUTF16(buffer), 0));
      return true;
    }

    buffer_size *= 2;
  }

  *aRetVal = *(new ReadlinkResponse(NS_LITERAL_STRING(""), ENOMEM));
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
