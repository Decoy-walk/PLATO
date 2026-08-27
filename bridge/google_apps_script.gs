// Paste into Extensions > Apps Script on the target Google Sheet, then
// Deploy > New deployment > Web app (execute as yourself, access: Anyone
// with the link). Pass the resulting /exec URL to plato_bridge.py via
// --sheets-url or the GOOGLE_SHEETS_WEBHOOK_URL env var.
function doPost(e) {
  var sheet = SpreadsheetApp.getActiveSpreadsheet().getActiveSheet();
  var row = JSON.parse(e.postData.contents);

  sheet.appendRow([
    row.timestamp_utc,
    row.node,
    row.fsr,
    row.grip,
    row.ambient,
    row.bpm,
  ]);

  return ContentService
    .createTextOutput(JSON.stringify({ ok: true }))
    .setMimeType(ContentService.MimeType.JSON);
}
