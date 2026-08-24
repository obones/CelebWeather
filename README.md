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
4. ~~Mettre en forme ces données pour créer un message POCSAG~~
5. ~~Emettre ce message au bon format~~
6. ~~Constater que c'est bien lu par la station~~
7. Activer l'alerte en cas de vigilance jaune ou supérieure

## Matériel

Ce projet cible un montage à base d'ESP32 communiquant avec un module RFM69 tout prêt et a été testé avec cette combinaison:

ESP32-WROOM32D
RFM69C calé sur 433MHz

Les connexions sont les suivantes:

| ESP32 | RFM69 | Force refresh<br/>button | AP config<br/>button | AP status LED | AP status LED<br/>resistor |
|-------|-------|:-----:|:-----:|:------------:|:------------:|
|  GND  |  GND  | Leg 1 | Leg 1 |              |              |
|  3V3  |  3.3  |       |       |              |     Leg 1    |
|   4   | RESET |       |       |              |              |
|   5   |  NSS  |       |       |              |              |
|  13   |       |       | Leg 2 |              |              |
|  18   |  SCK  |       |       |              |              |
|  19   | MISO  |       |       |              |              |
|  21   |       | Leg 2 |       |              |              |
|  23   | MOSI  |       |       |              |              |
|  32   |       |       |       | Long leg (-) |              |

La deuxième broche résistance pour la LED AP Status est connectée à la patte courte (+) de la LED AP status

## Développement

### IDE

Ce projet est développé avec VSCode et le plugin PlatformIO

Il suffit d'ouvrir le projet dans VSCode puis d'utiliser les raccourcis PlatformIO (Monitor, Build, Upload...) pour compiler le code et l'envoyer dans le module.

### FlatBuffers

La communication avec Open-Meteo se fait via le format flatbuffers pour diminuer fortement le besoin en RAM et en CPU que le JSON nécessiterait.
Il faut donc générer le fichier `weather_api_generated.h` via cette ligne de commande lancée dans le répertoire `open-meteo-flatbuffers`:

```
flatc --cpp  --scoped-enums --gen-all --no-emit-min-max-enum-values -o ../include weather_api.fbs
```

Assurez-vous d'avoir la même version de flatc.exe que celle utilisée par le projet et indiquée dans `platform.io`

### Département

Le département à afficher sur la station est obtenu à partir des coordonnées GPS via le service [Découpage administratif](https://www.data.gouv.fr/dataservices/api-decoupage-administratif-api-geo)

En cas d'échec, le département 75 est utilisé par défaut, cette valeur n'étant que purement cosmétique.

### Protocole radio

Le protocole employé pour communiquer avec la station météo est décrit dans un [document dédié](doc/protocol.md)

## Utilisation

Au lancement, un point d'accès wifi est mis à disposition, nommé CelebWeather auquel vous pouvez vous connecter avec le mot de passe ESP32CelebWeather

Accédez à la page de configuration sur http://192.168.1.41/config

Il faut alors modifier le mot de passe du portail (AP password) puis donner les éléments pour se connecter à votre réseau WiFi

Par ailleurs, à la section "General parameters", renseignez la latitude et la longitude pour laquelle vous voulez les prévisions. Attention, il faut utiliser le point comme séparateur décimal

La section "Open-Meteo parameters" contient des valeurs par défaut qui suffisent pour 99% des utilisateurs, ne la modifiez que si vous savez ce que vous faites.

Une fois la configuration effectuée, déconnectez vous du point d'accès temporaire pour laisser le code s'exécuter. Il se connecte alors à votre WiFi et vous pourrez vous connecter à la page de configuration sur http://IP/config avec "admin" et le mot de passe que vous avez indiqué juste avant. La valeur de IP dépend de votre réseau local, elle est fournie en DHCP et vous devriez pouvoir la voir dans la page d'état de votre routeur WiFi.

Plus détails sont disponibles dans la [documentation de démarrage](doc/startup.md)

### Boutons

Le bouton "Force refresh" est utile pour forcer une transmission immédiate de l'heure et de la prévision météorologique

Le bouton "AP config" est à maintenir enfoncé à la mise sous tension du système pour forcer l'activation du point d'accès avec le mot de passe par défaut
si vous avez oublié la valeur que vous avez donné lors de l'initialisation.
Il faudra alors procéder au changement du mot de passe AP comme lors de la configuration initiale