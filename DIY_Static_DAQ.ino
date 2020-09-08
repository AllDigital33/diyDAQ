
/*
     DIY Static DAQ - Super Static Pressure and Load Tester
     Open source to the Amateur Rocket Community by Mike and Preston

     Uses the following components:
       Arduino Mega 2560
       Adafruit display 3.5" TFT touchscreen HXD8357D 320x480 with SD card (not using touch)
       https://learn.adafruit.com/adafruit-3-5-color-320x480-tft-touchscreen-breakout/adafruit-gfx-library 
       Pressure transducer 0-4.5v
       Load Cell 500Kg w/HX711 amplifier
       Three Modes:  Monitor, Sample, Review
       Three Review types:  Summary, Pressure Graph, Load Graph
       Uses HX711 library from Bogde:  https://github.com/bogde/HX711

*/

// Configuration (will get overwritten by SD file)

  byte pressureEnabled = 1;
  byte loadEnabled = 1;
  byte sampleInterval = 10;  // Sample every 10ms (100 hz)
  short pressureBase = 0; //100
  short pressure100 = 0; //50
  short triggerStartPressure = 130; // about 60 psi
  short triggerStopPressure = 130;
  short triggerStartLoad = 15;
  short triggerStopLoad = 20;  
  int timeoutDuration = 10000; // ten second timeout on sampling
  int mainMinTime = 2000;  // two seconds minimum sampling - protects against start bouncing
  unsigned long postDuration = 10000; // ten seconds of post
  //float calibration_factor = 1906.0; //1906 for the 1000Kg load cell
  float calibration_factor = 3900.0; //3900 for the 500Kg load cell 


// LCD Display Includes
  #include <SPI.h>
  #include <SD.h>
  #include "Adafruit_GFX.h"
  #include "Adafruit_HX8357.h"
  // These are 'flexible' lines that can be changed for TFT display
  #define TFT_CS 53
  #define TFT_DC 48
  #define TFT_RST 8 // TFT Reset
  #define SD_CS 46
  // Use hardware SPI 
  Adafruit_HX8357 tft = Adafruit_HX8357(TFT_CS, TFT_DC, TFT_RST);

// Switch and LED pins

  byte swMode1 = 32;
  byte swMode2 = 34;
  byte swResults1 = 36;
  byte swResults2 = 38;
  byte swPush1 = 42;
  byte swPush2 = 40;
  byte ledRed = 26;
  byte ledGreen = 28;
  byte ledBlue = 30;
  byte pressurePin = A0;

// declare for HX711 scale sensor
  #include "HX711.h"
  //extern unsigned long rawValue; //custom HX711 library add to get raw reading also
  const int LOADCELL_DOUT_PIN = 3;
  const int LOADCELL_SCK_PIN = 2; 
  HX711 scale;
  float tWeight; 

// working variables
  byte theMode; // 1=moniter 2=sample 3=review
  byte newMode = 1;
  byte theReview; // 1=summary 2=graph1 3=graph2
  byte sampleState = 0;
  short reviewFile = 0;
  byte reviewState = 0;
  byte theError = 0;
  String theErrorString = "";  
  File configFile;
  


