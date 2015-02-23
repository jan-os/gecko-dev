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
  
  Promise<MozOsFile> openFile(DOMString path, optional object mode, optional object options);
};

[JSImplementation="@mozilla.org/b2g-os-exec-response;1"]
interface MozOsExecResponse {
  readonly attribute long exitCode;
  readonly attribute DOMString stdout;
  readonly attribute DOMString stderr;
};

enum PositionOrigin {
  "POS_START",
  "POS_CUR",
  "POS_END"
};

[JSImplementation="@mozilla.org/b2g-os-file;1"]
interface MozOsFile {
  Promise<void> close();
  Promise<void> flush();
  Promise<long> getPosition();
  Promise<(DOMString or Uint8Array)> read(optional long bytes, optional DOMString encoding = "binary");
  Promise<void> setDates(DOMTimeStamp accessDate, DOMTimeStamp modificationDate);
  Promise<void> setPosition(long bytes, optional PositionOrigin origin = "POS_CUR");
  Promise<MozOsFileInfo> stat();
  Promise<long> write((Uint8Array or DOMString) data, optional DOMString encoding = "binary"); /* ArrayBufferView source, optional any options); */
};

[JSImplementation="@mozilla.org/b2g-os-fileinfo;1"]
interface MozOsFileInfo {
  readonly attribute boolean isDir;
  readonly attribute boolean isSymLink;
  readonly attribute long size;
  readonly attribute DOMTimeStamp lastAccessDate;
  readonly attribute DOMTimeStamp lastModificationDate;
};