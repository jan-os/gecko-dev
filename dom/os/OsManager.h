/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_os_OsManager_h
#define mozilla_dom_os_OsManager_h

#include <list>
#include "File.h"
#include "mozilla/dom/POsFileManagerChild.h"
#include "mozilla/DOMEventTargetHelper.h"
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

  // remove this yo
  static already_AddRefed<OsManager> Constructor(GlobalObject& aGlobal,
                                                 ErrorResult& aRv);

  void Init();
  void Shutdown();

  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  /**
   * WebIDL Interface
   */

  // file operations
  already_AddRefed<File> Fopen(const nsAString& aPath, const nsAString& aMode, ErrorResult& aRv);
  int Fclose(File& aFile);

  // stat operations
  already_AddRefed<os::Stat> Stat(const nsAString& aPath, ErrorResult& aRv);
  already_AddRefed<os::Stat> Lstat(const nsAString& aPath, ErrorResult& aRv);

protected:
  virtual ~OsManager() {}

private:
  workers::WorkerGlobalScope* mScope;
  
  // I guess this should also be a nsRefPtr but the child doesn't implement it
  POsFileManagerChild* mOsFileManagerChild;
};

} // namespace os
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_os_OsManager_h
