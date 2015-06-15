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

  bool VerifyRights(char* aPath);

  virtual bool RecvInit(const int& aAppId);

  virtual bool RecvOpen(const nsString& aPath,
                        const int& aAccess,
                        const int& aPermission,
                        FileDescriptorResponse* aFd) override;

  virtual bool RecvLstat(const nsString& aPath, StatWrapper* aRetVal) override;
  virtual bool RecvStat(const nsString& aPath, StatWrapper* aRetVal) override;
  virtual bool RecvUnlink(const nsString& aPath, int* aRetVal) override;

  virtual bool RecvChmod(const nsString& aPath,
                         const int& aPermission,
                         int* aRetVal) override;

  virtual bool RecvUtimes(const nsString& aPath,
                          const double& aActime,
                          const double& aModtime,
                          int* aRetVal) override;

  virtual bool RecvLutimes(const nsString& aPath,
                           const double& aActime,
                           const double& aModtime,
                           int* aRetVal) override;

  virtual bool RecvTruncate(const nsString& aPath,
                            const int& aLength,
                            int* aRetVal) override;

  virtual bool RecvMkdir(const nsString& aPath,
                         const int& aMode,
                         int* aRetVal) override;

  virtual bool RecvRmdir(const nsString& aPath, int* aRetVal) override;

  virtual bool RecvRename(const nsString& aOldPath,
                          const nsString& aNewPath,
                          int* aRetVal) override;

  virtual bool RecvReaddir(const nsString& aPath,
                           ReaddirResponse* aRetVal) override;

  virtual bool RecvSymlink(const nsString& aPath1,
                           const nsString& aPath2,
                           int* aRetVal) override;

  virtual bool RecvReadlink(const nsString& aPath,
                            ReadlinkResponse* aRetVal) override;

  bool mInitialized = false;
  nsTArray<nsString> mAllowedPaths;
};

} // os namespace
} // dom namespace
} // mozilla namespace

#endif // mozilla_dom_OsFileChannelParent_h
