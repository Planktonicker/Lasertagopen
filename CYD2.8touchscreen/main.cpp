#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ILI9341.h>
#include <XPT2046_Touchscreen.h>

// --- CYD TOUCHSCREEN PINS (VSPI) ---
#define XPT2046_IRQ  36
#define XPT2046_MOSI 32
#define XPT2046_MISO 39
#define XPT2046_CLK  25
#define XPT2046_CS   33

// --- CYD TFT SCREEN PINS (HSPI) ---
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_CLK  14
#define TFT_CS   15
#define TFT_DC   2
#define TFT_RST  -1
#define TFT_BL   21 // Backlight

// Initialize dual SPI buses for CYD
XPT2046_Touchscreen ts(XPT2046_CS, XPT2046_IRQ);
SPIClass tftSpi = SPIClass(HSPI);
Adafruit_ILI9341 tft = Adafruit_ILI9341(&tftSpi, TFT_DC, TFT_CS, TFT_RST);
WebServer server(80);
DNSServer dnsServer; 
const byte DNS_PORT = 53;

// --- SERVER VARIABLES ---
const char* apSSID = "LaserTag_Hub";
const char* apPass = "pewpewpew";
const char* routerSSID   = "LaserArena";
const char* routerPass   = "pewpewpew";
const char* customDomain = "lasertagarena.game"; 
bool usingExternalRouter = false; 

// --- GAME STATE SYNCED TO GUNS ---
int hostStatus = 0; // 0=Lobby, 1=Running, 2=Kill All
int baseDamage = 50; 
int configLives = 3;
int configMags = 5;
int configHealth = 100;
int gameMode = 0; // 0=Combat, 1=Unlimited

#define MAX_PLAYERS 64

// --- SCOREBOARD TRACKING ---
struct PlayerRecord {
  int id;
  int team;
  int kills;
  int deaths;
  int assists;
  bool active;
};
PlayerRecord players[MAX_PLAYERS];
int playerCount = 0;

// --- UI STATE ---
int currentScreen = 0; // 0=Main, 1=Setup1, 2=Setup2, 4=Scoreboard
unsigned long gameStartTime = 0;
unsigned long lastTimerUpdate = 0;
int scoreboardOffset = 0; // Tracks which page of players we are viewing

// Cyberpunk / Neon Colors
#define COLOR_WHITE     0xFFFF
#define COLOR_BLACK     0x0000
#define C_BG            tft.color565(5, 5, 10)    // Void Black/Deep Blue
#define C_CYAN          tft.color565(0, 255, 255) // Primary Neon
#define C_MAGENTA       tft.color565(255, 0, 255) // Secondary Neon
#define C_GREEN         tft.color565(0, 255, 100) // Success/Active
#define C_RED           tft.color565(255, 0, 100) // Warning/Kill
#define C_YELLOW        tft.color565(255, 255, 0) // Alerts
#define C_GREY          tft.color565(80, 80, 100) // Inactive outlines

// --- FORWARD DECLARATIONS ---
void drawMainScreen();
void drawSetupScreen1();
void drawSetupScreen2();
void drawScoreboardScreen();

PlayerRecord* getPlayer(int id) {
  if (id <= 0) return nullptr;
  for(int i=0; i<MAX_PLAYERS; i++) {
    if(players[i].active && players[i].id == id) return &players[i];
  }
  for(int i=0; i<MAX_PLAYERS; i++) {
    if(!players[i].active) {
      players[i].id = id;
      players[i].team = 1;
      players[i].kills = 0;
      players[i].deaths = 0;
      players[i].assists = 0;
      players[i].active = true;
      playerCount++;
      return &players[i];
    }
  }
  return nullptr;
}

