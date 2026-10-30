# serial — Erweiterungen der Konsolen-Eingabe

Dieses Dokument beschreibt die Änderungen an `wire/src/serial.c` und
`wire/inc/serial.h` vom Oktober 2026:

1. **Zeilenmodus** (`serial_set_line_mode()`): mehrstellige Eingaben, die ein
   Mensch Zeichen für Zeichen tippt, kommen als *eine* Eingabe an.
2. **`serial_waitForNumber()`**: `*key` zeigt immer auf gültigen Text, und
   Eingaben ohne Ziffern gelten nicht mehr als die Zahl 0.
3. **`_read()`**: Der USB-Zweig (`HAL_PCD_MODULE_ENABLED`) ist wieder
   erreichbar, und die Datei kompiliert ohne USB wieder.

Benutzt werden die Erweiterungen von den Boot-Abfragen der Firmware
(`Core/Src/main.c`: SLOT, CHANNEL, FREQBAND, FREQUENCY OFFSET).

---

## 1. Hintergrund: so kommt eine Eingabe an

```
 UART RX ──DMA──▶ rx_buf[RX_BUFFER_SIZE]
                     │  Leitung eine Zeichenzeit still (IDLE)
                     ▼
        HAL_UARTEx_RxEventCallback(huart, size)        ← Interrupt-Kontext
                     │
                     ▼
        isio.buffer[SIO_RX]   (buffer_t, LINEAR)
                     │  wird vom Hauptprogramm gepollt
                     ▼
        serial_waitForNumber(&key)  /  keyboard_scan()  /  _read()
```

* `serial_rx_restart()` startet den Empfang mit
  `HAL_UARTEx_ReceiveToIdle_DMA(isio.uart, rx_buf, RX_BUFFER_SIZE)`.
* Die HAL ruft `HAL_UARTEx_RxEventCallback()` auf, sobald die Leitung eine
  Zeichenzeit lang still ist (Idle-Line-Erkennung) oder `rx_buf` voll ist.
  `size` ist die Zahl der empfangenen Bytes.
* Ohne Zeilenmodus kopiert der Callback diese Bytes mit `buffer_set()` nach
  `isio.buffer[SIO_RX]`, wandelt sie in Kleinbuchstaben und übergibt sie an
  `serial_apply_change()`, also an die Tastatur-Emulation. Danach startet er
  den Empfang neu.

### Das Problem mit menschlicher Eingabe

Eine Zeichenzeit dauert bei 115 200 Baud etwa **87 µs**. Zwischen zwei
Tastendrücken eines Menschen liegen 100 ms oder mehr. Darum wird **jedes
getippte Zeichen zu einem eigenen Empfangs-Ereignis**, und jedes Ereignis
**überschreibt** den Inhalt von `isio.buffer[SIO_RX]`.

Die Boot-Abfragen in `main.c` fragen `serial_waitForNumber()` in einer
Schleife ab und übernehmen den ersten gültigen Wert sofort:

| Abfrage | getippt | ohne Zeilenmodus angenommen |
|---|---|---|
| SLOT | `1` `3` | **1**, gleich nach der ersten Taste |
| CHANNEL | `1` `2` | **1** |
| FREQUENCY OFFSET | `-` `2` `1` `9` `7` `8` | `-` ergibt 0 (alter Code) bzw. wird verworfen (neuer Code), dann **2** |

Werkzeuge wie `set_offset.py` oder der Kalibrier-Tab des Viewers waren nicht
betroffen: Sie senden Wert und `\r` in einem Stück, ohne Pause. Das ergibt
genau ein Ereignis mit der ganzen Eingabe.

---

## 2. Zeilenmodus

### API

```c
void serial_set_line_mode(bool on);   // serial.h
```

* `on = true`: Eingaben werden gesammelt und erst bei `\r` oder `\n` als
  ganze Zeile weitergegeben.
* `on = false`: Verhalten wie bisher, jedes Empfangs-Ereignis wird sofort
  weitergegeben.
* Nach dem Start ist der Zeilenmodus **aus**.

### Zustand (in `serial.c`, modul-lokal)

```c
static volatile bool line_mode = false;      // Modus an/aus
static uint8_t  rx_line[RX_BUFFER_SIZE];     // angefangene Zeile
static uint16_t rx_line_len = 0;             // Länge der angefangenen Zeile
```

