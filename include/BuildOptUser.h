// RadioLib BuildOptUser.h
// RadioLib's BuildOpt.h automatically includes this file if present in include path
// This file MUST be found by RadioLib during its compilation
// Location: include/BuildOptUser.h (added to include path via -I include in platformio.ini)
// This file is force-included via -include flag in platformio.ini
// NOTE: This file is included in BOTH C and C++ files, so C++-only code must be guarded
// NOTE: SPI.h is a library header and may not be available when framework files are compiled
//       So we only include SPI.h when it's actually available

#ifndef BUILDOPTUSER_H
#define BUILDOPTUSER_H

// Only include C++ headers when compiling C++ code
// This file is force-included in C files too (like TinyUSB .c files)
#ifdef __cplusplus
  // Include Arduino.h first - this sets up the Arduino environment
  #include <Arduino.h>

  // Try to include SPI.h - it may not be available when framework files are compiled
  // Use __has_include to check if SPI.h is available before including it
  #if __has_include(<SPI.h>)
    #include <SPI.h>
    
    // On nRF52 Adafruit core, SPI.h defines SPIClass but may not declare the SPI object
    // Declare SPI as extern - the actual definition is in SPI.cpp from the SPI library
    // This ensures SPI is available when RADIOLIB_DEFAULT_SPI macro expands
    extern SPIClass SPI;

    // Define RADIOLIB_DEFAULT_SPI BEFORE RadioLib's BuildOpt.h checks for it
    // This prevents RadioLib from trying to define it as SPI when SPI might not be in scope
    #ifndef RADIOLIB_DEFAULT_SPI
      #define RADIOLIB_DEFAULT_SPI SPI
    #endif
  #else
    // SPI.h is not available yet (e.g., when compiling framework files)
    // Don't define RADIOLIB_DEFAULT_SPI here - let RadioLib's BuildOpt.h handle it
    // The actual source files that use RadioLib will include SPI.h themselves
  #endif
#endif // __cplusplus

#endif // BUILDOPTUSER_H
