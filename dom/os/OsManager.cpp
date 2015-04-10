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
#include "StatSerializer.h"

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
OsManager::Open(const nsAString& aPath, int aAccess, int aPermission, ErrorResult &aRv)
{
  // Where are we gonna do the security checks, here or in OsFileChannelParent

  int fd = 0;
  bool ret = mActor->SendOpen((nsString&)aPath, aAccess, aPermission, &fd);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<File> file = new File(this, fd);
  return file.forget();
}

void
OsManager::Read(JSContext* aCx, File& aFile, int aBytes, JS::MutableHandle<JSObject*> aRet, ErrorResult& aRv)
{
  unsigned char* buffer = (unsigned char*)malloc(aBytes + 1);
  size_t bytes_read = read(aFile.GetFd(), buffer, aBytes);

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
OsManager::Write(File& aFile, const Uint8Array& buffer, int aBytes)
{
  return 0;
}

int
OsManager::Close(File& aFile)
{
  return close(aFile.GetFd());
}

already_AddRefed<os::Stat>
OsManager::Lstat(const nsAString& aPath, ErrorResult &aRv)
{
  StatWrapper* sw = new StatWrapper();
  mActor->SendLstat((nsString&)aPath, sw);

  nsRefPtr<os::Stat> stat = new os::Stat(this, sw->GetWrappedObject());

  free(sw);

  return stat.forget();
}

already_AddRefed<os::Stat>
OsManager::Stat(const nsAString& aPath, ErrorResult &aRv)
{
  int fd = 0;
  mActor->SendOpen((nsString&)aPath, O_RDONLY, S_IREAD, &fd);

  if (fd == -1) {
    aRv.Throw(NS_ERROR_FAILURE); // file does not exist
    return nullptr;
  }

  struct stat sb;
  if (fstat(fd, &sb) != 0) {
    close(fd);
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  close(fd);

  nsRefPtr<os::Stat> stat = new os::Stat(this, sb);
  return stat.forget();
}

} // namespace os
} // namespace dom
} // namespace mozilla
