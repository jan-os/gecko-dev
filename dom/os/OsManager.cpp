/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <algorithm>
#include <errno.h>
#include <limits>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <utime.h>
#include "jsapi.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/dom/OsManagerBinding.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/BackgroundUtils.h"
#include "mozilla/ipc/FileDescriptor.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "nsArrayUtils.h"
#include "nsIDOMClassInfo.h"
#include "nsIServiceManager.h"
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

/**
 * The os module consists of the OsManager (this file), which runs in a worker
 * and an IPC part (OsFileChannel), which runs in the background thread.
 * Whenever the OsManager wants to do an action against the filesystem,
 * it dispatches it to OsFileChannel over IPC. For operations that operate
 * on file descriptors (f.e. fread/fstat) we open the fd in the main thread
 * then pass it on to the worker over IPC (see FileDescriptorResponse in
 * POsFileChannel.ipdl). Then we can use the file descriptor directly from
 * the worker, and thus not introduce more IPC overhead for these type of
 * calls.
 */

OsManager::OsManager(workers::WorkerPrivate* aWorkerPrivate)
  : DOMEventTargetHelper((DOMEventTargetHelper*)aWorkerPrivate->GlobalScope()),
    mScope(aWorkerPrivate->GlobalScope()),
    mWorkerPrivate(aWorkerPrivate)
{
  MOZ_ASSERT(aWorkerPrivate);
  aWorkerPrivate->AssertIsOnWorkerThread();

  ipc::PBackgroundChild* backgroundChild =
    ipc::BackgroundChild::GetForCurrentThread();
  mActor = backgroundChild->SendPOsFileChannelConstructor();

  MOZ_ASSERT(mActor);
}

void
OsManager::Init()
{
  auto principal = mWorkerPrivate->GetPrincipal();
  uint32_t appId;
  nsresult rv = principal->GetAppId(&appId);
  if (NS_FAILED(rv)) {
    return; // @todo: how do we proper handle failure here?
  }

  bool initValue = mActor->SendInit(appId);
  // So this is only in debug builds, right? But it should always halt when this fails...
  MOZ_ASSERT(initValue == true);
}

JSObject*
OsManager::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return OsManagerBinding::Wrap(aCx, this, aGivenProto);
}

void
OsManager::HandleErrno(int aErr, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

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
OsManager::Open(const nsAString& aPath, int aAccess, int aPermission,
                ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  FileDescriptorResponse fdr = {};
  bool ret = mActor->SendOpen((nsString)aPath, aAccess, aPermission, &fdr);
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
OsManager::Read(JSContext* aCx, File& aFile, int aBytes,
                JS::MutableHandle<JSObject*> aRet, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  nsAutoArrayPtr<char> buffer(new (fallible) char[aBytes + 1]);
  if (!buffer) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }

  size_t bytes_read = read(aFile.GetFd(), buffer, aBytes);
  if (bytes_read == (size_t)-1) {
    HandleErrno(errno, aRv);
    return;
  }

  JSObject* outView = Uint8Array::Create(aCx, bytes_read,
    reinterpret_cast<uint8_t*>(buffer.get()));

  if (!outView) {
    aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }

  aRet.set(outView);
}

int
OsManager::Write(File& aFile, const Uint8Array& aBuffer, ErrorResult &aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  aBuffer.ComputeLengthAndData();
  int ret = write(aFile.GetFd(), aBuffer.Data(), aBuffer.Length());
  if (ret == -1) {
    HandleErrno(errno, aRv);
    return -1;
  }
  return ret;
}

void
OsManager::Close(File& aFile, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int cr = close(aFile.GetFd());
  if (cr == -1) {
    HandleErrno(errno, aRv);
  }
}

already_AddRefed<os::Stat>
OsManager::Lstat(const nsAString& aPath, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  nsAutoPtr<StatWrapper> sw(new StatWrapper());
  bool ret = mActor->SendLstat((nsString)aPath, sw);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  if (sw->GetError() != 0) {
    HandleErrno(sw->GetError(), aRv);
    return nullptr;
  }

  nsRefPtr<os::Stat> stat = new os::Stat(this, sw->GetWrappedObject());

  return stat.forget();
}

already_AddRefed<os::Stat>
OsManager::Stat(const nsAString& aPath, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  nsAutoPtr<StatWrapper> sw(new StatWrapper());
  bool ret = mActor->SendStat((nsString)aPath, sw);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  if (sw->GetError() != 0) {
    HandleErrno(sw->GetError(), aRv);
    return nullptr;
  }

  nsRefPtr<os::Stat> stat = new os::Stat(this, sw->GetWrappedObject());

  return stat.forget();
}

already_AddRefed<os::Stat>
OsManager::Fstat(const File& aFile, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  struct stat sb;
  if (fstat(aFile.GetFd(), &sb) == -1) {
    HandleErrno(errno, aRv);
    return nullptr;
  }

  nsRefPtr<os::Stat> stat = new os::Stat(this, sb);

  return stat.forget();
}

void
OsManager::Chmod(const nsAString& aPath, int aMode, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int rv;
  bool ret = mActor->SendChmod((nsString)aPath, aMode, &rv);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }
  if (rv != 0) {
    HandleErrno(rv, aRv);
  }
}

