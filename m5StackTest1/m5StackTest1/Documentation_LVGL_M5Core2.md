# Documentation du Code LVGL + M5Core2  

##  Vue d’ensemble

Voici la version en **un seul paragraphe** :

Ce document explique en détail comment un **M5Core2** (basé sur ESP32) utilise la bibliothèque graphique **LVGL** pour initialiser l’écran et l’entrée tactile, configurer les buffers de dessin nécessaires à l’affichage, gérer le temps interne de LVGL via un système de ticks, charger automatiquement une interface graphique générée par **SquareLine Studio** grâce aux fichiers `ui.h` et à la fonction `ui_init()`, et enfin maintenir et rafraîchir cette interface de manière continue dans la boucle principale `loop()` du programme Arduino.




##  Inclus au début du programme

```cpp
#include <M5Core2.h>
#include <Ticker.h> // Minuteur périodique pour les ticks LVGL
#include <lvgl.h>
#include "ui.h" // Déclarations d'interface générées par SquareLine
```

### Rôle de chaque fichier :

* **`M5Core2.h`**
  Fournit l’accès à l’écran, au tactile, aux boutons, à l’alimentation, etc.

* **`Ticker.h`**
  Permet de configurer une fonction appelée régulièrement (toutes les X millisecondes).
  Ici, il sert à **incrémenter en continu le temps de LVGL**.

* **`lvgl.h`**
  C’est la bibliothèque principale de **LVGL**, qui gère :

  * les widgets (boutons, labels, sliders…)
  * les écrans
  * les animations
  * le dessin dans des buffers

* **`"ui.h"`**
  Fichier généré par **SquareLine Studio**.
  Il contient la fonction `ui_init()` qui construit ton interface (écrans, boutons, etc.).

Dans  `ui.h`, il faut remplacer la ligne :

```cpp
#include "/home/ramy/Desktop/WorkShop/m5StackTest1"
```

par :

```cpp
#include <lvgl.h>
```
---

## Constantes et variables globales LVGL

```cpp
static const uint32_t LVGL_TICK_RATE_MS = 5; // Intervalle de tick attendu par LVGL
static const uint32_t LVGL_BUF_HEIGHT = 40;  // Hauteur du buffer de dessin intermédiaire
static const uint32_t LVGL_BUFFER_PIXELS = 320 * LVGL_BUF_HEIGHT; // Pixels mis en mémoire pour les buffers intermédiaires
```


* `LVGL_TICK_RATE_MS = 5`
  → Toutes les **5 ms**, on dira à LVGL que le temps a avancé de 5 ms.
  LVGL utilise cette info pour les animations, les timers, etc.

* `LVGL_BUF_HEIGHT = 40`
  → On ne dessine pas tout l’écran d’un coup dans le buffer.
  On utilise un buffer correspondant à **40 lignes de hauteur** (pour limiter la RAM utilisée).

* `LVGL_BUFFER_PIXELS = 320 * 40`
  → Nombre total de pixels dans le buffer : la largeur de l’écran (320) × 40 lignes.

```cpp
static Ticker lv_tick_timer;                // Minuteur qui incrémente les ticks LVGL
static lv_disp_draw_buf_t draw_buf;         // Descripteur du buffer de dessin transmis à LVGL
static lv_color_t draw_buf_data[LVGL_BUFFER_PIXELS]; // Mémoire buffer
static lv_disp_drv_t disp_drv;              // Driver d'affichage LVGL
static lv_indev_drv_t touch_drv;            // Driver d'entrée (input) LVGL
static lv_indev_t * touch_indev = nullptr;  // Gestionnaire du périphérique d'entrée LVGL
```

Ces variables servent à :

* Définir **un buffer de dessin** où LVGL va dessiner avant d’envoyer à l’écran.
* Décrire à LVGL **comment afficher** (driver d’affichage).
* Décrire à LVGL **comment lire le tactile** (driver d’entrée).
* Mettre en place un **timer** périodique (`Ticker`) pour le temps LVGL.

