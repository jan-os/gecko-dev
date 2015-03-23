/* vim: set shiftwidth=2 tabstop=8 autoindent cindent expandtab: */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "File.h"
#include "mozilla/dom/OsManagerBinding.h"

namespace mozilla {
namespace dom {
namespace os {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(File, mParent)

NS_IMPL_CYCLE_COLLECTION_ROOT_NATIVE(File, AddRef)
NS_IMPL_CYCLE_COLLECTION_UNROOT_NATIVE(File, Release)

} // namespace os
} // namespace dom
} // namespace mozilla