void setup() {

  Serial.begin(115200);  //initialize debaug serial monitor
  delay(100);
  Serial.println("Starting up..."); 

  // Declare pins
  pinMode(swMode1, INPUT_PULLUP);
  pinMode(swMode2, INPUT_PULLUP);
  pinMode(swResults1, INPUT_PULLUP);
  pinMode(swResults2, INPUT_PULLUP);
  pinMode(swPush1, INPUT_PULLUP);
  pinMode(swPush2, INPUT_PULLUP);
  pinMode(ledRed, OUTPUT);
  pinMode(ledGreen, OUTPUT);
  pinMode(ledBlue, OUTPUT);  
  pinMode(pressurePin, INPUT);

  // TFT Display Setup
  tft.begin(HX8357D);
  // read diagnostics (optional but can help debug problems)
  uint8_t x = tft.readcommand8(HX8357_RDPOWMODE);
  Serial.print("Display Power Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(HX8357_RDMADCTL);
  Serial.print("MADCTL Mode: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(HX8357_RDCOLMOD);
  Serial.print("Pixel Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(HX8357_RDDIM);
  Serial.print("Image Format: 0x"); Serial.println(x, HEX);
  x = tft.readcommand8(HX8357_RDDSDR);
  Serial.print("Self Diagnostic: 0x"); Serial.println(x, HEX); 
  Serial.println(F("Benchmark                Time (microseconds)"));
  tft.setRotation(1);  // sets TFT screen to landscape
  delay(100);

  //Screen startup 
    tft.fillScreen(HX8357_BLACK);
    tft.setCursor(0, 15);
    tft.setTextColor(HX8357_WHITE);  tft.setTextSize(3);
    tft.println("STARTUP CHECK");
    tft.setCursor(0, 100);
    tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);
    delay(500);

  //initialize SD card
  Serial.println("Initializing SD card...");
  if (!SD.begin(SD_CS)) {
    tft.println("ERROR: No SD Card Detected!");
    theError = 1;
  } else {
    tft.println("SD card is good");
  }
  delay(100);  

  //  createConfigFile(); //only do this when initializing a new config file
  if (theError == 0) readConfigFile();

 // Initialize Load Scale
   if(loadEnabled == 1) {
     scale.begin(LOADCELL_DOUT_PIN, LOADCELL_SCK_PIN);
     scale.set_scale(calibration_factor); //Adjust load cell to this calibration factor
     delay(500);
     scale.tare();  //Reset the scale to 0
     delay(500);
     tWeight = scale.read_average(20); //initiate a read
     tWeight = scale.get_units(5); //in pounds five times
     if (tWeight > 10.0 || tWeight < -10.0) {
       theError = 1;
       theErrorString = "ERROR: Load Cell out of range ";
       theErrorString += tWeight,0;
       tft.println(theErrorString);
     } else {
       tft.println("Load cell is good");
     }
    } else {
      tft.println("Load not enabled - skipping");
   }

  //Check the pressure sensor
  int tempAdd;
  if (pressureEnabled == 1) {
    tempAdd = analogRead(pressurePin);
    if (tempAdd > (pressureBase + 50) || tempAdd < (pressureBase - 50)) {  
      theError = 1;
      theErrorString = "ERROR:  Pressure Sensor value ";
      theErrorString += tempAdd;
      tft.println(theErrorString);   
    } else {
      tft.println("Pressure Sensor is good");
   }
  } else {
    tft.println("Pressure not enabled - skipping");   
  }
  
  // Error check
    if (theError == 0) {
       tft.println(".");
       tft.println("Startup Success!");
       digitalWrite(ledGreen, HIGH);
       delay(3000);
       digitalWrite(ledGreen, LOW);
    } else {
       tft.println(".");
       tft.println("ERROR ON STARTUP!");
       tft.println("<holding for 100 seconds>");
       tempAdd = 1;
       while (tempAdd < 200) { // stay here 100 seconds when error
           digitalWrite(ledRed, HIGH);
           delay(250);
           digitalWrite(ledRed, LOW);
           delay(250);
           tempAdd++;
       }
    }

}

void loop() {         // *********************************** MAIN LOOP BEGIN *************************************

// check mode switches
  if (digitalRead(swMode2) == LOW) {
    if (theMode != 1) newMode = 1;
    theMode = 1;
    monitorMode();
  }
  if (digitalRead(swMode1) == LOW) {
      if (theMode != 3) {
        newMode = 1;
        reviewFile = 0;
      }
      theMode = 3;
      reviewMode();
  }
  if (digitalRead(swMode1) == HIGH && digitalRead(swMode2) == HIGH) {
    if (theMode != 2) newMode = 1;
    theMode = 2;
    sampleMode();
  }


}

void monitorMode() {             // *********************************** MONITOR MODE *************************************

    if (newMode == 1) {
      tft.fillScreen(HX8357_BLACK);
      tft.setCursor(0, 0);
      tft.setTextColor(HX8357_WHITE);  tft.setTextSize(4);
      tft.print("MONITOR MODE");  
      tft.setCursor(0, 280);
      tft.setTextColor(HX8357_WHITE);  tft.setTextSize(3);
      tft.print("   Hold Select to Tare");  
      newMode = 0;
    }
    tft.setCursor(0, 60);
    tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);
    tft.print("      Weight: ");    
    if (loadEnabled == 1) {
      tWeight = scale.get_units(1);  //weight in pounds
      tft.print(tWeight,0);
      tft.println(" lbs");
      //tft.print("  Load value: ");
      //tft.println(rawValue);
    } else {
      tft.println("Disabled in config");
    }
    tft.println(" ");
    tft.print("    Pressure: ");
    float pressurePSI;
    int tempAdd;
    if (pressureEnabled == 1) {
        tempAdd = analogRead(pressurePin);
        tempAdd += analogRead(pressurePin);
        tempAdd += analogRead(pressurePin); 
        pressurePSI = (float)tempAdd / 3.0; // use three samples for stability 
        pressurePSI -= (float)pressureBase;
        pressurePSI = pressurePSI / (float)pressure100;
        pressurePSI = pressurePSI * 100.00;
        tft.print(pressurePSI,0);
        tft.println(" Psi");
        tft.print("Pressure val: ");
        tft.println(tempAdd / 3);
    } else {
      tft.println("Disabled in config");
    }

    if (digitalRead(swPush2) == LOW && loadEnabled == 1) { // Select button - do tare logic
      //tare logic
      scale.tare();
      digitalWrite(ledBlue, HIGH);
      tft.setCursor(0, 210);
      tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);
      tft.print("Tare set complete");  
      delay(250);
      digitalWrite(ledBlue, LOW);   
    }
    if (digitalRead(swPush1) == LOW) { // Scroll button - Show Configuration 10 seconds
      digitalWrite(ledBlue, HIGH);
      displayConfig();
      newMode = 1;
      digitalWrite(ledBlue, LOW);   
    }

    delay(500);
    tft.fillRect(160, 60, 200, 80, HX8357_BLACK); //repaint values without refreshing whole screen
    
  
}


void sampleMode() {           // *********************************** SAMPLE MODE *************************************

    if (newMode == 1) {
      tft.fillScreen(HX8357_BLACK);
      tft.setCursor(0, 0);
      tft.setTextColor(HX8357_WHITE);  tft.setTextSize(4);
      tft.print("SAMPLE MODE");  
      tft.setCursor(0, 280);
      tft.setTextColor(HX8357_WHITE);  tft.setTextSize(3);
      tft.print("   Press Select to Start");  
      newMode = 0;

      tft.setCursor(0, 60);
      tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);
      tft.println("SETTINGS:");tft.println(" ");
      tft.print("    Pressure: ");    
      if(pressureEnabled == 1) tft.println("Yes");
      if(pressureEnabled == 0) tft.println("No");      
      tft.print("        Load: ");    
      if(loadEnabled == 1) tft.println("Yes");
      if(loadEnabled == 0) tft.println("No");     
      tft.print("  Pre Sample: ");    
      tft.print("3"); //hard coded for now to save array memory
      tft.println(" Sec.");
      tft.print(" Post Sample: ");   
      tft.print((postDuration / 1000));
      tft.println(" Sec.");
      tft.print(" Main Timout: ");
      tft.print((timeoutDuration/1000));
      tft.println(" Sec.");
      tft.println(" ");
      if(loadEnabled == 1) {    
      tft.print("      Weight: ");
      tWeight = scale.get_units(1);  //weight in pounds  
      tft.print(tWeight,0);
      tft.println(" lbs");
      }
      if(pressureEnabled == 1) {
      tft.print("    Pressure: ");    
      float pressurePSI;
      int tempAdd;
      tempAdd = analogRead(pressurePin);
      tempAdd += analogRead(pressurePin);
      tempAdd += analogRead(pressurePin); 
      pressurePSI = tempAdd / 3; // use three samples for stability 
        pressurePSI -= (float)pressureBase;
        pressurePSI = pressurePSI / (float)pressure100;
        pressurePSI = pressurePSI * 100.00;
      tft.print(pressurePSI,0);
      tft.println(" Psi");
      }
      delay(500);
    }
    
    if (digitalRead(swPush2) == LOW) {
      
      //reset the screen and jump to the sample loop to do all the work 
      digitalWrite(ledBlue, HIGH);
      tft.fillScreen(HX8357_BLACK); 
      tft.setCursor(0, 280);
      tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);
      tft.print("    Hold Scroll & Select to Abort");  
      tft.setCursor(0, 80);
      tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);
      tft.println("Green LED = Pre Sampling");tft.println(" ");
      tft.println("  Red LED = Main/Hot Sampling");tft.println(" ");
      tft.println(" Blue LED = Post Sampling");tft.println(" ");
      tft.println("White LED = Completed ");tft.println(" ");
      delay(250);
      digitalWrite(ledBlue, LOW);
      delay(500);
      sampleState = 1;
      sampleLoop();      
    }
    
}

