/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <errno.h>
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
OsFileChannelParent::RecvOpen(const nsString& aPath, const int& aAccess, const int& aPermission, FileDescriptorResponse* aFd)
{
  AssertIsOnBackgroundThread();

  // realpath with NULL as second param does malloc()
  // @todo: problem with realpath is that when it doesnt exist it makes it NULL...
  // char* real_path = realpath(NS_LossyConvertUTF16toASCII(aPath).get(), NULL);
  const char* real_path = NS_LossyConvertUTF16toASCII(aPath).get();

  int fd = open(real_path, aAccess, aPermission);
  *aFd = *(new FileDescriptorResponse(FileDescriptor(fd), fd == -1 ? errno : 0));
  // free(real_path);

  return true;
}

bool
OsFileChannelParent::RecvStat(const nsString& aPath, StatWrapper* aRetval)
{
  AssertIsOnBackgroundThread();

  char* real_path = realpath(NS_LossyConvertUTF16toASCII(aPath).get(), NULL);

  int error = 0;
  struct stat sb;
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

  char* real_path = realpath(NS_LossyConvertUTF16toASCII(aPath).get(), NULL);

  int error = 0;
  struct stat sb;
  if (lstat(real_path, &sb) == -1) {
    error = errno;
  }

  *aRetval = *(new StatWrapper(sb, error));

  free(real_path);

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
