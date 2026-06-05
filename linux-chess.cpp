#include <SFML/Graphics.hpp>
#include <iostream>
#include <cmath>
#include <cstdint> // Necessario per std::uint8_t
#include <map>
#include <vector>
#include <functional>
#include <algorithm>
#include <random>
#include <deque>

int main()
{
    // Crea la finestra in modalità fullscreen usando la risoluzione nativa del desktop
    sf::RenderWindow window(sf::VideoMode::getDesktopMode(), "Chess Board", sf::State::Fullscreen);
    window.setFramerateLimit(60);
    
    // Colori della scacchiera
    sf::Color lightColor(240, 217, 181);  // Beige chiaro
    sf::Color darkColor(181, 136, 99);    // Marrone scuro

    // Rappresentazione della scacchiera (0 = vuoto, positivo = bianco, negativo = nero)
    // 1: Pedone, 2: Cavallo, 3: Alfiere, 4: Torre, 5: Regina, 6: Re
    int board[8][8] = {
        {-4, -2, -3, -5, -6, -3, -2, -4},
        {-1, -1, -1, -1, -1, -1, -1, -1},
        { 0,  0,  0,  0,  0,  0,  0,  0},
        { 0,  0,  0,  0,  0,  0,  0,  0},
        { 0,  0,  0,  0,  0,  0,  0,  0},
        { 0,  0,  0,  0,  0,  0,  0,  0},
        { 1,  1,  1,  1,  1,  1,  1,  1},
        { 4,  2,  3,  5,  6,  3,  2,  4}
    };

    // Caricamento delle texture (Assicurati di avere i file .png nella cartella corretta)
    std::map<int, sf::Texture> textures;
    std::map<int, std::string> fileMap = {
        {1, "pawn-w.png"},   {-1, "pawn-b.png"},
        {2, "knight-w.png"}, {-2, "knight-b.png"},
        {3, "bishop-w.png"}, {-3, "bishop-b.png"},
        {4, "rook-w.png"},   {-4, "rook-b.png"},
        {5, "queen-w.png"},  {-5, "queen-b.png"},
        {6, "king-w.png"},   {-6, "king-b.png"}
    };

    for (auto const& [id, path] : fileMap) {
        if (!textures[id].loadFromFile(path)) {
            std::cerr << "Errore: impossibile caricare " << path << std::endl;
        }
    }
    
    // Stato del gioco
    int currentTurn = 1; // 1 = Bianco, -1 = Nero
    bool isGameOver = false;
    std::string gameOverText = "";

    // Assegna in modo casuale il bianco (1) o il nero (-1) al giocatore
    std::mt19937 gen_side(std::random_device{}());
    int playerSide = (std::uniform_int_distribution<>(0, 1)(gen_side) == 0) ? 1 : -1;

    // Flag per Arrocco ed En Passant
    bool whiteKingMoved = false, blackKingMoved = false;
    bool whiteRook0Moved = false, whiteRook7Moved = false;
    bool blackRook0Moved = false, blackRook7Moved = false;
    int enPassantCol = -1; // Colonna del pedone che ha appena mosso di 2

    // Caricamento Font per il testo a schermo
    sf::Font font;
    bool fontLoaded = font.openFromFile("arial.ttf"); // In SFML 3 si usa openFromFile
    sf::Text uiText(font); // In SFML 3 la font è obbligatoria nel costruttore
    if (fontLoaded) {
        uiText.setCharacterSize(60);
        uiText.setFillColor(sf::Color::White);
        uiText.setOutlineColor(sf::Color::Black);
        uiText.setOutlineThickness(3);
    }

    // Variabili per il Drag and Drop
    bool isDragging = false;
    int dragRow = -1, dragCol = -1;
    sf::Vector2f mousePos;
    sf::Clock animationClock;
    sf::Clock aiClock; // Per dare un leggero ritardo alla mossa dell'AI (se attiva)
    bool aiThinking = (playerSide == -1); // L'AI pensa subito se il giocatore è Nero

    // Vettori per i pezzi catturati
    std::vector<int> whiteCapturedPieces; // Pezzi bianchi catturati (da neri)
    std::vector<int> blackCapturedPieces; // Pezzi neri catturati (da bianchi)

    // Variabili per l'animazione del pezzo AI
    bool isAiAnimating = false;
    int aiAnimFR = -1, aiAnimFC = -1, aiAnimTR = -1, aiAnimTC = -1;
    const float aiAnimDuration = 0.6f; // Durata del movimento (più alto = più lento)

    std::deque<sf::Vector2f> dragTrail;
    const size_t maxTrailSize = 6;

    // Funzione per trovare il Re di un certo colore
    auto findKing = [&](int color) -> sf::Vector2i {
        for (int r = 0; r < 8; r++)
            for (int c = 0; c < 8; c++)
                if (board[r][c] == color * 6) return sf::Vector2i(c, r);
        return sf::Vector2i(-1, -1);
    };

    // Funzione per verificare se una casella è sotto attacco da parte di un colore
    auto isUnderAttack = [&](int r, int c, int attackerSide) -> bool {
        // Cavallo
        int drN[] = {2, 2, 1, 1, -2, -2, -1, -1}, dcN[] = {1, -1, 2, -2, 1, -1, 2, -2};
        for (int i = 0; i < 8; i++) {
            int nr = r + drN[i], nc = c + dcN[i];
            if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8 && board[nr][nc] == attackerSide * 2) return true;
        }
        // Re (per evitare che i re si avvicinino troppo)
        for (int i = -1; i <= 1; i++)
            for (int j = -1; j <= 1; j++) {
                int nr = r + i, nc = c + j;
                if (nr >= 0 && nr < 8 && nc >= 0 && nc < 8 && board[nr][nc] == attackerSide * 6) return true;
            }
        // Pedone
        int pDir = (attackerSide > 0) ? 1 : -1;
        if (r + pDir >= 0 && r + pDir < 8) {
            if (c - 1 >= 0 && board[r + pDir][c - 1] == attackerSide * 1) return true;
            if (c + 1 < 8 && board[r + pDir][c + 1] == attackerSide * 1) return true;
        }
        // Sliders (Torre, Alfiere, Regina)
        int dir[8][2] = {{1,0},{-1,0},{0,1},{0,-1},{1,1},{1,-1},{-1,1},{-1,-1}};
        for (int i = 0; i < 8; i++) {
            int nr = r + dir[i][0], nc = c + dir[i][1];
            while (nr >= 0 && nr < 8 && nc >= 0 && nc < 8) {
                int p = board[nr][nc];
                if (p != 0) {
                    if (p == attackerSide * 5) return true; // Regina
                    if (i < 4 && p == attackerSide * 4) return true; // Torre
                    if (i >= 4 && p == attackerSide * 3) return true; // Alfiere
                    break;
                }
                nr += dir[i][0]; nc += dir[i][1];
            }
        }
        return false;
    };

    // Funzione per validare le regole dei pezzi (senza controllare lo scacco)
    auto checkPieceRules = [&](int fR, int fC, int tR, int tC) -> bool {
        int piece = board[fR][fC];
        int target = board[tR][tC];
        if (fR == tR && fC == tC) return false;
        if (target != 0 && (target * piece > 0)) return false; // Non può mangiare i propri

        int dr = std::abs(tR - fR);
        int dc = std::abs(tC - fC);

        // Pedone
        if (std::abs(piece) == 1) {
            int dir = (piece > 0) ? -1 : 1;
            int startRow = (piece > 0) ? 6 : 1;
            if (tC == fC && tR == fR + dir && target == 0) return true;
            if (tC == fC && tR == fR + 2 * dir && fR == startRow && target == 0 && board[fR + dir][fC] == 0) return true;
            if (std::abs(tC - fC) == 1 && tR == fR + dir && target != 0) return true;
            
            // En Passant
            if (std::abs(tC - fC) == 1 && tR == fR + dir && target == 0 && tC == enPassantCol) {
                int epRow = (piece > 0) ? 3 : 4;
                if (fR == epRow) return true;
            }
            return false;
        }
        // Cavallo
        if (std::abs(piece) == 2) return (dr == 2 && dc == 1) || (dr == 1 && dc == 2);
        // Alfiere
        if (std::abs(piece) == 3 || std::abs(piece) == 5) {
            if (dr == dc) {
                int rs = (tR > fR) ? 1 : -1;
                int cs = (tC > fC) ? 1 : -1;
                for (int i = 1; i < dr; ++i) if (board[fR + i * rs][fC + i * cs] != 0) return false;
                return true;
            } else if (std::abs(piece) == 3) return false;
        }
        // Torre
        if (std::abs(piece) == 4 || std::abs(piece) == 5) {
            if (fR == tR || fC == tC) {
                int rs = (tR == fR) ? 0 : (tR > fR ? 1 : -1);
                int cs = (tC == fC) ? 0 : (tC > fC ? 1 : -1);
                int steps = std::max(dr, dc);
                for (int i = 1; i < steps; ++i) if (board[fR + i * rs][fC + i * cs] != 0) return false;
                return true;
            }
        }
        // Re
        if (std::abs(piece) == 6) {
            if (dr <= 1 && dc <= 1) return true;
            
            // Arrocco (Movimento orizzontale di 2 caselle)
            if (dr == 0 && dc == 2) {
                if (piece > 0 && whiteKingMoved) return false;
                if (piece < 0 && blackKingMoved) return false;
                
                if (piece > 0) { // Bianco
                    if (tC == 6 && !whiteRook7Moved && board[7][5] == 0 && board[7][6] == 0) return true; // Corto
                    if (tC == 2 && !whiteRook0Moved && board[7][1] == 0 && board[7][2] == 0 && board[7][3] == 0) return true; // Lungo
                } else { // Nero
                    if (tC == 6 && !blackRook7Moved && board[0][5] == 0 && board[0][6] == 0) return true; // Corto
                    if (tC == 2 && !blackRook0Moved && board[0][1] == 0 && board[0][2] == 0 && board[0][3] == 0) return true; // Lungo
                }
            }
        }
        return false;
    };

    // Funzione per verificare se una mossa è legale (regole + sicurezza del Re)
    auto isMoveLegal = [&](int fR, int fC, int tR, int tC) -> bool {
        if (!checkPieceRules(fR, fC, tR, tC)) return false;
        
        int piece = board[fR][fC];
        int target = board[tR][tC];
        int side = (piece > 0) ? 1 : -1;

        // Controllo speciale Arrocco: il Re non deve essere sotto scacco e non deve passare per caselle attaccate
        if (std::abs(piece) == 6 && std::abs(tC - fC) == 2) {
            if (isUnderAttack(fR, fC, -side)) return false; // Casella iniziale
            int stepC = (tC > fC) ? 1 : -1;
            if (isUnderAttack(fR, fC + stepC, -side)) return false; // Casella di passaggio
        }

        // Simula
        board[tR][tC] = piece;
        board[fR][fC] = 0;
        sf::Vector2i k = findKing(side);
        bool safe = !isUnderAttack(k.y, k.x, -side);
        
        // Ripristina
        board[fR][fC] = piece;
        board[tR][tC] = target;
        return safe;
    };

    // Funzione per controllare se il giocatore corrente ha mosse legali
    auto hasLegalMoves = [&](int side) -> bool {
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (board[r][c] * side > 0) {
                    for (int tr = 0; tr < 8; tr++) {
                        for (int tc = 0; tc < 8; tc++) {
                            if (isMoveLegal(r, c, tr, tc)) return true;
                        }
                    }
                }
            }
        }
        return false;
    };

    // Valutazione della scacchiera per Minimax
    auto evaluateBoard = [&]() -> int {
        int score = 0;
        int pieceValues[] = {0, 10, 30, 30, 50, 90, 900};
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                int p = board[r][c];
                if (p > 0) score += pieceValues[p];
                else if (p < 0) score -= pieceValues[-p];
            }
        }
        return score;
    };

    // Ricerca di Quiete per evitare l'effetto orizzonte
    std::function<int(int, int, bool)> quiesce = [&](int alpha, int beta, bool isMaximizing) -> int {
        int standPat = evaluateBoard();
        if (isMaximizing) {
            if (standPat >= beta) return beta;
            if (alpha < standPat) alpha = standPat;
        } else {
            if (standPat <= alpha) return alpha;
            if (beta > standPat) beta = standPat;
        }

        int side = isMaximizing ? 1 : -1;
        for (int r = 0; r < 8; r++) {
            for (int c = 0; c < 8; c++) {
                if (board[r][c] * side > 0) {
                    for (int tr = 0; tr < 8; tr++) {
                        for (int tc = 0; tc < 8; tc++) {
                            if (board[tr][tc] != 0 && isMoveLegal(r, c, tr, tc)) { // Solo catture
                                int saved = board[tr][tc];
                                board[tr][tc] = board[r][c]; board[r][c] = 0;
                                int score = quiesce(alpha, beta, !isMaximizing);
                                board[r][c] = board[tr][tc]; board[tr][tc] = saved;
                                if (isMaximizing) {
                                    if (score >= beta) return beta;
                                    if (score > alpha) alpha = score;
                                } else {
                                    if (score <= alpha) return alpha;
                                    if (score < beta) beta = score;
                                }
                            }
                        }
                    }
                }
            }
        }
        return isMaximizing ? alpha : beta;
    };

    // Algoritmo Minimax con Alpha-Beta Pruning
    std::function<int(int, int, int, bool)> minimax = [&](int depth, int alpha, int beta, bool isMaximizing) -> int {
        if (depth == 0) return quiesce(alpha, beta, isMaximizing);

        int side = isMaximizing ? 1 : -1;
        if (!hasLegalMoves(side)) {
            sf::Vector2i k = findKing(side);
            if (isUnderAttack(k.y, k.x, -side)) return isMaximizing ? -10000 : 10000;
            return 0;
        }

        if (isMaximizing) {
            int maxEval = -20000;
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    if (board[r][c] > 0) {
                        for (int tr = 0; tr < 8; tr++) {
                            for (int tc = 0; tc < 8; tc++) {
                                if (isMoveLegal(r, c, tr, tc)) {
                                    int saved = board[tr][tc];
                                    board[tr][tc] = board[r][c]; board[r][c] = 0;
                                    int eval = minimax(depth - 1, alpha, beta, false);
                                    board[r][c] = board[tr][tc]; board[tr][tc] = saved;
                                    maxEval = std::max(maxEval, eval);
                                    alpha = std::max(alpha, eval);
                                    if (beta <= alpha) return maxEval;
                                }
                            }
                        }
                    }
                }
            }
            return maxEval;
        } else {
            int minEval = 20000;
            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    if (board[r][c] < 0) {
                        for (int tr = 0; tr < 8; tr++) {
                            for (int tc = 0; tc < 8; tc++) {
                                if (isMoveLegal(r, c, tr, tc)) {
                                    int saved = board[tr][tc];
                                    board[tr][tc] = board[r][c]; board[r][c] = 0;
                                    int eval = minimax(depth - 1, alpha, beta, true);
                                    board[r][c] = board[tr][tc]; board[tr][tc] = saved;
                                    minEval = std::min(minEval, eval);
                                    beta = std::min(beta, eval);
                                    if (beta <= alpha) return minEval;
                                }
                            }
                        }
                    }
                }
            }
            return minEval;
        }
    };

    // Funzione per eseguire fisicamente la mossa e aggiornare lo stato del gioco
    auto applyMove = [&](int fR, int fC, int tR, int tC) {
        int piece = board[fR][fC];
        
        // Gestione pezzi catturati
        int capturedPiece = board[tR][tC];
        if (capturedPiece != 0) {
            if (capturedPiece > 0) { // Pezzo bianco catturato
                blackCapturedPieces.push_back(capturedPiece);
            } else { // Pezzo nero catturato
                whiteCapturedPieces.push_back(std::abs(capturedPiece));
            }
        }
        // Gestione En Passant
        if (std::abs(piece) == 1 && tC == enPassantCol && board[tR][tC] == 0 && tC != fC) {
            board[fR][tC] = 0;
        }

        // Gestione Arrocco
        if (std::abs(piece) == 6 && std::abs(tC - fC) == 2) {
            if (tC == 6) { board[tR][5] = board[tR][7]; board[tR][7] = 0; }
            else if (tC == 2) { board[tR][3] = board[tR][0]; board[tR][0] = 0; }
        }

        // Aggiorna flag per Arrocco
        if (piece == 6) whiteKingMoved = true;
        if (piece == -6) blackKingMoved = true;
        if (fR == 7 && fC == 0) whiteRook0Moved = true;
        if (fR == 7 && fC == 7) whiteRook7Moved = true;
        if (fR == 0 && fC == 0) blackRook0Moved = true;
        if (fR == 0 && fC == 7) blackRook7Moved = true;

        // Aggiorna flag En Passant
        if (std::abs(piece) == 1 && std::abs(tR - fR) == 2) enPassantCol = tC;
        else enPassantCol = -1;

        // Gestione Promozione (Auto-Regina)
        if (std::abs(piece) == 1 && (tR == 0 || tR == 7)) {
            board[tR][tC] = (piece > 0) ? 5 : -5;
        } else {
            board[tR][tC] = piece;
        }

        board[fR][fC] = 0;
        currentTurn *= -1;
        aiThinking = (currentTurn == -playerSide); // Attiva l'AI se è il turno dell'avversario
        aiClock.restart();

        // Controlla fine partita
        if (!hasLegalMoves(currentTurn)) {
            isGameOver = true;
            sf::Vector2i k = findKing(currentTurn);
            if (isUnderAttack(k.y, k.x, -currentTurn)) {
                if (currentTurn == playerSide) { // Il Re del giocatore è sotto scacco matto
                    gameOverText = (playerSide == 1) ? "Scacco Matto! Vince l'AI" : "Scacco Matto! Hai Vinto!";
                } else { // Il Re dell'AI è sotto scacco matto
                    gameOverText = (playerSide == 1) ? "Scacco Matto! Hai Vinto!" : "Scacco Matto! Vince l'AI";
                }
            } else {
                gameOverText = "Stallo! Pareggio";
            }
            std::cout << gameOverText << std::endl;
        }
    };

    // Loop principale
    while (window.isOpen())
    {
        // Calcola la dimensione della scacchiera basata sulla finestra attuale
        // Spostato qui per essere accessibile anche nella gestione degli eventi
        sf::Vector2u currentSize = window.getSize();
        unsigned int squareSize = std::min(currentSize.x, currentSize.y) / 8;
        unsigned int boardWidth = squareSize * 8;
        unsigned int offsetX = (currentSize.x - boardWidth) / 2;
        unsigned int offsetY = (currentSize.y - boardWidth) / 2;

        while (auto event = window.pollEvent())
        {
            if (event->getIf<sf::Event::Closed>())
            {
                window.close();
            }
            else if (const auto* keyEvent = event->getIf<sf::Event::KeyPressed>())
            {
                if (keyEvent->code == sf::Keyboard::Key::Escape)
                    window.close();
            }
            else if (const auto* resizeEvent = event->getIf<sf::Event::Resized>())
            {
                // Aggiorna la vista della finestra per mantenere il rapporto 1:1 con i pixel
                // Questo evita che la scacchiera appaia deformata dopo il ridimensionamento.
                sf::FloatRect visibleArea({0.f, 0.f}, sf::Vector2f(resizeEvent->size));
                window.setView(sf::View(visibleArea));
            }
            else if (const auto* mouseButtonEvent = event->getIf<sf::Event::MouseButtonPressed>())
            {
                if (mouseButtonEvent->button == sf::Mouse::Button::Left)
                {
                    sf::Vector2i pos = mouseButtonEvent->position;
                    int col = (pos.x - (int)offsetX) / (int)squareSize; // Colonna relativa alla scacchiera
                    int row = (pos.y - (int)offsetY) / (int)squareSize; // Riga relativa alla scacchiera

                    // Inverti le coordinate se il giocatore è Nero
                    if (playerSide == -1) {
                        row = 7 - row;
                        col = 7 - col;
                    }

                    // Verifica che sia il turno del pezzo selezionato
                    if (!isGameOver && row >= 0 && row < 8 && col >= 0 && col < 8 && board[row][col] != 0 && (board[row][col] * currentTurn > 0))
                    {
                        isDragging = true;
                        dragRow = row;
                        dragCol = col;
                    }
                }
            }
            else if (const auto* mouseButtonEvent = event->getIf<sf::Event::MouseButtonReleased>())
            {
                if (mouseButtonEvent->button == sf::Mouse::Button::Left && isDragging)
                {
                    sf::Vector2i pos = mouseButtonEvent->position;
                    int col = (pos.x - (int)offsetX) / (int)squareSize; // Colonna relativa alla scacchiera
                    int row = (pos.y - (int)offsetY) / (int)squareSize; // Riga relativa alla scacchiera

                    // Inverti le coordinate se il giocatore è Nero
                    if (playerSide == -1) {
                        row = 7 - row;
                        col = 7 - col;
                    }

                    if (row >= 0 && row < 8 && col >= 0 && col < 8)
                    {
                        if (isMoveLegal(dragRow, dragCol, row, col)) 
                        {
                            applyMove(dragRow, dragCol, row, col);
                        }
                    }
                    isDragging = false;
                    dragRow = -1;
                    dragCol = -1;
                }
            }
            else if (const auto* mouseMoveEvent = event->getIf<sf::Event::MouseMoved>())
            {
                mousePos = sf::Vector2f(mouseMoveEvent->position);
            }
        }
        
        // Aggiorna la scia del pezzo trascinato
        if (isDragging) {
            dragTrail.push_front(mousePos);
            if (dragTrail.size() > maxTrailSize) dragTrail.pop_back();
        } else {
            dragTrail.clear();
        }

        // Logica Giocatore Automatico (AI)
        if (!isGameOver && currentTurn == -playerSide && !isAiAnimating && aiClock.getElapsedTime().asSeconds() > 0.8f) {
            struct AIMove { int fR, fC, tR, tC; };
            std::vector<AIMove> legalMoves;

            for (int r = 0; r < 8; r++) {
                for (int c = 0; c < 8; c++) {
                    if (board[r][c] * currentTurn > 0) { 
                        for (int tr = 0; tr < 8; tr++) {
                            for (int tc = 0; tc < 8; tc++) {
                                if (isMoveLegal(r, c, tr, tc)) legalMoves.push_back({r, c, tr, tc});
                            }
                        }
                    }
                }
            }

            if (!legalMoves.empty()) {
                int aiSide = currentTurn;
                int bestEval = (aiSide == 1) ? -20000 : 20000;
                std::vector<AIMove> bestMoves;

                for (auto& m : legalMoves) {
                    int saved = board[m.tR][m.tC];
                    board[m.tR][m.tC] = board[m.fR][m.fC]; board[m.fR][m.fC] = 0;
                    int eval = minimax(2, -20000, 20000, aiSide == -1); 
                    board[m.fR][m.fC] = board[m.tR][m.tC]; board[m.tR][m.tC] = saved;

                    if (aiSide == 1) {
                        if (eval > bestEval) { bestEval = eval; bestMoves = {m}; }
                        else if (eval == bestEval) { bestMoves.push_back(m); }
                    } else {
                        if (eval < bestEval) { bestEval = eval; bestMoves = {m}; }
                        else if (eval == bestEval) { bestMoves.push_back(m); }
                    }
                }
                
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, bestMoves.size() - 1);
                AIMove best = bestMoves[dis(gen)];
                
                // Avvia l'animazione invece di teletrasportare il pezzo
                isAiAnimating = true;
                aiAnimFR = best.fR; aiAnimFC = best.fC;
                aiAnimTR = best.tR; aiAnimTC = best.tC;
                aiClock.restart(); // Usiamo lo stesso orologio per gestire il progresso
            }
        }

        // Pulisci lo schermo con colore nero
        window.clear(sf::Color::Black);
        
        // Disegna la scacchiera
        for (int r = 0; r < 8; r++)
        {
            for (int c = 0; c < 8; c++)
            {
                int displayRow = (playerSide == 1) ? r : 7 - r;
                int displayCol = (playerSide == 1) ? c : 7 - c;

                // Determina il colore della casella (bianco se (row + col) è pari)
                sf::Color squareColor = ((r + c) % 2 == 0) ? lightColor : darkColor;
                
                // Crea il rettangolo per la casella
                sf::RectangleShape square(sf::Vector2f(squareSize, squareSize));
                square.setPosition(sf::Vector2f(offsetX + displayCol * squareSize, offsetY + displayRow * squareSize));
                square.setFillColor(squareColor);
                
                // Disegna la casella
                window.draw(square);

                // Se è un Re, controlla se deve essere evidenziato in rosso (scacco)
                int pieceValue = board[r][c];
                if (std::abs(pieceValue) == 6 && board[r][c] != 0) { // Controlla il Re nella posizione reale della board
                    int side = (pieceValue > 0) ? 1 : -1;
                    if (isUnderAttack(r, c, -side)) {
                        sf::RectangleShape checkHighlight(sf::Vector2f(squareSize, squareSize));
                        checkHighlight.setPosition(sf::Vector2f(offsetX + displayCol * squareSize, offsetY + displayRow * squareSize));
                        
                        std::uint8_t alpha = 150; // Usare std::uint8_t invece di sf::Uint8
                        // Se il gioco è finito per scacco matto, il Re lampeggia
                        if (isGameOver && gameOverText.find("Scacco Matto") != std::string::npos) {
                            float t = animationClock.getElapsedTime().asSeconds();
                            alpha = static_cast<std::uint8_t>(127 + 127 * std::sin(t * 15.0f));
                        }
                        
                        checkHighlight.setFillColor(sf::Color(255, 0, 0, alpha)); 
                        window.draw(checkHighlight);
                    }
                }

                // Disegna il pezzo se presente
                if (board[r][c] != 0 && textures.count(board[r][c]))
                {
                    // Se stiamo trascinando questo pezzo, non disegnarlo nella casella
                    if (isDragging && r == dragRow && c == dragCol)
                        continue;

                    // Non disegnare il pezzo nella casella se l'AI lo sta animando
                    if (isAiAnimating && r == aiAnimFR && c == aiAnimFC)
                        continue;

                    sf::Sprite sprite(textures[board[r][c]]);
                    
                    // Calcola il fattore di scala per adattare il pezzo alla dimensione della casella
                    sf::Vector2u texSize = textures[board[r][c]].getSize();
                    float scaleX = (float)squareSize / texSize.x;
                    float scaleY = (float)squareSize / texSize.y;
                    
                    sprite.setScale(sf::Vector2f(scaleX, scaleY)); // Usa displayRow/Col per la posizione
                    sprite.setPosition(sf::Vector2f(offsetX + displayCol * squareSize, offsetY + displayRow * squareSize));
                    
                    window.draw(sprite);
                }
            }
        }

        // Disegna il pezzo che viene trascinato sopra tutto il resto
        if (isDragging && board[dragRow][dragCol] != 0)
        {
            int pieceValue = board[dragRow][dragCol];
            sf::Sprite dragSprite(textures[pieceValue]);
            
            sf::Vector2u texSize = textures[pieceValue].getSize();
            float scale = (float)squareSize / texSize.x;
            dragSprite.setScale(sf::Vector2f(scale, scale));
            sf::Vector2f offset(squareSize / 2.0f, squareSize / 2.0f);

            // 1. Disegna l'OMBRA (leggermente spostata e nera trasparente)
            sf::Sprite shadowSprite = dragSprite;
            shadowSprite.setColor(sf::Color(0, 0, 0, 100));
            shadowSprite.setPosition(mousePos - offset + sf::Vector2f(squareSize * 0.1f, squareSize * 0.1f));
            window.draw(shadowSprite);

            // 2. Disegna la SCIA (ghosting delle posizioni precedenti)
            for (size_t i = 1; i < dragTrail.size(); ++i) {
                sf::Sprite trailSprite = dragSprite;
                // Opacità decrescente: più è vecchia la posizione, più è trasparente
                std::uint8_t alpha = static_cast<std::uint8_t>(100 / (i + 1)); // Usare std::uint8_t
                trailSprite.setColor(sf::Color(255, 255, 255, alpha));
                trailSprite.setPosition(dragTrail[i] - offset);
                window.draw(trailSprite);
            }

            // 3. Disegna il PEZZO REALE (leggermente più grande per l'effetto "sollevato")
            dragSprite.setScale(sf::Vector2f(scale * 1.1f, scale * 1.1f));
            sf::Vector2f raisedOffset(squareSize * 1.1f / 2.0f, squareSize * 1.1f / 2.0f);
            dragSprite.setPosition(mousePos - raisedOffset);
            window.draw(dragSprite);
        }
        
        // Disegna i pezzi catturati a destra della scacchiera
        unsigned int capturedAreaStartX = offsetX + boardWidth + squareSize / 2;
        unsigned int capturedPieceSize = squareSize / 2;
        unsigned int capturedPiecePadding = 10;

        // Disegna uno sfondo marrone per i pezzi catturati (migliora la visibilità dei pezzi neri)
        sf::RectangleShape capturedBg(sf::Vector2f(static_cast<float>(capturedPieceSize * 2 + capturedPiecePadding + 20), static_cast<float>(boardWidth)));
        capturedBg.setPosition(sf::Vector2f(static_cast<float>(capturedAreaStartX - 10), static_cast<float>(offsetY)));
        capturedBg.setFillColor(sf::Color(60, 40, 30)); // Marrone scuro per contrasto
        window.draw(capturedBg);

        unsigned int currentY_playerCaptured = offsetY;
        unsigned int currentY_opponentCaptured = offsetY;

        unsigned int playerCapturedColX = capturedAreaStartX;
        unsigned int opponentCapturedColX = capturedAreaStartX + capturedPieceSize + capturedPiecePadding;

        // Disegna i pezzi catturati dal giocatore (pezzi dell'avversario)
        // Se il giocatore è Bianco (playerSide == 1), ha catturato pezzi Neri (blackCapturedPieces)
        // Se il giocatore è Nero (playerSide == -1), ha catturato pezzi Bianchi (whiteCapturedPieces)
        std::vector<int>& playerCapturedPiecesList = (playerSide == 1) ? blackCapturedPieces : whiteCapturedPieces;
        // Il segno per la texture dipende dal colore del pezzo catturato.
        // Se il giocatore è Bianco, i pezzi catturati sono Neri (ID negativo).
        // Se il giocatore è Nero, i pezzi catturati sono Bianchi (ID positivo).
        int textureSignForPlayerCaptured = (playerSide == 1) ? -1 : 1;

        for (int p_type : playerCapturedPiecesList) {
            // Assicurati che la texture esista prima di tentare di disegnarla
            if (textures.count(p_type * textureSignForPlayerCaptured)) {
                sf::Sprite sprite(textures[p_type * textureSignForPlayerCaptured]);
                sf::Vector2u texSize = textures[p_type * textureSignForPlayerCaptured].getSize();
                float scale = (float)capturedPieceSize / texSize.x;
                sprite.setScale(sf::Vector2f(scale, scale));
                sprite.setPosition(sf::Vector2f(static_cast<float>(playerCapturedColX), static_cast<float>(currentY_playerCaptured)));
                window.draw(sprite);
                currentY_playerCaptured += capturedPieceSize;
            }
        }

        // Disegna i pezzi catturati dall'avversario (pezzi del giocatore)
        // Se il giocatore è Bianco (playerSide == 1), l'avversario ha catturato pezzi Bianchi (whiteCapturedPieces)
        // Se il giocatore è Nero (playerSide == -1), l'avversario ha catturato pezzi Neri (blackCapturedPieces)
        std::vector<int>& opponentCapturedPiecesList = (playerSide == 1) ? whiteCapturedPieces : blackCapturedPieces;
        // Il segno per la texture dipende dal colore del pezzo catturato.
        // Se il giocatore è Bianco, l'avversario ha catturato pezzi Bianchi (ID positivo).
        // Se il giocatore è Nero, l'avversario ha catturato pezzi Neri (ID negativo).
        int textureSignForOpponentCaptured = (playerSide == 1) ? 1 : -1;

        for (int p_type : opponentCapturedPiecesList) {
            if (textures.count(p_type * textureSignForOpponentCaptured)) {
                sf::Sprite sprite(textures[p_type * textureSignForOpponentCaptured]);
                sf::Vector2u texSize = textures[p_type * textureSignForOpponentCaptured].getSize();
                float scale = (float)capturedPieceSize / texSize.x;
                sprite.setScale(sf::Vector2f(scale, scale));
                sprite.setPosition(sf::Vector2f(static_cast<float>(opponentCapturedColX), static_cast<float>(currentY_opponentCaptured)));
                window.draw(sprite);
                currentY_opponentCaptured += capturedPieceSize;
            }
        }

        // Logica e Rendering Animazione AI
        if (isAiAnimating) {
            float t = aiClock.getElapsedTime().asSeconds();
            float progress = std::min(t / aiAnimDuration, 1.0f);

            // Helper per ottenere il centro di una casella in coordinate schermo
            auto getSquareCenter = [&](int r_idx, int c_idx) -> sf::Vector2f {
                int dR = (playerSide == 1) ? r_idx : 7 - r_idx;
                int dC = (playerSide == 1) ? c_idx : 7 - c_idx;
                return sf::Vector2f(offsetX + dC * squareSize + squareSize / 2.0f, 
                                    offsetY + dR * squareSize + squareSize / 2.0f);
            };

            sf::Vector2f startP = getSquareCenter(aiAnimFR, aiAnimFC);
            sf::Vector2f endP = getSquareCenter(aiAnimTR, aiAnimTC);
            sf::Vector2f currentP = startP + (endP - startP) * progress;

            int pieceValue = board[aiAnimFR][aiAnimFC];
            sf::Sprite aiSprite(textures[pieceValue]);
            sf::Vector2u texSize = textures[pieceValue].getSize();
            float scale = (float)squareSize / texSize.x;
            aiSprite.setScale(sf::Vector2f(scale, scale));
            sf::Vector2f halfSquare(squareSize / 2.0f, squareSize / 2.0f);
            
            // Disegna l'ombra per dare profondità al movimento
            sf::Sprite shadowSprite = aiSprite;
            shadowSprite.setColor(sf::Color(0, 0, 0, 100));
            shadowSprite.setPosition(currentP - halfSquare + sf::Vector2f(squareSize * 0.1f, squareSize * 0.1f));
            window.draw(shadowSprite);

            aiSprite.setPosition(currentP - halfSquare);
            window.draw(aiSprite);

            if (progress >= 1.0f) {
                applyMove(aiAnimFR, aiAnimFC, aiAnimTR, aiAnimTC);
                isAiAnimating = false;
            }
        }

        // Disegna il testo di Game Over al centro dello schermo
        if (isGameOver && fontLoaded) {
            uiText.setString(gameOverText);
            sf::FloatRect textBounds = uiText.getLocalBounds();
            // Centra l'origine e posiziona
            uiText.setOrigin({textBounds.getCenter().x, textBounds.getCenter().y});
            uiText.setPosition({currentSize.x / 2.0f, currentSize.y / 2.0f});
            window.draw(uiText);
        }

        // Mostra il risultato
        window.display();
    }
    
    return 0;
}