void sampleLoop(){           // *********************************** MAIN SAMPLE LOOP BEGIN *************************************

    // declare sample variables here
    byte samplePhase = 1;
    unsigned long SampleTimer = 0;
    unsigned long PreSampleTime[301];
    short PreSamplePressure[301];
    float PreSampleLoad[301];
    short pressureMax = 0;
    float pressureAvg = 0.0;
    float loadMax = 0.0;
    float loadAvg = 0.0;
    short thePressureSample = 0;
    short preCounter = 0;
    short mainCounter = 0;
    short postCounter = 0;
    unsigned long mainStartTime = 0;
    unsigned long mainStopTime = 0;
    unsigned long postTime = 0;
    byte timeout = 0;
    short testNum = 0;

    float TotalImpulse = 0.0;
  
    digitalWrite(ledGreen, HIGH);
    testNum = GetTestNumber();
    testNum++;
    if(testNum > 99) testNum = 1;
    SD.remove("testnum.txt");
    File dataFile = SD.open("testnum.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.println(testNum);    
      dataFile.close();
    } else {
      Serial.println("failed to write new file count!");
    }
  
     tft.setCursor(0, 0);
     tft.setTextColor(HX8357_WHITE);  tft.setTextSize(4);
     tft.print("SAMPLING: "); 
     tft.print("TEST ");tft.println(testNum);
     tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);   
    
    String mainFileName = "main";
    mainFileName += String(testNum);
    mainFileName += ".txt";
    SD.remove(mainFileName);
    File mainFile = SD.open(mainFileName, FILE_WRITE);
    String preFileName = "pre";
    preFileName += String(testNum);
    preFileName += ".txt";
    SD.remove(preFileName);
    File preFile = SD.open(preFileName, FILE_WRITE);  
    String postFileName = "post";
    postFileName += String(testNum);
    postFileName += ".txt";
    SD.remove(postFileName);
    File postFile = SD.open(postFileName, FILE_WRITE);

   //                  ********** THIS IS THE LIVE SAMPLING LOOP ***************
  
  
  while (sampleState == 1) {          // **************  PRE SAMPLE  ***************

        if (digitalRead(swPush2) == LOW && digitalRead(swPush1) == LOW ) {  //abort
          sampleState = 9;
          delay(500);
        }

        if (samplePhase == 1) {      // PRE SAMPLE (keep three seconds of data in an array)
          
          if (millis() >= SampleTimer) {
            SampleTimer = millis() + sampleInterval;
            preCounter++;
            if (preCounter == 301) preCounter = 1;
            PreSampleTime[preCounter] = millis();
            if(pressureEnabled == 1) {
              PreSamplePressure[preCounter] = analogRead(pressurePin);
            } else {
              PreSamplePressure[preCounter] = 0;
            }
            if(loadEnabled == 1) {
              tWeight = scale.get_units(1);
              PreSampleLoad[preCounter] = tWeight;
              if (PreSampleLoad[preCounter] < 0) PreSampleLoad[preCounter] = 0;
            } else {
              PreSampleLoad[preCounter] = 0;
            }
            
            if (PreSamplePressure[preCounter] >= triggerStartPressure || PreSampleLoad[preCounter] >= triggerStartLoad) { //start the party
              samplePhase = 2;
              digitalWrite(ledGreen, LOW);
              digitalWrite(ledRed, HIGH);
              mainStartTime = millis();        
            }
          }
        }
      
        if (samplePhase == 2) {       // **************  MAIN SAMPLE  ***************
          if (millis() >= SampleTimer) {
            SampleTimer = millis() + sampleInterval;
            mainCounter++;
            if (millis() > (mainStartTime + timeoutDuration)) {  // timeout error after ten seconds
              timeout = 1;
              samplePhase = 3;
              digitalWrite(ledRed, LOW);
              digitalWrite(ledBlue, HIGH);
              mainStopTime = millis();    
            }
              if(pressureEnabled == 1) {
                thePressureSample = analogRead(pressurePin);
              } else {
                thePressureSample = 0;
              }
              if(loadEnabled == 1) {
                tWeight = scale.get_units(1);
              } else {
                tWeight = 0;
              }
              TotalImpulse = TotalImpulse + tWeight;
              
              mainFile.print(millis());
              mainFile.print(",");
              mainFile.print(thePressureSample);
              mainFile.print(",");
              mainFile.println(tWeight,0);
              //mainFile.print(",");
              //mainFile.println(rawValue); //added load cell raw value for troubleshooting                        
            if (thePressureSample > pressureMax) pressureMax = thePressureSample;
            pressureAvg += thePressureSample;
            if (tWeight > loadMax) loadMax = tWeight;
            loadAvg += tWeight;
            if (millis() > (mainStartTime + mainMinTime)) { // get a minimum of 2 seconds - anti-bounce
              if (thePressureSample <= triggerStopPressure && tWeight <= triggerStopLoad) {  // all done sampling
                samplePhase = 3;
                digitalWrite(ledRed, LOW);
                digitalWrite(ledBlue, HIGH);
                mainStopTime = millis();
                postTime = postDuration + millis();
                timeout = 0;        
               }
            }
          }
        }
      
      
        if (samplePhase == 3) {      // **************  POST SAMPLE  ***************
          if (millis() >= SampleTimer) {
            SampleTimer = millis() + sampleInterval;
            postCounter++;
              if (pressureEnabled == 1) {
                thePressureSample = analogRead(pressurePin);
              } else {
                thePressureSample = 0;
              }
              if(loadEnabled == 1) {
                tWeight = scale.get_units(1);
              } else {
                tWeight = 0;
              }
              postFile.print(millis());
              postFile.print(",");
              postFile.print(thePressureSample);
              postFile.print(",");
              postFile.println(tWeight,0);
              //postFile.print(",");
              //postFile.println(rawValue); //added load cell raw value for troubleshooting                                
            if (millis() > postTime) { // all done
              samplePhase = 4; 
            }
          }
        }  
      
        if (samplePhase == 4) {             // **************  DONE!  SAMPLE PROCESSING  ***************
          
          // close and write the data files
          Serial.println(F("Sampling is Complete!"));
          mainFile.close();
          postFile.close();
          int tempPos;
          for (int i=1; i <= 300; i++) {  
            tempPos = preCounter + i;
            if (tempPos > 300) tempPos = tempPos - 300;
              preFile.print(PreSampleTime[tempPos]);
              preFile.print(",");
              preFile.print(PreSamplePressure[tempPos]);
              preFile.print(",");
              preFile.println(PreSampleLoad[tempPos],0);              
          }  
          preFile.close();
          // create mainnum.txt for graphing
          String tempFileName = "num";
          tempFileName += String(testNum);
          tempFileName += ".txt";
          SD.remove(tempFileName);
          File tempFile = SD.open(tempFileName, FILE_WRITE);
          tempFile.print(mainCounter);
          tempFile.print(",");
          float maxPSI = 0;
          maxPSI = pressureMax - (float)pressureBase;  
          maxPSI = maxPSI / (float) pressure100;
          maxPSI = maxPSI * 100.0;    
          tempFile.print(maxPSI,0);
          tempFile.print(",");
          tempFile.println(loadMax,0); 
          tempFile.close();
          
          // create the summary file
          tempFileName = "sum";
          tempFileName += String(testNum);
          tempFileName += ".txt";
          SD.remove(tempFileName);
          File sumFile = SD.open(tempFileName, FILE_WRITE);
          sumFile.print("    Burn Time: ");  // Burn Time
          int burnTime = 0;
          burnTime = mainStopTime - mainStartTime;
          sumFile.print(burnTime);
          sumFile.println(" ms");
          sumFile.println(" ");
          sumFile.print(" Max Pressure: ");  // Max Pressure
          float avgPSI = 0;
          if (pressureEnabled == 1) {     
            sumFile.print(maxPSI,0);
            sumFile.println(" PSI");
            sumFile.print(" Avg Pressure: ");  // Avg Pressure
            avgPSI = pressureAvg / mainCounter;
            avgPSI -= (float) pressureBase;
            avgPSI = avgPSI / (float) pressure100;
            avgPSI = avgPSI * 100.0;  
            sumFile.print(avgPSI,0);
            sumFile.println(" PSI");
          } else {
            sumFile.println("N/A");
          }

          float AverageImpulse = 0.0;
          float tempFreq = 0.0;
          tempFreq = burnTime / (float) 1000;
          tempFreq = (float) mainCounter / tempFreq;
          float sampleRateActual = (float) 1 / tempFreq;
          float fireSampleSeconds = (float) burnTime / (float) 1000;
          float AverageNewtons = 0.0;
          float TotalNewtons = 0.0;
          float MaxNewtons = 0.0;
          String TheMotor = "";

          if (loadEnabled == 1) {
            TotalImpulse = TotalImpulse * sampleRateActual; // in lbs-S
            AverageImpulse = TotalImpulse / fireSampleSeconds;
            MaxNewtons = loadMax * 4.44839;
            TotalNewtons = TotalImpulse * 4.44839; // Impulse in N-sec
            AverageNewtons = AverageImpulse * 4.44839; // in N-sec
            TheMotor = CalculateMotor(AverageNewtons, TotalNewtons);          
  
            sumFile.println(" ");
            sumFile.print("Total Impulse: "); sumFile.print(TotalNewtons,0); sumFile.println(" N-sec");
            sumFile.print("   Avg Thrust: "); sumFile.print(AverageNewtons,0); sumFile.println(" N-sec");                  
            sumFile.print("   Max Thrust: "); sumFile.print(MaxNewtons,0); sumFile.println(" N");     
            sumFile.print("   Max Thrust: ");
            sumFile.print(loadMax,0); sumFile.println(" lbs.");     
  
            sumFile.println(" ");
            sumFile.print("  Common Name: ");
            sumFile.println(TheMotor);
            } else {
            sumFile.print("Total Impulse: "); 
            sumFile.println("N/A no load cell");    
          }
          
          sumFile.print(" Main Samples: "); // number of samples
          sumFile.print(mainCounter);
          sumFile.print("   ");

          sumFile.print(tempFreq,0);
          sumFile.println(" per/sec");
          sumFile.close();
          
          // LED to white indicates all done
          digitalWrite(ledRed, HIGH);
          digitalWrite(ledBlue, HIGH);
          digitalWrite(ledGreen, HIGH);          
           // Write the output to the TFT screen          
                    
          tft.fillScreen(HX8357_BLACK);
          tft.setCursor(0, 0);
          tft.setTextColor(HX8357_WHITE);  tft.setTextSize(4);
          tft.print("RESULTS FOR TEST ");
          tft.print(testNum);  
          tft.setCursor(0, 280);
          tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);
          tft.print("     Push Select to Exit Results");  
          tft.setCursor(0, 60);
          tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);

          tft.print("    Burn Time: ");  // Burn Time
          tft.print(burnTime);
          tft.println(" ");
          tft.print(" Max Pressure: ");  // Max Pressure      
          if(pressureEnabled == 1) {        
            tft.print(maxPSI,0);
            tft.println(" PSI");
            tft.print(" Avg Pressure: ");  // Avg Pressure 
            tft.print(avgPSI,0);
            tft.println(" PSI");
          } else {
            tft.println("N/A no pressure test");
          }
          tft.println(" ");
          tft.print("Total Impulse: "); 
          if(loadEnabled == 1) {
            tft.print(TotalNewtons,0); tft.println(" N-sec");
            tft.print("   Avg Thrust: "); tft.print(AverageNewtons,0); tft.println(" N-sec");                  
            tft.print("   Max Thrust: "); tft.print(MaxNewtons,0); tft.println(" N");     
            tft.print("   Max Thrust: ");
            tft.print(loadMax,0); tft.println(" lbs.");     
            tft.println(" ");
            tft.print("  Common Name: ");
            tft.println(TheMotor);
          } else {
           tft.println("N/A no load test");    
          }
          tft.print(" Main Samples: "); // number of samples
          tft.print(mainCounter);
          tft.print("   ");
          tft.print(tempFreq,0);
          tft.println(" per/sec");


          if (timeout == 1) {
             tft.println(" ");
             tft.println("TIMEOUT ERROR OCCURED ON MAIN");
             Serial.print(F("Timeout Error on Main"));
          }
          byte test = 0;
          while (test == 0) {
            if (digitalRead(swPush2) == LOW) test = 1;
          }
          digitalWrite(ledRed, LOW);
          digitalWrite(ledBlue, LOW);
          digitalWrite(ledGreen, LOW);  
          
          samplePhase = 0;
          sampleState = 0;
          newMode = 1;
          delay(500); //anti-bounce
          
        }

 }  //                                  ********************  END OF THE SAMPLE LOOP  ***********************



 if (sampleState == 9) {  // we aborted the sample test - close files
      mainFile.close();
      postFile.close();

      tft.fillScreen(HX8357_BLACK);
      tft.setCursor(0, 0);
      tft.setTextColor(HX8357_WHITE);  tft.setTextSize(4);
      tft.print("ABORTED TEST ");
      tft.print(testNum);  
      tft.setCursor(0, 280);
      tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);
      tft.print("      Push Select to Exit");  

          digitalWrite(ledRed, HIGH);
          digitalWrite(ledBlue, HIGH);
          digitalWrite(ledGreen, HIGH); 

      while (digitalRead(swPush2) == HIGH) {
            // hold on results untile exit
      }
      delay(250);
      
 }

  
}



