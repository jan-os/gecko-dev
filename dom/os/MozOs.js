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
Cu.import("resource://gre/modules/ctypes.jsm");

/**
 * ctypes declarations
 */
const FILE = new ctypes.StructType("FILE");

let gXulRuntime = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULRuntime);

function getPlatformValue(valueType) {
  const platformDefaults = {
    // Windows API:
    'winnt':   [ 'kernel32.dll' ],

    // Unix API:
    //            library name   O_NONBLOCK RLIM_T                RLIMIT_NOFILE
    'darwin':  [ 'libc.dylib',   0x04     , ctypes.uint64_t     , 8 ],
    'linux':   [ 'libc.so.6',    2024     , ctypes.unsigned_long, 7 ],
    'freebsd': [ 'libc.so.7',    0x04     , ctypes.int64_t      , 8 ],
    'openbsd': [ 'libc.so.61.0', 0x04     , ctypes.int64_t      , 8 ],
    'sunos':   [ 'libc.so',      0x80     , ctypes.unsigned_long, 5 ],
    'android': [ 'libc.so',      2024     , ctypes.unsigned_long, 7 ]
  };

  return platformDefaults[gXulRuntime.OS.toLowerCase()][valueType];
}

let libc = ctypes.open(getPlatformValue(0));

let fseek = libc.declare("fseek",
                          ctypes.default_abi,
                          ctypes.int,
                          FILE.ptr,
                          ctypes.int,
                          ctypes.int);
                          
let fopen = libc.declare("fopen",
                          ctypes.default_abi,
                          FILE.ptr,
                          ctypes.char.ptr,
                          ctypes.char.ptr);
                                
let fread = libc.declare("fread",
                        ctypes.default_abi,
                        ctypes.int,
                        ctypes.char.ptr,
                        ctypes.int,
                        ctypes.int,
                        FILE.ptr);
                        
// size_t fwrite(const void *ptr, size_t size, size_t nmemb, FILE *stream)
let fwrite = libc.declare("fwrite",
                        ctypes.default_abi,
                        ctypes.int,
                        ctypes.char.ptr,
                        ctypes.int,
                        ctypes.int,
                        FILE.ptr);

let fclose = libc.declare("fclose",
                       ctypes.default_abi,
                       ctypes.int,
                       FILE.ptr);

/**
 * ==============================================
 * OS
 * ==============================================
 */
function MozOs() {

}

