#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// =========================================================================
// HARDWARE CONFIGURATION (2.2" PARALLEL ST7789)
// =========================================================================

class LGFX : public lgfx::LGFX_Device {
    lgfx::Panel_ST7789 _panel_instance; 
    lgfx::Bus_Parallel8 _bus_instance;  
public:
    LGFX(void) {
        {
            auto cfg = _bus_instance.config();
            cfg.freq_write = 25000000;
            cfg.pin_wr = 4;
            cfg.pin_rd = 2;
            cfg.pin_rs = 16;
            cfg.pin_d0 = 15;
            cfg.pin_d1 = 13;
            cfg.pin_d2 = 12;
            cfg.pin_d3 = 14;
            cfg.pin_d4 = 27;
            cfg.pin_d5 = 25;
            cfg.pin_d6 = 33;
            cfg.pin_d7 = 32;

            _bus_instance.config(cfg);
            _panel_instance.setBus(&_bus_instance);
        }
        {
            auto cfg = _panel_instance.config();
            cfg.pin_cs = 17;
            cfg.pin_rst = -1;
            cfg.pin_busy = -1;
            cfg.panel_width = 240;
            cfg.panel_height = 320;
            cfg.offset_x = 0;
            cfg.offset_y = 0;
            cfg.offset_rotation = 0;
            cfg.readable = false;
            cfg.invert = false;
            cfg.rgb_order = false;
            cfg.dlen_16bit = false;
            cfg.bus_shared = true;

            _panel_instance.config(cfg);
        }
        setPanel(&_panel_instance);
    }
};

static LGFX tft; 

// =========================================================================
// NETWORK & SERVER VARIABLES
// =========================================================================

WebServer server(80);
DNSServer dnsServer; 
const byte DNS_PORT = 53;

const char* apSSID       = "LaserTag_Hub";
const char* apPass       = "pewpewpew";
const char* routerSSID   = "LaserArena";
const char* routerPass   = "pewpewpew";
const char* customDomain = "lasertagarena.game"; 
bool usingExternalRouter = false; // Tracks if we are on LA or LAH

// =========================================================================
// GAME STATE & PLAYER TRACKING
// =========================================================================

int hostStatus   = 0;  // 0=Lobby, 1=Running, 2=Kill All
int baseDamage   = 50; 
int configLives  = 3;
int configMags   = 5;
int configHealth = 100;
int gameMode     = 0;  // 0=Combat, 1=Unlimited

#define MAX_PLAYERS 64

struct PlayerRecord {
  int id;
  int team; // 1=Red, 2=Blue, 3=Green, 4=Yellow
  int kills;
  int deaths;
  bool active;
};

PlayerRecord players[MAX_PLAYERS]; 
int playerCount = 0;

// =========================================================================
// UI & DISPLAY STATE
// =========================================================================

unsigned long gameStartTime = 0;
unsigned long lastDisplayUpdate = 0;
bool forceRedraw = true;

#define C_BG      tft.color565(5, 5, 15)    
#define C_GREEN   tft.color565(0, 255, 100) 
#define C_RED     tft.color565(255, 50, 100) 
#define C_BLUE    tft.color565(0, 255, 255) 
#define C_YELLOW  tft.color565(255, 200, 0) 
#define C_WHITE   0xFFFF

PlayerRecord* getPlayer(int id) {
  if (id <= 0) return nullptr;
  
  // Find existing player
  for(int i = 0; i < MAX_PLAYERS; i++) {
    if(players[i].active && players[i].id == id) return &players[i];
  }
  
  // Register new player
  for(int i = 0; i < MAX_PLAYERS; i++) {
    if(!players[i].active) {
      players[i].id = id;
      players[i].team = 1; // Default to Red if missing
      players[i].kills = 0;
      players[i].deaths = 0;
      players[i].active = true;
      playerCount++;
      return &players[i];
    }
  }
  return nullptr;
}

// =========================================================================
// WEB SERVER ENDPOINTS (MOBILE DASHBOARD)
// =========================================================================