`line_mode` ist `volatile`, weil das Hauptprogramm ihn setzt und der
Interrupt ihn liest.

### Ablauf im Callback

```c
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t size) {
    if (huart != isio.uart) return;
    if (line_mode) {
        serial_rx_collect_line(rx_buf, size);    // neu
    } else {
        buffer_set(...); buffer_tolower(...); serial_apply_change(...);  // wie bisher
    }
    serial_rx_restart();                          // in beiden Fällen
}
```

`serial_rx_collect_line()` geht die empfangenen Bytes einzeln durch:

| Zeichen | Wirkung |
|---|---|
| `\r` oder `\n` | Ist `rx_line_len > 0`, wird die Zeile **übergeben** (siehe unten). Danach `rx_line_len = 0`. |
| `\b` (0x08) oder DEL (0x7f) | Rücktaste: `rx_line_len` um eins verringern, falls > 0 |
| jedes andere Zeichen | wird an `rx_line` angehängt, solange Platz ist. Überzählige Zeichen werden verworfen. |

Ein Ereignis kann mehrere Zeichen enthalten, z.B. `"9\r"` von einem
Werkzeug oder `"\r\n"` vom Terminal. Darum läuft die Schleife über alle
`size` Bytes, und ein Ereignis kann auch mehrere Zeilen enthalten.

### Übergeben einer Zeile

```c
buffer_t *rx = isio.buffer[SIO_RX];
uint16_t  len = MIN(rx_line_len, rx->size - 1);  // Platz für die 0 lassen
buffer_reset(rx);                                // ganzen Puffer mit 0 füllen
buffer_set(rx, rx_line, len);
buffer_tolower(rx);
serial_apply_change(rx->mem);
```

* **`buffer_reset()` vor `buffer_set()` ist nötig**, weil `buffer_set()` keine
  abschliessende 0 schreibt. `serial_waitForNumber()` und `_read()` arbeiten
  mit `strlen()`/`strtol()`. Ohne den Reset blieben Reste einer längeren
  früheren Eingabe stehen: Aus `"13"` und danach `"9"` würde `"93"`.
* `len` ist auf `rx->size - 1` begrenzt, damit immer mindestens eine 0
  folgt.
* Kleinschreibung und `serial_apply_change()` laufen wie im normalen Modus,
  nur einmal pro Zeile statt einmal pro Zeichen.

### Leere Zeilen

Eine leere Zeile (Zeilenende ohne vorherige Zeichen) wird **nicht**
übergeben. Das ist aus zwei Gründen wichtig:

* Ein Terminal, das `\r\n` sendet, würde sonst die eben übergebene Zeile beim
  `\n` sofort durch eine leere ersetzen. Die Abfrage-Schleife sähe sie dann
  womöglich nie.
* Der Kalibrier-Tab des Viewers und `measure_offset.py` senden vor einer
  Eingabe ein einzelnes `\r`, weil die erste Eingabe nach einer Pause
  manchmal verloren geht. Dieses `\r` darf nichts überschreiben.

### Beispiel: Mensch tippt `13` und Enter (gtkterm)

```
Zeit     Leitung   Ereignis     rx_line / rx_line_len     isio.buffer[SIO_RX]
  0 ms   '1'       size=1       "1"  / 1                   (unverändert)
250 ms   '3'       size=1       "13" / 2                   (unverändert)
600 ms   '\r'      size=1       ""   / 0   → übergeben     "13"
```

`serial_waitForNumber()` liefert erst ab 600 ms die Zahl **13**, vorher −1.

### Beispiel: Werkzeug sendet `"\r"`, 300 ms Pause, `"9\r"`

```
Ereignis "\r"   → leere Zeile, wird ignoriert
Ereignis "9\r"  → "9" angehängt, dann übergeben → isio.buffer[SIO_RX] = "9"
```

### Ein- und Ausschalten

```c
void serial_set_line_mode(bool on) {
    __disable_irq();
    line_mode   = on;
    rx_line_len = 0;      // angefangene Zeile verwerfen
    __enable_irq();
}
```

Modus und Zeilenlänge werden zusammen unter gesperrten Interrupts gesetzt.
So sieht der Callback nie einen halb umgeschalteten Zustand.

### Zusammenspiel mit `serial_reset()`

