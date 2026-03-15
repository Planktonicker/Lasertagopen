#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH110X.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HTTPClient.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <IRutils.h>
#include <Preferences.h>

// =========================================================================
// PIN DEFINITIONS
// =========================================================================
#define PIN_OLED_SDA    21
#define PIN_OLED_SCL    22
#define OLED_ADDR       0x3C

#define PIN_ENC_CLK     25
#define PIN_ENC_DT      26
#define PIN_ENC_SW      27

#define PIN_TRIGGER     32  
#define PIN_BUZZER      23
#define PIN_IR_TX       4
#define PIN_IR_RX       15
#define PIN_BATT        34
#define PIN_DEBUG_HIT   14 // Touch this pin to GND to simulate getting shot!

// =========================================================================
// HARDWARE OBJECTS
// =========================================================================
Adafruit_SH1106G display = Adafruit_SH1106G(128, 64, &Wire, -1);
IRsend irsend(PIN_IR_TX);
IRrecv irrecv(PIN_IR_RX);
decode_results results;
Preferences preferences;

// =========================================================================
// STATE VARIABLES
// =========================================================================

// Hardware states
int lastCLK      = HIGH;
int lastSW       = HIGH;
int lastTrigger  = HIGH;
int lastDebugHit = HIGH;

// Debounce & Hold Timers 
unsigned long configHoldTimer = 0;
unsigned long swHoldTimer     = 0;
unsigned long lastTurnTime    = 0;
unsigned long lastButtonTime  = 0;
bool reloadTriggered          = false;

// System States
enum GameState { BOOT_MENU, HOST_LOBBY, TEAM_SELECT, PLAYING, RESPAWNING, DEAD, CONFIG_MENU, RELOADING };
GameState currentState = BOOT_MENU;

// Permanent Device Settings
int savedRole  = 1;     
int myGunID    = 1;       
int baseDamage = 50;   

// Player Variables
bool isHost      = false;
int selectedTeam = 1; 

// Combat Rules & Live Stats
int lives        = 3;
int hp           = 100;
int magazines    = 5;
int ammo         = 7; 
int maxAmmo      = 7;
unsigned long respawnTimer = 0;

// Config Menu Variables
int configMenuIndex  = 0;
bool configIsEditing = false;

// Host Server Variables
const char* apSSID       = "LaserTag_Hub";
const char* apPass       = "pewpewpew";
const char* routerSSID   = "LaserArena";
const char* routerPass   = "pewpewpew";
String serverIP          = "http://192.168.4.1"; // Defaults to CYD Hub, updates dynamically
int lastHostStatus       = 0;         

// Host Configurable Rules
int configLives      = 3;
int configMags       = 5;
int configHealth     = 100;
int gameMode         = 0; // 0 = Combat, 1 = Unlimited
bool registeredWithServer = false;


// =========================================================================
// AUDIO & UTILITY SYSTEMS
// =========================================================================

void playSound(int type) {
  switch(type) {
    case 1: tone(PIN_BUZZER, 1200, 50); break;  // Shoot
    case 2: tone(PIN_BUZZER, 400, 150); delay(150); tone(PIN_BUZZER, 600, 150); break; // Reload Complete
    case 3: tone(PIN_BUZZER, 200, 500); break;  // Hit
    case 4: tone(PIN_BUZZER, 100, 1000); break; // Death
    case 5: tone(PIN_BUZZER, 800, 100); break;  // Error/Menu beep
    case 6: tone(PIN_BUZZER, 1500, 30); break;  // Reloading tick
  }
  delay(10); // Timer breathing room
}

int getBatteryPercent() {
  int raw = analogRead(PIN_BATT);
  float voltage = (raw / 4095.0) * 3.3 * 2.0; 
  int percent = map(voltage * 100, 320, 420, 0, 100);
  
  if (percent > 100) percent = 100;
  if (percent < 0) percent = 0;
  
  return percent;
}

const char* getTeamName(int t) {
  if (t == 1) return "RED";
  if (t == 2) return "BLUE";
  if (t == 3) return "GREEN";
  if (t == 4) return "YELLOW";
  return "UNK";
}

// =========================================================================
// DISPLAY ROUTINES
// =========================================================================