// --- WEB SERVER ENDPOINTS ---
void handleRoot() {
  int teamKills[5] = {0, 0, 0, 0, 0};
  bool teamActive[5] = {false, false, false, false, false};
  int activeTeamCount = 0; 
  
  for(int i = 0; i < MAX_PLAYERS; i++) {
    if(players[i].active) {
      teamKills[players[i].team] += players[i].kills;
      teamActive[players[i].team] = true;
    }
  }
  
  // Count how many teams are playing to calculate grid layout
  for(int t = 1; t <= 4; t++) {
    if(teamActive[t]) activeTeamCount++;
  }

  String html = "<!DOCTYPE html><html><head><title>Arena Command</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover'>";
  
  html += "<style>";
  html += "* { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }";
  html += "body { font-family: -apple-system, system-ui, sans-serif; background: #050510; color: #0ff; margin: 0; padding: env(safe-area-inset-top) 15px 40px 15px; }";
  html += ".card { background: rgba(0, 255, 255, 0.08); padding: 20px; border: 1px solid #0ff; border-radius: 20px; margin-bottom: 20px; box-shadow: 0 10px 40px rgba(0,0,0,0.7); }";
  html += "h1 { font-size: 26px; text-align: center; color: #fff; text-shadow: 0 0 15px #0ff; margin: 25px 0; letter-spacing: 1px; }";
  html += "h2 { font-size: 13px; margin-top: 0; border-bottom: 1px solid rgba(0, 255, 255, 0.2); padding-bottom: 12px; margin-bottom: 15px; text-transform: uppercase; letter-spacing: 2px; color: rgba(255,255,255,0.7); }";
  
  html += "button { width: 100%; padding: 22px; margin-bottom: 15px; background: #001a1a; border: 2px solid #0ff; color: #0ff; border-radius: 14px; font-weight: bold; font-size: 18px; transition: 0.1s; cursor: pointer; }";
  html += "button:active { transform: scale(0.97); background: #0ff; color: #000; }";
  html += ".btn-red { border-color: #f0f; color: #f0f; background: #1a001a; }";
  html += ".btn-green { border-color: #0f0; color: #0f0; background: #001a00; }";
  
  html += ".grid { display: flex; flex-wrap: wrap; gap: 15px; }";
  html += ".grid > div { flex: 1 1 calc(50% - 15px); min-width: 100px; }"; // Reduced min-width slightly to fit 5 boxes better
  html += ".score-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 20px; }";
  
  html += "label { font-size: 10px; display: block; margin-bottom: 8px; color: #fff; opacity: 0.8; font-weight: 800; letter-spacing: 1px; }";
  html += "input, select { width: 100%; padding: 16px; background: #000; color: #fff; border: 1px solid #0ff; border-radius: 12px; font-size: 18px; outline: none; -webkit-appearance: none; }";
  
  html += "table { width: 100%; text-align: center; border-collapse: collapse; margin-top: 10px; }";
  html += "th, td { padding: 12px 5px; border-bottom: 1px solid rgba(255, 255, 255, 0.1); font-size: 15px; }";
  html += "th { font-size: 10px; opacity: 0.5; color: #fff; }";
  html += ".kd-val { font-weight: bold; color: #fff; }";
  html += "tr.player-row { cursor: pointer; transition: background 0.2s; }"; 
  html += "tr.player-row:hover { background: rgba(255, 255, 255, 0.05); }"; 
  html += "tr.player-row:active { background: rgba(0, 255, 255, 0.2); }"; 

  // --- ESPORTS FULL SCREEN CSS ---
  html += "#fs-overlay{display:none;position:fixed;top:0;left:0;width:100vw;height:100vh;background-color:#050510;background-size:40px 40px;background-image:linear-gradient(to right, rgba(0,255,255,0.05) 1px, transparent 1px),linear-gradient(to bottom, rgba(0,255,255,0.05) 1px, transparent 1px);z-index:9999;padding:30px;flex-direction:column;overflow:hidden;box-sizing:border-box;} ";
  html += "#fs-overlay.active{display:flex;} ";
  html += ".fs-header{display:flex;justify-content:space-between;align-items:center;margin-bottom:20px;} ";
  html += ".fs-title{font-size:28px;font-weight:900;letter-spacing:2px;color:#fff;margin:0;text-shadow:0 0 10px rgba(255,255,255,0.5);} ";
  html += ".fs-grid{display:grid;gap:30px;flex:1;min-height:0;} ";
  html += ".fs-grid[data-teams='1']{grid-template-columns:1fr;} ";
  html += ".fs-grid[data-teams='2']{grid-template-columns:1fr 1fr;} ";
  html += ".fs-grid[data-teams='3'],.fs-grid[data-teams='4']{grid-template-columns:1fr 1fr;grid-template-rows:1fr 1fr;} ";
  html += ".fs-team-panel{border:2px solid;border-radius:12px;display:flex;flex-direction:column;background:rgba(0,0,0,0.5);box-shadow:inset 0 0 50px rgba(0,0,0,0.8);overflow:hidden;} ";
  html += ".fs-team-header{padding:20px;display:flex;justify-content:space-between;align-items:center;background:rgba(255,255,255,0.1);border-bottom:2px solid;} ";
  html += ".fs-team-name{font-size:24px;font-weight:900;letter-spacing:2px;} ";
  html += ".fs-team-score{font-size:32px;font-weight:900;} ";
  html += ".fs-table-wrap{overflow-y:auto;flex:1;padding:0 10px;} ";
  html += ".fs-table{width:100%;border-collapse:collapse;} ";
  html += ".fs-table th{position:sticky;top:0;background:#000;padding:15px;font-size:14px;text-transform:uppercase;color:rgba(255,255,255,0.5);z-index:2;} ";
  html += ".fs-table td{padding:15px;font-size:20px;border-bottom:1px solid rgba(255,255,255,0.05);text-align:center;} ";
  html += ".fs-close{padding:10px 30px;background:transparent;border:2px solid #f0f;color:#f0f;border-radius:8px;cursor:pointer;font-size:18px;font-weight:bold;width:auto;margin:0;transition:0.2s;} ";
  html += ".fs-close:hover{background:#f0f;color:#000;} ";
  html += "</style>";
  
  html += "<script>";
  html += "function swapTeam(id, currentTeam) {";
  html += "  let nextTeam = currentTeam + 1; if(nextTeam > 4) nextTeam = 1;";
  html += "  fetch('/join?id=' + id + '&team=' + nextTeam).then(() => location.reload());";
  html += "}";
  html += "function toggleFS() {";
  html += "  let el = document.getElementById('fs-overlay');";
  html += "  let isActive = el.classList.toggle('active');";
  html += "  sessionStorage.setItem('fs-active', isActive ? 'true' : 'false');";
  html += "}";
  html += "document.addEventListener('DOMContentLoaded', function() {";
  html += "  if(sessionStorage.getItem('fs-active') === 'true') {";
  html += "    document.getElementById('fs-overlay').classList.add('active');";
  html += "  }";
  html += "});";
  html += "setInterval(function(){ location.reload(); }, 12000);";
  html += "</script>";
  html += "</head><body>";
  
  html += "<h1>ARENA COMMAND ";
  html += usingExternalRouter ? "<span style='color:#0f0; font-size:16px;'>[LA ROUTER]</span>" : "<span style='color:#ff0; font-size:16px;'>[LAH HUB]</span>";
  html += "</h1>";
  
  html += "<div class='card'><h2>STATUS: " + String(hostStatus == 1 ? "RUNNING" : (hostStatus == 0 ? "LOBBY" : "KILL ALL")) + "</h2>";
  
  html += "<form action='/set'>";
  if (hostStatus == 0) html += "<button name='cmd' value='start' class='btn-green'>START MISSION</button>";
  else html += "<button name='cmd' value='lobby'>RETURN LOBBY</button>";
  html += "<button name='cmd' value='kill' class='btn-red' style='border-style:dashed; margin-top:10px;'>EMERGENCY KILL ALL</button>";
  html += "</form></div>";
  
  html += "<div class='card'><h2>TEAM STANDINGS <span style='font-size:10px; color:#f0f; float:right;'>(TAP ROW TO SWAP)</span></h2>";
  if (playerCount == 0) {
    html += "<div style='padding:40px; text-align:center; opacity:0.3;'>NO OPERATORS DETECTED</div>";
  } else {
    html += "<div class='score-grid'>";
    for(int t = 1; t <= 4; t++) {
      if(!teamActive[t]) continue;
      
      String tName = "UNKNOWN"; String tCol = "#fff"; String bgCol = "rgba(255,255,255,0.05)";
      if (t == 1) { tName = "RED TEAM"; tCol = "#ff3366"; bgCol = "rgba(255,51,102,0.1)"; }
      else if (t == 2) { tName = "BLUE TEAM"; tCol = "#00ffff"; bgCol = "rgba(0,255,255,0.1)"; }
      else if (t == 3) { tName = "GREEN TEAM"; tCol = "#00ff88"; bgCol = "rgba(0,255,136,0.1)"; }
      else if (t == 4) { tName = "YELLOW TEAM"; tCol = "#ffcc00"; bgCol = "rgba(255,204,0,0.1)"; }

      html += "<div style='background:" + bgCol + "; border-left: 4px solid " + tCol + "; padding: 15px; border-radius: 8px;'>";
      html += "<div style='display:flex; justify-content:space-between; align-items:center; margin-bottom: 10px;'>";
      html += "<span style='color:" + tCol + "; font-weight:bold; font-size:18px;'>" + tName + "</span>";
      html += "<span style='color:#fff; font-size:22px; font-weight:bold;'>" + String(teamKills[t]) + " KILLS</span></div>";
      html += "<table><tr><th>GUN</th><th>K</th><th>D</th><th>K/D</th></tr>";
      for(int i = 0; i < MAX_PLAYERS; i++) {
        if(players[i].active && players[i].team == t) {
          float kd = players[i].deaths == 0 ? (float)players[i].kills : (float)players[i].kills / players[i].deaths;
          
          html += "<tr class='player-row' onclick='swapTeam(" + String(players[i].id) + "," + String(players[i].team) + ")'>";
          html += "<td style='color:#fff; font-weight:bold;'>#" + String(players[i].id) + "</td>";
          html += "<td>" + String(players[i].kills) + "</td>";
          html += "<td>" + String(players[i].deaths) + "</td>";
          html += "<td class='kd-val'>" + String(kd, 1) + "</td></tr>";
        }
      }
      html += "</table></div>";
    }
    html += "</div>";
  }
  
  // Dashboard Action Buttons
  html += "<div style='display:flex; gap:10px; margin-top:20px;'>";
  html += "<button type='button' class='btn-green' style='margin:0; padding:12px; font-size:12px; flex:1; border-style:solid;' onclick='toggleFS()'>⛶ ENLARGE SCOREBOARD</button>";
  html += "<form action='/clearscores' style='margin:0; flex:1;'><button class='btn-red' style='margin:0; padding:12px; font-size:12px; width:100%; border-style:dotted;'>RESET SCOREBOARD</button></form>";
  html += "</div></div>";
  
  html += "<div class='card'><h2>DIRECTIVES</h2><form action='/setrules'><div class='grid'>";
  html += "<div><label>MODE</label><select name='mode'><option value='0' "+String(gameMode==0?"selected":"")+">Combat</option><option value='1' "+String(gameMode==1?"selected":"")+">Unlimited</option></select></div>";
  html += "<div><label>HEALTH (HP)</label><input type='number' name='hp' value='"+String(configHealth)+"'></div>";
  html += "<div><label>DAMAGE</label><input type='number' name='dmg' value='"+String(baseDamage)+"'></div>";
  html += "<div><label>LIVES</label><input type='number' name='lvs' value='"+String(configLives)+"'></div>";
  // FIX: Added the missing MAGS input field here!
  html += "<div><label>MAGS</label><input type='number' name='mags' value='"+String(configMags)+"'></div>";
  html += "</div><button type='submit' style='margin-top:25px; background:#0ff; color:#000;'>SYNC DIRECTIVES</button></form></div>";
  
  // =====================================================================
  // ESPORTS FULL SCREEN SCOREBOARD OVERLAY
  // =====================================================================
  html += "<div id='fs-overlay'>";
  html += "<div class='fs-header'><h2 class='fs-title'>// COMBAT TELEMETRY : LIVE</h2><button class='fs-close' onclick='toggleFS()'>EXIT</button></div>";
  html += "<div class='fs-grid' data-teams='" + String(activeTeamCount) + "'>";

  if (activeTeamCount > 0) {
    for(int t = 1; t <= 4; t++) {
      if(!teamActive[t]) continue;
      String tName = "UNKNOWN"; String tCol = "#fff"; 
      if (t == 1) { tName = "RED TEAM"; tCol = "#ff3366"; }
      else if (t == 2) { tName = "BLUE TEAM"; tCol = "#00ffff"; }
      else if (t == 3) { tName = "GREEN TEAM"; tCol = "#00ff88"; }
      else if (t == 4) { tName = "YELLOW TEAM"; tCol = "#ffcc00"; }
      
      html += "<div class='fs-team-panel' style='border-color:" + tCol + ";'>";
      html += "<div class='fs-team-header' style='border-bottom-color:" + tCol + ";'>";
      html += "<span class='fs-team-name' style='color:" + tCol + ";'>" + tName + "</span>";
      html += "<span class='fs-team-score' style='color:#fff;'>" + String(teamKills[t]) + " KILLS</span></div>";
      html += "<div class='fs-table-wrap'><table class='fs-table'>";
      html += "<tr><th style='text-align:left;'>OPERATOR</th><th>KILLS</th><th>DEATHS</th><th>K/D RATIO</th></tr>";
      for(int i = 0; i < MAX_PLAYERS; i++) {
        if(players[i].active && players[i].team == t) {
          float kd = players[i].deaths == 0 ? (float)players[i].kills : (float)players[i].kills / players[i].deaths;
          html += "<tr class='player-row' onclick='swapTeam(" + String(players[i].id) + "," + String(players[i].team) + ")'>";
          html += "<td style='color:#fff; font-weight:bold; text-align:left;'>GUN #" + String(players[i].id) + "</td>";
          html += "<td>" + String(players[i].kills) + "</td>";
          html += "<td>" + String(players[i].deaths) + "</td>";
          html += "<td style='color:#fff; font-weight:bold;'>" + String(kd, 1) + "</td></tr>";
        }
      }
      html += "</table></div></div>";
    }
  } else {
    html += "<div style='color:#fff; opacity:0.3; text-align:center; padding:50px; font-size:24px; grid-column:1/-1;'>NO OPERATORS DETECTED</div>";
  }
  
  html += "</div></div>"; // End fs-grid and fs-overlay
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void handleJoin() {
  if (server.hasArg("id")) {
    int prevCount = playerCount;
    PlayerRecord* p = getPlayer(server.arg("id").toInt());
    
    if (p && server.hasArg("team")) {
      p->team = server.arg("team").toInt();
    }
  }
  server.send(200, "text/plain", "OK");
  if (currentScreen == 4) drawScoreboardScreen();
}

void handleHitRecord() {
  if (server.hasArg("isDeath") && server.arg("isDeath") == "1") {
    if (server.hasArg("shooterID")) {
      PlayerRecord* s = getPlayer(server.arg("shooterID").toInt());
      if(s) s->kills++;
    }
    if (server.hasArg("victimID")) {
      PlayerRecord* v = getPlayer(server.arg("victimID").toInt());
      if(v) v->deaths++;
    }
  }
  server.send(200, "text/plain", "Recorded");
  if (currentScreen == 4) drawScoreboardScreen();
}

void handleClearScores() {
  for(int i=0; i<MAX_PLAYERS; i++) players[i].active = false;
  playerCount = 0;
  scoreboardOffset = 0; // Reset page to 1
  server.sendHeader("Location", "/");
  server.send(303);
  if (currentScreen == 4) drawScoreboardScreen();
}

void handleStatus() {
  String assignedTeam = "0";
  if (server.hasArg("id")) {
    PlayerRecord* p = getPlayer(server.arg("id").toInt());
    if (p) assignedTeam = String(p->team); 
  }

  String payload = String(hostStatus) + "," + String(baseDamage) + "," + String(configLives) + "," + String(configMags) + "," + String(configHealth) + "," + String(gameMode) + "," + assignedTeam;
  server.send(200, "text/plain", payload);
}

void handleSet() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    if (cmd == "start") { hostStatus = 1; gameStartTime = millis(); }
    else if (cmd == "lobby") hostStatus = 0;
    else if (cmd == "kill") hostStatus = 2;
  }
  
  server.sendHeader("Location", "/");
  server.send(303);
  
  if(currentScreen == 0) drawMainScreen();
}

