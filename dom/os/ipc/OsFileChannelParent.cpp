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
OsFileChannelParent::RecvFopen(const nsString& aPath, const nsString& aMode, size_t* aFilePtr)
{
  AssertIsOnBackgroundThread();

  // realpath with NULL as second param does malloc()
  char* real_path = realpath(NS_LossyConvertUTF16toASCII(aPath).get(), NULL);
  printf("RecvFopen %s\n", real_path);

  FILE* file = fopen(real_path, NS_LossyConvertUTF16toASCII(aMode).get());
  free(real_path);

  *aFilePtr = (size_t)file;

  return file != NULL;
}

void
OsFileChannelParent::ActorDestroy(ActorDestroyReason aWhy)
{
  AssertIsOnBackgroundThread();
}

}
} // dom namespace
} // mozilla namespace
