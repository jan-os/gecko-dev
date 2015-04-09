/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_OsFileChannelChild_h
#define mozilla_dom_OsFileChannelChild_h

#include "mozilla/dom/os/OsManager.h"
#include "mozilla/dom/os/POsFileChannelChild.h"

namespace mozilla {

namespace ipc {
class BackgroundChildImpl;
}

namespace dom {
namespace os {

class OsFileChannelChild final : public POsFileChannelChild
{
  friend class mozilla::ipc::BackgroundChildImpl;

public:
  NS_INLINE_DECL_REFCOUNTING(OsFileChannelChild)

  void SetParent(OsManager* aOsManager)
  {
    mOsManager = aOsManager;
  }

  bool IsActorDestroyed() const
  {
    return mActorDestroyed;
  }

private:
  explicit OsFileChannelChild();
  ~OsFileChannelChild();

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;

  // This raw pointer is actually the parent object.
  // It's set to null when the parent object is deleted.
  OsManager* mOsManager;

  bool mActorDestroyed;
};

} // os namespace
} // dom namespace
} // mozilla namespace

#endif // mozilla_dom_OsFileChannelChild_h
