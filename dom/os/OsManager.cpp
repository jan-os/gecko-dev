/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <limits>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "jsapi.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/OsManagerBinding.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/BackgroundUtils.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "nsIDOMClassInfo.h"
#include "OsManager.h"

namespace mozilla {
namespace dom {
namespace os {

NS_IMPL_ADDREF_INHERITED(OsManager, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(OsManager, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN(OsManager)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

OsManager::OsManager(workers::WorkerGlobalScope* aScope)
  : DOMEventTargetHelper(static_cast<DOMEventTargetHelper*>(aScope)),
    mScope(aScope)
{
  ipc::PBackgroundChild* backgroundChild = ipc::BackgroundChild::GetForCurrentThread();
  mActor = backgroundChild->SendPOsFileChannelConstructor();

  MOZ_ASSERT(mActor);
}

already_AddRefed<OsManager>
OsManager::Constructor(GlobalObject& aGlobal, ErrorResult& aRv)
{
  // We don't allow Gecko to create OsManager through JS codes like
  // window.OsManager() on the worker, so disable this for now.
  NS_NOTREACHED("Cannot use the chrome constructor on the worker!");
  return nullptr;
}

JSObject*
OsManager::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return OsManagerBinding::Wrap(aCx, this, aGivenProto);
}

already_AddRefed<File>
OsManager::Fopen(const nsAString& aPath, const nsAString& aMode, ErrorResult &aRv)
{
  size_t file_ptr = 0;
  bool ret = mActor->SendFopen((nsString&)aPath, (nsString&)aMode, &file_ptr);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<File> file = new File(this, (FILE*)file_ptr);
  return file.forget();
}

void
OsManager::Fread(JSContext* aCx, int aBytes, File& aFile, JS::MutableHandle<JSObject*> aRet, ErrorResult& aRv)
{
  unsigned char* buffer = (unsigned char*)malloc(aBytes + 1);
  size_t bytes_read = fread(buffer, 1, aBytes, aFile.GetFilePtr());
  
  buffer[bytes_read] = '\0';
  printf("read data from file '%s'\n", buffer);

  JSObject* outView = nullptr;
  outView = Uint8Array::Create(aCx, bytes_read, buffer);

  free(buffer);

  if (!outView) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }

  aRet.set(outView);
}

int
OsManager::Fclose(File& aFile)
{
  return fclose(aFile.GetFilePtr());
}

already_AddRefed<os::Stat>
OsManager::Lstat(const nsAString& aPath, ErrorResult &aRv)
{
  char* real_path = realpath(NS_LossyConvertUTF16toASCII(aPath).get(), NULL);

  struct stat sb;
  if (lstat(real_path, &sb) != 0) {
    // todo: make a new error type
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<os::Stat> stat = new os::Stat(this, sb);
  return stat.forget();
}

already_AddRefed<os::Stat>
OsManager::Stat(const nsAString& aPath, ErrorResult &aRv)
{
  char* real_path = realpath(NS_LossyConvertUTF16toASCII(aPath).get(), NULL);

  struct stat sb;
  if (stat(real_path, &sb) != 0) {
    // todo: make a new error type
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<os::Stat> stat = new os::Stat(this, sb);
  return stat.forget();
}

} // namespace os
} // namespace dom
} // namespace mozilla