`serial_reset()` (wird z.B. über `keyboard_reset()` vor jeder Boot-Abfrage
aufgerufen) setzt zusätzlich `rx_line_len = 0`. Eine halb getippte Zeile aus
der vorherigen Abfrage gelangt so nicht in die nächste.

---

## 3. `serial_waitForNumber()`

```c
int32_t serial_waitForNumber(char **key);
```

Die Funktion wartet nicht selbst, sie wird in einer Schleife gepollt. Sie
liefert die Zahl aus `isio.buffer[SIO_RX]` oder −1, wenn dort keine Zahl
steht. Über `key` erhält der Aufrufer den Text der Eingabe, damit er Befehle
wie `r` (Reset) oder `d` (EEPROM löschen) erkennen kann.

### Was falsch war

1. Im UART-Pfad wurde `*key` nie gesetzt, nur im USB-Zweig. `main.c` startet
   mit `char *key = NULL` und prüfte danach `*key == 'r'`. Das ist eine
   NULL-Dereferenzierung. Auf dem STM32L4 stürzt sie nicht ab, weil Adresse 0
   auf das Flash gespiegelt ist. Sie liest aber das erste Byte der
   Vektortabelle, nie `'r'` oder `'d'`. **`r` und `d` funktionierten an den
   Boot-Abfragen deshalb nie.**
2. `strtol()` wurde ohne Prüfung von `endptr` ausgewertet. Eine Eingabe ohne
   Ziffern wie `r` oder `del` ergab **0** und galt als gültige Zahl. An der
   Offset-Abfrage wäre ein `d` als Offset 0 gespeichert worden.
3. `errno` wurde vor `strtol()` nicht zurückgesetzt. Die Prüfung auf
   Überlauf konnte deshalb einen alten Fehler sehen.

### Jetziges Verhalten

* `*key` zeigt **immer** auf gültigen Text: auf `""`, solange nichts
  empfangen wurde, sonst auf die Eingabe selbst (`isio.buffer[SIO_RX]->mem`
  bzw. `urx_buffer.mem` bei USB).
* `key == NULL` ist erlaubt. Dann wird nur die Zahl geliefert.
* Rückgabewerte:

| Inhalt von `isio.buffer[SIO_RX]` | Rückgabe | `*key` |
|---|---|---|
| leer | −1 | `""` |
| `"13"` | 13 | `"13"` |
| `"-21978"` | −21978 | `"-21978"` |
| `"r"`, `"d"`, `"del"` | −1 (keine Ziffern) | `"r"` … |
| `"12abc"` | 12 (`strtol` liest nur den Anfang) | `"12abc"` |
| Zahl ausserhalb von `long` | −1 (`ERANGE`) | der Text |
| Modul nicht initialisiert | `EM_ERR` (−1) | `""` |

Hinweis: −1 ist zugleich „keine Zahl“. Die Boot-Abfragen benutzen −1 auch
als „noch nicht gesetzt“. Eine echte Eingabe von `-1` wird daher wie keine
Eingabe behandelt.

---

## 4. `_read()`

Beim Entfernen von `clabel` war die Zeile
`if (urx_buffer.state == BUFFER_USED) {` versehentlich mitgelöscht worden.
Danach stand der USB-Zweig im Block `if (!isio.init)`, hinter dessen
`return`, und war damit toter Code. Die schliessende Klammer lag zudem
innerhalb des `#ifdef HAL_PCD_MODULE_ENABLED`. Ohne USB fehlte sie, und die
Datei kompilierte nicht mehr (`invalid storage class for function …`).

Die alte Struktur ist wiederhergestellt:

```c
if (!isio.init) return rLen;
#ifdef HAL_PCD_MODULE_ENABLED
if (urx_buffer.state == BUFFER_USED) {
    serial_apply_change(urx_buffer.mem);
    buffer_transfer(&urx_buffer, isio.buffer[SIO_RX]);
    time_stop(urxhdl, NULL);
    return isio.buffer[SIO_RX]->used;
}
#endif
```

---

## 5. Verwendung in der Firmware (`Core/Src/main.c`)

```c
serial_set_line_mode(true);          // vor der SLOT-Abfrage
keyboard_reset(rb_system.sDev);
printf("SLOT (1, 3, 5, 7, 9, 11, 13, 15)? " NL);
while (pers->my_slot == -1) {
    pers->my_slot = serial_waitForNumber(&key);
    ...
    if (*key == 'r') rb_system_reset();
    if (*key == 'd') persistent_delete_reboot();
    ...
}
... CHANNEL?, FREQBAND?, FREQUENCY OFFSET [Hz]? ...
persistent_close(true);
serial_set_line_mode(false);         // nach der Offset-Abfrage
```

