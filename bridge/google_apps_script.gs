// Paste into Extensions > Apps Script on the target Google Sheet, then
// Deploy > New deployment > Web app (execute as yourself, access: Anyone
// with the link). Pass the resulting /exec URL to plato_bridge.py via
// --sheets-url or the GOOGLE_SHEETS_WEBHOOK_URL env var.
function doPost(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var row = JSON.parse(e.postData.contents);

  var values = [row.timestamp_utc, row.node, row.haptic_active, row.haptic_enabled];
  for (var i = 0; i < 20; i++) {
    var key = "hinge_" + (i < 10 ? "0" + i : i);
    values.push(row[key]);
  }
  sheet.appendRow(values);

  return ContentService
    .createTextOutput(JSON.stringify({ ok: true }))
    .setMimeType(ContentService.MimeType.JSON);
}