MozOs.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  libc: null,

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

  openFile: function(path, mode, options) {
    return this.createPromise((res, rej) => {
      // @todo fix modes to be an object
      let fd = fopen(path, mode || 'rb+');

      if (!fd || fd.isNull()) {
        dump('ERR: Could not open file\n');
        return rej('Could not open file "' + path + '"');
      }

      let osFile = new MozOsFile(fd);
      osFile.init(this._window);
      let wrappedOsFile = this._window.MozOsFile._create(this._window, osFile);
      res(wrappedOsFile);

      // OS.File.open(path, mode, options)
      //   .then(file => {
      //     let osFile = new MozOsFile(file);
      //     osFile.init(this._window);
      //     let wrappedOsFile = this._window.MozOsFile._create(this._window, osFile);
      //     res(wrappedOsFile);
      //   }, rej);
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
  },
};

 /**
 * ==============================================
 * InputContext
 * ==============================================
 */
// function MozOsFile(file) {
//   this._file = file;
// }
function MozOsFile(fd) {
  this.fd = fd;
}

MozOsFile.prototype = {
  __proto__: DOMRequestIpcHelper.prototype,

  _file: null,
  fd: null,
  libc: null,
  _window: null,
  _closed: false,

  classID: Components.ID("{c42e6848-b8b1-45cf-a127-6df725cff842}"),

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIObserver,
    Ci.nsISupportsWeakReference
  ]),

  // wrapFileMethod: function(functionName) {
  //   let self = this;

  //   return function() {
  //     let args = arguments;

  //     return self.createPromise((res, rej) => {
  //       self._file[functionName].apply(self._file, args).then((v) => {
  //         dump(functionName + ' OK ' + v + '\n');
  //         res(Cu.cloneInto(v, self._window));
  //       }, err => {
  //         dump(functionName + ' NOK ' + err + '\n');
  //         rej(err);
  //       });
  //     });
  //   };
  // },

  init: function(win) {
    this._window = win;
    this._utils = win.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindowUtils);
    this.initDOMRequestHelper(win, []);

    // this.close = this.wrapFileMethod('close');
    // this.flush = this.wrapFileMethod('flush');
    // this.getPosition = this.wrapFileMethod('getPosition');
    // this.setDates = this.wrapFileMethod('setDates');
  },

  destroy: function() {
    this.destroyDOMRequestHelper();

    this._window = null;
    
    if (!this._closed) {
      this.close();
    }
  },

  getPosition: function() {
    throw 'Not implemented';
  },

  setPosition: function() {
    throw 'Not implemented';
  },

  setDates: function() {
    throw 'Not implemented';
  },

  flush: function() {
    throw 'Not implemented';
  },

  read: function(maxBytes, encoding) {
    return this.createPromise((res, rej) => {
      try {
        var n = Date.now();
        
        let buffers = [];
        let totalSize = 0;
        let bufferSize = 32 * 1024; // 32KB

        while (true) {
          let charArray = ctypes.ArrayType(ctypes.char);
          let readBuffer = new charArray(bufferSize);
          let bytesRead = fread(readBuffer, 1, bufferSize, this.fd);

          let newBuffer = new Uint8Array(bytesRead);
          // newBuffer.set does not work here because the src array is bigger
          for (let ix = 0; ix < bytesRead; ix++)
            newBuffer[ix] = readBuffer[ix];

          buffers.push(newBuffer);

          totalSize += bytesRead;

          if (bytesRead !== bufferSize || totalSize >= maxBytes) {
            // we're done
            break;
          }
        }

        if (totalSize > maxBytes) {
          totalSize = maxBytes;
        }

        // Copy value of all buffers into returnBuffer
        let returnBuffer = new Uint8Array(totalSize);
        let bytesWritten = 0;

        (function() {
          for (let bix = 0; bix < buffers.length; bix++) {
            let buffer = buffers[bix];

            for (let ix = 0; ix < buffer.length; ix++) {
              returnBuffer[bytesWritten] = buffer[ix];
              if (++bytesWritten === totalSize) {
                return;
              }
            }
          }
        })();

        if (encoding === 'utf-8') {
          returnBuffer = new TextDecoder().decode(returnBuffer);
        }
        
        dump('read took ' + (Date.now() - n) + ' ms.\n');
        
        var n = Date.now();
        res(Cu.cloneInto(returnBuffer, this._window));
        dump('cloneInto took ' + (Date.now() - n) + ' ms\n');
      }
      catch (ex) {
        dump('ERR: fread fails ' + ex + '\n');
        rej(ex);
      }
    });
  },

  write: function(data, encoding) {
    return this.createPromise((res, rej) => {
      try {
        var n = Date.now();
        let charArray = ctypes.ArrayType(ctypes.char);
        if (encoding === 'utf-8') {
          data = new TextEncoder().encode(data);
        }

        let buffer = new charArray(data.length);
        for (let i = 0; i < data.length; i++) {
          buffer[i] = data[i];
        }

        let written = fwrite(buffer, 1, data.length, this.fd);
        if (written !== data.length) {
          dump('ERR: fwrite wrote ' + written + ' but should be ' + data.length + '\n');
          return rej('fwrite wrote ' + written + ' but should be ' + data.length);
        }
        res();
        dump('write took ' + (Date.now() - n) + ' ms\n');
      }
      catch (ex) {
        dump('ERR: fwrite failed ' + ex + '\n');
        rej(ex);
      }
    });
  },

  setPosition: function(position, origin) {
    return this.createPromise((res, rej) => {
      var n = Date.now();
      
      let sk;
      switch (origin) {
        case 'POS_START':
          sk = OS.Constants.libc.SEEK_SET;
          break;
        case 'POS_END':
          sk = OS.Constants.libc.SEEK_END;
          break;
        case 'POS_CUR':
        default:
          sk = OS.Constants.libc.SEEK_CUR;
          break;
      }

      try {
        let ret = fseek(this.fd, position, sk);
        if (ret !== 0) {
          dump('ERR: fseek returned ' + ret + '\n');
          rej(ret);
        }
        else {
          res();
        }
        dump('setPosition took ' + (Date.now() - n) + ' ms\n');
      }
      catch(ex) {
        dump('ERR: setPosition failed ' + ex + '\n');
        rej(ex);
      }
    });
  },

  close: function() {
    return this.createPromise((res, rej) => {
      try {
        let c = fclose(this.fd);
        
        // @todo: do I need to check return code?
        // when writing to /sys/class/gpio/export it gives -1
        // if (c !== 0) {
        //   dump('ERR: fclose returned ' + c + '\n');
        //   return rej('fclose failed ' + c);
        // }
        res();
        
        this._closed = true;
      }
      catch (ex) {
        dump('ERR: fclose failed ' + ex + '\n');
        rej(ex);
      }
    });
  },

  // write: function(data, encoding) {
  //   if (encoding === 'utf-8') {
  //     let encoder = new TextEncoder();
  //     return this.createPromise((res, rej) => {
  //       this._file.write(encoder.encode(data)).then(res, rej);
  //     });
  //   }
  //   else if (encoding === 'binary') {
  //     return this.createPromise((res, rej) => {
  //       this._file.write(data).then(res, rej);
  //     });
  //   }
  //   else {
  //     throw 'Encoding "' + encoding + '" not supported. Try utf-8 or binary.';
  //   }
  // },

  // read: function(bytes, encoding) {
  //   if (encoding === 'utf-8') {
  //     return this.readUtf8(bytes);
  //   }
  //   else if (encoding === 'binary') {
  //     return this.readBinary(bytes);
  //   }
  //   else {
  //     return this.createPromise((res, rej) => {
  //       rej('Encoding "' + encoding + '" not supported. Try utf-8 or binary.');
  //     });
  //   }
  // },

  // readBinary: function(bytes) {
  //   return this.createPromise((res, rej) => {
  //     this._file.read(bytes).then(array => {
  //       res(Cu.cloneInto(array, this._window));
  //     }).catch(err => rej(err));
  //   });
  // },

  // readUtf8: function(bytes) {
  //   let decoder = new TextDecoder();

  //   return this.createPromise((res, rej) => {
  //     this._file.read(bytes).then(array => {
  //       res(decoder.decode(array));
  //     }).catch(err => rej(err));
  //   });
  // },

  stat: function() {
    throw 'Yolo';
  },

  // setPosition: function(position, origin) {
  //   dump('origin is ' + origin + '\n')

  //   origin = OS.File[origin];

  //   dump('origin is ' + origin + '\n')

  //   return this.createPromise((res, rej) => {
  //     this._file.setPosition(position, origin).then(res, rej);
  //   });
  // }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([MozOs]);
