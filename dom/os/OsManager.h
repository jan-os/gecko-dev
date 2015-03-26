/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_os_OsManager_h
#define mozilla_dom_os_OsManager_h

#include "mozilla/DOMEventTargetHelper.h"
#include "../workers/WorkerFeature.h"
#include "../workers/WorkerPrivate.h"
#include "../workers/WorkerRunnable.h"
#include "../workers/WorkerScope.h"
#include <list>

class nsPIDOMWindow;
class nsIScriptContext;

namespace mozilla {
namespace dom {
namespace os {

class OsManager : public DOMEventTargetHelper
{
public:
  explicit OsManager(workers::WorkerGlobalScope* aScope);

  void Init();
  void Shutdown();

  /**
   * WebIDL Interface
   */
  virtual JSObject* WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;
  
  std::list<size_t> valid_file_pointers;
  
  // file operations
  void Fopen(const nsAString& path, const nsAString& mode, DOMString& result);
  int        Fclose(const nsAString& ptr);
};

} // namespace os
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_os_OsManager_h