void reviewMode() {             //**************************************** REVIEW MODE *******************************************

    short testCount;
    short fileNumber[6];
    short fileCount;
    short selectScroll;
    byte theRefresh;
    short theCount;
    byte theExit = 0;
    
    if (newMode == 1) {
      tft.fillScreen(HX8357_BLACK);
      tft.setCursor(0, 0);
      tft.setTextColor(HX8357_WHITE);  tft.setTextSize(4);
      tft.print("REVIEW MODE");  
      tft.setCursor(0, 280);
      tft.setTextColor(HX8357_WHITE);  tft.setTextSize(3);
      tft.print("Scroll to select file");  
      newMode = 0;
      testCount = GetTestNumber();
      fileCount = 1;
    
      while (fileCount < 6 and testCount > 0) {
        fileNumber[fileCount] = testCount;
        testCount--;
        fileCount++;
      }
      theRefresh = 1;
      selectScroll = 1;
      reviewFile = 0;
    }

    while (theExit == 0 && reviewFile == 0) {
      if (theRefresh == 1) {
       tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);
       short yPos = 0;
       theCount = fileCount -1;
       for (int i=1;i <= theCount; i++) {
        yPos = i*30;
        yPos += 30;
        String tempS;
        tempS = "Get Test #" + String(fileNumber[i]);
        byte invertIt=0;
        if (selectScroll == i) invertIt = 1;
        listDisplay(tempS,0,yPos,invertIt);
       }
       theRefresh = 0;
      }
  
      if (digitalRead(swPush1) == LOW) { //scroll down
        selectScroll++;
        if (selectScroll > theCount) selectScroll = 1;
        theRefresh = 1; 
        delay(250);
      }
      
      if (digitalRead(swPush2) == LOW) { //select it
        reviewFile = fileNumber[selectScroll];
        reviewState = 0;
        theExit = 1;
        delay(250);
      }

      if (digitalRead(swMode1) != LOW) theExit = 1;
    } // end select loop




  // ********* Review Summary *****************

    if (digitalRead(swResults1) == LOW && reviewFile > 0) {
      if (reviewState != 1) {
        reviewState = 1; // minimize refresh

          tft.fillScreen(HX8357_BLACK);
          tft.setCursor(0, 0);
          tft.setTextColor(HX8357_WHITE);  tft.setTextSize(4);
          tft.print("SUMMARY FOR TEST ");
          tft.print(reviewFile);  

          tft.setCursor(0, 60);
          tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);
          char cr;
          String theline;
          String theFileN;
          int thevalue = 0;
          theFileN = "sum";
          theFileN += String(reviewFile);
          theFileN += ".txt";
          File dataFile = SD.open(theFileN);
          if (dataFile) {   
             while (dataFile.available()) {
                  tft.write(dataFile.read());
                }
          }
          dataFile.close();
      
      }

    }
   delay(250); //switch bounce
   // ********* Pressure Chart *****************
    if (digitalRead(swResults1) == HIGH && digitalRead(swResults2) == HIGH && reviewFile > 0) {
      if (reviewState != 2) {
        reviewState = 2; // minimize refresh
        
        //float MaxNewtons = MaxImpulse * 4.44839;
        //float TotalNewtons = TotalImpulse * 4.44839;
        float Yfactor;

        int TempEnd;
        int PlotXnum = 1;
        int PlotX;
        int PlotY; 
        int oldPlotX;
        int oldPlotY;

        tft.fillScreen(HX8357_BLACK);
        tft.drawLine(0,0,0,300,HX8357_GREEN);
        tft.drawLine(0,300,478,300,HX8357_GREEN);
        tft.setTextColor(HX8357_WHITE);    tft.setTextSize(2);
        tft.setCursor(0, 303);
        tft.print("TEST#"); tft.print(reviewFile);
        int maxPSI;
        String tempFileName = "num";
        tempFileName += String(reviewFile);
        tempFileName += ".txt";
        maxPSI = GetGraphFileData(reviewFile, 2);
        tft.print("  PRESSURE CHART   Max PSI: "); tft.print(maxPSI);        
        int FireLength = 0;
        FireLength = GetGraphFileData(reviewFile, 1); // number of samples    
        Yfactor = (float) 300 / (float) maxPSI; 
        Serial.print("Y factor ");Serial.println(maxPSI);
          
        // read the data from SD
        short pressureSamples[2000];
        short sampleCt = 0;
        tempFileName = "main";
        tempFileName += String(reviewFile);
        tempFileName += ".txt";         
        char cr;
        String theline;
        String templine;
        int thevalue = 0;          
        File dataFile = SD.open(tempFileName);
        if (dataFile) {   
           while (dataFile.available()) {
              cr = dataFile.read();
              theline = theline + cr;
              if(cr == '\n'){
                sampleCt++;
                templine = parseCSL(theline,2);
                thevalue = templine.toInt();
                float thePSI = 0;
                thePSI = thevalue - (float) pressureBase;  
                thePSI = thePSI / (float) pressure100;
                thePSI = thePSI * 100.0;
                if (thePSI < 0) thePSI = 0.0;
                pressureSamples[sampleCt]=thePSI;
                theline = ""; 
              }
            }
         }
         dataFile.close();        
         
        Serial.print("File Samples: ");Serial.println(sampleCt);
        Serial.println("Starting to Graph Pressure");

        float xscale = 0;
        if (sampleCt < 476) {  //scale graph up
            xscale = (float) 475 / (float) sampleCt;
            oldPlotY = 300;
            oldPlotX = 2;
            Serial.println("Smaller than 475");
            Serial.print("xscale is ");
            Serial.println(xscale, 3);                
            for (int i=1; i <= sampleCt; i++) {
              PlotXnum += 1;
              PlotX = PlotXnum * xscale;
              PlotY = Yfactor * pressureSamples[i];
              PlotY = 300 - PlotY;
              tft.drawLine(oldPlotX,oldPlotY,PlotX,PlotY,HX8357_GREEN);
              oldPlotX = PlotX;
              oldPlotY = PlotY;
            }  
        } else { //scale graph down to fit screen
            int yPos;
            xscale = (float) sampleCt / 475.0;
            oldPlotY = 300;
            oldPlotX = 2;      
            Serial.println("Larger than 475");
            Serial.print("xscale is ");
            Serial.println(xscale, 3);     
            for (int i=2; i <= 475; i++) {
              PlotX = i;
              yPos = i * xscale;   
              if (yPos > sampleCt) yPos = sampleCt;
              PlotY = Yfactor * pressureSamples[yPos];
              PlotY = 300 - PlotY;
              tft.drawLine(oldPlotX,oldPlotY,PlotX,PlotY,HX8357_GREEN);
              oldPlotX = PlotX;
              oldPlotY = PlotY; 
              
            }
        } // end graph section
      }
    } // end pressure graph section

   // ********* Load Chart *****************
    if (digitalRead(swResults2) == LOW && reviewFile > 0) {
      if (reviewState != 3) {
        reviewState = 3; // minimize refresh
       
        float Yfactor;
        int TempEnd;
        int PlotXnum = 1;
        int PlotX;
        int PlotY; 
        int oldPlotX;
        int oldPlotY;

        tft.fillScreen(HX8357_BLACK);
        tft.drawLine(0,0,0,300,HX8357_GREEN);
        tft.drawLine(0,300,478,300,HX8357_GREEN);
        tft.setTextColor(HX8357_WHITE);    tft.setTextSize(2);
        tft.setCursor(0, 303);
        tft.print("TEST#"); tft.print(reviewFile);
        int maxLoad;
        float maxN = 0.0;
        String tempFileName = "num";
        tempFileName += String(reviewFile);
        tempFileName += ".txt";
        maxLoad = GetGraphFileData(reviewFile, 3);
        maxN = (float)maxLoad * 4.44839;
        tft.print("  LOAD CHART   Max N: "); tft.print(maxN,0);        
        int FireLength = 0;
        FireLength = GetGraphFileData(reviewFile, 1); // number of samples    
        Serial.print("File samples: ");Serial.println(FireLength);
        Yfactor = (float) 300 / (float) maxN; 
        Serial.print("Y factor ");Serial.println(maxLoad);
          
        // read the data from SD
        short loadSamples[2000];
        short sampleCt = 0;
        tempFileName = "main";
        tempFileName += String(reviewFile);
        tempFileName += ".txt";         
        char cr;
        String theline;
        String templine;
        int thevalue = 0;          
        File dataFile = SD.open(tempFileName);
        if (dataFile) {   
           while (dataFile.available()) {
              cr = dataFile.read();
              theline = theline + cr;
              if(cr == '\n'){
                sampleCt++;
                templine = parseCSL(theline,3);
                thevalue = templine.toInt();
                float theNewtons = 0.0;
                theNewtons = (float) thevalue * 4.44839; 
                loadSamples[sampleCt]=theNewtons;
                theline = ""; 
              }
            }
         }
         dataFile.close();       
        Serial.print("sample count: "); Serial.println(sampleCt); 

        Serial.println("Starting to Graph Load");

        float xscale = 0;
        if (sampleCt < 476) {  //scale graph up
            xscale = (float) 475 / (float) sampleCt;
            oldPlotY = 300;
            oldPlotX = 2;
            Serial.println("Smaller than 475");
            Serial.print("xscale is ");
            Serial.println(xscale, 3);                
            for (int i=1; i <= sampleCt; i++) {
              PlotXnum += 1;
              PlotX = PlotXnum * xscale;
              PlotY = Yfactor * loadSamples[i];
              PlotY = 300 - PlotY;
              tft.drawLine(oldPlotX,oldPlotY,PlotX,PlotY,HX8357_GREEN);
              oldPlotX = PlotX;
              oldPlotY = PlotY;
            }  
        } else { //scale graph down to fit screen
            int yPos;
            xscale = (float) sampleCt / 475.0;
            oldPlotY = 300;
            oldPlotX = 2;      
            Serial.println("Larger than 475");
            Serial.print("xscale is ");
            Serial.println(xscale, 3);     
            for (int i=2; i <= 475; i++) {
              PlotX = i;
              yPos = i * xscale;   
              if (yPos > sampleCt) yPos = sampleCt;
              PlotY = Yfactor * loadSamples[yPos];
              PlotY = 300 - PlotY;
              tft.drawLine(oldPlotX,oldPlotY,PlotX,PlotY,HX8357_GREEN);
              oldPlotX = PlotX;
              oldPlotY = PlotY; 
              
            }
        } // end graph section
      }
    } // end load graph section

} // end review mode


