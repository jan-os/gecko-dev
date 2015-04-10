/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "OsFileChannelParent.h"
#include "mozilla/ipc/BackgroundParent.h"
#include "mozilla/unused.h"

namespace mozilla {

using namespace ipc;

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
OsFileChannelParent::RecvOpen(const nsString& aPath, const int& aAccess, const int& aPermission, int* aFd)
{
  AssertIsOnBackgroundThread();

  // realpath with NULL as second param does malloc()
  char* real_path = realpath(NS_LossyConvertUTF16toASCII(aPath).get(), NULL);

  *aFd = open(real_path, aAccess, aPermission);
  free(real_path);

  return true; // -1 is valid return value here
}

bool
OsFileChannelParent::RecvLstat(const nsString& aPath, StatWrapper* aRetval)
{
  AssertIsOnBackgroundThread();

  char* real_path = realpath(NS_LossyConvertUTF16toASCII(aPath).get(), NULL);

  struct stat sb;
  lstat(real_path, &sb);

  *aRetval = *(new StatWrapper(sb));

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
