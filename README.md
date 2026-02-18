# MQTT Status Bridge & Logger

Prosty, wydajny serwis napisany w języku C, działający jako most między systemem komunikatów MQTT a lokalnym systemem plików.
Aplikacja nasłuchuje komunikatów JSON na określonym temacie MQTT, analizuje je pod kątem błędów i aktualizuje plik statusu (wykorzystywany np. przez interfejs przeglądarkowy/WebKiosk) oraz prowadzi logi zdarzeń.
