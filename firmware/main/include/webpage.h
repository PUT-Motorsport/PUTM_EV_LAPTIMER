
/* HTML Content */
const char *index_html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
<meta name="viewport" content="width=device-width, initial-scale=1">
<style>
body { font-family: monospace; background: #111; color: #eee; margin: 0; padding: 10px; font-size: 16px; }
h1 { text-align: center; margin-top: 10px; }
.container { display: flex; flex-wrap: wrap; justify-content: center; gap: 20px; max-width: 1400px; margin: 0 auto; }
.main-panel { flex: 1; min-width: 300px; max-width: 900px; text-align: center; }
.side-panel { flex: 0 0 500px; background: #222; padding: 15px; border-radius: 8px; height: fit-content; text-align: left; border: 1px solid #444; }
.val { font-size: 3em; font-weight: bold; }
.label { color: #aaa; font-size: 1.2em; margin-bottom: 5px; }
.status { margin: 20px; font-size: 1.2em; }
.status span { padding: 8px 15px; margin: 5px; border: 1px solid #444; display: inline-block; border-radius: 4px; }
.on { background: #0a0; color: #fff; }
.off { background: #333; color: #888; }
.tables-wrapper { display: flex; flex-direction: row; flex-wrap: wrap; gap: 20px; margin-top: 20px; justify-content: center; }
.table-box { flex: 1; min-width: 300px; background: #1a1a1a; padding: 10px; border-radius: 5px; border: 1px solid #333; }
.table-title { font-size: 1.5em; color: #fff; margin-bottom: 10px; text-align: center; border-bottom: 1px solid #444; padding-bottom: 5px; }
table { width: 100%; font-size: 1.1em; border-collapse: collapse; }
th, td { padding: 8px; border-bottom: 1px solid #333; text-align: center; }
th { color: #aaa; }
.penalty { color: #ffeb3b; }
.penalty-val { font-size: 2em; font-weight: bold; color: #ffeb3b; }
.pen-col { color: #ffeb3b; }
.driver-color-box { display: inline-block; width: 20px; height: 20px; border-radius: 4px; vertical-align: middle; border: 2px solid #fff; margin-right: 10px; }
.current-container { display: flex; justify-content: center; align-items: baseline; gap: 20px; flex-wrap: wrap; }
.download-btn { padding: 15px 30px; font-size: 1.2em; background: #333; color: #fff; border: 1px solid #444; cursor: pointer; margin: 5px 0; width: 100%; border-radius: 4px; display: block; text-decoration: none; text-align: center;}
.legend-item { font-size: 1.1em; display: flex; align-items: center; padding: 8px 0; border-bottom: 1px solid #333; }
.legend-item:last-child { border-bottom: none; }
h3 { margin-top: 0; color: #ccc; border-bottom: 1px solid #555; padding-bottom: 10px; text-align: center; }
/* Info Table Styles */
.info-table { margin: 20px auto; width: auto; min-width: 60%; background: #222; border-radius: 5px; overflow: hidden; }
.info-table th { background: #333; padding: 10px 20px; }
.info-table td { padding: 10px 20px; font-size: 1.3em; font-weight: bold; }
@media (max-width: 800px) {
    .container { flex-direction: column; }
    .side-panel { width: auto; flex: none; order: 2; }
    .main-panel { order: 1; }
    .tables-wrapper { flex-direction: column; }
}
</style>
</head>
<body>
<h1>PUTM Laptimer</h1>
<div class="container">

  <div class="main-panel">
      <div>
            <div class="status">
        <span id="mode">1 GATE</span>
        <span id="stop">STOP</span>
        <span id="sd">SD</span>
      </div>
        <div class="label">CURRENT LAP</div>
        <div class="current-container">
            <div class="val" id="curr">--,--:--:--</div>
            <div class="penalty-val" id="pen_time">+00:00</div>
        </div>
      </div>

      <!-- Driver & Penalty Info Table -->
      <table class="info-table">
        <thead>
            <tr>
                <th>DRIVER</th>
                <th class="penalty">OC</th>
                <th class="penalty">DOO</th>
            </tr>
        </thead>
        <tbody>
            <tr>
                <td>
                    <span id="curr_drv_tag">XXX</span>
                    <span id="curr_drv_col" class="driver-color-box" style="background: black; margin-left: 10px;"></span>
                </td>
                <td class="penalty" id="pen_oc">0</td>
                <td class="penalty" id="pen_doo">0</td>
            </tr>
        </tbody>
      </table>

      <div class="tables-wrapper">
          <!-- LAST LAPS TABLE -->
          <div class="table-box">
            <div class="table-title">LAST LAPS</div>
            <table>
                <thead>
                    <tr>
                        <th>Time</th>
                        <th>Driver</th>
                        <th>Pen.</th>
                        <th>OC</th>
                        <th>DOO</th>
                    </tr>
                </thead>
                <tbody id="list_last"></tbody>
            </table>
          </div>

          <!-- TOP LAPS TABLE -->
          <div class="table-box">
            <div class="table-title">TOP LAPS</div>
            <table>
                <thead>
                    <tr>
                        <th>Time</th>
                        <th>Driver</th>
                    </tr>
                </thead>
                <tbody id="list_top"></tbody>
            </table>
          </div>
      </div>

  </div>

  <div class="side-panel">
     <h3>DRIVER LIST</h3>
     <div id="driver_legend"></div>
     <div style="margin-top: 20px;">
        <form action="/api/csv" method="get">
          <button type="submit" class="download-btn">DOWNLOAD LAPS CSV</button>
        </form>
        <form action="/api/drivers_csv" method="get">
          <button type="submit" class="download-btn">DOWNLOAD DRIVERS CSV</button>
        </form>
        <button onclick="toggleSettings()" class="download-btn">SETTINGS</button>
     </div>

     <div id="settings-box" style="display:none; margin-top: 20px; border-top: 1px solid #444; padding-top: 20px; text-align: left;">
        <h3>CONFIGURATION</h3>
        <div style="margin-bottom: 10px;">
            <label style="color:#aaa; display:block; margin-bottom:5px;">Mode</label>
            <label><input type="checkbox" id="cfg_gates"> 2 GATES MODE</label>
        </div>
        <div style="margin-bottom: 10px;">
            <label style="color:#aaa; display:block; margin-bottom:5px;">WiFi Mode</label>
            <label><input type="checkbox" id="cfg_wifi_mode"> Enable Station Mode (STA)</label>
        </div>
        <div style="margin-bottom: 10px;">
            <label style="color:#aaa; display:block; margin-bottom:5px;">WiFi SSID</label>
            <input type="text" id="cfg_ssid" style="width:100%; padding:8px; background:#333; border:1px solid #555; color:#fff;">
        </div>
        <div style="margin-bottom: 10px;">
            <label style="color:#aaa; display:block; margin-bottom:5px;">WiFi Password</label>
            <input type="password" id="cfg_pass" style="width:100%; padding:8px; background:#333; border:1px solid #555; color:#fff;">
        </div>
        <div style="margin-bottom: 10px;">
            <label style="color:#aaa; display:block; margin-bottom:5px;">System Time</label>
            <input type="time" step="1" id="cfg_time_set" style="width:100%; padding:8px; background:#333; border:1px solid #555; color:#fff;">
        </div>
        <div style="margin-bottom: 10px;">
            <label style="color:#aaa; display:block; margin-bottom:5px;">System Date</label>
            <input type="date" id="cfg_date_set" style="width:100%; padding:8px; background:#333; border:1px solid #555; color:#fff;">
        </div>
        <div style="margin-bottom: 10px;">
            <label style="color:#aaa; display:block; margin-bottom:5px;">Drivers (one per line)</label>
            <textarea id="cfg_drivers" rows="6" style="width:100%; padding:8px; background:#333; border:1px solid #555; color:#fff;"></textarea>
        </div>
        <button onclick="saveConfig()" class="download-btn" style="background:#0056b3;">SAVE CONFIG</button>
     </div>
  </div>

</div>

<script>
const colors = ['black', 'blue', 'red', 'yellow', 'green', 'magenta', 'brown', 'cyan', 'purple', 'olive'];

function toggleSettings() {
    let box = document.getElementById('settings-box');
    if (box.style.display === 'none') {
        box.style.display = 'block';
        loadConfig();
    } else {
        box.style.display = 'none';
    }
}

function loadConfig() {
    fetch('/api/config').then(r => r.json()).then(d => {
        document.getElementById('cfg_gates').checked = d.two_gate_mode;
        // if wifi_mode is WIFI_MODE_STA (true), checkbox should be checked
        document.getElementById('cfg_wifi_mode').checked = (d.wifi_mode === 1); // WIFI_MODE_STA is 1
        document.getElementById('cfg_ssid').value = d.wifi_ssid || "";
        document.getElementById('cfg_pass').value = d.wifi_password || "";
        document.getElementById('cfg_time_set').value = d.time_set || "";
        // Convert DD/MM/YYYY to YYYY-MM-DD for input type=date
        if (d.date_set) {
            let parts = d.date_set.split('/');
            if(parts.length === 3) {
                document.getElementById('cfg_date_set').value = `${parts[2]}-${parts[1]}-${parts[0]}`;
            } else {
                document.getElementById('cfg_date_set').value = "";
            }
        }
        if (d.driver_list) {
            document.getElementById('cfg_drivers').value = d.driver_list.join('\n');
        }
    }).catch(e => console.error(e));
}

function saveConfig() {
    let drivers = document.getElementById('cfg_drivers').value.split('\n').map(s => s.trim()).filter(s => s.length > 0);
    // Convert YYYY-MM-DD to DD/MM/YYYY
    let rawDate = document.getElementById('cfg_date_set').value;
    let formattedDate = "";
    if (rawDate) {
        let parts = rawDate.split('-');
        if(parts.length === 3) {
            formattedDate = `${parts[2]}/${parts[1]}/${parts[0]}`;
        }
    }

    let data = {
        two_gate_mode: document.getElementById('cfg_gates').checked,
        wifi_mode: document.getElementById('cfg_wifi_mode').checked ? 1 : 0, // 1 for STA, 0 for AP
        wifi_ssid: document.getElementById('cfg_ssid').value,
        wifi_password: document.getElementById('cfg_pass').value,
        time_set: document.getElementById('cfg_time_set').value,
        date_set: formattedDate,
        driver_list: drivers
    };
    fetch('/api/config', {
        method: 'POST',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(data)
    }).then(r => {
        if (r.ok) alert("Config Saved!");
        else alert("Error saving config");
    }).catch(e => alert("Error: " + e));
}

function update() {
  fetch('/api/data?t=' + new Date().getTime()).then(r => r.json()).then(d => {
    document.getElementById('curr').innerText = d.current;

    document.getElementById('curr_drv_tag').innerText = d.current_driver_tag || "---";
    let currColorIdx = d.current_driver_id || 0;
    if(currColorIdx < 0 || currColorIdx >= colors.length) currColorIdx = 0;
    document.getElementById('curr_drv_col').style.background = colors[currColorIdx];

    document.getElementById('pen_time').innerText = d.penalty_time || "+00:00";
    document.getElementById('pen_oc').innerText = (d.penalty_oc || "0");
    document.getElementById('pen_doo').innerText = (d.penalty_doo || "0");

    document.getElementById('mode').innerText = d.status.mode ? "2 GATE" : "1 GATE";
    document.getElementById('stop').className = d.status.stop ? "on" : "off";
    document.getElementById('sd').className = d.status.sd ? "on" : "off";

    if (d.all_drivers) {
        let legendHtml = "<table><thead><tr><th>Driver</th><th>Best</th><th>Laps</th><th>Pen.</th><th>OC</th><th>DOO</th></tr></thead><tbody>";
        d.all_drivers.forEach((drv, i) => {
             // Skip placeholder only if it matches standard placeholder
             if (drv === "---") return;
             let color = colors[i] || 'black';

             let best = (d.driver_best && d.driver_best[i]) ? d.driver_best[i] : '-';
             let laps = (d.driver_lap_count && d.driver_lap_count[i] !== undefined) ? d.driver_lap_count[i] : '0';
             let p_time = (d.driver_pen_time && d.driver_pen_time[i]) ? d.driver_pen_time[i] : '0';
             let p_oc = (d.driver_pen_oc && d.driver_pen_oc[i]) ? d.driver_pen_oc[i] : '0';
             let p_doo = (d.driver_pen_doo && d.driver_pen_doo[i]) ? d.driver_pen_doo[i] : '0';

             legendHtml += `<tr>
                <td style=\"text-align: left;\">
                    <span class=\"driver-color-box\" style=\"background:${color}; margin-right:5px;\"></span>${drv}
                </td>
                <td>${best}</td>
                <td>${laps}</td>
                <td class=\"pen-col\">${p_time}</td>
                <td class=\"pen-col\">${p_oc}</td>
                <td class=\"pen-col\">${p_doo}</td>
             </tr>`;
        });
        legendHtml += "</tbody></table>";
        document.getElementById('driver_legend').innerHTML = legendHtml;
    }

    // Update LAST list
    let lastHtml = "";
    if (d.last) {
        for(let i=0; i<d.last.length; i++) {
            let t = d.last[i] || '-';
            let drv = (d.last_driver_tag && d.last_driver_tag[i]) ? d.last_driver_tag[i] : '';
            let drv_id = (d.last_driver_id && d.last_driver_id[i] !== undefined) ? d.last_driver_id[i] : 0;
            let p_time = (d.last_pen_time && d.last_pen_time[i]) ? d.last_pen_time[i] : '';
            let p_oc = (d.last_pen_oc && d.last_pen_oc[i]) ? d.last_pen_oc[i] : '';
            let p_doo = (d.last_pen_doo && d.last_pen_doo[i]) ? d.last_pen_doo[i] : '';

            if (t === '-' || t.includes('--:--.--')) continue;

            let color = colors[drv_id] || 'black';
            lastHtml += `<tr>
                <td>${t}</td>
                <td>${drv}<span class=\"driver-color-box\" style=\"background:${color}; margin-left:5px; margin-right:0;\"></span></td>
                <td class=\"pen-col\">${p_time}</td>
                <td class=\"pen-col\">${p_oc}</td>
                <td class=\"pen-col\">${p_doo}</td>
            </tr>`;
        }
    }
    document.getElementById('list_last').innerHTML = lastHtml;

    // Update TOP list
    let topHtml = "";
    if (d.top) {
        for(let i=0; i<d.top.length; i++) {
            let t = d.top[i] || '-';
            let drv = (d.top_driver_tag && d.top_driver_tag[i]) ? d.top_driver_tag[i] : '';
            let drv_id = (d.top_driver_id && d.top_driver_id[i] !== undefined) ? d.top_driver_id[i] : 0;

            if (t === '-' || t.includes('--:--.--')) continue;

            let color = colors[drv_id] || 'black';
            topHtml += `<tr>
                <td>${t}</td>
                <td>${drv}<span class=\"driver-color-box\" style=\"background:${color}; margin-left:5px; margin-right:0;\"></span></td>
            </tr>`;
        }
    }
    document.getElementById('list_top').innerHTML = topHtml;
  });
}
setInterval(update, 200);
</script>
</body>
</html>
)rawliteral";