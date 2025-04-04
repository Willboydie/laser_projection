#include <Arduino.h> // Required for Serial and other Arduino functions
#include <stdlib.h> // Required for malloc(), realloc(), free()

#define MAX_ARRAY_SIZE 20000 // Maximum size of the array

float* dynamicArray = nullptr; // Pointer to dynamically allocated array
size_t arraySize = 0;          // Current number of elements in the array

void freeMemory() {
    free(dynamicArray); // Free memory when done
    dynamicArray = nullptr;
    arraySize = 0;
}

void proxy_setup() {

    Serial.begin(115200); // Initialize serial communication
    while (!Serial) {
        ; // Wait for serial port to connect. Needed for native USB port only
    }

    while (arraySize < MAX_ARRAY_SIZE) {
        // Temporary buffer to hold incoming floats
        float buffer[3];
        for (int i = 0; i < 3; i++) {
            float eg = 0.1 * i; // Example float value for testing
            // Read 4 bytes into the float (assuming same endianness)
            // Serial.readBytes((char*)&buffer[i], 4);
            buffer[i] = eg; 
        }

        // Resize the dynamic array to accommodate new data
        float* newArray = (float*)realloc(dynamicArray, (arraySize + 3) * sizeof(float));
        
        if (newArray == nullptr) {
            // Serial.println("Memory allocation failed!");
            return; // Handle error (e.g., reset or skip)
        }

        dynamicArray = newArray; // Update pointer

        // Append new data to the array
        for (int i = 0; i < 3; i++) {
            dynamicArray[arraySize + i] = buffer[i];
        }
        arraySize += 3; // Update size

        // Debug print
        Serial.print("Array size: ");
        Serial.print(arraySize);
        Serial.print(" | Last values: ");
        for (size_t i = arraySize - 3; i < arraySize; i++) {
            Serial.print(dynamicArray[i]);
            Serial.print(" ");
        }
        Serial.println();
    }
    freeMemory(); // Free memory after use
}

void proxy_loop() {}