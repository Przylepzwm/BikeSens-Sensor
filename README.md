# BikeSens-Sensor

Firmware dla sensora obrotu opartego o `ESP32-C3`, którego zadaniem jest:

- wybudzenie z `deep sleep` po impulsie z czujnika Halla,
- zliczanie impulsów w zadanym oknie czasowym,
- wysłanie wyniku do bramki w postaci kilku powtórzonych reklam BLE,
- możliwie szybki powrót do trybu energooszczędnego.

## Cel projektu

Sensor działa bateryjnie i jest zoptymalizowany pod niski pobór energii. W stanie spoczynku urządzenie pozostaje uśpione. Pojawienie się impulsu z sensora obrotu wybudza układ i uruchamia okno zliczania. Po jego zakończeniu wynik jest raportowany przez BLE advertising, bez utrzymywania połączenia.

## Cykl pracy

1. Urządzenie przebywa w `deep sleep`.
2. Zdarzenie na wejściu Halla wybudza układ.
3. Startuje okno zliczania `WINDOW_MS`.
4. Impulsy są zliczane w ISR przez cały czas trwania okna.
5. Po starcie okna wykonywany jest pojedynczy pomiar baterii.
6. Po zakończeniu okna liczba impulsów zostaje zamrożona i przygotowywany jest telegram BLE.
7. Telegram jest nadawany `BLE_REPEAT_COUNT` razy, z losowym backoffem przed pierwszym nadaniem i losowymi przerwami pomiędzy kolejnymi reklamami.
8. Po zakończeniu komunikacji urządzenie pozostaje jeszcze aktywne przez `NEXT_WINDOW_TIMEOUT_MS`.
9. Jeśli w tym czasie pojawi się kolejny impuls, nowe okno startuje bez ponownego usypiania.
10. Jeśli nie pojawi się żaden impuls, urządzenie wraca do `deep sleep`.

Założenie projektowe: przy wybudzeniu z GPIO startowe `+1` do licznika jest akceptowalne, ponieważ w domenie pomiarowej pół obrotu również traktujemy jako zaliczone zdarzenie.

## Pomiar baterii

Pomiar baterii wykonywany jest raz na okno. W buildzie debugowym niski poziom baterii nie blokuje działania, ponieważ urządzenie bywa zasilane wyłącznie z USB do debugowania, bez podłączonego zasilania bateryjnego. W buildzie produkcyjnym próg krytyczny powinien powodować powrót do `deep sleep` bez komunikacji BLE.

## Telegram BLE

Sensor używa `Manufacturer Specific Data` i wysyła:

- `company_id`
- `prefix`
- `device_id`
- `seq`
- `pulses_in_window`
- `battery_percent`

Format jest zgodny z obecną implementacją bramki i nie zawiera dodatkowego pola `flags`.

## Struktura projektu

Minimalny szkic Arduino pozostaje w katalogu głównym:

- [BikeSensSensor.ino](/Users/przemyslawkurantowicz/Library/Mobile%20Documents/com~apple~CloudDocs/Dokumenty/BikeSens/05%20Software/Sensors/BikeSensSensor/BikeSensSensor.ino)

Kod źródłowy jest podzielony na warstwy:

- `src/app`
  - główna orkiestracja aplikacji i cyklu pracy sensora
- `src/comm`
  - przygotowanie i nadawanie telegramów BLE
- `src/platform`
  - obsługa deep sleep, wake-up i pomiaru baterii
- `src/debug`
  - narzędzia diagnostyczne i śledzenie ISR
- `src/config`
  - konfiguracja sprzętu i parametrów czasowych

Najważniejsze pliki:

- [src/app/AppController.cpp](/Users/przemyslawkurantowicz/Library/Mobile%20Documents/com~apple~CloudDocs/Dokumenty/BikeSens/05%20Software/Sensors/BikeSensSensor/src/app/AppController.cpp) - główny przepływ aplikacji
- [src/comm/BleAdvComm.cpp](/Users/przemyslawkurantowicz/Library/Mobile%20Documents/com~apple~CloudDocs/Dokumenty/BikeSens/05%20Software/Sensors/BikeSensSensor/src/comm/BleAdvComm.cpp) - warstwa BLE advertising
- [src/platform/SleepManager.cpp](/Users/przemyslawkurantowicz/Library/Mobile%20Documents/com~apple~CloudDocs/Dokumenty/BikeSens/05%20Software/Sensors/BikeSensSensor/src/platform/SleepManager.cpp) - usypianie i konfiguracja wybudzania
- [src/platform/BatteryMonitor.cpp](/Users/przemyslawkurantowicz/Library/Mobile%20Documents/com~apple~CloudDocs/Dokumenty/BikeSens/05%20Software/Sensors/BikeSensSensor/src/platform/BatteryMonitor.cpp) - pomiar napięcia baterii
- [src/debug/IsrTrace.cpp](/Users/przemyslawkurantowicz/Library/Mobile%20Documents/com~apple~CloudDocs/Dokumenty/BikeSens/05%20Software/Sensors/BikeSensSensor/src/debug/IsrTrace.cpp) - diagnostyka pracy ISR
- [src/config/Config.h](/Users/przemyslawkurantowicz/Library/Mobile%20Documents/com~apple~CloudDocs/Dokumenty/BikeSens/05%20Software/Sensors/BikeSensSensor/src/config/Config.h) - konfiguracja projektu

## Konfiguracja

Wszystkie podstawowe parametry projektu znajdują się w:

- [src/config/Config.h](/Users/przemyslawkurantowicz/Library/Mobile%20Documents/com~apple~CloudDocs/Dokumenty/BikeSens/05%20Software/Sensors/BikeSensSensor/src/config/Config.h)

W szczególności:

- mapowanie pinów,
- długość okna pomiarowego,
- timeout powrotu do snu,
- liczba i timing reklam BLE,
- progi baterii,
- ustawienia debugowe.

## Debug

Debugowanie szeregowe jest sterowane przez `DEBUG_SERIAL`.

Gdy `DEBUG_SERIAL == 1`:

- aktywowany jest `Serial`,
- dostępne są logi startu, okna i transmisji BLE,
- działają moduły diagnostyczne z `src/debug`,
- niski poziom baterii nie zatrzymuje pracy urządzenia.

Gdy `DEBUG_SERIAL == 0`:

- logowanie jest wyłączone,
- kod pracuje w trybie docelowym,
- niski poziom baterii może wymusić natychmiastowy powrót do `deep sleep`.

## Status

README opisuje aktualną strukturę repozytorium po refactorze katalogów do układu `src/app`, `src/comm`, `src/platform`, `src/debug`, `src/config`.