void listDisplay(String theText, int pos, int pos2, byte invert) {

      char vText[100];     
      if(invert == 0) {
        tft.setTextColor(HX8357_WHITE, HX8357_BLACK);
      } else {
        tft.setTextColor(HX8357_BLACK, HX8357_WHITE);
      }
      tft.setCursor(pos, pos2);
      tft.print(theText);
}


short GetTestNumber() {  //read the number of previous tests from th SD card 

  char cr;
  String theline;
  int thevalue = 0;
  File dataFile = SD.open("testnum.txt");
  if (dataFile) {   
   while(true){
    cr = dataFile.read();
    theline = theline + cr;
    if(cr == '\n'){
      break;
    }
   }
    Serial.print("Reading Test Number = ");
    Serial.println(theline);
    dataFile.close();
    thevalue = theline.toInt();
  } else {
    return 0;
    Serial.println("error opening testnum.txt");
  }
  return thevalue;  
}



int GetGraphFileData(int fileNum, byte thepos) {

  char cr;
  String theline;
  int thevalue = 0;
  String tempFileName = "num";
  tempFileName += String(fileNum);
  tempFileName += ".txt";
  File dataFile = SD.open(tempFileName);
  if (dataFile) {   
   while(true){
    cr = dataFile.read();
    theline = theline + cr;
    if(cr == '\n'){
      break;
    }
   }
    dataFile.close();
    String tempVal;
    tempVal = parseCSL(theline,thepos);
    thevalue = tempVal.toInt();
  } else {
    return 0;
    Serial.println("error getting graphing data");
  }
  return thevalue;  
}


