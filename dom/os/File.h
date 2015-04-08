/* vim: set shiftwidth=2 tabstop=8 autoindent cindent expandtab: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_os_File_h
#define mozilla_dom_os_File_h

#include "mozilla/dom/OsManagerBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "OsManager.h"

struct JSContext;

namespace mozilla {
namespace dom {
namespace os {

class File final : public nsWrapperCache
{
public:
  File(OsManager* aParent, FILE* aFile)
    : mParent(aParent)
    , mFile(aFile)
  {}

  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(File)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(File)

  nsRefPtr<OsManager> GetParentObject() const { return mParent; }
  
  JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
  {
    return OsManagerFileBinding::Wrap(aCx, this, aGivenProto);
  }
  
  FILE* GetFilePtr() const
  {
    return mFile;
  }

private:
  ~File() { }
  nsRefPtr<OsManager> mParent;
  FILE* mFile;
};

} // namespace os
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_os_File_h