void handleSetRules() {
  if (server.hasArg("hp")) configHealth = server.arg("hp").toInt();
  if (server.hasArg("dmg")) baseDamage = server.arg("dmg").toInt();
  if (server.hasArg("lvs")) configLives = server.arg("lvs").toInt();
  if (server.hasArg("mags")) configMags = server.arg("mags").toInt();
  if (server.hasArg("mode")) gameMode = server.arg("mode").toInt();
  
  server.sendHeader("Location", "/");
  server.send(303);
  
  // Instantly redraw if CYD is looking at the settings page while rules are updated via Web
  if(currentScreen == 1) drawSetupScreen1();
  else if(currentScreen == 2) drawSetupScreen2();
}

// --- SMART NETWORK ROUTINE ---
void connectToNetwork() {
  tft.fillScreen(C_BG);
  tft.setTextColor(C_CYAN);
  tft.setTextSize(2);
  tft.setCursor(10, 120); tft.print("SEARCHING FOR");
  tft.setCursor(10, 150); tft.print("ARENA ROUTER...");

  WiFi.mode(WIFI_STA);
  IPAddress local_IP(192, 168, 1, 100);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(local_IP, gateway, subnet);
  WiFi.begin(routerSSID, routerPass);

  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { 
    delay(500);
    attempts++;
  }

  tft.fillScreen(C_BG);
  if (WiFi.status() == WL_CONNECTED) {
    usingExternalRouter = true;
    tft.setTextColor(C_GREEN);
    tft.setCursor(10, 120); tft.print("ROUTER FOUND!");
    tft.setCursor(10, 150); tft.print("IP: 192.168.1.100");
    dnsServer.start(DNS_PORT, customDomain, WiFi.localIP());
  } else {
    usingExternalRouter = false;
    tft.setTextColor(C_YELLOW);
    tft.setCursor(10, 120); tft.print("ROUTER NOT FOUND");
    tft.setCursor(10, 150); tft.print("STARTING CYD HUB");
    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPass);
    dnsServer.start(DNS_PORT, customDomain, WiFi.softAPIP());
  }
  delay(2000); 
}