void handleRoot() {
  int teamKills[5] = {0, 0, 0, 0, 0};
  bool teamActive[5] = {false, false, false, false, false};
  
  for(int i = 0; i < MAX_PLAYERS; i++) {
    if(players[i].active) {
      teamKills[players[i].team] += players[i].kills;
      teamActive[players[i].team] = true;
    }
  }

  String html = "<!DOCTYPE html><html><head><title>Arena Command</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0, maximum-scale=1.0, user-scalable=no, viewport-fit=cover'>";
  
  html += "<style>";
  html += "* { box-sizing: border-box; -webkit-tap-highlight-color: transparent; }";
  html += "body { font-family: -apple-system, system-ui, sans-serif; background: #050510; color: #0ff; margin: 0; padding: env(safe-area-inset-top) 15px 40px 15px; }";
  html += ".card { background: rgba(0, 255, 255, 0.08); padding: 20px; border: 1px solid #0ff; border-radius: 20px; margin-bottom: 20px; box-shadow: 0 10px 40px rgba(0,0,0,0.7); }";
  html += "h1 { font-size: 26px; text-align: center; color: #fff; text-shadow: 0 0 15px #0ff; margin: 25px 0; letter-spacing: 1px; }";
  html += "h2 { font-size: 13px; margin-top: 0; border-bottom: 1px solid rgba(0, 255, 255, 0.2); padding-bottom: 12px; margin-bottom: 15px; text-transform: uppercase; letter-spacing: 2px; color: rgba(255,255,255,0.7); }";
  
  html += "button { width: 100%; padding: 22px; margin-bottom: 15px; background: #001a1a; border: 2px solid #0ff; color: #0ff; border-radius: 14px; font-weight: bold; font-size: 18px; transition: 0.1s; }";
  html += "button:active { transform: scale(0.97); background: #0ff; color: #000; }";
  html += ".btn-red { border-color: #f0f; color: #f0f; background: #1a001a; }";
  html += ".btn-green { border-color: #0f0; color: #0f0; background: #001a00; }";
  
  html += ".grid { display: flex; flex-wrap: wrap; gap: 15px; }";
  html += ".grid > div { flex: 1 1 calc(50% - 15px); min-width: 130px; }";
  html += ".score-grid { display: grid; grid-template-columns: repeat(auto-fit, minmax(280px, 1fr)); gap: 20px; }";
  
  html += "label { font-size: 10px; display: block; margin-bottom: 8px; color: #fff; opacity: 0.8; font-weight: 800; letter-spacing: 1px; }";
  html += "input, select { width: 100%; padding: 16px; background: #000; color: #fff; border: 1px solid #0ff; border-radius: 12px; font-size: 18px; outline: none; -webkit-appearance: none; }";
  
  html += "table { width: 100%; text-align: center; border-collapse: collapse; margin-top: 10px; }";
  html += "th, td { padding: 12px 5px; border-bottom: 1px solid rgba(255, 255, 255, 0.1); font-size: 15px; }";
  html += "th { font-size: 10px; opacity: 0.5; color: #fff; }";
  html += ".kd-val { font-weight: bold; color: #fff; }";
  html += "</style>";
  
  html += "<script>setInterval(function(){ location.reload(); }, 12000);</script>";
  html += "</head><body>";
  
  html += "<h1>ARENA COMMAND ";
  html += usingExternalRouter ? "<span style='color:#0f0; font-size:18px;'>[LA ROUTER]</span>" : "<span style='color:#ff0; font-size:18px;'>[LAH HUB]</span>";
  html += "</h1>";
  
  // Status Card
  html += "<div class='card'><h2>SYSTEM STATUS: ";
  if (hostStatus == 0) html += "<span style='color:#ff0'>LOBBY</span></h2>";
  else if (hostStatus == 1) html += "<span style='color:#0f0'>ACTIVE</span></h2>";
  else html += "<span style='color:#f0f'>OVERRIDE</span></h2>";
  
  html += "<form action='/set'>";
  if (hostStatus == 0) html += "<button name='cmd' value='start' class='btn-green'>START MISSION</button>";
  else html += "<button name='cmd' value='lobby'>RETURN LOBBY</button>";
  html += "<button name='cmd' value='kill' class='btn-red' style='border-style:dashed; margin-top:10px;'>EMERGENCY KILL ALL</button>";
  html += "</form></div>";
  
  // DYNAMIC TEAM SCOREBOARD
  html += "<div class='card'><h2>TEAM STANDINGS</h2>";
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
      html += "<span style='color:#fff; font-size:22px; font-weight:bold;'>" + String(teamKills[t]) + " KILLS</span>";
      html += "</div>";
      
      html += "<table><tr><th>GUN</th><th>K</th><th>D</th><th>K/D</th></tr>";
      for(int i = 0; i < MAX_PLAYERS; i++) {
        if(players[i].active && players[i].team == t) {
          float kd = players[i].deaths == 0 ? (float)players[i].kills : (float)players[i].kills / players[i].deaths;
          html += "<tr><td style='color:#fff; font-weight:bold;'>#" + String(players[i].id) + "</td>";
          html += "<td>" + String(players[i].kills) + "</td>";
          html += "<td>" + String(players[i].deaths) + "</td>";
          html += "<td class='kd-val'>" + String(kd, 1) + "</td></tr>";
        }
      }
      html += "</table></div>";
    }
    
    html += "</div>";
  }
  html += "<form action='/clearscores'><button class='btn-red' style='margin-top:20px; padding:12px; font-size:12px; width:auto; border-style:dotted;'>RESET SCOREBOARD</button></form></div>";
  
  // Directives Grid
  html += "<div class='card'><h2>DIRECTIVES</h2><form action='/setrules'><div class='grid'>";
  html += "<div><label>MODE</label><select name='mode'><option value='0' "+String(gameMode==0?"selected":"")+">Combat</option><option value='1' "+String(gameMode==1?"selected":"")+">Unlimited</option></select></div>";
  html += "<div><label>HEALTH (HP)</label><input type='number' name='hp' value='"+String(configHealth)+"'></div>";
  html += "<div><label>DAMAGE</label><input type='number' name='dmg' value='"+String(baseDamage)+"'></div>";
  html += "<div><label>LIVES</label><input type='number' name='lvs' value='"+String(configLives)+"'></div>";
  html += "<div><label>MAGS</label><input type='number' name='mags' value='"+String(configMags)+"'></div>";
  html += "</div><button type='submit' style='margin-top:25px; background:#0ff; color:#000;'>SYNC DIRECTIVES</button></form></div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleJoin() {
  if (server.hasArg("id")) {
    int prevCount = playerCount;
    PlayerRecord* p = getPlayer(server.arg("id").toInt());
    
    if (p && server.hasArg("team")) {
      int incomingTeam = server.arg("team").toInt();
      if (p->team != incomingTeam) {
        p->team = incomingTeam;
        forceRedraw = true;
      }
    }
    if (playerCount > prevCount) forceRedraw = true; 
  }
  server.send(200, "text/plain", "OK");
}