void drawUI() {
  display.clearDisplay();
  
  // --------------------------------------------------
  // CONFIG MENU
  // --------------------------------------------------
  if (currentState == CONFIG_MENU) {
    display.fillRect(0, 0, 128, 12, SH110X_WHITE);
    display.setTextColor(SH110X_BLACK);
    display.setTextSize(1);
    display.setCursor(10, 2); 
    display.print("--- CONFIG MODE ---");
    
    // Config: Role
    if (configMenuIndex == 0) display.fillRect(0, 14, 128, 12, SH110X_WHITE);
    display.setTextColor(configMenuIndex == 0 ? SH110X_BLACK : SH110X_WHITE);
    display.setCursor(4, 16); 
    display.print("Role: "); 
    display.print(savedRole == 0 ? "[ASK]" : (savedRole == 1 ? "[PLAYER]" : "[HOST]"));
    if (configIsEditing && configMenuIndex == 0) display.print(" <");

    // Config: Gun ID
    display.setTextColor(SH110X_WHITE);
    if (configMenuIndex == 1) display.fillRect(0, 26, 128, 12, SH110X_WHITE);
    display.setTextColor(configMenuIndex == 1 ? SH110X_BLACK : SH110X_WHITE);
    display.setCursor(4, 28); 
    display.print("Gun ID: "); 
    display.print(myGunID); 
    if (configIsEditing && configMenuIndex == 1) display.print(" <");

    // Config: Base Damage
    display.setTextColor(SH110X_WHITE);
    if (configMenuIndex == 2) display.fillRect(0, 38, 128, 12, SH110X_WHITE);
    display.setTextColor(configMenuIndex == 2 ? SH110X_BLACK : SH110X_WHITE);
    display.setCursor(4, 40); 
    display.print("Base Dmg: "); 
    display.print(baseDamage); 
    if (configIsEditing && configMenuIndex == 2) display.print(" <");

    // Config: Save
    display.setTextColor(SH110X_WHITE);
    if (configMenuIndex == 3) display.fillRect(0, 52, 128, 12, SH110X_WHITE);
    display.setTextColor(configMenuIndex == 3 ? SH110X_BLACK : SH110X_WHITE);
    display.setCursor(10, 54); 
    display.print(">> SAVE & REBOOT <<");
    
    display.display();
    return;
  }

  // --------------------------------------------------
  // GLOBAL TOP STATUS BAR
  // --------------------------------------------------
  display.fillRect(0, 0, 128, 12, SH110X_WHITE);
  display.setTextColor(SH110X_BLACK);
  display.setTextSize(1);
  display.setCursor(2, 2); 
  
  if (isHost) {
    display.print("HOST SERVER");
  } else {
    display.print("GUN "); 
    display.print(myGunID);
    if (WiFi.status() == WL_CONNECTED) display.print(" [WIFI]");
  }
  
  // Battery Indicator
  int batt = getBatteryPercent();
  if (batt == 100) display.setCursor(100, 2);
  else if (batt >= 10) display.setCursor(106, 2);
  else display.setCursor(112, 2);
  display.print(batt); 
  display.print("%");
  
  display.setTextColor(SH110X_WHITE);

  // --------------------------------------------------
  // SCREEN CONTEXTS
  // --------------------------------------------------
  if (currentState == BOOT_MENU) {
    display.setCursor(10, 20); 
    display.print("SELECT DEVICE ROLE:");
    
    if (selectedTeam == 1) { 
      display.fillRoundRect(10, 35, 108, 12, 3, SH110X_WHITE); 
      display.setTextColor(SH110X_BLACK); 
    }
    display.setCursor(15, 37); 
    display.print("1. PLAYER MODE");
    
    display.setTextColor(SH110X_WHITE);
    if (selectedTeam == 2) { 
      display.fillRoundRect(10, 48, 108, 12, 3, SH110X_WHITE); 
      display.setTextColor(SH110X_BLACK); 
    }
    display.setCursor(15, 50); 
    display.print("2. HOST SERVER");
  } 
  
  else if (currentState == TEAM_SELECT) {
    display.drawRoundRect(10, 16, 108, 28, 3, SH110X_WHITE);
    display.setTextSize(2);
    
    int xOffset = 46;
    if (selectedTeam == 2) xOffset = 40; 
    if (selectedTeam == 3) xOffset = 34; 
    if (selectedTeam == 4) xOffset = 28; 
    display.setCursor(xOffset, 22); 
    display.print(getTeamName(selectedTeam));
    
    display.setTextSize(1);
    display.setCursor(8, 48); 
    display.print("Push = Start Offline");
    display.setCursor(8, 56); 
    display.print(WiFi.status() == WL_CONNECTED ? "Connected to Server!" : "Waiting for Server...");
  }
  
  else if (currentState == RESPAWNING) {
    display.drawRoundRect(4, 16, 120, 44, 4, SH110X_WHITE);
    display.setTextSize(2);
    display.setCursor(16, 22); 
    display.print("REVIVING");
    
    display.setTextSize(1);
    display.setCursor(20, 44); 
    display.print("Respawn in: ");
    
    int timeLeft = 5 - ((millis() - respawnTimer) / 1000);
    if (timeLeft < 0) timeLeft = 0;
    display.print(timeLeft);
    display.print("s");
  }
  
  else if (currentState == PLAYING || currentState == RELOADING) {
    display.drawRoundRect(0, 14, 128, 38, 3, SH110X_WHITE); 
    display.setTextSize(2);
    display.setCursor(4, 18); 
    display.print("HP:"); 
    display.print(hp);
    
    display.setTextSize(1);
    
    // Lives display
    display.setCursor(80, 18); 
    display.print("LVS:"); 
    if (gameMode == 1) display.print("INF"); 
    else display.print(lives);
    
    // Magazines display
    display.setCursor(80, 28); 
    display.print("MAG:"); 
    if (gameMode == 1) display.print("INF"); 
    else display.print(magazines);
    
    // Team display
    display.setCursor(4, 38); 
    display.print("TEAM:"); 
    display.print(getTeamName(selectedTeam));
    
    // Visual Ammo Blocks
    for (int i = 0; i < 7; i++) {
      int bx = 11 + (i * 15);
      if (i < ammo) display.fillRoundRect(bx, 54, 10, 8, 2, SH110X_WHITE);
      else display.drawRoundRect(bx, 54, 10, 8, 2, SH110X_WHITE);
    }

    if (currentState == RELOADING) {
      display.fillRect(34, 25, 60, 14, SH110X_BLACK);
      display.drawRect(34, 25, 60, 14, SH110X_WHITE);
      display.setCursor(40, 28);
      display.print("RELOAD");
    }
  }
  
  else if (currentState == DEAD) {
    display.drawRoundRect(4, 16, 120, 44, 4, SH110X_WHITE);
    display.setTextSize(2);
    display.setCursor(10, 22); 
    display.print("GAME OVER");
    
    display.setTextSize(1);
    display.setCursor(15, 44); 
    display.print("Permanently Dead");
  }

  display.display();
}

