/* -*- Mode: C++; tab-width: 8; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/* vim: set sw=4 ts=8 et tw=80 : */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_OsFileManagerParent_h
#define mozilla_dom_OsFileManagerParent_h

#include <stdio.h>
#include "mozilla/dom/POsFileManagerParent.h"

namespace mozilla {
namespace dom {

class OsFileManagerParent :
  public POsFileManagerParent
{
  virtual bool
  RecvFopen(const nsString& path,
            const nsString& mode,
            size_t* file_ptr);

  virtual void
  ActorDestroy(ActorDestroyReason aWhy);

  MOZ_IMPLICIT OsFileManagerParent();
  virtual ~OsFileManagerParent();
};

} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_OsFileManagerParent_h
