/* ***** BEGIN LICENSE BLOCK *****
 * Version: MPL 1.1/GPL 2.0/LGPL 2.1
 *
 * The contents of this file are subject to the Mozilla Public License Version
 * 1.1 (the "License"); you may not use this file except in compliance with
 * the License. You may obtain a copy of the License at
 * http://www.mozilla.org/MPL/
 *
 * Software distributed under the License is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
 * for the specific language governing rights and limitations under the
 * License.
 *
 * The Original Code is Bookmarks Sync.
 *
 * The Initial Developer of the Original Code is Mozilla.
 * Portions created by the Initial Developer are Copyright (C) 2008
 * the Initial Developer. All Rights Reserved.
 *
 * Contributor(s):
 *  Myk Melez <myk@mozilla.org>
 *  Jono DiCarlo <jdicarlo@mozilla.com>
 *
 * Alternatively, the contents of this file may be used under the terms of
 * either the GNU General Public License Version 2 or later (the "GPL"), or
 * the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
 * in which case the provisions of the GPL or the LGPL are applicable instead
 * of those above. If you wish to allow use of your version of this file only
 * under the terms of either the GPL or the LGPL, and not to allow others to
 * use your version of this file under the terms of the MPL, indicate your
 * decision by deleting the provisions above and replace them with the notice
 * and other provisions required by the GPL or the LGPL. If you do not delete
 * the provisions above, a recipient may use your version of this file under
 * the terms of any one of the MPL, the GPL or the LGPL.
 *
 * ***** END LICENSE BLOCK ***** */

const EXPORTED_SYMBOLS = ['TabEngine'];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://weave/util.js");
Cu.import("resource://weave/engines.js");
Cu.import("resource://weave/stores.js");
Cu.import("resource://weave/trackers.js");
Cu.import("resource://weave/constants.js");
Cu.import("resource://weave/type_records/tabs.js");
Cu.import("resource://weave/engines/clientData.js");

function TabEngine() {
  this._init();
}
TabEngine.prototype = {
  __proto__: SyncEngine.prototype,
  name: "tabs",
  _displayName: "Tabs",
  description: "Access tabs from other devices via the History menu",
  logName: "Tabs",
  _storeObj: TabStore,
  _trackerObj: TabTracker,
  _recordObj: TabSetRecord,

  // API for use by Weave UI code to give user choices of tabs to open:
  getAllClients: function TabEngine_getAllClients() {
    return this._store._remoteClients;
  },

  getClientById: function TabEngine_getClientById(id) {
    return this._store._remoteClients[id];
  },

  _resetClient: function TabEngine__resetClient() {
    this.resetLastSync();
    this._store.wipe();
  },

  _syncFinish: function _syncFinish() {
    SyncEngine.prototype._syncFinish.call(this);
    this._tracker.resetChanged();
  },

  /* The intent is not to show tabs in the menu if they're already
   * open locally.  There are a couple ways to interpret this: for
   * instance, we could do it by removing a tab from the list when
   * you open it -- but then if you close it, you can't get back to
   * it.  So the way I'm doing it here is to not show a tab in the menu
   * if you have a tab open to the same URL, even though this means
   * that as soon as you navigate anywhere, the original tab will
   * reappear in the menu.
   */
  locallyOpenTabMatchesURL: function TabEngine_localTabMatches(url) {
    // url should be string, not object
    /* Some code duplication from _addFirefoxTabsToRecord and
     * _addFennecTabsToRecord.  Unify? */
    if (Cc["@mozilla.org/browser/sessionstore;1"])  {
      let state = this._store._sessionStore.getBrowserState();
      let session = JSON.parse(state);
      for (let i = 0; i < session.windows.length; i++) {
        let window = session.windows[i];
        for (let j = 0; j < window.tabs.length; j++) {
          let tab = window.tabs[j];
          if (tab.entries.length > 0) {
            let tabUrl = tab.entries[tab.entries.length-1].url;
            if (tabUrl == url) {
              return true;
            }
          }
        }
      }
    } else {
      let wm = Cc["@mozilla.org/appshell/window-mediator;1"]
	.getService(Ci.nsIWindowMediator);
      let browserWindow = wm.getMostRecentWindow("navigator:browser");
      for each (let tab in browserWindow.Browser._tabs ) {
        let tabUrl = tab.browser.contentWindow.location.toString();
        if (tabUrl == url) {
          return true;
        }
      }
    }
    return false;
  }
};


