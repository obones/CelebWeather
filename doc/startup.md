# Démarrage

## Première exécution

Lors de la première exécution, le module se met en mode "Point d'accès", c'est à dire qu'il propose un point d'accès WiFi auquel vous devez vous connecter pour effectuer la configuration initiale.

Pour cela, utilisez un appareil capable de se connecter en WiFi (Smartphone, ordinateur, tablette...) puis connectez-vous au réseau WiFi *CelebWeather* en donnant le mot de passe *ESP32CelebWeather*

Il est possible qu'un avertissement soit affiché vous annonçant qu'il n'y a pas de connexion à Internet via ce point d'accès.<br/>C'est tout à fait normal, il faut bien resté connecté.

Ouvrez un navigateur et tapez cette adresse: 192.168.4.1
Cliquez sur le lien "configure page" qui vous demande un login et mode passe, utilisez les éléments suivants :

* login: admin
* mot de passe: ESP32CelebWeather

Sur la page de configuration, vous devez remplir les éléments suivants en laissant les autres valeurs à leur état initial :

| Champ | Valeur attendue |
|---|---|
|AP password| Le nouveau mot de passe pour le point d'accès et la page de configuration. Vous DEVEZ le changer et donner une valeur différente du mot de passe par défaut|
|WiFi SSID| L'identifiant de votre réseau WiFi, celui qui vous permet de vous connecter à Internet |
|WiFi password| Le mot de passe pour se connecter à votre réseau WiFi |
|Latitude| La latitude du lieu pour lequel vous voulez les prévisions en utilisant le point comme séparateur décimal. Normalement autour de 45 |
|Longitude| La longitude du lieu pour lequel vous voulez les prévisions en utilisant le point comme séparateur décimal. Positif pour l'est, Négatif pour l'ouest |

Cliquez sur *Apply* et déconnectez votre appareil du point d'accès.

Le module va alors se connecter à votre réseau Wifi et commencer à récupérer les infos dont il a besoin: date et heure, département et prévisions associés aux coordonnées données.

## Synchronisation de la station météo

Une fois le module correctement connecté, vous pouvez y associer votre station météo en suivant ces étapes à la lettre

1. Enlever les piles de la station ET du capteur extérieur
2. Mettre les piles dans le capteur extérieur
3. Mettre les piles dans la station, moins de 5 minutes après le capteur
4. Attendre que la station affiche la température du capteur extérieur
5. Le symbole de réception clignote alors en mode "Point seul".
6. Attendre 10 clignotements du symbole de réception
7. Appuyer sur le bouton "Force refresh" du module
8. Attendre que la date et l'heure s'affichent en même temps que le département clignote
9. Attendre que les prévisions soient affichées
10. Appuyer le bouton `Set` longtemps pour passer la station en mode *configuration*
11. Il est normal qu'un seul département soit disponible, inutile d'utiliser `+`
12. Appuyer le bouton `Set` plusieurs fois pour configurer les autres éléments (contraste, alerte sonore en cas de vigilance...) jusqu'à revenir à l'affichage des prévisions

Une fois tout ceci fait, la station passe en mode veille et ne met à jour que la température extérieure toutes les 4 secondes.

Les prévisions sont récupérées une fois par heure par le module et envoyées à la station toutes les 6 heures vers 00h, 06h, 12h, et 18h.

En cas de perte du signal, il faut tout recommencer !
