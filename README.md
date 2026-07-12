# CelebWeather

Le but de ce projet est de relayer les prévisions météorologiques fournies par [Open-Meteo](https://open-meteo.com/) vers une station d'affichage recevant ces messages au format POCSAG.

Le principe est le suivant:

```
Open-Meteo ---> ESP32 ---> RFM69 ---> Station
```

Ce projet est un "travail en cours", voici les étapes envisagées, celles barrées sont déjà faites:


1. ~~Créer un projet de base ESP32 Arduino~~
2. ~~Permettre une configuration du module en mode "Point d'accès temporaire"~~
3. ~~Lire les données Open-Meteo toutes les 60 minutes~~
4. Mettre en forme ces données pour créer un message POCSAG
5. Emettre ce message au bon format

## Matériel

Ce projet cible un montage à base d'ESP32 communiquant avec un module RFM69 tout prêt et a été testé avec cette combinaison:

ESP32-WROOM32D
RFM69C calé sur 433MHz

Les connexions sont les suivantes:

| ESP32 | RFM69 |
|-------|-------|
|  GND  |  GND  |
|  3V3  |  3.3  |
|   4   | RESET |
|   5   |  NSS  |
|  18   |  SCK  |
|  19   | MISO  |
|  23   | MOSI  |
|  26   | DIO2  |
|  27   | DIO3  |

26 et 27 ne sont probablement pas utiles mais repris d'un montage pour [RFLink32](https://github.com/cpainchaud/RFLink32)

## Développement

### FlatBuffers

La communication avec Open-Meteo se fait via le format flatbuffers pour diminuer fortement le besoin en RAM et en CPU que le JSON nécessiterait.
Il faut donc générer le fichier `weather_api_generated.h` via cette ligne de commande lancée dans le répertoire `open-meteo-flatbuffers`:

```
flatc --cpp  --scoped-enums --gen-all --no-emit-min-max-enum-values -o ../include weather_api.fbs
```

Assurez-vous d'avoir la même version de flatc.exe que celle utilisée par le projet et indiquée dans `platform.io`

## Utilisation

Au lancement, un point d'accès wifi est mis à disposition, nommé CelebWeather auquel vous pouvez vous connecter avec le mot de passe ESP32CelebWeather

Accédez à la page de configuration sur http://192.168.1.41/config

Il faut alors modifier le mot de passe du portail (AP password) puis donner les éléments pour se connecter à votre réseau WiFi

Par ailleurs, à la section "General parameters", renseignez la latitude et la longitude pour laquelle vous voulez les prévisions. Attention, il faut utiliser le point comme séparateur décimal

La section "Open-Meteo parameters" contient des valeurs par défaut qui suffisent pour 99% des utilisateurs, ne la modifiez que si vous savez ce que vous faites.