/* -*- Mode: IDL; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/.
 */

[JSImplementation="@mozilla.org/b2g-os;1",
 NavigatorProperty="mozOs"]
interface MozOs : EventTarget {
  Promise<(DOMString or Uint8Array)> readFile(DOMString path, optional DOMString encoding = "binary");
  Promise<void> writeFile(DOMString path, (Uint8Array or DOMString) data, optional DOMString encoding = "binary");
  Promise<MozOsExecResponse> exec(DOMString path, optional sequence<DOMString> args);
  Promise<void> removeFile(DOMString path);
  Promise<void> appendFile(DOMString path, (Uint8Array or DOMString) data, optional DOMString encoding = "binary");
  Promise<void> createDirectory(DOMString path, optional boolean ignoreExisting = true);
};

[JSImplementation="@mozilla.org/b2g-os-exec-response;1"]
interface MozOsExecResponse {
  readonly attribute long exitCode;
  readonly attribute DOMString stdout;
  readonly attribute DOMString stderr;
};
