/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* vim: set sw=4 ts=8 et tw=80 : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "mozilla/unused.h"
#include "nsIWidget.h"
#include "nsServiceManagerUtils.h"
#include "OsFileManagerParent.h"

namespace mozilla {
namespace dom {

bool
OsFileManagerParent::RecvFopen(
        const nsString& aPath,
        const nsString& aMode,
        size_t* file_ptr)
{
  printf("RecvFopen\n");

  char* real_path = realpath(NS_LossyConvertUTF16toASCII(aPath).get(), NULL);
  printf("RecvFopen %s\n", real_path);

  FILE* file = fopen(real_path, NS_LossyConvertUTF16toASCII(aMode).get());
  free(real_path);

  *file_ptr = (size_t)file;

  return file != NULL;
}

void
OsFileManagerParent::ActorDestroy(ActorDestroyReason aWhy)
{
}

MOZ_IMPLICIT OsFileManagerParent::OsFileManagerParent()
{
  MOZ_COUNT_CTOR(OsFileManagerParent);
}

MOZ_IMPLICIT OsFileManagerParent::~OsFileManagerParent()
{
  MOZ_COUNT_DTOR(OsFileManagerParent);
}

} // namespace dom
} // namespace mozilla