// =========================================================================
// GAME LOGIC ROUTINES
// =========================================================================

void processDamage(int shooterID, int incomingDamage) {
  hp -= incomingDamage;
  
  // Screen flash effect
  display.invertDisplay(true);
  delay(60); 
  display.invertDisplay(false);
  
  bool isDeath = false;

  if (hp <= 0) {
    hp = 0;
    isDeath = true;
    
    // Only decrement lives if we are playing normal Combat mode
    if (gameMode == 0) {
      lives--;
    }

    // Always respawn if we have lives left OR if we are playing Unlimited mode
    if (lives > 0 || gameMode == 1) {
      currentState = RESPAWNING;
      respawnTimer = millis();
      playSound(4); 
    } else {
      currentState = DEAD;
      playSound(4); 
    }
  } else {
    playSound(3); 
  }

  // Report to Server
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    http.setTimeout(300); 
    String url = serverIP + "/hit?shooterID=" + String(shooterID) + "&victimID=" + String(myGunID);
    if (isDeath) {
      url += "&isDeath=1";
    }
    http.begin(url); 
    http.GET(); 
    http.end();
  }
  
  drawUI();
}

void animateReload() {
  currentState = RELOADING;
  ammo = 0; 
  drawUI();
  delay(150);

  for (int i = 1; i <= maxAmmo; i++) {
    ammo = i;
    playSound(6); 
    drawUI();
    delay(120); 
  }

  playSound(2); 
  currentState = PLAYING;
  drawUI();
}

// =========================================================================
// SMART NETWORK ROUTINE
// =========================================================================