void setupHost() {
  server.on("/", handleRoot);
  server.on("/join", handleJoin);
  server.on("/hit", handleHitRecord);
  server.on("/status", handleStatus);
  
  // Web dashboard controls
  server.on("/set", handleSet);
  server.on("/setrules", handleSetRules);
  server.on("/clearscores", handleClearScores);
  
  server.begin();
}

// --- CYBERPUNK UI DRAWING HELPERS ---

void drawHeader(const char* title) {
  tft.fillRect(0, 0, 240, 25, C_BG); 
  tft.drawLine(0, 25, 240, 25, C_CYAN);
  tft.drawLine(0, 27, 240, 27, C_CYAN); 
  
  tft.setTextColor(C_CYAN);
  tft.setTextSize(2);
  tft.setCursor(5, 5);
  tft.print(title);
  
  // Network Tag - Dynamically right-aligned
  tft.setTextSize(2);
  String netTag = usingExternalRouter ? "[LA]" : "[HUB]";
  int textWidth = netTag.length() * 12; 
  tft.setCursor(235 - textWidth, 5); 
  tft.setTextColor(usingExternalRouter ? C_GREEN : C_YELLOW);
  tft.print(netTag);
}

// Hollow Neon Button Style
void drawButton(int x, int y, int w, int h, uint16_t outlineColor, const char* text, bool isSelected = false) {
  if (isSelected) {
    tft.fillRoundRect(x, y, w, h, 3, outlineColor);
    tft.setTextColor(COLOR_BLACK);
  } else {
    tft.fillRoundRect(x, y, w, h, 3, C_BG); 
    tft.drawRoundRect(x, y, w, h, 3, outlineColor);
    tft.drawRoundRect(x+1, y+1, w-2, h-2, 3, outlineColor); 
    tft.setTextColor(outlineColor);
  }
  
  tft.setTextSize(2);
  int textWidth = strlen(text) * 12; 
  int textHeight = 16;               
  tft.setCursor(x + (w - textWidth) / 2, y + (h - textHeight) / 2);
  tft.print(text);
}

