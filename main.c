#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include <unistd.h>
#include "cJSON.h"
#include "MQTTClient.h"

#define ADRES       "Cenzurka" //Adres serwera komara
#define ID_KLIENTA    "Cenzurka" //ID klienta
#define TEMAT       "Cenzurka" //Temat do subskrybowania
#define PLIK_STATUSU "Cenzurka" //Plik statusu
#define PLIK_LOGU    "Cenzurka" //Plik logów
#define JAKOSC_USLUGI 1

void aktualizuj_status_przegladarki(int aktywny, const char *kod) {
    FILE *plik = fopen(PLIK_STATUSU, "w");
    if (plik == NULL) {
        printf("BŁĄD: Nie można otworzyć %s do zapisu\n", PLIK_STATUSU);
        return;
    }

    if (aktywny) {
        fprintf(plik, "{\"active\": true, \"code\": \"%s\"}", kod ? kod : "Nieznany");
    } else {
        fprintf(plik, "{\"active\": false, \"code\": \"\"}");
    }
    
    fflush(plik);
    fclose(plik);
    chmod(PLIK_STATUSU, 0666); // NIE USUWAĆ
}

void zapisz_do_logu(const char *wiadomosc) {
    FILE *plik = fopen(PLIK_LOGU, "a");
    if (plik == NULL) return;
    time_t teraz; time(&teraz);
    char bufor[20];
    strftime(bufor, sizeof(bufor), "%Y-%m-%d %H:%M:%S", localtime(&teraz));
    fprintf(plik, "[%s] %s\n", bufor, wiadomosc);
    fclose(plik);
}

void analizuj_i_przetwarzaj(char *tresc) {
    cJSON *json = cJSON_Parse(tresc);
    if (json == NULL) return;
    if (cJSON_IsArray(json)) {
        int rozmiar_tablicy = cJSON_GetArraySize(json);

        if (rozmiar_tablicy > 0) {
            cJSON *pierwszy_blad = cJSON_GetArrayItem(json, 0);
            if (pierwszy_blad != NULL) {
                cJSON *element_kod = cJSON_GetObjectItemCaseSensitive(pierwszy_blad, "code");
                cJSON *element_waga = cJSON_GetObjectItemCaseSensitive(pierwszy_blad, "severity");
                if (cJSON_IsString(element_kod)) {
                    char komunikat_logu[256];
                    snprintf(komunikat_logu, sizeof(komunikat_logu), "Wykryty błąd: %s", element_kod->valuestring);
                    zapisz_do_logu(komunikat_logu);
                    aktualizuj_status_przegladarki(1, element_kod->valuestring);
                }
            }
        } else {
            zapisz_do_logu("Brak błędów.");
            aktualizuj_status_przegladarki(0, "");
        }
    }

    cJSON_Delete(json);
}

int wiadomosc_odebrana(void *kontekst, char *nazwaTematu, int dlTematu, MQTTClient_message *wiadomosc) {
    if (wiadomosc->payloadlen > 0) {
        char *napis_tresci = malloc(wiadomosc->payloadlen + 1);
        if (napis_tresci) {
            memcpy(napis_tresci, wiadomosc->payload, wiadomosc->payloadlen);
            napis_tresci[wiadomosc->payloadlen] = '\0';
            analizuj_i_przetwarzaj(napis_tresci);
            free(napis_tresci);
        }
    }
    MQTTClient_freeMessage(&wiadomosc);
    MQTTClient_free(nazwaTematu);
    return 1;
}

void utrata_polaczenia(void *kontekst, char *przyczyna) {
    zapisz_do_logu("Utracono połączenie z komarem. Próba ponownego połączenia...");
}

int main(int licznik_arg, char* argumenty[]) {
    aktualizuj_status_przegladarki(0, "");
    MQTTClient klient;
    MQTTClient_connectOptions opcje_polaczenia = MQTTClient_connectOptions_initializer;
    int kod_wyniku;
    MQTTClient_create(&klient, ADRES, ID_KLIENTA, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    MQTTClient_setCallbacks(klient, NULL, utrata_polaczenia, wiadomosc_odebrana, NULL);
    opcje_polaczenia.keepAliveInterval = 20;
    opcje_polaczenia.cleansession = 1;
    while (1) {
        if (!MQTTClient_isConnected(klient)) {
            if ((kod_wyniku = MQTTClient_connect(klient, &opcje_polaczenia)) == MQTTCLIENT_SUCCESS) {
                zapisz_do_logu("Połączono z komarem.");
                MQTTClient_subscribe(klient, TEMAT, JAKOSC_USLUGI);
            }
        }
        sleep(5);
    }

    MQTTClient_destroy(&klient);
    return 0;
}
