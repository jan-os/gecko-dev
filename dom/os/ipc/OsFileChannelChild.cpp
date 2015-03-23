/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "OsFileChannelChild.h"
#include "jsapi.h"
#include "mozilla/dom/MessageEvent.h"
#include "mozilla/dom/MessageEventBinding.h"
#include "mozilla/dom/WorkerPrivate.h"
#include "mozilla/dom/WorkerScope.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "WorkerPrivate.h"

namespace mozilla {

using namespace ipc;

namespace dom {
namespace os {

using namespace workers;

OsFileChannelChild::OsFileChannelChild()
  : mActorDestroyed(false)
{
}

OsFileChannelChild::~OsFileChannelChild()
{
  MOZ_ASSERT(!mOsManager);
}

void
OsFileChannelChild::ActorDestroy(ActorDestroyReason aWhy)
{
  mActorDestroyed = true;
}

} // os namespace
} // dom namespace
} // mozilla namespace
