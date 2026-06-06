# Linux Chess

Un simulatore di scacchi completo sviluppato in C++ utilizzando la libreria **SFML 3**. Il gioco include un'intelligenza artificiale basata sull'algoritmo Minimax e diverse funzionalità avanzate di gioco.

## Funzionalità

- **Regole Complete**: Supporto per Arrocco (lungo e corto), cattura *En Passant* e promozione automatica a Regina.
- **IA Integrata**: Motore di gioco basato su algoritmo **Minimax con Alpha-Beta Pruning** e **Quiescence Search** per evitare l'effetto orizzonte durante le catture (profondità di ricerca ottimizzata).
- **Interfaccia Dinamica**: 
  - Sistema di trascinamento dei pezzi (*Drag and Drop*).
  - Effetti visivi avanzati: ombre dinamiche, scie di movimento (ghosting) e Re lampeggiante in caso di scacco.
  - Scacchiera responsiva che rimane quadrata e centrata al ridimensionamento.
  - Pannello laterale per la **visualizzazione dei pezzi catturati** con sfondo a contrasto.
- **Prospettiva Variabile**: Assegnazione casuale del colore all'avvio con rotazione automatica della visuale per il giocatore.
- **Animazioni**: Movimento fluido dei pezzi controllati dall'IA.

## Requisiti

- Compilatore C++ (supporto C++17 o superiore).
- **SFML 3** (Libreria per grafica, finestre e sistema).
- Font `arial.ttf` nella cartella principale per la visualizzazione del testo di fine partita.

## Installazione e Compilazione

Assicurati di avere le librerie di sviluppo SFML installate sul tuo sistema Linux. Puoi compilare il progetto utilizzando `g++`:

```bash
# Compilazione dell'oggetto
g++ -c -Wall linux-chess.cpp -o linux-chess.o

# Link delle librerie SFML
g++ linux-chess.o -o linux-chess -lsfml-graphics -lsfml-window -lsfml-system
```

## Come Giocare

Esegui il file binario generato:

```bash
./linux-chess
```

- **Mouse**: Trascina i pezzi con il tasto sinistro per muoverli.
- **Esc**: Chiude l'applicazione.

## Struttura Asset

Il gioco richiede i seguenti file nella cartella di esecuzione:
- Immagini dei pezzi in formato `.png` (es. `pawn-w.png`, `king-b.png`, ecc.).
- Il file font `arial.ttf`.

## Licenza

Distribuito sotto licenza MIT.