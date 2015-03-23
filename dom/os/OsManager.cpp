/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>
#include "OsManager.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "nsIDOMClassInfo.h"
#include "mozilla/dom/OsManagerBinding.h"

namespace mozilla {
namespace dom {
namespace os {

OsManager::OsManager(workers::WorkerGlobalScope* aScope)
  : DOMEventTargetHelper(static_cast<DOMEventTargetHelper*>(aScope))
{}

void
OsManager::Init()
{}

void
OsManager::Shutdown()
{}

JSObject*
OsManager::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return OsManagerBinding::Wrap(aCx, this, aGivenProto);
}

long
OsManager::Hello()
{
  return 41;
}

} // namespace os
} // namespace dom
} // namespace mozilla