void connectToNetwork() {
  display.clearDisplay();
  display.setTextColor(SH110X_WHITE);
  display.setTextSize(1);
  display.setCursor(10, 20); 
  display.print("SEARCHING FOR");
  display.setCursor(10, 35); 
  display.print("ARENA ROUTER...");
  display.display();

  WiFi.mode(WIFI_STA);
  WiFi.begin(routerSSID, routerPass);

  // Wait up to 10 seconds for the big router
  int attempts = 0;
  while (WiFi.status() != WL_CONNECTED && attempts < 20) { 
    delay(500);
    attempts++;
  }

  if (WiFi.status() == WL_CONNECTED) {
    serverIP = "http://192.168.1.100"; // Big router found! Target the fixed CYD IP.
  } else {
    display.clearDisplay();
    display.setCursor(10, 20); 
    display.print("ROUTER NOT FOUND");
    display.setCursor(10, 35); 
    display.print("FALLBACK: CYD HUB");
    display.display();

    WiFi.disconnect();
    WiFi.begin(apSSID, apPass);
    
    // Wait up to 5 seconds for the CYD Hub
    attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 10) { 
      delay(500);
      attempts++;
    }
    serverIP = "http://192.168.4.1"; // Hub mode active! Target the default CYD IP.
  }
}

// =========================================================================
// MAIN SETUP
// =========================================================================

void setup() {
  Serial.begin(115200);
  
  // Hardware Setup
  pinMode(PIN_ENC_CLK, INPUT);
  pinMode(PIN_ENC_DT, INPUT);
  pinMode(PIN_ENC_SW, INPUT_PULLUP);
  pinMode(PIN_TRIGGER, INPUT_PULLUP);
  pinMode(PIN_DEBUG_HIT, INPUT_PULLUP); 
  pinMode(PIN_BUZZER, OUTPUT);
  
  lastCLK     = digitalRead(PIN_ENC_CLK);
  lastTrigger = digitalRead(PIN_TRIGGER);

  irsend.begin();
  irrecv.enableIRIn();

  Wire.begin(PIN_OLED_SDA, PIN_OLED_SCL);
  display.begin(OLED_ADDR, true);
  
  // Boot Screen
  display.clearDisplay();
  display.setTextSize(2);
  display.setTextColor(SH110X_WHITE);
  display.setCursor(15, 25); 
  display.print("OS BOOT");
  display.display();
  
  // Load Saved Preferences
  preferences.begin("lasertag", false);
  savedRole  = preferences.getInt("role", 1);
  myGunID    = preferences.getInt("gunID", 1);
  baseDamage = preferences.getInt("damage", 50);
  delay(1000);
  
  // Initialize Role
  if (savedRole == 1) {
    isHost = false;
    connectToNetwork(); 
    currentState = TEAM_SELECT;
  } else {
    currentState = BOOT_MENU;
  }
  
  drawUI();
}

// =========================================================================
// MAIN LOOP
// =========================================================================