void handleStatus() {
  if (server.hasArg("id")) {
    int prevCount = playerCount;
    PlayerRecord* p = getPlayer(server.arg("id").toInt());
    
    if (p && server.hasArg("team")) {
      int incomingTeam = server.arg("team").toInt();
      if (p->team != incomingTeam) {
        p->team = incomingTeam;
        forceRedraw = true;
      }
    }
    if (playerCount > prevCount) forceRedraw = true; 
  }
  String payload = String(hostStatus) + "," + String(baseDamage) + "," + String(configLives) + "," + String(configMags) + "," + String(configHealth) + "," + String(gameMode);
  server.send(200, "text/plain", payload);
}

void handleSet() {
  if (server.hasArg("cmd")) {
    String cmd = server.arg("cmd");
    if (cmd == "start") { hostStatus = 1; gameStartTime = millis(); }
    else if (cmd == "lobby") hostStatus = 0;
    else if (cmd == "kill") hostStatus = 2;
  }
  forceRedraw = true;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleSetRules() {
  if (server.hasArg("hp")) configHealth = server.arg("hp").toInt();
  if (server.hasArg("dmg")) baseDamage = server.arg("dmg").toInt();
  if (server.hasArg("lvs")) configLives = server.arg("lvs").toInt();
  if (server.hasArg("mags")) configMags = server.arg("mags").toInt();
  if (server.hasArg("mode")) gameMode = server.arg("mode").toInt();
  forceRedraw = true;
  server.sendHeader("Location", "/");
  server.send(303);
}

void handleHit() {
  // Only award Kills AND Deaths if the shot was a lethal blow
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
  
  forceRedraw = true;
  server.send(200, "text/plain", "OK");
}

void handleClearScores() {
  for(int i = 0; i < MAX_PLAYERS; i++) players[i].active = false;
  playerCount = 0;
  forceRedraw = true;
  server.sendHeader("Location", "/");
  server.send(303);
}

// =========================================================================
// CYD TFT DISPLAY ROUTINES
// =========================================================================

void updateTFT() {
  // Calculate team scores
  int teamKills[5] = {0};
  bool teamActive[5] = {false};
  for(int i = 0; i < MAX_PLAYERS; i++) {
    if(players[i].active) {
      teamKills[players[i].team] += players[i].kills;
      teamActive[players[i].team] = true;
    }
  }

  char buf[64];
  tft.setTextColor(C_BLUE, C_BG);
  tft.setTextSize(2);
  tft.setCursor(5, 5); tft.print("ARENA MASTER v10.5");

  // Draw Network Indicator (LA = Router, LAH = CYD Hub)
  tft.setCursor(255, 5);
  tft.setTextColor(usingExternalRouter ? C_GREEN : C_YELLOW, C_BG);
  tft.print(usingExternalRouter ? "[LA] " : "[LAH]");

  tft.setCursor(5, 30);
  if (hostStatus == 0)      { tft.setTextColor(C_YELLOW, C_BG); tft.print("STATUS: LOBBY     "); }
  else if (hostStatus == 1) { tft.setTextColor(C_GREEN, C_BG);  tft.print("STATUS: RUNNING   "); }
  else                      { tft.setTextColor(C_RED, C_BG);    tft.print("STATUS: KILL ALL  "); }

  tft.setTextColor(C_WHITE, C_BG);
  tft.setCursor(220, 30);
  if (hostStatus == 1) {
    unsigned long el = (millis() - gameStartTime) / 1000;
    sprintf(buf, "T:%02d:%02d", (int)(el / 60), (int)(el % 60));
  } else { sprintf(buf, "T:00:00"); }
  tft.print(buf);

  tft.setTextColor(C_GREEN, C_BG); tft.setTextSize(1);
  tft.setCursor(5, 60); 
  sprintf(buf, "MODE:%s HP:%d DMG:%d LVS:%d", gameMode == 0 ? "COMBAT" : "UNLIM ", configHealth, baseDamage, configLives);
  tft.print(buf);

  // Dynamic Team Totals Row 
  tft.fillRect(0, 75, 320, 12, C_BG); 
  int cursorX = 5;
  if (teamActive[1]) { tft.setTextColor(C_RED, C_BG); tft.setCursor(cursorX, 75); tft.print("RED:"); tft.print(teamKills[1]); cursorX += 55; }
  if (teamActive[2]) { tft.setTextColor(C_BLUE, C_BG); tft.setCursor(cursorX, 75); tft.print("BLU:"); tft.print(teamKills[2]); cursorX += 55; }
  if (teamActive[3]) { tft.setTextColor(C_GREEN, C_BG); tft.setCursor(cursorX, 75); tft.print("GRN:"); tft.print(teamKills[3]); cursorX += 55; }
  if (teamActive[4]) { tft.setTextColor(C_YELLOW, C_BG); tft.setCursor(cursorX, 75); tft.print("YLW:"); tft.print(teamKills[4]); }

  if (forceRedraw) {
    tft.fillRect(0, 90, 320, 2, C_BLUE);
    tft.setTextColor(C_YELLOW, C_BG);
    tft.setCursor(5, 96); tft.print("OPERATOR");
    tft.setCursor(90, 96); tft.print("KILLS");
    tft.setCursor(150, 96); tft.print("DEATHS");
    tft.setCursor(250, 96); tft.print("K/D");
    tft.fillRect(0, 108, 320, 1, C_BLUE);
    tft.fillRect(0, 110, 320, 130, C_BG); 
  }

  int row = 0;
  for(int i = 0; i < MAX_PLAYERS; i++) {
    // Note: CYD TFT physically only has room to show the top 8 rows.
    // The web browser interface will show all 64 players!
    if(players[i].active && row < 8) {
      int y = 115 + (row * 15);
      float kd = players[i].deaths == 0 ? (float)players[i].kills : (float)players[i].kills / players[i].deaths;
      tft.fillRect(0, y, 320, 14, C_BG); 
      
      uint16_t tColor = C_WHITE;
      if (players[i].team == 1) tColor = C_RED;
      else if (players[i].team == 2) tColor = C_BLUE;
      else if (players[i].team == 3) tColor = C_GREEN;
      else if (players[i].team == 4) tColor = C_YELLOW;

      tft.setTextColor(tColor, C_BG);
      tft.setCursor(5, y); 
      sprintf(buf, "GUN %-2d", players[i].id);
      tft.print(buf);
      
      tft.setTextColor(C_WHITE, C_BG);
      tft.setCursor(95, y); tft.print(players[i].kills);
      tft.setCursor(160, y); tft.print(players[i].deaths);
      tft.setCursor(250, y); sprintf(buf, "%-5.1f", kd); tft.print(buf);
      
      row++;
    }
  }
}

// =========================================================================
// SMART NETWORK ROUTINE
// =========================================================================

void connectToNetwork() {
  tft.fillScreen(C_BG);
  tft.setTextColor(C_WHITE, C_BG);
  tft.setTextSize(2);
  tft.setCursor(10, 100); tft.print("SEARCHING FOR");
  tft.setCursor(10, 130); tft.print("ARENA ROUTER...");

  // 1. Try to connect to Field Router
  WiFi.mode(WIFI_STA);
  
  // Force the CYD to be IP address 192.168.1.100 so guns can always find it
  IPAddress local_IP(192, 168, 1, 100);
  IPAddress gateway(192, 168, 1, 1);
  IPAddress subnet(255, 255, 255, 0);
  WiFi.config(local_IP, gateway, subnet);
  
  WiFi.begin(routerSSID, routerPass);

  // Wait up to 10 seconds for the big router
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { 
    delay(500);
    attempts++;
  }

  // 2. THE SMART DECISION
  tft.fillScreen(C_BG);
  
  if (WiFi.status() == WL_CONNECTED) {
    // ROUTER MODE ACTIVE (50+ Players)
    usingExternalRouter = true;
    tft.setTextColor(C_GREEN, C_BG);
    tft.setCursor(10, 100); tft.print("ROUTER FOUND!");
    tft.setCursor(10, 130); tft.print("IP: 192.168.1.100");
    dnsServer.start(DNS_PORT, customDomain, WiFi.localIP());
  } else {
    // HUB MODE FALLBACK (10 Players)
    usingExternalRouter = false;
    tft.setTextColor(C_YELLOW, C_BG);
    tft.setCursor(10, 100); tft.print("ROUTER NOT FOUND");
    tft.setCursor(10, 130); tft.print("STARTING CYD HUB");

    WiFi.disconnect();
    WiFi.mode(WIFI_AP);
    WiFi.softAP(apSSID, apPass);
    
    dnsServer.start(DNS_PORT, customDomain, WiFi.softAPIP());
  }
  
  delay(2000); // Give the user time to read the connection status
  tft.fillScreen(C_BG);
}

// =========================================================================
// MAIN SETUP
// =========================================================================

void setup() {
  Serial.begin(115200);

  // Turn on CYD backlight
  pinMode(0, OUTPUT);
  digitalWrite(0, HIGH);

  // Initialize LovyanGFX
  tft.init();
  tft.setRotation(1); 
  
  for(int i = 0; i < MAX_PLAYERS; i++) players[i].active = false;
  
  // Run the Smart Network Routine!
  connectToNetwork();
  
  // Set up Server Endpoints
  server.on("/", handleRoot);
  server.on("/join", handleJoin);
  server.on("/status", handleStatus);
  server.on("/set", handleSet);
  server.on("/setrules", handleSetRules);
  server.on("/hit", handleHit);
  server.on("/clearscores", handleClearScores);
  
  // Catch-all 
  server.onNotFound([]() {
    server.sendHeader("Location", "/", true);
    server.send(302, "text/plain", "");
  });

  server.begin();
  updateTFT();
}

// =========================================================================
// MAIN LOOP
// =========================================================================

void loop() {
  dnsServer.processNextRequest(); 
  server.handleClient();
  
  if (millis() - lastDisplayUpdate >= 1000 || forceRedraw) {
    updateTFT();
    lastDisplayUpdate = millis();
    forceRedraw = false;
  }
}