// --- PORTRAIT SCREENS (240x320) ---

void drawMainScreen() {
  tft.fillScreen(C_BG);
  drawHeader("// ARENA");

  tft.setTextSize(2);
  tft.setCursor(10, 40);
  tft.setTextColor(C_CYAN);
  tft.print("STATUS: ");
  if (hostStatus == 0)      { tft.setTextColor(C_YELLOW); tft.print("LOBBY"); }
  else if (hostStatus == 1) { tft.setTextColor(C_GREEN);  tft.print("LIVE!"); }
  else                      { tft.setTextColor(C_RED);    tft.print("KILLED"); }

  // Massive Timer Box (y=75 to 175)
  tft.fillRoundRect(20, 75, 200, 100, 6, C_BG);
  tft.drawRoundRect(20, 75, 200, 100, 6, C_CYAN);
  tft.drawRoundRect(21, 76, 198, 98, 6, C_CYAN);
  tft.drawRoundRect(22, 77, 196, 96, 6, C_CYAN); // Triple thick glow
  
  tft.setTextColor(C_CYAN); 
  tft.setTextSize(6); // Massive 36x48 font
  if (hostStatus == 1 && gameStartTime > 0) {
    unsigned long el = (millis() - gameStartTime) / 1000;
    char buf[10]; sprintf(buf, "%d:%02d", (int)(el/60), (int)(el%60));
    int textWidth = strlen(buf) * 36;
    tft.setCursor(20 + (200 - textWidth) / 2, 101); 
    tft.print(buf);
  } else {
    int textWidth = 4 * 36;
    tft.setCursor(20 + (200 - textWidth) / 2, 101); 
    tft.print("0:00");
  }

  // Row 1 Buttons (Pushed Down)
  if (hostStatus == 1) {
    drawButton(10, 195, 105, 50, C_YELLOW, "LOBBY", true); 
  } else if (hostStatus == 2) {
    drawButton(10, 195, 105, 50, C_YELLOW, "LOBBY"); 
  } else {
    drawButton(10, 195, 105, 50, C_GREEN, "START");
  }
  
  drawButton(125, 195, 105, 50, C_RED, "KILL ALL", hostStatus == 2); 
  
  // Row 2 Buttons (Anchored to Bottom)
  drawButton(10, 260, 105, 50, C_CYAN, "SCORES"); 
  drawButton(125, 260, 105, 50, C_MAGENTA, "RULES"); 
}

