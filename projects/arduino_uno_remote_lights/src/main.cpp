#include <Arduino.h>

const byte numChars = 14;
char receivedChars[numChars];
char tempChars[numChars];
boolean newData = false;

int r, g, b;

int rPin = 9;
int gPin = 10;
int bPin = 11;

void recvWithStartEndMarkers();
void parseData();
void writeData();

void setup() {
    Serial.begin(9600);
    
    pinMode(rPin, OUTPUT);
    pinMode(gPin, OUTPUT);
    pinMode(bPin, OUTPUT);
}

void loop() {
    recvWithStartEndMarkers();
    if (newData == true) {
        strcpy(tempChars, receivedChars);
        parseData();
        writeData();
        newData = false;
    }
}

void recvWithStartEndMarkers() {
    static boolean recvInProgress = false;
    static byte index = 0;
    char startMarker = '[';
    char endMarker = ']';
    char rc;

    while (Serial.available() > 0 && newData == false) {
        rc = Serial.read();

        if (recvInProgress == true) {
            if (rc != endMarker) {
                receivedChars[index] = rc;
                index++;
                if (index >= numChars) {
                    index = numChars - 1;
                }
            } else {
                receivedChars[index] = '\0';
                recvInProgress = false;
                index = 0;
                newData = true;
            }
        } else if (rc == startMarker) {
            recvInProgress = true;
        }
    }
}

void parseData() {
    char* strtokIndex;

    strtokIndex = strtok(tempChars, ",");
    r = atoi(strtokIndex);

    strtokIndex = strtok(NULL, ",");
    g = atoi(strtokIndex);

    strtokIndex = strtok(NULL, ",");
    b = atoi(strtokIndex);
}

void writeData() {
    analogWrite(rPin, r);
    analogWrite(gPin, g);
    analogWrite(bPin, b);
    
    char outputString[32];
    int rPerc = map(r, 0, 255, 0, 100);
    int gPerc = map(g, 0, 255, 0, 100);
    int bPerc = map(b, 0, 255, 0, 100);
    
    sprintf(outputString, "[%d, %d, %d] [%d, %d, %d]",
        r, g, b, rPerc, gPerc, bPerc);
    Serial.println(outputString);
}