String parseCSL(String vSentence, byte vPos) {

  byte theCommas[40];
  byte totalnum = 0;
  byte testnum;
  byte vStringPos = 0;
  theCommas[0]=0;
  while (testnum != 255) {
   testnum = vSentence.indexOf(',',vStringPos);
   if(testnum != 255) {
     totalnum ++;
     theCommas[totalnum] = testnum;
     vStringPos = testnum + 1;
   }
  }
  totalnum ++;
  theCommas[totalnum]=vSentence.length();
  byte startcs;
  if (vPos == 1) startcs = theCommas[vPos-1]; else startcs = theCommas[vPos-1]+1;
  byte endcs = theCommas[vPos];
  return vSentence.substring(startcs,endcs);
}





String CalculateMotor(float TheAverage, float TheTotal) {   // Calculate the motor designation using total and average Ns thrust

  String TempResult = "Z";

  if (TheTotal <= 2.5) TempResult = "A";
  if (TheTotal > 2.5 && TheTotal <= 5) TempResult = "B";
  if (TheTotal > 5 && TheTotal <= 10) TempResult = "C";
  if (TheTotal > 10 && TheTotal <= 20) TempResult = "D";
  if (TheTotal > 20 && TheTotal <= 40) TempResult = "E";
  if (TheTotal > 40 && TheTotal <= 80) TempResult = "F";
  if (TheTotal > 80 && TheTotal <= 160) TempResult = "G";
  if (TheTotal > 160 && TheTotal <= 320) TempResult = "H";
  if (TheTotal > 320 && TheTotal <= 640) TempResult = "I";  
  if (TheTotal > 640 && TheTotal <= 1280) TempResult = "J";
  if (TheTotal > 1280 && TheTotal <= 2560) TempResult = "K";
  if (TheTotal > 2560 && TheTotal <= 5120) TempResult = "L";
  if (TheTotal > 5120 && TheTotal <= 10240) TempResult = "M";
  if (TheTotal > 10240 && TheTotal <= 20560) TempResult = "N";
  if (TheTotal > 20560 && TheTotal <= 40960) TempResult = "O";
  if (TheTotal > 40960 && TheTotal <= 81920) TempResult = "P";

  TempResult += String(TheAverage,0);
  return TempResult;

}