void drawSetupScreen1() {
  tft.fillScreen(C_BG);
  drawHeader("// DIR_1: MODE");
  tft.setTextSize(2);

  tft.setCursor(10, 45); tft.setTextColor(C_CYAN); tft.print("Game Mode:");
  drawButton(10, 70, 220, 35, gameMode == 1 ? C_MAGENTA : C_CYAN, gameMode == 1 ? "UNLIMITED" : "COMBAT");

  tft.setCursor(10, 125); tft.setTextColor(C_CYAN); tft.print("Lives:");
  drawButton(85, 115, 45, 35, C_GREY, "-");
  
  tft.fillRect(135, 115, 40, 35, C_BG);
  tft.setTextColor(C_MAGENTA);
  char buf[5]; sprintf(buf, "%d", configLives);
  int textWidth = strlen(buf) * 12;
  tft.setCursor(155 - (textWidth / 2), 125); 
  tft.print(buf);
  
  drawButton(185, 115, 45, 35, C_GREY, "+");

  tft.setCursor(10, 185); tft.setTextColor(C_CYAN); tft.print("Mags:");
  drawButton(85, 175, 45, 35, C_GREY, "-");
  
  tft.fillRect(135, 175, 40, 35, C_BG);
  tft.setTextColor(C_MAGENTA);
  sprintf(buf, "%d", configMags);
  textWidth = strlen(buf) * 12;
  tft.setCursor(155 - (textWidth / 2), 185);
  tft.print(buf);
  
  drawButton(185, 175, 45, 35, C_GREY, "+");

  drawButton(10, 270, 105, 40, C_RED, "BACK");
  drawButton(125, 270, 105, 40, C_GREEN, "NEXT");
}

void drawSetupScreen2() {
  tft.fillScreen(C_BG);
  drawHeader("// DIR_2: DMG");
  tft.setTextSize(2);

  tft.setCursor(10, 65); tft.setTextColor(C_CYAN); tft.print("Dmg:");
  drawButton(75, 55, 45, 35, C_GREY, "-");
  
  tft.fillRect(125, 55, 45, 35, C_BG);
  tft.setTextColor(C_MAGENTA);
  char buf[5]; sprintf(buf, "%d", baseDamage);
  int textWidth = strlen(buf) * 12;
  tft.setCursor(147 - (textWidth / 2), 65); 
  tft.print(buf);
  
  drawButton(185, 55, 45, 35, C_GREY, "+");

  tft.setCursor(10, 135); tft.setTextColor(C_CYAN); tft.print("HP:");
  drawButton(75, 125, 45, 35, C_GREY, "-");
  
  tft.fillRect(125, 125, 45, 35, C_BG);
  tft.setTextColor(C_MAGENTA);
  sprintf(buf, "%d", configHealth);
  textWidth = strlen(buf) * 12;
  tft.setCursor(147 - (textWidth / 2), 135);
  tft.print(buf);
  
  drawButton(185, 125, 45, 35, C_GREY, "+");

  drawButton(10, 270, 105, 40, C_RED, "BACK");
  drawButton(125, 270, 105, 40, C_GREEN, "DONE");
}

