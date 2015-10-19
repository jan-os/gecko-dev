/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_os_OsManager_h
#define mozilla_dom_os_OsManager_h

#include <fcntl.h>
#include <list>
#include "File.h"
#include "mozilla/dom/os/POsFileChannelChild.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "nsDirectoryServiceDefs.h"
#include "nsDirectoryServiceUtils.h"
#include "nsIFile.h"
#include "nsTArray.h"
#include "Stat.h"
#include "WorkerFeature.h"
#include "WorkerPrivate.h"
#include "WorkerScope.h"

class nsPIDOMWindow;
class nsIScriptContext;

namespace mozilla {
namespace dom {
namespace os {

using mozilla::dom::Date;

typedef void (*PermissionsCallback)(OsManager* aOsManager,
                                    const nsTArray<nsString>& aResult);

class OsManager final : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  explicit OsManager(workers::WorkerPrivate* aWorkerPrivate);

  void Init(ErrorResult& aRv);
  void Shutdown();

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;

  /**
   * WebIDL Interface
   */

  // file operations
  already_AddRefed<File> Open(const nsAString& aPath,
                              int32_t aAccess,
                              int32_t aPermission,
                              ErrorResult& aRv);

  void Read(JSContext* aCx,
            File& aFile,
            int32_t aBytes,
            JS::MutableHandle<JSObject*> aRet,
            ErrorResult& aRv);

  int32_t Write(File& aFile, const Uint8Array& buffer, ErrorResult& aRv);
  void Close(File& aFile, ErrorResult& aRv);

  void Chmod(const nsAString& aPath, int32_t aMode, ErrorResult& aRv);
  void Fchmod(const File& aFile, int32_t aMode, ErrorResult& aRv);

  void Unlink(const nsAString& aPath, ErrorResult& aRv);

  void Utimes(const nsAString& aPath, const Date& aActime,
              const Date& aModtime, ErrorResult& aRv);
  void Lutimes(const nsAString& aPath, const Date& aActime,
               const Date& aModtime, ErrorResult& aRv);
  void Futimes(File& aFile, const Date& aActime,
               const Date& aModtime, ErrorResult& aRv);

  void Truncate(const nsAString& aPath, int32_t aLength, ErrorResult& aRv);
  void Ftruncate(const File& aFile, int32_t aLength, ErrorResult& aRv);

  void Mkdir(const nsAString& aPath, int32_t aMode, ErrorResult& aRv);
  void Rmdir(const nsAString& aPath, ErrorResult& aRv);

  void Rename(const nsAString& aOldPath, const nsAString& aNewPath,
              ErrorResult& aRv);

  void Readdir(const nsAString& aPath, nsTArray<nsString>& aRetVal,
               ErrorResult& aRv);

  void Symlink(const nsAString& aPath1, const nsAString& aPath2,
               ErrorResult& aRv);
  void Readlink(const nsAString& aPath, nsAString& aRetVal, ErrorResult& aRv);

  // stat operations
  already_AddRefed<os::Stat> Stat(const nsAString& aPath, ErrorResult& aRv);
  already_AddRefed<os::Stat> Lstat(const nsAString& aPath, ErrorResult& aRv);
  already_AddRefed<os::Stat> Fstat(const File& aFile, ErrorResult& aRv);

  // Open flags
  int32_t RDONLY() const { return O_RDONLY; }
  int32_t WRONLY() const { return O_WRONLY; }
  int32_t RDWR() const { return O_RDWR; }
  int32_t APPEND() const { return O_APPEND; }
  int32_t CREAT() const { return O_CREAT; }
  int32_t DSYNC() const { return O_DSYNC; }
  int32_t EXCL() const { return O_EXCL; }
  int32_t NOCTTY() const { return O_NOCTTY; }
  int32_t NONBLOCK() const { return O_NONBLOCK; }
  int32_t SYNC() const { return O_SYNC; }
  int32_t TRUNC() const { return O_TRUNC; }

  // Permission flags
  int32_t ISUID() const { return S_ISUID; }
  int32_t ISGID() const { return S_ISGID; }
  int32_t ISVTX() const { return S_ISVTX; }
  int32_t IRUSR() const { return S_IRUSR; }
  int32_t IWUSR() const { return S_IWUSR; }
  int32_t IXUSR() const { return S_IXUSR; }
  int32_t IRGRP() const { return S_IRGRP; }
  int32_t IWGRP() const { return S_IWGRP; }
  int32_t IXGRP() const { return S_IXGRP; }
  int32_t IROTH() const { return S_IROTH; }
  int32_t IWOTH() const { return S_IWOTH; }
  int32_t IXOTH() const { return S_IXOTH; }
  int32_t IRWXU() const { return S_IRWXU; }
  int32_t IRWXG() const { return S_IRWXG; }
  int32_t IRWXO() const { return S_IRWXO; }

  void GetTEMP_DIR(nsString& aRetVal, ErrorResult& aRv)
  {
    nsCOMPtr<nsIFile> tmpDir;
    aRv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR,
                                 getter_AddRefs(tmpDir));
    if (NS_WARN_IF(aRv.Failed())) {
      return;
    }
    tmpDir->GetPath(aRetVal);
  }

protected:
  virtual ~OsManager() {}

private:
  void HandleErrno(int32_t aErr, ErrorResult& aRv);
  POsFileChannelChild* mActor;
  workers::WorkerGlobalScope* mScope;
  workers::WorkerPrivate* mWorkerPrivate;
};

} // namespace os
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_os_OsManager_h
