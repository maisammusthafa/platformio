#include <Arduino.h>

void setup () {
    Serial.begin(115200);

    Serial.println();
    printf("Chip Revision: %d\n", ESP.getChipRevision());
    printf("CPU Frequency: %d\n", ESP.getCpuFreqMHz());
    printf("Cycle Count: %d\n", ESP.getCycleCount());
    printf("eFuse MAC: %d\n", ESP.getEfuseMac());
    printf("Flash Chip Mode: %d\n", ESP.getFlashChipMode());
    printf("Flash Chip Size: %d\n", ESP.getFlashChipSize());
    printf("Flash Chip Speed: %d\n", ESP.getFlashChipSpeed());
    printf("Free Heap: %d\n", ESP.getFreeHeap());
    printf("SDK Version: %s\n", ESP.getSdkVersion());
}

void loop () {
}