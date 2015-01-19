/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

/*global TextDecoder DOMRequestIpcHelper Components XPCOMUtils Services OS */

"use strict";

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/subprocess.jsm");

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

  readFile: function(path, encoding) {
    if (encoding === 'utf-8') {
      return this.readFileUtf8(path);
    }
    else if (encoding === 'binary') {
      return this.readFileBinary(path);
    }
    else {
      return this.createPromise((res, rej) => {
        rej('Encoding "' + encoding + '" not supported. Try utf-8 or binary.');
      });
    }
  },

  readFileBinary: function(path) {
    return this.createPromise((res, rej) => {
      OS.File.read(path).then(array => {
        res(Cu.cloneInto(array, this._window));
      }).catch(err => rej(err));
    });
  },

  readFileUtf8: function(path) {
    let decoder = new TextDecoder();

    return this.createPromise((res, rej) => {
      OS.File.read(path).then(array => {
        res(decoder.decode(array));
      }).catch(err => rej(err));
    });
  },

  writeFile: function(path, data, encoding) {
    return this.createPromise((res, rej) => {
      OS.File.writeAtomic(path, data, { encoding: encoding, flush: false })
        .then(() => res())
        .catch(rej);
    });
  },

  removeFile: function(path) {
    return this.createPromise((res, rej) => {
      OS.File.remove(path)
        .then(res)
        .catch(rej);
    });
  },

  appendFile: function(path, data, encoding) {
    return this.createPromise((res, rej) => {
      let file;

      OS.File.open(path, { write: true, append: true })
        .then(aFile => file = aFile)
        .then(() => {
          if (encoding === 'utf-8') {
            let encoder = new TextEncoder();
            return file.write(encoder.encode(data));
          }
          else if (encoding === 'binary') {
            return file.write(data);
          }
          else {
            throw 'Encoding "' + encoding + '" not supported. Try utf-8 or binary.';
          }
        })
        .then(() => file.close())
        .then(() => res())
        .catch(err => {
          if (file) {
            file.close();
          }
          rej(err);
        });
    });
  },
  
  createDirectory: function(path, ignoreExisting) {
    return this.createPromise((res, rej) => {
      OS.File.makeDir(path, { ignoreExisting: ignoreExisting })
        .then(res, rej);
    });
  },

  exec: function(path, args) {
    return this.createPromise((res, rej) => {
      try {
        subprocess.call({
          command:     path,
          arguments:   args || [],
          // environment: [ "XYZ=abc", "MYVAR=def" ],
          charset: 'UTF-8',
          // workdir: '/home/foo',
          //stdin: "some value to write to stdin\nfoobar",
          // stdin: function(stdin) {
          //   stdin.write("some value to write to stdin\nfoobar");
          //   stdin.close();
          // },
          done: function(result) {
            res({
              exitCode: result.exitCode,
              stdout: result.stdout,
              stderr: result.stderr
            });
          },
          mergeStderr: false
        });
      }
      catch (ex) {
        rej(ex);
      }
    });
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MozOs]);
