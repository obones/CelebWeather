# Protocole

L'identification des éléments constituant ce protocole de transmission a été rendu possible par le travail de plusieurs membres du forum [TetraHub](https://forum.tetrahub.net/decodage-meteo/)

- [Protocole](#protocole)
  - [Généralités](#généralités)
  - [Niveau physique](#niveau-physique)
  - [Niveau Réseau](#niveau-réseau)
  - [Niveau présentation](#niveau-présentation)
  - [Niveau application](#niveau-application)
    - [Date/Heure + Départements (0xF)](#dateheure--départements-0xf)
    - [Prévisions météo longues (0x0)](#prévisions-météo-longues-0x0)
    - [Prévisions météo courtes (0x4)](#prévisions-météo-courtes-0x4)
    - [Trame inconnue (0xE)](#trame-inconnue-0xe)

## Généralités

Les messages reçus par la station sont émis selon le protocole POCSAG qui était utilisé par les pagers à la grande époque où ça servait encore.

## Niveau physique

La transmission se fait par modulation de fréquence (FSK) sur une porteuse à 466.206250 MHz et une déviation de 4 kHz

ATTENTION: La polarité choisie pour les bits 1 et 0 doit être la bonne sinon la station ignore le contenu reçu.

Avec un analyseur de spectre, on doit observer ceci:

<img src="images/sdrangel_waterfall.png" />

## Niveau Réseau

Le message POCSAG doit utiliser les paramètres suivants :

* RIC = 25176
* Fonction = 3
* Mode alphanumérique

Le reste du découpage en batch, les mots de synchronisation et de veille sont ceux par défaut du protocole POCSAG.

## Niveau présentation

Chaque caractère présent dans le message correspond à 6 bits selon cette correspondance:

| Valeur | caractère |
|---|---|
| 3 | s |
| 32 | p |
| 59 | k |
| 60 | l |
| 61 | m |
| 62 | n |
| 63 | o |
| 0-2; 4-31; 33-58 | 32 + valeur |

Les valeurs sur 6 bits sont ensuite à répartir par groupe de 4 afin de créer des trames de quartets (nibbles) dans l'ordre des bits lus :

<table border=1 cellpadding=4 cellspacing=0>
    <tr>
        <td>Bit</td>
        <td>0</td>
        <td>1</td>
        <td>2</td>
        <td>3</td>
        <td>4</td>
        <td>5</td>
        <td>6</td>
        <td>7</td>
        <td>8</td>
        <td>9</td>
        <td>10</td>
        <td>11</td>
        <td>12</td>
        <td>13</td>
        <td>14</td>
        <td>15</td>
        <td>16</td>
        <td>17</td>
        <td>18</td>
        <td>19</td>
        <td>20</td>
        <td>21</td>
        <td>22</td>
        <td>23</td>
    </tr>
    <tr>
        <td>Caractère</td>
        <td colspan=6 align="center">0</td>
        <td colspan=6 align="center">1</td>
        <td colspan=6 align="center">2</td>
        <td colspan=6 align="center">3</td>
    </tr>
    <tr>
        <td>Quartet</td>
        <td colspan=4 align="center">0</td>
        <td colspan=4 align="center">1</td>
        <td colspan=4 align="center">2</td>
        <td colspan=4 align="center">3</td>
        <td colspan=4 align="center">4</td>
        <td colspan=4 align="center">5</td>
    </tr>
</table>

## Niveau application

Plusieurs types de trames ont été identifiés, seuls certaines sont utilisées dans ce projet afin de fournir un fonctionnement minimaliste.

En particulier, l'envoi d'alertes de vigilance n'est pas encore compris.

Toutes les trames commencent par un quartet d'identification et la plupart incluent une somme de contrôle pour certains éléments.<br/>
Cette somme ce calcule en additionnant tous les quartets d'une zone donnée à un compteur 8 bits non signé initialisé avec la valeur 7.</br>
La somme peut déborder naturellement ce qui est l'équivalent d'un modulo 256.<br/>
Le résultat est stocké sur un ou deux quartets selon les cas. Sur un quartet, on ignore les 4 bits de poids fort ce qui est l'équivalent d'un modulo 16.

### Date/Heure + Départements (0xF)

|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|
| Quartet | 0 | 1 | 2 | 3 | 4 | | | | 8 | 9 | 10 | 11 | |
| Nombre de bits | | | | | | 2 | 4 | 6 | | | | | 5 | 5 | 7 | 7 | 7 | 4 | 8 | 4 | 4 |
| Utilisation | Marqueur<br/>0xF | Heure | Minutes: dizaines | Minutes: unités | Mois | Jour: dizaine | Jour: unités | Année | Checksum | 0 | 0 | 0 | Intervalle entre les prévisions | Nombre N de départements | Département 0 | ... | Département N-1 | 0x5 | Checksum | 0x2 | 0xD

Le codage de la date et l'heure est assez alambiqué :

* les heures sont sur 4 bits mais avec ces ajustements :
    * entre 0 et 9 -> valeur directe
    * entre 10 et 19 --> valeur moins 10 et + 10 au chiffre des dizaines de minutes
    * entre 20 et 23 --> valeur moins 10
* les minutes sont codées "en BCD" : dizaines sur un quartet, unités sur un quartet. Ne pas oublier que le chiffre "BCD" des dizaines de minutes se voit ajouter 10 pour les heures entre 10 et 19
* les mois sont codés en binaire sur un quartet
* les jours sont codés en BCD, avec les dizaines sur 2 bits et les unités sur 4 bits
* les années sont codée en binaire sur 6 bits, la valeur 0 étant l'année 2000.

Le codage des départements se fait sous la forme d'une liste de valeurs sur 7 bits précédée du nombre de valeurs sur 5 bits.<br/>
Le dernier numéro de département est complété à droite par 0 à 3 bits afin d'aligner le résultat sur un quartet. Ainsi, la valeur 0x5 située juste après est parfaitement alignée sur le début d'un quartet.

L'intervalle entre les prévisions est utilisé pour que la station sache à quelle minute écouter aux heures suivantes :

* 00h
* 06h
* 12h
* 18h

Par exemple, pour le département 2, elle écoutera à 00h + 2 * Intervalle minutes et pendant environ 4 minutes.</br>
Dans les observations, cet intervalle vaut toujours 12, dans l'exemple précédent la station écoutera donc pendant 4 minutes à 00h24, 06h24, 12h24 et 18h24

### Prévisions météo longues (0x0)

|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|
| Quartet | 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 | 8 | 9 | 10 | 11 | 12 | 13 | 14 | 15 | 16 | 17 | 18 | 19 |
| Utilisation | Marqueur<br/>0x0 | Département (Poids fort) | Département (Poids faible) | 0x0 | 0x04 | Température basse : dizaines | Température basse : unités | Température haute : dizaines | Température haute : unités | Icône 0 (Poids fort) | Icône 0 (Poids faible) | Icône 1 (Poids fort) | Icône 1 (Poids faible) | Icône 2 (Poids fort) | Icône 2 (Poids faible) | Icône 3 (Poids fort) | Icône 3 (Poids faible) | Icône 4 (Poids fort) | Icône 4 (Poids faible) | Checksum |

Les quartets 5 à 19 sont répétés pour autant de jours de prévisions que nécessaire, soit 6  fois : jour en cours + 5 jours suivants.

La somme de contrôle est présente dans chaque répétition et calculée sur les 14 quartets précédents.

La température est encodée en BCD et décalée de 40. Ainsi, une température de 12°C est encodée par 0x52

Les valeurs possibles pour les icônes sont présentées dans ce [document spécifique](icons.md).<br/>
L'utilisation des icônes est la suivante:

| Icône | Plage horaire |
|--|--|
| 0 | Journée entière |
| 1 | Nuit (00:00 - 05:59) |
| 2 | Matinée (06:00 - 11:59) |
| 3 | Après midi (12:00 - 17:59) |
| 4 | Soirée (18:00 - 23:59) |

Attention : l'icône de nuit d'un groupe de 5 est associée au jour précédent. Ainsi, la station affichera l'icône du troisième groupe pour la période "Nuit" de la journée de demain.

A la suite de ces éléments, on trouve les prévisions de probabilité de pluie selon ce format, répété six fois :

|  |  |  |  |  |  |
|--|--|--|--|--|--|
| Quartet | 0 | 1 | 2 | 3 | 4  |
| Utilisation | 0x3 | 0xC | Probabilité de pluie pour la journée | 0x6 | 0xE |

La probabilité de pluie est une correspondance entre une valeur et un niveau donné :

|  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |  |
|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|--|
| Valeur | 0x0 | 0x1 | 0x2 | 0x3 | 0x4 | 0x5 | 0x6 | 0x7 | 0x8 | 0x9 | 0xA | 0xB | 0xC | 0xD | 0xE |
| Probabilité | 0 |  5 |  10 |  20 |  25 |  30 |  40 |  50 |  60 |  70 |  75 |  80 |  90 |  95 |  99 |

Enfin, les prévisions de pluie sont suivies des éléments suivants :

|  |  |  |  |  |
|--|--|--|--|--|
| Quartet | 0 | 1 | 2 | 3 |
| Utilisation | Checksum poids fort | Checksum poids faible | 0x0 | 0xB |

La somme de contrôle est calculée sur l'intégralité des 5 * 6 = 30 quartets utilisés pour les prédictions de pluie.

### Prévisions météo courtes (0x4)

Cette trame n'est prévue pour encoder que 3 jours de prévisions et n'a été observée que pour le département 75, elle n'est donc pas utilisée par ce module.

Son format est identique aux prévisions météo longues (0x0) en limitant les pictogrammes à 3 listes et en excluant la partie probabilité de pluie

### Trame inconnue (0xE)

Les messages reçus sont par exemple ceux-ci

```
ZK<*H2H:HBHJHRI"I*I2I:IZH'
ZG<2H:HBHRI*I2I:JN
ZG<2H:HBHRI*I2!?%'?BWHC+da?rp
```

décodés en quartets ainsi:

```
E A B 7 0 A A 1 2 A 1 A A 2 2 A 2 A A 3 2 A 4 2 A 4 A A 5 2 A 5 A A 7 A A 0 7 B
E A 7 7 1 2 A 1 A A 2 2 A 3 2 A 4 A A 5 2 A 5 A A A E B
E A 7 7 1 2 A 1 A A 2 2 A 3 2 A 4 A A 5 2 0 5 F 1 4 7 7 E 2 D E 8 8 C B 1 0 1 7 D 2 8 2 D
```

Aucun impact sur la station n'a été remarqué, ce projet ne l'utilise donc pas