function TabStore() {
  this._TabStore_init();
}
TabStore.prototype = {
  __proto__: Store.prototype,
  name: "tabs",
  _logName: "Tabs.Store",
  _filePath: "meta/tabSets",
  _remoteClients: {},

  _TabStore_init: function TabStore__init() {
    this._init();
    this._readFromFile();
  },

  _writeToFile: function TabStore_writeToFile() {
    Utils.jsonSave(this._filePath, this, this._remoteClients);
  },

  _readFromFile: function TabStore_readFromFile() {
    Utils.jsonLoad(this._filePath, this, function(json) {
      this._remoteClients = json;
    });
  },

  get _sessionStore() {
    let sessionStore = Cc["@mozilla.org/browser/sessionstore;1"].
		       getService(Ci.nsISessionStore);
    this.__defineGetter__("_sessionStore", function() { return sessionStore;});
    return this._sessionStore;
  },

  itemExists: function TabStore_itemExists(id) {
    return id == Clients.clientID;
  },

  createRecord: function TabStore_createRecord(id, cryptoMetaURL) {
    let record = new TabSetRecord();
    record.clientName = Clients.clientName;

    // Iterate through each tab of each window
    let allTabs = [];
    let wins = Svc.WinMediator.getEnumerator("navigator:browser");
    while (wins.hasMoreElements()) {
      // Get the tabs for both Firefox and Fennec
      let window = wins.getNext();
      let tabs = window.gBrowser && window.gBrowser.tabContainer.childNodes;
      tabs = tabs || window.Browser._tabs;
      Array.forEach(tabs, function(tab) {
        allTabs.push(tab);
      });
    }

    // Extract various pieces of tab data and sort them in descending used
    let tabData = allTabs.map(function(tab) {
      let browser = tab.linkedBrowser || tab.browser;
      return {
        title: browser.contentTitle || "",
        urlHistory: [browser.currentURI.spec],
        icon: browser.mIconURL || "",
        lastUsed: tab.lastUsed || 0
      };
    }).sort(function(a, b) b.lastUsed - a.lastUsed);

    // Only grab the most recently used tabs to sync
    record.tabs = tabData.slice(0, 25);
    record.tabs.forEach(function(tab) {
      this._log.debug("Wrapping tab: " + JSON.stringify(tab));
    }, this);

    record.id = id;
    record.encryption = cryptoMetaURL;
    return record;
  },

  getAllIDs: function TabStore_getAllIds() {
    let ids = {};
    ids[Clients.clientID] = true;
    return ids;
  },

  wipe: function TabStore_wipe() {
    this._remoteClients = {};
  },

  create: function TabStore_create(record) {
    this._log.debug("Adding remote tabs from " + record.clientName);
    this._remoteClients[record.id] = record.cleartext;
    this._writeToFile();
    // TODO writing to file after every change is inefficient.  How do we
    // make sure to do it (or at least flush it) only after sync is done?
    // override syncFinished
  }
};


function TabTracker() {
  this._TabTracker_init();
}
TabTracker.prototype = {
  __proto__: Tracker.prototype,
  name: "tabs",
  _logName: "TabTracker",
  file: "tab_tracker",

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver]),

  _TabTracker_init: function TabTracker__init() {
    this._init();
    this.resetChanged();

    // Make sure "this" pointer is always set correctly for event listeners
    this.onTab = Utils.bind2(this, this.onTab);

    // Register as an observer so we can catch windows opening and closing:
    Svc.WinWatcher.registerNotification(this);

    // Also register listeners on already open windows
    let wins = Svc.WinMediator.getEnumerator("navigator:browser");
    while (wins.hasMoreElements())
      this._registerListenersForWindow(wins.getNext());
  },

  _registerListenersForWindow: function TabTracker__registerListen(window) {
    this._log.trace("Registering tab listeners in new window");

    // For each topic, add or remove onTab as the listener
    let topics = ["TabOpen", "TabClose", "TabSelect"];
    let onTab = this.onTab;
    let addRem = function(add) topics.forEach(function(topic) {
      window[(add ? "add" : "remove") + "EventListener"](topic, onTab, false);
    });

    // Add the listeners now and remove them on unload
    addRem(true);
    window.addEventListener("unload", function() addRem(false), false);
  },

  observe: function TabTracker_observe(aSubject, aTopic, aData) {
    // Add tab listeners now that a window has opened
    let window = aSubject.QueryInterface(Ci.nsIDOMWindow);
    if (aTopic == "domwindowopened")
      this._registerListenersForWindow(window);
  },

  onTab: function onTab(event) {
    this._log.trace(event.type);
    this.score += 1;
    this._changedIDs[Clients.clientID] = true;

    // Store a timestamp in the tab to track when it was last used
    event.originalTarget.lastUsed = Math.floor(Date.now() / 1000);
  },

  get changedIDs() this._changedIDs,

  // Provide a way to empty out the changed ids
  resetChanged: function resetChanged() this._changedIDs = {}
}
