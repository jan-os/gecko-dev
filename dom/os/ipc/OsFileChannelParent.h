/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_OsFileChannelParent_h
#define mozilla_dom_OsFileChannelParent_h

#include "mozilla/dom/os/POsFileChannelParent.h"
#include "mozilla/ipc/FileDescriptor.h"
#include "StatSerializer.h"

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

  bool VerifyRights(const char* aPath);

  virtual bool RecvOpen(const nsString& aPath,
                        const int& aAccess,
                        const int& aPermission,
                        FileDescriptorResponse* aFd) override;

  virtual bool RecvLstat(const nsString& aPath, StatWrapper* aRetval) override;
  virtual bool RecvStat(const nsString& aPath, StatWrapper* aRetval) override;
  virtual bool RecvUnlink(const nsString& aPath, int* aRetval) override;

  virtual bool RecvChmod(const nsString& aPath,
                         const int& aPermission,
                         int* aRetval) override;

  virtual bool RecvUtimes(const nsString& aPath,
                          const double& aActime,
                          const double& aModtime,
                          int* aRetval) override;

  virtual bool RecvLutimes(const nsString& aPath,
                           const double& aActime,
                           const double& aModtime,
                           int* aRetval) override;

  virtual void ActorDestroy(ActorDestroyReason aWhy) override;
};

} // os namespace
} // dom namespace
} // mozilla namespace

#endif // mozilla_dom_OsFileChannelParent_h