void drawScoreboardScreen() {
  tft.fillScreen(C_BG);
  drawHeader("// TELEMETRY");
  
  tft.setTextSize(1);
  tft.setTextColor(C_MAGENTA);
  
  // Display Page Indicator
  char pageStr[30];
  sprintf(pageStr, "(TAP ROW TO SWAP)  P.%d", (scoreboardOffset / 6) + 1);
  tft.setCursor(5, 35); tft.print(pageStr);

  int teamKills[5] = {0};
  bool teamActive[5] = {false};
  for(int i=0; i<MAX_PLAYERS; i++) {
    if(players[i].active) {
      teamKills[players[i].team] += players[i].kills;
      teamActive[players[i].team] = true;
    }
  }

  tft.setTextSize(2);
  tft.setTextColor(C_RED); tft.setCursor(5, 55); 
  if(teamActive[1]) { tft.print("R:"); tft.print(teamKills[1]); }
  
  tft.setTextColor(C_CYAN); tft.setCursor(125, 55); 
  if(teamActive[2]) { tft.print("B:"); tft.print(teamKills[2]); }
  
  tft.setTextColor(C_GREEN); tft.setCursor(5, 75); 
  if(teamActive[3]) { tft.print("G:"); tft.print(teamKills[3]); }
  
  tft.setTextColor(C_YELLOW); tft.setCursor(125, 75); 
  if(teamActive[4]) { tft.print("Y:"); tft.print(teamKills[4]); }

  tft.fillRect(0, 95, 240, 2, C_MAGENTA);
  tft.setTextColor(C_CYAN);
  tft.setTextSize(1);
  tft.setCursor(5, 101); tft.print("ID");
  tft.setCursor(65, 101); tft.print("KILLS");
  tft.setCursor(130, 101); tft.print("DEATH");
  tft.setCursor(185, 101); tft.print("K/D");
  tft.fillRect(0, 113, 240, 1, C_MAGENTA);

  tft.setTextSize(2);
  int drawn = 0;
  int skipped = 0;
  
  for(int i=0; i<MAX_PLAYERS; i++) {
    if(players[i].active) {
      // PAGINATION LOGIC: Skip players if we are on a later page
      if (skipped < scoreboardOffset) {
        skipped++;
        continue;
      }
      
      // Draw up to 6 players for this current page
      if (drawn < 6) { 
        int y = 122 + (drawn * 22); 
        float kd = players[i].deaths == 0 ? (float)players[i].kills : (float)players[i].kills / players[i].deaths;

        uint16_t tColor = COLOR_WHITE;
        if(players[i].team == 1) tColor = C_RED;
        else if(players[i].team == 2) tColor = C_CYAN;
        else if(players[i].team == 3) tColor = C_GREEN;
        else if(players[i].team == 4) tColor = C_YELLOW;

        tft.setTextColor(tColor);
        tft.setCursor(5, y); tft.print("#"); tft.print(players[i].id);

        tft.setTextColor(COLOR_WHITE);
        tft.setCursor(75, y); tft.print(players[i].kills);
        tft.setCursor(140, y); tft.print(players[i].deaths);
        tft.setCursor(185, y); tft.print(kd, 1);
        drawn++;
      }
    }
  }

  // Paged Nav Buttons
  drawButton(5, 275, 40, 35, C_CYAN, "<");
  drawButton(50, 275, 65, 35, C_RED, "WIPE");
  drawButton(120, 275, 65, 35, C_GREY, "BACK");
  drawButton(190, 275, 40, 35, C_CYAN, ">");
}

// --- SETUP ---
void setup() {
  Serial.begin(115200);
  
  pinMode(TFT_BL, OUTPUT);
  digitalWrite(TFT_BL, HIGH);

  SPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  ts.begin();
  ts.setRotation(0); 

  tftSpi.begin(TFT_CLK, TFT_MISO, TFT_MOSI, TFT_CS);
  tft.begin();
  tft.setRotation(0); 
  
  connectToNetwork();
  setupHost();
  drawMainScreen();
}

// --- TOUCH HANDLING ---
bool checkTouchArea(int tx, int ty, int bx, int by, int bw, int bh) {
  return (tx >= bx && tx <= bx + bw && ty >= by && ty <= by + bh);
}

