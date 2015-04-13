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
#include "Stat.h"
#include "WorkerFeature.h"
#include "WorkerPrivate.h"
#include "WorkerScope.h"

class nsPIDOMWindow;
class nsIScriptContext;

namespace mozilla {
namespace dom {
namespace os {

class OsManager final : public DOMEventTargetHelper
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  explicit OsManager(workers::WorkerGlobalScope* aScope);

  static already_AddRefed<OsManager> Constructor(GlobalObject& aGlobal,
                                                 ErrorResult& aRv);

  void Init();
  void Shutdown();

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  /**
   * WebIDL Interface
   */

  // file operations
  already_AddRefed<File> Open(const nsAString& aPath, int aAccess, int aPermission, ErrorResult& aRv);
  void Read(JSContext* aCx, File& aFile, int aBytes, JS::MutableHandle<JSObject*> aRet, ErrorResult& aRv);
  int Write(File& aFile, const Uint8Array& buffer, ErrorResult& aRv);
  int Close(File& aFile);

  // stat operations
  already_AddRefed<os::Stat> Stat(const nsAString& aPath, ErrorResult& aRv);
  already_AddRefed<os::Stat> Lstat(const nsAString& aPath, ErrorResult& aRv);
  already_AddRefed<os::Stat> Fstat(const File& aFile, ErrorResult& aRv);

  int RDONLY() const { return O_RDONLY; }
  int WRONLY() const { return O_WRONLY; }
  int RDWR() const { return O_RDWR; }
  int APPEND() const { return O_APPEND; }
  int CREAT() const { return O_CREAT; }
  int DSYNC() const { return O_DSYNC; }
  int EXCL() const { return O_EXCL; }
  int NOCTTY() const { return O_NOCTTY; }
  int NONBLOCK() const { return O_NONBLOCK; }
  int SYNC() const { return O_SYNC; }
  int TRUNC() const { return O_TRUNC; }

  int IWRITE() const { return S_IWRITE; }
  int IREAD() const { return S_IREAD; }

  void GetTEMP_DIR(nsString& aRetVal) {
    nsCOMPtr<nsIFile> tmpDir;
    nsresult rv = NS_GetSpecialDirectory(NS_OS_TEMP_DIR,
                                         getter_AddRefs(tmpDir));
    if (NS_FAILED(rv)) {
      return;
    }
    tmpDir->GetPath(aRetVal);
  }

protected:
  virtual ~OsManager() {
    // ? free(mActor) ?
  }

private:
  POsFileChannelChild* mActor;
  workers::WorkerGlobalScope* mScope;
};

} // namespace os
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_os_OsManager_h