---

##  Fonction de lecture du tactile

```cpp
static void touch_read(lv_indev_drv_t * drv, lv_indev_data_t * data)
{
    (void)drv; 
    M5.Touch.update(); // actualise les lectures du tactile
    const bool pressed = M5.Touch.points > 0; // détermine si le tactile est actif
    data->state = pressed ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED; // rapporte l'état à LVGL
    if(pressed) {
        lv_point_t point; // convertit en point LVGL
        point.x = M5.Touch.point[0].x;
        point.y = M5.Touch.point[0].y;
        data->point = point;
    }
    data->continue_reading = false; //  On dit à LVGL que cette lecture est terminée (une seule lecture à chaque appel).
}
```

### Que fait cette fonction ?

C’est la **fonction de callback d’entrée** pour LVGL :
LVGL l’appelle pour savoir :

* si l’écran est pressé ou non,
* à quelle position x/y le doigt touche l’écran.

---

## Gestion du temps LVGL (tick)

```cpp
static void lv_tick_task()
{
    lv_tick_inc(LVGL_TICK_RATE_MS);
}
```

* LVGL doit connaître l’écoulement du temps pour gérer :

  * animations
  * transitions
  * timeouts
  * timers internes

* `lv_tick_inc(ms)` : indique à LVGL que `5 ms` se sont écoulées.


---

##  Callback d’affichage (display flush)

```cpp
static void display_flush(lv_disp_drv_t * disp_drv, const lv_area_t * area, lv_color_t * color_map)
{
    const uint32_t width = (uint32_t)area->x2 - area->x1 + 1; // largeur de la zone mise à jour
    const uint32_t height = (uint32_t)area->y2 - area->y1 + 1; // hauteur de la zone mise à jour

    M5.Lcd.startWrite(); // commence la transaction DMA TFT
    M5.Lcd.setAddrWindow(area->x1, area->y1, width, height); // définit le rectangle de destination
    M5.Lcd.pushColors(reinterpret_cast<uint16_t *>(color_map), width * height, true); // envoie les pixels
    M5.Lcd.endWrite(); // termine la transaction

    lv_disp_flush_ready(disp_drv); // informe LVGL que l'effacement est terminé
}
```

### Rôle :

Cette fonction est appelée par LVGL lorsqu’il a fini de **dessiner des pixels dans le buffer** et qu’il souhaite les envoyer vers l’écran.

* `area` : rectangle de l’écran à mettre à jour (x1, y1, x2, y2)
* `color_map` : tableau de pixels (couleurs) que LVGL a calculés pour cette zone

Étapes :

1. Calcul de la **largeur** et de la **hauteur** de la zone à dessiner.
2. `M5.Lcd.startWrite();`
   → Démarre une transaction d’écriture vers l’écran (optimisé).
3. `M5.Lcd.setAddrWindow(...)`
   → Indique à l’écran où les nouveaux pixels doivent être placés.
4. `M5.Lcd.pushColors(...)`
   → Envoie les pixels du buffer LVGL (`color_map`) vers l’écran.
5. `M5.Lcd.endWrite();`
   → Termine la transaction.
6. `lv_disp_flush_ready(disp_drv);`
   → Indique à LVGL : “J’ai fini d’envoyer les données à l’écran, tu peux réutiliser le buffer”.

---

## 🚀 Fonction `setup()`

```cpp
void setup()
{
    M5.begin();
    M5.Lcd.setRotation(1); // oriente l'écran pour le système de coordonnées LVGL
```
---

### Initialisation de LVGL

```cpp
    lv_init(); // initialise les structures centrales de LVGL
```
---

### Configuration du buffer d’affichage LVGL

```cpp
    const uint32_t hor_res = M5.Lcd.width();  // récupère la largeur de l'écran
    const uint32_t ver_res = M5.Lcd.height(); // récupère la hauteur de l'écran

    lv_disp_draw_buf_init(&draw_buf, draw_buf_data, nullptr, LVGL_BUFFER_PIXELS); // prépare le buffer de dessin pour LVGL
```