* Der Zeilenmodus gilt **nur an den Boot-Abfragen**. Im normalen Betrieb
  bleibt die Konsole im Zeichenmodus. Die Tastatur-Emulation über die
  Konsole (`serial_apply_change()` pro Taste) funktioniert dort wie bisher,
  ohne Enter.
* Zwischen den Abfragen startet die Firmware mehrmals neu: nach dem
  Speichern (`persistent_close(true)`), nach `d` und nach `r`. Jeder Neustart
  läuft wieder durch `serial_set_line_mode(true)`, der Modus ist an den
  Abfragen also immer aktiv.
* Der SLOT-Prompt nennt die Slots dezimal (`11, 13, 15` statt `b, d, f`),
  weil `serial_waitForNumber()` dezimal liest. Ein `d` an der SLOT-Abfrage
  löscht das EEPROM, es bedeutet nicht Slot 13.

### Bedienung von Hand (z.B. gtkterm)

* Wert tippen und **Enter** drücken. Erst dann wird der Wert übernommen.
* Auch `r` und `d` brauchen an den Abfragen ein Enter.
* Tippfehler lassen sich mit der Rücktaste korrigieren.
* Die Firmware gibt **kein Echo** zurück, siehe unten. Wer sehen will, was
  er tippt, schaltet im Terminal das lokale Echo ein (gtkterm:
  *Configuration → Local echo*).

---

## 6. Grenzen und Hinweise

* **Kein Echo:** Der Callback läuft im Interrupt-Kontext. Ein Echo von dort
  würde mit der DMA-Ausgabe (`printf` → `_write`) um den UART konkurrieren.
  Ein Echo aus dem Hauptprogramm ginge zwar, aber `printf` stellt im Modus
  `TIMESTAMP` jeder Ausgabe einen Zeitstempel voran. Darum gibt es bewusst
  keines.
* **Zeilenlänge:** höchstens `RX_BUFFER_SIZE` Zeichen werden gesammelt, und
  höchstens `isio.buffer[SIO_RX]->size - 1` werden übergeben. Für Zahlen und
  kurze Befehle reicht das. Längere Eingaben werden ohne Meldung
  abgeschnitten.
* **Die übergebene Zeile bleibt stehen**, bis eine neue Zeile sie ersetzt
  oder `serial_reset()` sie löscht. Eine Abfrage-Schleife liest bei einer
  ungültigen Eingabe (z.B. Slot `4`) denselben Wert also immer wieder, bis
  etwas Neues kommt. So war es auch vorher.
* **USB (`HAL_PCD_MODULE_ENABLED`):** Der USB-Empfang läuft über
  `urx_buffer` und ist vom Zeilenmodus nicht betroffen.
* **Nebenläufigkeit:** `rx_line`/`rx_line_len` werden nur im Callback
  geschrieben, ausser in `serial_set_line_mode()` (unter gesperrten
  Interrupts) und in `serial_reset()`. Setzt `serial_reset()` `rx_line_len`
  genau während eines Empfangs-Ereignisses zurück, geht höchstens die
  angefangene Zeile verloren. Das ist dort ohnehin beabsichtigt.

---

## 7. Geprüft

* Firmware-Build mit allen Änderungen ohne Fehler (STM32CubeCLT 1.21).
* Auf einem Board mit leerem EEPROM (Slot 9) wurden alle vier Boot-Abfragen
  zeichenweise mit **200 ms Pause** zwischen den Zeichen beantwortet, wie
  beim Tippen von Hand. `9`, `4`, `2` und der mehrstellige Offset `-27164`
  kamen vollständig an. Die anschliessende EEPROM-Ausgabe (`p`) zeigte
  `my_slot = 9`, `my_channel = 4`, `offset = -27164 Hz`.
* `set_offset.py` und der Kalibrier-Tab des Viewers, die Wert und `\r` in
  einem Stück senden, funktionieren unverändert. Damit wurden danach drei
  Boards neu eingerichtet und kalibriert.
* Nicht getestet: der USB-Zweig (`HAL_PCD_MODULE_ENABLED`), weil dieses
  Projekt ohne USB-Treiber gebaut wird.