void loop() {
  // Prevent interactions during reload animation
  if (currentState == RELOADING) return;

  // --------------------------------------------------
  // 1. CONFIG MENU COMBO (Trigger + Encoder Click)
  // --------------------------------------------------
  if (digitalRead(PIN_TRIGGER) == LOW && digitalRead(PIN_ENC_SW) == LOW) {
    if (configHoldTimer == 0) {
      configHoldTimer = millis();
    } else if (millis() - configHoldTimer > 5000 && currentState != CONFIG_MENU) {
      playSound(5); delay(100); playSound(5); 
      currentState = CONFIG_MENU;
      configMenuIndex = 0;
      configIsEditing = false;
      drawUI();
    }
  } else {
    configHoldTimer = 0;
  }

  // --------------------------------------------------
  // 2. DEBUG HIT SIMULATION
  // --------------------------------------------------
  bool nowDebugHit = digitalRead(PIN_DEBUG_HIT);
  if (nowDebugHit == LOW && lastDebugHit == HIGH) {
    if (currentState == PLAYING && hp > 0) {
      processDamage(99, 50);
    }
  }
  lastDebugHit = nowDebugHit;

  // --------------------------------------------------
  // 3. SERVER POLLING & REGISTRATION
  // --------------------------------------------------
  static unsigned long lastServerPoll = 0;
  if (!isHost && WiFi.status() == WL_CONNECTED) {
    
    // Register if new
    if (!registeredWithServer) {
      HTTPClient http;
      http.setTimeout(300);
      http.begin(serverIP + "/join?id=" + String(myGunID) + "&team=" + String(selectedTeam));
      int code = http.GET();
      if (code == 200) registeredWithServer = true;
      http.end();
    }

    // Poll for status
    if (millis() - lastServerPoll > 2000) {
      lastServerPoll = millis();
      HTTPClient http;
      http.setTimeout(300); 
      http.begin(serverIP + "/status?id=" + String(myGunID) + "&team=" + String(selectedTeam));
      int httpCode = http.GET();
      
      if (httpCode == 200) {
        String payload = http.getString();
        
        // Parse Comma Separated Payload
        int c1 = payload.indexOf(',');
        int c2 = payload.indexOf(',', c1 + 1);
        int c3 = payload.indexOf(',', c2 + 1);
        int c4 = payload.indexOf(',', c3 + 1);
        int c5 = payload.indexOf(',', c4 + 1);
        int c6 = payload.indexOf(',', c5 + 1);
        
        int hostStatus = payload.substring(0, c1).toInt();
        baseDamage     = payload.substring(c1 + 1, c2).toInt(); 
        configLives    = payload.substring(c2 + 1, c3).toInt();
        configMags     = payload.substring(c3 + 1, c4).toInt();
        configHealth   = payload.substring(c4 + 1, c5).toInt();
        
        if (c6 == -1) {
          // Backward compatibility with older CYD codes
          gameMode = payload.substring(c5 + 1).toInt();
        } else {
          // Parse Game Mode AND the new CYD Team Override
          gameMode = payload.substring(c5 + 1, c6).toInt();
          int serverAssignedTeam = payload.substring(c6 + 1).toInt();
          
          // LIVE ROSTER SYNC: If CYD assigned a new team, update instantly!
          if (serverAssignedTeam > 0 && selectedTeam != serverAssignedTeam) {
            selectedTeam = serverAssignedTeam;
            playSound(5); // Play a menu beep so the player knows they were swapped
            drawUI();     // Instantly refresh the OLED
          }
        }
        
        // Event: Host started game
        if (hostStatus == 1 && lastHostStatus == 0) { 
          lives = configLives;
          magazines = configMags;
          hp = configHealth;
          ammo = maxAmmo;
          currentState = PLAYING;
          playSound(5); 
          drawUI();
        } 
        // Event: Host returned to lobby
        else if (hostStatus == 0 && lastHostStatus != 0) { 
          currentState = TEAM_SELECT;
          drawUI();
        }
        // Event: Host activated KILL ALL
        else if (hostStatus == 2 && currentState == PLAYING) { 
          hp = 0;
          currentState = DEAD;
          playSound(4);
          drawUI();
        }
        
        lastHostStatus = hostStatus;
      }
      http.end();
    }
  }

  // --------------------------------------------------
  // 4. RESPAWN TIMER LOGIC
  // --------------------------------------------------
  if (currentState == RESPAWNING) {
    if (millis() - respawnTimer >= 5000) { 
      hp = configHealth;
      ammo = maxAmmo;
      currentState = PLAYING;
      playSound(5); 
      drawUI();
    } else {
      static unsigned long lastCountdownUpdate = 0;
      if (millis() - lastCountdownUpdate > 1000) {
        drawUI();
        lastCountdownUpdate = millis();
      }
    }
  }

  // --------------------------------------------------
  // 5. ENCODER ROTATION (Navigation)
  // --------------------------------------------------
  int nowCLK = digitalRead(PIN_ENC_CLK);
  if (nowCLK != lastCLK && nowCLK == LOW) {
    if (millis() - lastTurnTime > 50) { 
      lastTurnTime = millis();
      int dir = (digitalRead(PIN_ENC_DT) == HIGH) ? 1 : -1;
      playSound(5); 

      if (currentState == CONFIG_MENU) {
        if (configIsEditing) {
          if (configMenuIndex == 0) { savedRole += dir;  if(savedRole > 2) savedRole = 0;   if(savedRole < 0) savedRole = 2; }
          if (configMenuIndex == 1) { myGunID += dir;    if(myGunID > 16) myGunID = 1;      if(myGunID < 1) myGunID = 16; } 
          if (configMenuIndex == 2) { baseDamage += (dir * 5); if(baseDamage > 100) baseDamage = 100; if(baseDamage < 5) baseDamage = 5; }
        } else {
          configMenuIndex += dir;
          if (configMenuIndex > 3) configMenuIndex = 0;
          if (configMenuIndex < 0) configMenuIndex = 3;
        }
      }
      else if (currentState == BOOT_MENU) {
        selectedTeam += dir;
        if(selectedTeam > 2) selectedTeam = 1;
        if(selectedTeam < 1) selectedTeam = 2;
      } 
      else if (currentState == TEAM_SELECT) {
        selectedTeam += dir;
        if(selectedTeam > 4) selectedTeam = 1;
        if(selectedTeam < 1) selectedTeam = 4;
        registeredWithServer = false; 
      }
      drawUI();
    }
  }
  lastCLK = nowCLK;

  // --------------------------------------------------
  // 6. ENCODER PUSH (Hold to Reload, Click to Select)
  // --------------------------------------------------
  bool nowSW = digitalRead(PIN_ENC_SW);
  
  // Hold Logic (Reload)
  if (nowSW == LOW && currentState == PLAYING) {
    if (swHoldTimer == 0) {
      swHoldTimer = millis();
    }
    
    if (!reloadTriggered && (millis() - swHoldTimer >= 3000)) {
      // Allow reload if we have spare mags, OR if we are playing Unlimited mode
      if ((magazines > 0 || gameMode == 1) && ammo < maxAmmo) {
        if (gameMode == 0) {
          magazines--; // Only decrement a mag if we are playing standard Combat mode
        }
        animateReload(); 
      } else {
        playSound(5); 
      }
      reloadTriggered = true;
      drawUI();
    }
  } else {
    swHoldTimer = 0;
    reloadTriggered = false;
  }

  // Click Logic (Menu Selection)
  if (nowSW == LOW && lastSW == HIGH) {
    if (millis() - lastButtonTime > 300) { 
      lastButtonTime = millis();
      
      if (currentState == BOOT_MENU || currentState == TEAM_SELECT || currentState == CONFIG_MENU) {
        playSound(5); 

        if (currentState == CONFIG_MENU) {
          if (configMenuIndex == 3) {
            preferences.putInt("role", savedRole);
            preferences.putInt("gunID", myGunID);
            preferences.putInt("damage", baseDamage);
            display.clearDisplay();
            display.setCursor(20,30); display.print("SAVED. REBOOTING");
            display.display();
            delay(1000);
            ESP.restart(); 
          } else {
            configIsEditing = !configIsEditing;
          }
        }
        else if (currentState == BOOT_MENU) {
          if (selectedTeam == 2) {
            currentState = HOST_LOBBY;
          } else {
            connectToNetwork(); 
            currentState = TEAM_SELECT;
          }
          selectedTeam = 1; 
        } 
        else if (currentState == TEAM_SELECT) {
          lives = configLives;
          hp = configHealth;
          magazines = configMags;
          ammo = maxAmmo;
          currentState = PLAYING;
        }
        drawUI();
      }
    }
  }
  lastSW = nowSW;

  // --------------------------------------------------
  // 7. TRIGGER BUTTON (Shoot)
  // --------------------------------------------------
  bool nowTrigger = digitalRead(PIN_TRIGGER);
  if (nowTrigger == LOW && lastTrigger == HIGH) {
    if (currentState == PLAYING && hp > 0) {
      if (ammo > 0) {
        ammo--;
        playSound(1); 
        uint32_t payload = (selectedTeam << 24) | (myGunID << 8) | baseDamage;
        irsend.sendNEC(payload, 32); 
      } else {
        playSound(5); 
      }
      drawUI();
    }
    delay(50); // Small software debounce
  }
  lastTrigger = nowTrigger;

  // --------------------------------------------------
  // 8. IR RECEIVER (Getting Shot)
  // --------------------------------------------------
  if (currentState == PLAYING && irrecv.decode(&results)) {
    if (results.decode_type == NEC || results.decode_type == 26) {
      uint32_t data = results.value;
      int shooterTeam = (data >> 24) & 0xFF;
      int shooterID = (data >> 8) & 0xFFFF;
      int incomingDamage = data & 0xFF; 

      if (shooterTeam >= 1 && shooterTeam <= 4 && incomingDamage >= 5 && incomingDamage <= 100) {
        if (shooterTeam != selectedTeam) {
          processDamage(shooterID, incomingDamage);
        }
      }
    }
    irrecv.resume(); 
  }

  // --------------------------------------------------
  // 9. IDLE UI UPDATES
  // --------------------------------------------------
  static unsigned long lastBatteryUpdate = 0;
  if (millis() - lastBatteryUpdate > 5000 && currentState != CONFIG_MENU) {
    drawUI();
    lastBatteryUpdate = millis();
  }
}