* `lv_disp_draw_buf_init` :

  * `draw_buf` : descripteur LVGL du buffer
  * `draw_buf_data` : le tableau de pixels que l’on a défini
  * `nullptr` : pas de second buffer (on utilise un buffer simple)
  * `LVGL_BUFFER_PIXELS` : nombre de pixels dans le buffer

---

### Configuration du driver d’affichage LVGL

```cpp
    lv_disp_drv_init(&disp_drv);
    disp_drv.draw_buf = &draw_buf;
    disp_drv.flush_cb = display_flush;
    disp_drv.hor_res = hor_res;
    disp_drv.ver_res = ver_res;
    lv_disp_t * disp = lv_disp_drv_register(&disp_drv); // crée un objet d'écran LVGL
```

* `lv_disp_drv_init` → met des valeurs par défaut dans `disp_drv`.

* On configure :

  * `draw_buf` : quel buffer LVGL doit utiliser.
  * `flush_cb` : quelle fonction appeler pour envoyer les pixels à l’écran (la nôtre).
  * `hor_res` / `ver_res` : résolution de l’écran.

* `lv_disp_drv_register` :
  → Enregistre ce driver auprès de LVGL et renvoie un pointeur `disp` représentant l’afficheur.

---

### Configuration du driver d’entrée (tactile)

```cpp
    lv_indev_drv_init(&touch_drv);
    touch_drv.type = LV_INDEV_TYPE_POINTER; // configure le mode pointeur (tactile)
    touch_drv.read_cb = touch_read; // enregistre notre lecteur tactile (lire l’état du tactile.)
    touch_drv.disp = disp; // lie l'entrée à l'écran
    touch_indev = lv_indev_drv_register(&touch_drv); //  on enregistre le driver d’entrée auprès de LVGL.
```
---

### Lancement du timer LVGL

```cpp
    lv_tick_timer.attach_ms(LVGL_TICK_RATE_MS, lv_tick_task); // alimente le compteur de ticks LVGL
```

* On attache la fonction `lv_tick_task` à un timer (`Ticker`).
* Cette fonction sera appelée toutes les **5 ms**.
* Chaque appel fait avancer le temps interne de LVGL (`lv_tick_inc(5)`).

---

### Initialisation de l’interface SquareLine

```cpp
    ui_init(); // construit l'interface SquareLine
}
```

* `ui_init()` est défini dans `ui.h` (généré par SquareLine Studio).
* Elle :

  * crée les écrans,
  * instancie les boutons, labels, etc.,
  * charge généralement l’écran principal (`lv_scr_load(...)`).

---

## Fonction `loop()`

```cpp
void loop() // Boucle principale Arduino : gère LVGL et le tactile
{
    M5.update(); // actualise l'état du tactile / des boutons
    lv_timer_handler(); // exécute le gestionnaire LVGL (animations, événements)
    delay(LVGL_TICK_RATE_MS); // temporisation simple pour limiter la charge CPU
}
```

### Que se passe-t-il dans la boucle ?

* `lv_timer_handler();`
  → C’est la fonction principale de LVGL appelée régulièrement :

  * gère les animations,
  * traite les événements (clics, appuis),
  * déclenche les redessins d’écrans si nécessaire.

* `delay(LVGL_TICK_RATE_MS);`
  → Petite pause (5 ms) pour ne pas saturer le CPU et synchroniser un peu la boucle avec la fréquence des ticks.

---

## Résumé global

* **Drivers LVGL** :

  * `display_flush` : envoie les pixels vers l’écran M5Core2.
  * `touch_read` : fournit à LVGL les coordonnées du tactile.

* **Temps LVGL** :

  * `lv_tick_task` + `Ticker` → avancent l’horloge LVGL toutes les 5 ms.

* **Initialisation** (`setup`) :

  * M5Core2 (écran + tactile)
  * LVGL (core + buffer + drivers)
  * Interface UI (`ui_init()`)
