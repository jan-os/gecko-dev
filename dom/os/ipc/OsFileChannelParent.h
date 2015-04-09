/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_OsFileChannelParent_h
#define mozilla_dom_OsFileChannelParent_h

#include "mozilla/dom/os/POsFileChannelParent.h"

namespace mozilla {

namespace ipc {
class BackgroundParentImpl;
}

namespace dom {
namespace os {

class OsFileChannelParent final : public POsFileChannelParent
{
  friend class mozilla::ipc::BackgroundParentImpl;

private:
  OsFileChannelParent();
  ~OsFileChannelParent();

  virtual bool RecvHello() override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;
};

} // os namespace
} // dom namespace
} // mozilla namespace

#endif // mozilla_dom_OsFileChannelParent_h
