/*
  BikeSensSensor (plik główny)

  Celowo minimalny .ino:
  - setup() oraz loop() są tylko przekierowaniem do AppController.
  - cała logika jest podzielona na moduły w *.cpp/*.h.
*/
#include "AppController.h"

void setup() { appSetup(); }
void loop()  { appLoop(); }