void readConfigFile() {
  

  String tempLine;
  
  configFile = SD.open("config.txt");
  if (configFile) {
   tempLine = readLine();
   if (tempLine.charAt(0) == '1' || tempLine.charAt(0) == 'Y') {
    pressureEnabled = 1;
   } else {
    pressureEnabled = 0;
   }
   tempLine = readLine();
   if (tempLine.charAt(0) == '1' || tempLine.charAt(0) == 'Y') {
    loadEnabled = 1;
   } else {
    loadEnabled = 0;
   }
   tempLine = readLine();
   sampleInterval = tempLine.toInt();
   tempLine = readLine();
   pressureBase = tempLine.toInt();
   tempLine = readLine();
   pressure100 = tempLine.toInt();
   tempLine = readLine();
   triggerStartPressure = tempLine.toInt();
   tempLine = readLine();
   triggerStopPressure = tempLine.toInt();
   tempLine = readLine();
   calibration_factor = (float) tempLine.toInt();
   tempLine = readLine();
   triggerStartLoad = tempLine.toInt();      
   tempLine = readLine();
   triggerStopLoad = tempLine.toInt();
   tempLine = readLine();
   timeoutDuration = tempLine.toInt();
   tempLine = readLine();
   mainMinTime = tempLine.toInt();
   tempLine = readLine();
   postDuration = tempLine.toInt();
   configFile.close();
   
  } else {
    tft.println("ERROR no config file on SD");
  }
}

