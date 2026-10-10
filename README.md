# TaskerFace

App per Pebble (Core Devices) che mostra a schermo i messaggi inviati da **Tasker** (Android) tramite intent all'app Pebble/Core Devices.

Il messaggio scorre lentamente verso l'alto in loop. La prima riga è in rosso (sugli orologi a colori).

## Funzionamento

1. Tasker invia l'intent `START` per aprire l'app sull'orologio.
2. Dopo 1-2 secondi invia l'intent `SEND` con il testo del messaggio.
3. L'orologio vibra all'avvio, mostra il messaggio e lo fa scorrere in continuazione.
4. Scuotendo il polso mentre il messaggio è visibile, l'app si chiude.

Senza messaggi l'app mostra la scritta "Wathsapp".

## Requisiti

- Orologio Pebble o Core Devices
- App Android **Pebble / Core Devices** (`coredevices.coreapp`)
- [Tasker](https://tasker.joaoapps.com/)
- [Pebble SDK](https://developer.rebble.io/) per compilare

## Compilazione

```bash
pebble build
pebble install --phone <IP_DEL_TELEFONO>
```

L'app è dichiarata con `"watchface": false` nel `package.json`: va avviata dal menu dell'orologio oppure con l'intent `START`.

## Configurazione di Tasker

Entrambe le azioni sono *Invia Intent* con **Destinazione: Ricevitore di trasmissione** (Broadcast Receiver) e **Pacchetto** `coredevices.coreapp`.

### 1. Avvio dell'app

| Campo | Valore |
|---|---|
| Azione | `com.getpebble.action.app.START` |
| Pacchetto | `coredevices.coreapp` |
| Extra | `uuid:32b58a1d-0a78-408a-a936-373c6f6b6e8c` |

### 2. Pausa

Aggiungi un'azione *Attendi* di 1-2 secondi, altrimenti l'app non è ancora pronta e il messaggio viene rifiutato (NACK).

### 3. Invio del messaggio

| Campo | Valore |
|---|---|
| Azione | `com.getpebble.action.app.SEND` |
| Pacchetto | `coredevices.coreapp` |
| Extra | `uuid:32b58a1d-0a78-408a-a936-373c6f6b6e8c` |
| Extra | `transaction_id:-1` |
| Extra | `msg_data:[{"key":0,"value":"Testo del messaggio","type":"string","length":19}]` |

Note sul `msg_data`:

- Il valore va tra virgolette doppie, altrimenti il JSON non è valido.
- Se il testo contiene `"` o a capo, va fatto l'escape (`\"`, `\n`) prima di inserirlo nel JSON.
- La **prima riga** del messaggio (fino al primo `\n`) viene mostrata in rosso.
- Il testo è limitato a circa 63 caratteri per la prima riga e 63 per il resto.

Per chiudere l'app da Tasker, usa l'azione `com.getpebble.action.app.STOP` con lo stesso `uuid`.

## Risoluzione dei problemi

Controlla il log dell'app Android (`adb logcat`, filtro `coredevices`):

- `Got app start: ...` → la START è arrivata.
- `Got outbound message` → la SEND è arrivata.
- `AppMessageNACK` → l'app non è in esecuzione sull'orologio, o l'inbox non è aperto.
- `JSONException` → `msg_data` non è un JSON valido (virgolette mancanti, variabile Tasker non espansa).
- Nessuna riga `SEND` → controlla che la Destinazione dell'intent sia "Ricevitore di trasmissione".

Il log dell'orologio si vede con `pebble logs`.

## Struttura del progetto

```
src/c/TaskerFace.c   codice dell'app
package.json         configurazione e messageKeys
wscript              script di build
```

## Licenza

Da definire.
