/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <limits>
#include "OsManager.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "nsIDOMClassInfo.h"
#include "mozilla/dom/OsManagerBinding.h"
#include <stdio.h>
#include <algorithm>

namespace mozilla {
namespace dom {
namespace os {

OsManager::OsManager(workers::WorkerGlobalScope* aScope)
  : DOMEventTargetHelper(static_cast<DOMEventTargetHelper*>(aScope))
{}

void
OsManager::Init()
{}

void
OsManager::Shutdown()
{}

JSObject*
OsManager::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return OsManagerBinding::Wrap(aCx, this, aGivenProto);
}

// ToNewCString ??

void
OsManager::Fopen(const nsAString& path, const nsAString& mode, DOMString& result)
{
  // realpath with NULL as second param does malloc()
  char* real_path = realpath(NS_LossyConvertUTF16toASCII(path).get(), NULL);

  FILE* file = fopen(real_path, NS_LossyConvertUTF16toASCII(mode).get());
  free(real_path);
  if (file == NULL) {
    result.SetNull();
    return;
  }

  char ptrStringBuffer[sizeof(size_t)];
  sprintf(ptrStringBuffer, "%p", file);

  this->valid_file_pointers.push_back((size_t)file);
  printf("fopen string is %s\n", ptrStringBuffer);
  
  result.SetOwnedString(NS_ConvertASCIItoUTF16(ptrStringBuffer));
}

int
OsManager::Fclose(const nsAString& ptr)
{
  const char* ptrstring = NS_LossyConvertUTF16toASCII(ptr).get();
  
  printf("fclose comes in %s\n", ptrstring);
  size_t realptr = (size_t)strtoull(ptrstring, const_cast<char**>(&ptrstring), 16);
  printf("realptr is %zu\n", realptr);
  
  std::list<size_t> ptrs = this->valid_file_pointers;

  // detect if ptr is valid
  if (std::find(ptrs.begin(), ptrs.end(), realptr) == ptrs.end()) {
    printf("Could not find the pointer\n");
    return EOF;
  }

  printf("gonna call fclose %p\n", (FILE*)realptr);

  return fclose((FILE*)realptr);
}

} // namespace os
} // namespace dom
} // namespace mozilla
