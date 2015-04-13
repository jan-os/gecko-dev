/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <errno.h>
#include <limits>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "jsapi.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/OsManagerBinding.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/BackgroundUtils.h"
#include "mozilla/ipc/FileDescriptor.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "nsIDOMClassInfo.h"
#include "OsManager.h"
#include "StatSerializer.h"

namespace mozilla {
namespace dom {
namespace os {

using mozilla::ipc::FileDescriptor;

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

JSObject*
OsManager::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return OsManagerBinding::Wrap(aCx, this, aGivenProto);
}

void
OsManager::HandleErrno(int aErr, ErrorResult& aRv)
{
  AutoJSAPI jsapi;
  if (!jsapi.Init(mScope)) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }
  auto cx = jsapi.cx();

  JSString* strErr = JS_NewStringCopyZ(cx, strerror(aErr));
  JS::Rooted<JS::Value> valErr(cx, STRING_TO_JSVAL(strErr));
  aRv.ThrowJSException(cx, valErr);
  return;
}

already_AddRefed<File>
OsManager::Open(const nsAString& aPath, int aAccess, int aPermission, ErrorResult& aRv)
{
  aRv.MightThrowJSException();
  // Where are we gonna do the security checks, here or in OsFileChannelParent?

  FileDescriptorResponse fdr = {};
  bool ret = mActor->SendOpen((nsString&)aPath, aAccess, aPermission, &fdr);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  if (fdr.error() != 0) {
    HandleErrno(fdr.error(), aRv);
    return nullptr;
  }

  nsRefPtr<File> file = new File(this, fdr.fd().PlatformHandle());
  return file.forget();
}

void
OsManager::Read(JSContext* aCx, File& aFile, int aBytes, JS::MutableHandle<JSObject*> aRet, ErrorResult& aRv)
{
  unsigned char* buffer = (unsigned char*)malloc(aBytes + 1);
  size_t bytes_read = read(aFile.GetFd(), buffer, aBytes);
  if (bytes_read == (size_t)-1) {
    HandleErrno(errno, aRv);
    return;
  }

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
OsManager::Write(File& aFile, const Uint8Array& aBuffer, ErrorResult &aRv)
{
  aBuffer.ComputeLengthAndData();
  int ret = write(aFile.GetFd(), aBuffer.Data(), aBuffer.Length());
  if (ret == -1) {
    HandleErrno(errno, aRv);
    return -1;
  }
  return ret;
}

int
OsManager::Close(File& aFile, ErrorResult& aRv)
{
  if (close(aFile.GetFd()) == -1) {
    HandleErrno(errno, aRv);
    return -1;
  }
  return 0;
}

already_AddRefed<os::Stat>
OsManager::Lstat(const nsAString& aPath, ErrorResult& aRv)
{
  StatWrapper* sw = new StatWrapper();
  bool ret = mActor->SendLstat((nsString&)aPath, sw);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  if (sw->GetError() != 0) {
    HandleErrno(sw->GetError(), aRv);
    return nullptr;
  }

  nsRefPtr<os::Stat> stat = new os::Stat(this, sw->GetWrappedObject());

  free(sw);

  return stat.forget();
}

already_AddRefed<os::Stat>
OsManager::Stat(const nsAString& aPath, ErrorResult& aRv)
{
  StatWrapper* sw = new StatWrapper();
  bool ret = mActor->SendStat((nsString&)aPath, sw);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  if (sw->GetError() != 0) {
    HandleErrno(sw->GetError(), aRv);
    return nullptr;
  }

  nsRefPtr<os::Stat> stat = new os::Stat(this, sw->GetWrappedObject());

  free(sw);

  return stat.forget();
}

already_AddRefed<os::Stat>
OsManager::Fstat(const File& aFile, ErrorResult& aRv)
{
  struct stat sb;
  if (fstat(aFile.GetFd(), &sb) == -1) {
    HandleErrno(errno, aRv);
    return nullptr;
  }

  nsRefPtr<os::Stat> stat = new os::Stat(this, sb);

  return stat.forget();
}

} // namespace os
} // namespace dom
} // namespace mozilla