void processTouch(int tx, int ty) {
  if (currentScreen == 0) { // MAIN SCREEN
    // ROW 1
    if (checkTouchArea(tx, ty, 10, 195, 105, 50)) { 
      if (hostStatus == 1 || hostStatus == 2) { hostStatus = 0; } 
      else { hostStatus = 1; gameStartTime = millis(); } 
      drawMainScreen(); 
    } 
    else if (checkTouchArea(tx, ty, 125, 195, 105, 50)) { hostStatus = 2; drawMainScreen(); } 
    // ROW 2
    else if (checkTouchArea(tx, ty, 10, 260, 105, 50)) { 
      currentScreen = 4; 
      scoreboardOffset = 0; // Reset to Page 1 when opening
      drawScoreboardScreen(); 
    } 
    else if (checkTouchArea(tx, ty, 125, 260, 105, 50)) { currentScreen = 1; drawSetupScreen1(); } 
  } 
  else if (currentScreen == 1) { // SETUP 1
    bool updated = false;
    if (checkTouchArea(tx, ty, 10, 70, 220, 35)) { gameMode = !gameMode; updated = true; } 
    if (checkTouchArea(tx, ty, 85, 115, 45, 35)) { if(configLives>1) configLives--; updated = true; } 
    if (checkTouchArea(tx, ty, 185, 115, 45, 35)) { if(configLives<99) configLives++; updated = true; } 
    if (checkTouchArea(tx, ty, 85, 175, 45, 35)) { if(configMags>1) configMags--; updated = true; } 
    if (checkTouchArea(tx, ty, 185, 175, 45, 35)) { if(configMags<99) configMags++; updated = true; } 
    
    if (checkTouchArea(tx, ty, 10, 270, 105, 40)) { currentScreen = 0; drawMainScreen(); return; } 
    if (checkTouchArea(tx, ty, 125, 270, 105, 40)) { currentScreen = 2; drawSetupScreen2(); return; } 
    
    if (updated) drawSetupScreen1(); 
  }
  else if (currentScreen == 2) { // SETUP 2
    bool updated = false;
    if (checkTouchArea(tx, ty, 75, 55, 45, 35)) { if(baseDamage>5) baseDamage-=5; updated = true; } 
    if (checkTouchArea(tx, ty, 185, 55, 45, 35)) { if(baseDamage<200) baseDamage+=5; updated = true; } 
    if (checkTouchArea(tx, ty, 75, 125, 45, 35)) { if(configHealth>10) configHealth-=10; updated = true; } 
    if (checkTouchArea(tx, ty, 185, 125, 45, 35)) { if(configHealth<500) configHealth+=10; updated = true; } 

    if (checkTouchArea(tx, ty, 10, 270, 105, 40)) { currentScreen = 1; drawSetupScreen1(); return; } 
    if (checkTouchArea(tx, ty, 125, 270, 105, 40)) { currentScreen = 0; drawMainScreen(); return; } 
    
    if (updated) drawSetupScreen2();
  }
  else if (currentScreen == 4) { // SCOREBOARD
    
    // Bottom Navigation Buttons
    if (checkTouchArea(tx, ty, 120, 275, 65, 35)) { currentScreen = 0; drawMainScreen(); return; } // BACK
    if (checkTouchArea(tx, ty, 50, 275, 65, 35)) { handleClearScores(); drawScoreboardScreen(); return; } // WIPE
    
    // Previous Page
    if (checkTouchArea(tx, ty, 5, 275, 40, 35)) { 
      if (scoreboardOffset >= 6) scoreboardOffset -= 6; 
      drawScoreboardScreen(); 
      return; 
    } 
    
    // Next Page
    if (checkTouchArea(tx, ty, 190, 275, 40, 35)) { 
      // Only flip page if there are actually more players waiting on the next screen
      int activeCount = 0;
      for(int i=0; i<MAX_PLAYERS; i++) if(players[i].active) activeCount++;
      if (scoreboardOffset + 6 < activeCount) scoreboardOffset += 6; 
      drawScoreboardScreen(); 
      return; 
    } 

    // TAP A ROW TO CHANGE TEAMS (Accounts for Pagination)
    int drawn = 0;
    int skipped = 0;
    for(int i=0; i<MAX_PLAYERS; i++) {
      if(players[i].active) {
        if (skipped < scoreboardOffset) {
          skipped++;
          continue;
        }
        
        if (drawn < 6) {
          int y = 122 + (drawn * 22);
          if (checkTouchArea(tx, ty, 0, y - 4, 240, 26)) {
            players[i].team++;
            if (players[i].team > 4) players[i].team = 1; 
            drawScoreboardScreen(); 
            return;
          }
          drawn++;
        }
      }
    }
  }
}

// --- MAIN LOOP ---
void loop() {
  dnsServer.processNextRequest();
  server.handleClient(); 

  if (ts.tirqTouched() && ts.touched()) {
    TS_Point p = ts.getPoint();
    int mapX = map(p.x, 300, 3800, 0, 240);
    int mapY = map(p.y, 300, 3800, 0, 320);
    processTouch(mapX, mapY);
    delay(200); 
  }

  // Update massive timer
  if (currentScreen == 0 && hostStatus == 1) {
    if (millis() - lastTimerUpdate >= 1000) {
      lastTimerUpdate = millis();
      unsigned long elapsedSeconds = (millis() - gameStartTime) / 1000;
      char buf[10]; sprintf(buf, "%d:%02d", (int)(elapsedSeconds/60), (int)(elapsedSeconds%60));

      // Wipe inner box
      tft.fillRoundRect(23, 78, 194, 94, 4, C_BG); 
      
      tft.setTextColor(C_CYAN);
      tft.setTextSize(6);
      
      int textWidth = strlen(buf) * 36;
      tft.setCursor(20 + (200 - textWidth) / 2, 101); 
      tft.print(buf);
    }
  }
}
