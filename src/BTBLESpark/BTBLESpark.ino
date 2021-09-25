#include "BluetoothSerial.h"
#include "M5Core2.h"

#include "Spark.h"
#include "SparkAppIO.h"
#include "SparkIO.h"

#define PGM_NAME "BTBLESpark"

SparkAppIO app_io(true);
SparkIO spark_io(true);

bool app_connected;
uint8_t pre;                          // current preset
uint8_t selected_preset;
int i, j, p;
int found;

uint8_t current_model, current_effect;
char *new_name;

unsigned int cmdsub;
SparkMessage msg;
SparkPreset preset;
SparkPreset presets[6];

char spark_amps[][STR_LEN]{"RolandJC120", "Twin", "ADClean", "94MatchDCV2", "Bassman", "AC Boost", "Checkmate",
                           "TwoStoneSP50", "Deluxe65", "Plexi", "OverDrivenJM45", // "OverDrivenLuxVerb",
                           "Bogner", "OrangeAD30", //"AmericanHighGain", 
                           "SLO100", "YJM100", "Rectifier",
                           "EVH", "SwitchAxeLead", "Invader", "BE101", "Acoustic", "AcousticAmpV2", //"FatAcousticV2", "FlatAcoustic", 
                           "GK800", "Sunny3000", "W600", "Hammer500"};

char spark_drives[][STR_LEN]{"Booster", //"DistortionTS9", //"Overdrive", 
                             "Fuzz", "ProCoRat", "BassBigMuff",
                             "GuitarMuff", // "MaestroBassmaster", 
                             "SABdriver"};
char spark_compressors[][STR_LEN]{"LA2AComp", "BlueComp", "Compressor", "BassComp" }; // "BBEOpticalComp"};


char spark_modulations[][STR_LEN]{"Tremolo", "ChorusAnalog", "Flanger", "Phaser", "Vibrato01", "UniVibe",
                                  "Cloner", "MiniVibe", "Tremolator"}; //"TremoloSquare"};
char spark_delays[][STR_LEN]{"DelayMono", 
                             //"DelayEchoFilt", 
                             "VintageDelay"}; 
                             //"DelayReverse",
                             //"DelayMultiHead", 
                             //"DelayRe201"
                             
char* effects[7];

void dump_preset(SparkPreset preset) {
  int i,j;

  Serial.print(preset.curr_preset); Serial.print(" ");
  Serial.print(preset.preset_num); Serial.print(" ");
  Serial.print(preset.Name); Serial.print(" ");

  Serial.println(preset.Description);

  for (j=0; j<7; j++) {
    Serial.print("    ");
    Serial.print(preset.effects[j].EffectName); Serial.print(" ");
    if (preset.effects[j].OnOff == true) Serial.print(" On "); else Serial.print (" Off ");
    for (i = 0; i < preset.effects[j].NumParameters; i++) {
      Serial.print(preset.effects[j].Parameters[i]); Serial.print(" ");
    }
    Serial.println();
  }
}


void setup() {
  M5.begin();
  M5.Lcd.fillScreen(TFT_BLACK);
  M5.Lcd.setTextSize(4);
  M5.Lcd.print(PGM_NAME);

  start_bt();         // true uses BLE
  connect_to_spark();
  
  start_ser();
  
  pre = 0;
  current_model = 0;
  current_effect = 3;

  app_connected = false;
}


  
void loop() {

  M5.update();
  
  if (!isBTConnected) {
    DEBUG("--------------------------");
    DEBUG("Connection has dropped :-(");
    DEBUG("--------------------------");
    while (true);
  }
  
  spark_io.process();
  app_io.process();

  // Messages from the amp
  
  if (spark_io.get_message(&cmdsub, &msg, &preset)) { //there is something there
    Serial.print("From Spark: ");
    Serial.println(cmdsub, HEX);
    
    if (cmdsub == 0x0301) {
      p = preset.preset_num;
      j = preset.curr_preset;
      if (p == 0x7f)       
        p = 4;
      if (j == 0x01)
        p = 5;
      presets[p] = preset;
      dump_preset(preset);
    }

    if (cmdsub == 0x0306) {
      strcpy(presets[5].effects[3].EffectName, msg.str2);
      Serial.print("Change to amp model ");
      Serial.println(presets[5].effects[3].EffectName);
    }

    if (cmdsub == 0x0337) {
      Serial.print("Change model parameter ");
      Serial.print(msg.str1);
      Serial.print(" ");
      Serial.print(msg.param1);   
      Serial.print(" ");   
      Serial.println(msg.val);
    }
    
    if (cmdsub == 0x0338) {
      selected_preset = msg.param2;
      presets[5] = presets[selected_preset];
      Serial.print("Change to preset: ");
      Serial.println(selected_preset, HEX);
    }      
    
    if (cmdsub == 0x0327) {
      selected_preset = msg.param2;
      if (selected_preset == 0x7f) 
        selected_preset=4;
      presets[selected_preset] = presets[5];
      Serial.print("Store in preset: ");
      Serial.println(selected_preset, HEX);
    }

    if (cmdsub == 0x0310) {
      selected_preset = msg.param2;
      j = msg.param1;
      if (selected_preset == 0x7f) 
        selected_preset = 4;
      if (j == 0x01) 
        selected_preset = 5;
      presets[5] = presets[selected_preset];
      Serial.print("Hadware preset is: ");
      Serial.println(selected_preset, HEX);
    }
  }

  // Messages from the app

  if (app_io.get_message(&cmdsub, &msg, &preset)) { //there is something there
    Serial.print("To Spark: ");
    Serial.println(cmdsub, HEX);

    if (cmdsub == 0x022f) 
      app_connected = true;
    
    if (cmdsub == 0x0104) {
      Serial.print("Change model parameter ");
      Serial.print(msg.str1);
      Serial.print(" ");
      Serial.print(msg.param1);   
      Serial.print(" ");   
      Serial.println(msg.val);
    }    
    
    if (cmdsub == 0x0101) {
      p = preset.preset_num;
      if (p == 0x7f) 
        p = 4;
      presets[p]=preset;  
      Serial.print("Send new preset: ");
      Serial.println(p, HEX);      
    }

    if (cmdsub == 0x0138) {
      if (msg.param1 == 0x01) DEBUG("Got a change to preset 0x100 from the app");
      
      selected_preset = msg.param2;
      if (selected_preset == 0x7f) 
        selected_preset=4;
      presets[5] = presets[selected_preset];
      Serial.print("Change to preset: ");
      Serial.println(selected_preset, HEX);
    }

    if (cmdsub == 0x0106) {
      found = -1;
      for (i = 1; found == -1 && i <= 5; i++) {
        if (strstr(effects[i], msg.str1) != NULL) {
          Serial.println (i);
          found = i;
        }
      }
      if (found >= 0) {
        strcpy(presets[5].effects[found].EffectName, msg.str2);
        Serial.print("Change to new effect ");
        Serial.print(msg.str1);
        Serial.print(" ");
        Serial.println(msg.str2);
      }
    }
  }   


}
