/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

/**
 * ==============================================
 * OS
 * ==============================================
 */
function MozOs() { }

MozOs.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  classID: Components.ID("{1c6eabab-d9a1-46ad-b9ad-8a4405ba5f3f}"),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIDOMGlobalPropertyInitializer,
    Ci.nsIObserver,
    Ci.nsISupportsWeakReference
  ]),

  init: function mozOsInit(win) {
    this._window = win;
    this.innerWindowID = win.QueryInterface(Ci.nsIInterfaceRequestor)
                            .getInterface(Ci.nsIDOMWindowUtils)
                            .currentInnerWindowID;
    this.initDOMRequestHelper(win, []);

    Services.obs.addObserver(this, "inner-window-destroyed", false);
  },

  uninit: function mozOsUninit() {
    this._window = null;
    this.destroyDOMRequestHelper();
    Services.obs.removeObserver(this, "inner-window-destroyed");
  },

  readFile: function mozReadFile(path) {
    let decoder = new TextDecoder();

    return this.createPromise((res, rej) => {
      OS.File.read(path).then(array => {
        res(decoder.decode(array));
      }).catch(err => rej(err));
    });
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MozOs]);