String readLine() {
   char cr;
   String theline = "";
   while(true){
    cr = configFile.read();
    theline = theline + cr;
    if(cr == '\n'){
      return theline;
    }
   }
}

void displayConfig() {

    tft.fillScreen(HX8357_BLACK);
    tft.setCursor(0, 15);
    tft.setTextColor(HX8357_WHITE);  tft.setTextSize(3);
    tft.println("CONFIG");
    tft.setCursor(0, 50);
    tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);
    tft.print("Pressure Enabled: ");
    tft.println(pressureEnabled);
    tft.print("Load Enabled: ");
    tft.println(loadEnabled);    
    tft.print("Sample Interval: ");
    tft.print(sampleInterval);  tft.println(" ms");
    tft.print("Pressure Base: ");
    tft.println(pressureBase);
    tft.print("Pressure 100: ");
    tft.println(pressure100);  
    tft.print("Start Pressure: ");
    tft.println(triggerStartPressure);   
    tft.print("Stop Pressure: ");
    tft.println(triggerStopPressure);  
    tft.print("Load Calibration: ");
    tft.println(calibration_factor,0);  
    tft.print("Start Load: ");
    tft.println(triggerStartLoad);  
    tft.print("Stop Load: ");
    tft.println(triggerStopLoad);  
    tft.print("Main Timeout: ");
    tft.print(timeoutDuration);   tft.println(" ms");
    tft.print("Main Min Time: ");
    tft.print(mainMinTime);   tft.println(" ms");
    tft.print("Post Sample Duration: ");
    tft.print(postDuration);   tft.println(" ms");
    delay(5000);

  
}




// ***** DELETE THE TESTING AND UTILITIES FUNCTIONS BELOW - NOT USED FOR PRODUCTION *****

void switchTest2() {
  
  if (digitalRead(swPush1) == LOW) {
    digitalWrite(ledRed, HIGH);   
    digitalWrite(ledBlue, LOW);
    digitalWrite(ledGreen, LOW);
    tft.fillScreen(HX8357_BLACK);
    tft.setCursor(0, 150);
    tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);
    tft.print("Hello Push Button One");       
  }
  if (digitalRead(swPush2) == LOW) {
    digitalWrite(ledRed, LOW);   
    digitalWrite(ledBlue, HIGH);
    digitalWrite(ledGreen, LOW);
    tft.fillScreen(HX8357_BLACK);
    tft.setCursor(0, 150);
    tft.setTextColor(HX8357_WHITE);  tft.setTextSize(2);
    tft.print("Hello Push Button Two"); 
  }

}

void ledTest() {

  digitalWrite(ledRed, HIGH);
  delay(1000);
   digitalWrite(ledRed, LOW);
  digitalWrite(ledGreen, HIGH);
  delay(1000);
    digitalWrite(ledGreen, LOW);
  digitalWrite(ledBlue, HIGH);
  delay(1000);
  digitalWrite(ledBlue, LOW);  
  digitalWrite(ledRed, HIGH);
  digitalWrite(ledGreen, HIGH);
  digitalWrite(ledBlue, HIGH);
  delay(1000);
  digitalWrite(ledBlue, LOW);
  digitalWrite(ledGreen, LOW);

}


void switchTest() {

  if (digitalRead(swMode1) == LOW) {
    digitalWrite(ledRed, HIGH);   
    digitalWrite(ledBlue, LOW);
    digitalWrite(ledGreen, LOW);
  }
  if (digitalRead(swMode2) == LOW) {
    digitalWrite(ledRed, LOW);   
    digitalWrite(ledBlue, LOW);
    digitalWrite(ledGreen, HIGH);
  }
  if (digitalRead(swMode1) == HIGH && digitalRead(swMode2) == HIGH) {
    digitalWrite(ledRed, LOW);   
    digitalWrite(ledBlue, HIGH);
    digitalWrite(ledGreen, LOW);
  }
}

void createConfigFile() {  //writes a new config file to SD
   
    SD.remove("config.txt");
    File dataFile = SD.open("config.txt", FILE_WRITE);
    if (dataFile) {
      dataFile.println("Y"); //pressure enabled (Y/N or 1/0)
      dataFile.println("Y"); //Load enabled (Y/N or 1/0)          
      dataFile.println("10"); //sample interval ms     
      dataFile.println("100"); //pressure sensor base value          
      dataFile.println("50"); //pressure sensor 100 PSI value  
      dataFile.println("130"); //pressure trigger value to start sampling (base # + value)  
      dataFile.println("130"); //pressure trigger value to stop sampling (base # + value)    
      dataFile.println("3900"); //load cell calibration value  
      dataFile.println("15"); //load value trigger to start sampling (units or lbs)  
      dataFile.println("20"); //load value trigger to stop sampling (units or lbs)   
      dataFile.println("10000"); //Main sample timeout in ms  
      dataFile.println("2000"); //Main sample minimum time (anti-chuff) in ms  
      dataFile.println("10000"); //Duration to post sample in ms  
      dataFile.close();
      delay(1000);
    }
  
}





/*
 * Config file information (each on a separate line in exact order):
 * 
 * Pressure enabled (1 or Y)
 * Load enabled (1 or Y)
 * Sample interval (ms)
 * Pressure sensor base value (integer)
 * Pressure sensor 100 PSI value (integer)
 * Pressure value to start (PSI)
 * Pressure value to end (PSI)
 * Load cell calibration value (set to 0 then divide by sample weight)
 * Load value to start sampling (in lbs or units)
 * Load value to end sampling (in lbs or units)
 * Main Sample loop timeout (ms - default to 10 seconds 10000)
 * Main sample loop minimum time (ms - default to 2 seconds 2000)
 * Post sample duration (ms - default to 10 seconds 10000)
 * 
 */
