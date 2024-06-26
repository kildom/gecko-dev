/* This Source Code Form is subject to the terms of the Mozilla Public
 *  License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

add_task(async function test() {
  let prefs = ["history", "historyFormDataAndDownloads"];

  for (let pref of prefs) {
    // This test relies on the form history being empty to start with delete
    // all the items first.
    // The TabContextMenu initializes its strings only on a focus or mouseover event.
    // Calls focus event on the TabContextMenu early in the test.
    gBrowser.selectedTab.focus();
    await FormHistory.update({ op: "remove" });

    // Sanitize now so we can test the baseline point.
    await Sanitizer.sanitize([pref]);
    await gFindBarPromise;
    ok(!gFindBar.hasTransactions, "pre-test baseline for sanitizer");

    gFindBar.getElement("findbar-textbox").value = "m";
    ok(gFindBar.hasTransactions, "formdata can be cleared after input");

    await Sanitizer.sanitize(["formdata"]);
    is(
      gFindBar.getElement("findbar-textbox").value,
      "",
      "findBar textbox should be empty after sanitize"
    );
    ok(!gFindBar.hasTransactions, "No transactions after sanitize");
  }
});