void
OsManager::Fchmod(const File& aFile, int aMode, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int cr = fchmod(aFile.GetFd(), aMode);
  if (cr == -1) {
    HandleErrno(errno, aRv);
  }
}

void
OsManager::Unlink(const nsAString& aPath, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int rv;
  bool ret = mActor->SendUnlink((nsString)aPath, &rv);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }
  if (rv != 0) {
    HandleErrno(rv, aRv);
  }
}

void
OsManager::Utimes(const nsAString& aPath, const Date& aActime,
                  const Date& aModtime, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int rv;
  bool ret = mActor->SendUtimes((nsString)aPath, aActime.TimeStamp(),
                                aModtime.TimeStamp(), &rv);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }
  if (rv != 0) {
    HandleErrno(rv, aRv);
  }
}

void
OsManager::Lutimes(const nsAString& aPath, const Date& aActime,
                   const Date& aModtime, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int rv;
  bool ret = mActor->SendLutimes((nsString)aPath, aActime.TimeStamp(),
                                 aModtime.TimeStamp(), &rv);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }
  if (rv != 0) {
    HandleErrno(rv, aRv);
  }
}

void
OsManager::Futimes(File& aFile, const Date& aActime,
                   const Date& aModtime, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  struct timeval tv[2];
  tv[0] = {
    .tv_sec = ((time_t)floor(aActime.TimeStamp())) / 1000,
    .tv_usec = (suseconds_t)(((long)floor(aActime.TimeStamp())) % 1000) * 1000
  };
  tv[1] = {
    .tv_sec = ((time_t)floor(aModtime.TimeStamp())) / 1000,
    .tv_usec = (suseconds_t)(((long)floor(aModtime.TimeStamp())) % 1000) * 1000
  };
  int fr = futimes(aFile.GetFd(), tv);
  if (fr == -1) {
    HandleErrno(errno, aRv);
  }
}

void
OsManager::Truncate(const nsAString& aPath, int aLength, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int rv;
  bool ret = mActor->SendTruncate((nsString)aPath, aLength, &rv);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }
  if (rv != 0) {
    HandleErrno(rv, aRv);
  }
}

void
OsManager::Ftruncate(const File& aFile, int aLength, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int cr = ftruncate(aFile.GetFd(), aLength);
  if (cr == -1) {
    HandleErrno(errno, aRv);
  }
}

void
OsManager::Mkdir(const nsAString& aPath, int aMode, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int rv;
  bool ret = mActor->SendMkdir((nsString)aPath, aMode, &rv);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }
  if (rv != 0) {
    HandleErrno(rv, aRv);
  }
}

void
OsManager::Rmdir(const nsAString& aPath, ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int rv;
  bool ret = mActor->SendRmdir((nsString)aPath, &rv);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }
  if (rv != 0) {
    HandleErrno(rv, aRv);
  }
}

void
OsManager::Rename(const nsAString& aOldPath, const nsAString& aNewPath,
                  ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int rv;
  bool ret = mActor->SendRename((nsString&)aOldPath, (nsString&)aNewPath, &rv);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }
  if (rv != 0) {
    HandleErrno(rv, aRv);
  }
}

void
OsManager::Readdir(const nsAString& aPath, nsTArray<nsString>& aRetVal,
                   ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  ReaddirResponse rdr = {};
  bool ret = mActor->SendReaddir((nsString)aPath, &rdr);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  if (rdr.error() != 0) {
    HandleErrno(rdr.error(), aRv);
    return;
  }

  aRetVal = rdr.files();
}


void
OsManager::Symlink(const nsAString& aPath1, const nsAString& aPath2,
                   ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  int rv;
  bool ret = mActor->SendSymlink((nsString)aPath1, (nsString)aPath2, &rv);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }
  if (rv != 0) {
    HandleErrno(rv, aRv);
  }
}

void
OsManager::Readlink(const nsAString& aPath, nsAString& aRetVal,
                    ErrorResult& aRv)
{
  mWorkerPrivate->AssertIsOnWorkerThread();

  ReadlinkResponse rlr = {};
  bool ret = mActor->SendReadlink((nsString)aPath, &rlr);
  if (!ret) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  if (rlr.error() != 0) {
    HandleErrno(rlr.error(), aRv);
    return;
  }

  aRetVal = rlr.link();
}

} // namespace os
} // namespace dom
} // namespace mozilla
