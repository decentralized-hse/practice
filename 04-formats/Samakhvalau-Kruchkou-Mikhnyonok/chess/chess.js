


class SelfReplicatingChess {
    constructor() {
        this.board = null;
        this.currentGame = null;
        this.moves = [];
        this.version = 0;
        this.moveCache = {};
        this.mySide = null;
        this.isSpectator = false;
        this._gameEndShown = false;
        this.selectedSquare = null;
        this.validMoves = [];
        this.currentPlayer = 'white';
        this.castlingRights = {
            white: { kingSide: true, queenSide: true },
            black: { kingSide: true, queenSide: true }
        };
        this.enPassantTarget = null;
        this.halfMoveClock = 0;
        this.fullMoveNumber = 1;


        this.pieceSymbols = {
            'r': '&#9820;',
            'n': '&#9822;',
            'b': '&#9821;',
            'q': '&#9819;',
            'k': '&#9818;',
            'p': '&#9823;',
            'R': '&#9814;',
            'N': '&#9816;',
            'B': '&#9815;',
            'Q': '&#9813;',
            'K': '&#9812;',
            'P': '&#9817;'
        };

        this.init();
    }

    getSessionId() {
        let id = null;
        try {
            id = localStorage.getItem('chess_session_id');
            if (!id) {
                id = 's_' + Math.random().toString(36).slice(2) + '_' + Date.now();
                localStorage.setItem('chess_session_id', id);
            }
        } catch (e) {}
        return id || 's_' + Date.now();
    }

    async assignSide() {
        const sessionId = this.getSessionId();
        const sideKey = 'chess_side_' + this.currentGame;
        try {
            const cached = localStorage.getItem(sideKey);
            if (cached === 'white' || cached === 'black') {
                const slots = await fetch(`/games/${this.currentGame}/slots.json`).then(r => r.ok ? r.json() : { white: null, black: null }).catch(() => ({ white: null, black: null }));
                if ((slots.white === sessionId && cached === 'white') || (slots.black === sessionId && cached === 'black')) {
                    this.mySide = cached;
                    return;
                }
            }
            let slots = await fetch(`/games/${this.currentGame}/slots.json`).then(r => r.ok ? r.json() : { white: null, black: null }).catch(() => ({ white: null, black: null }));
            if (!slots.white && !slots.black) {
                this.mySide = 'white';
                await fetch(`/games/${this.currentGame}/slots.json`, { method: 'PUT', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ white: sessionId, black: null }) });
            } else if (slots.white && !slots.black) {
                this.mySide = 'black';
                await fetch(`/games/${this.currentGame}/slots.json`, { method: 'PUT', headers: { 'Content-Type': 'application/json' }, body: JSON.stringify({ white: slots.white, black: sessionId }) });
            } else {
                if (slots.white === sessionId) this.mySide = 'white';
                else if (slots.black === sessionId) this.mySide = 'black';
                else this.isSpectator = true;
            }
            if (this.mySide) localStorage.setItem(sideKey, this.mySide);
        } catch (e) {
            this.mySide = null;
        }
    }

    async init() {
        const params = new URLSearchParams(window.location.search);
        let gid = params.get('game');
        if (!gid) {
            gid = 'game_' + Date.now();
            window.history.replaceState({}, '', '?' + new URLSearchParams({ game: gid }).toString());
        }
        this.currentGame = gid;

        await this.assignSide();
        await this.loadGame(!!this.mySide);
        this.startPolling();
        this.render();
        this.setupEventListeners();
        this.setupMoveSelector();
        this.updateMoveSelector();
    }

    async discoverMoves() {
        const list = [];
        for (let n = 1; n <= 9999; n++) {
            const name = String(n).padStart(4, '0') + '.json';
            const r = await fetch(`/games/${this.currentGame}/moves/${name}`);
            if (!r.ok) break;
            list.push(name);
        }
        return list;
    }

    storageKey() {
        return 'chess_' + this.currentGame;
    }

    saveGameToStorage() {
        if (this.mySide) return;
        try {
            const data = { version: this.version, moveCache: this.moveCache, moves: this.moves };
            localStorage.setItem(this.storageKey(), JSON.stringify(data));
        } catch (e) {
            console.warn('localStorage save failed', e);
        }
    }

    loadGameFromStorage() {
        try {
            const raw = localStorage.getItem(this.storageKey());
            if (!raw) return null;
            return JSON.parse(raw);
        } catch (e) {
            return null;
        }
    }

    async loadGame(fromServerOnly) {
        try {
            this.moves = await this.discoverMoves();
            const serverVersion = this.moves.length;
            const saved = fromServerOnly ? null : this.loadGameFromStorage();

            if (saved && saved.moveCache && typeof saved.version === 'number') {
                this.moveCache = saved.moveCache;
                if (saved.moves && saved.moves.length > this.moves.length) {
                    this.moves = saved.moves.slice().sort();
                }
                this.version = saved.version;
                if (this.moveCache[this.version]) {
                    this.applyMoveData(this.moveCache[this.version]);
                    this.updateGameUI();
                    this.saveGameToStorage();
                    return;
                }
            }

            let effectiveVersion = serverVersion;
            if (fromServerOnly) {
                try {
                    const state = await fetch(`/games/${this.currentGame}/state.json`).then(r => r.ok ? r.json() : {});
                    if (state.version != null) effectiveVersion = Math.min(serverVersion, Math.max(0, state.version));
                } catch (e) {}
            }
            this.version = effectiveVersion;
            if (this.version > 0) {
                const moveFile = this.moveFilename(this.version);
                const lastMove = await fetch(`/games/${this.currentGame}/moves/${moveFile}`).then(r => r.json());
                this.applyMoveData(lastMove);
                this.moveCache[this.version] = lastMove;
            } else {
                this.board = this.getInitialBoard();
            }
            this.saveGameToStorage();
        } catch (e) {
            console.log('Using initial board:', e);
            const saved = fromServerOnly ? null : this.loadGameFromStorage();
            if (saved && saved.moveCache && saved.version > 0 && saved.moveCache[saved.version]) {
                this.moveCache = saved.moveCache;
                this.moves = saved.moves || [];
                this.version = saved.version;
                this.applyMoveData(saved.moveCache[saved.version]);
            } else {
                this.board = this.getInitialBoard();
            }
            this.saveGameToStorage();
        }
        this.updateGameUI();
        this.maybeShowGameEndOverlay();
    }

    async fetchServerMoveCount() {
        try {
            const r = await fetch(`/games/${this.currentGame}/moves/`);
            if (!r.ok) return this.version;
            const text = await r.text();
            const list = (text.match(/\d{4}\.json/g) || []).sort();
            return list.length;
        } catch (e) {
            return this.version;
        }
    }

    startPolling() {
        if (this._pollTimer) return;
        const interval = this.mySide ? 1000 : 3000;
        this._pollTimer = setInterval(async () => {
            const serverCount = await this.fetchServerMoveCount();
            let stateVersion = null;
            try {
                const state = await fetch(`/games/${this.currentGame}/state.json`).then(r => r.ok ? r.json() : {});
                stateVersion = state.version;
            } catch (e) {}
            const effectiveServer = stateVersion != null ? Math.min(serverCount, stateVersion) : serverCount;
            if (effectiveServer !== this.version || serverCount !== this.moves.length) {
                await this.loadGame(true);
                this.render();
                this.updateMoveSelector();
            }
            this.updateUndoUI();
        }, interval);
    }

    async fetchUndoRequest() {
        try {
            const r = await fetch(`/games/${this.currentGame}/undo_request.json`);
            return r.ok ? await r.json() : {};
        } catch (e) {
            return {};
        }
    }

    updateUndoUI() {
        const wrap = document.getElementById('undoRequestWrap');
        if (!wrap) return;
        this.fetchUndoRequest().then(req => {
            if (req.from && req.atVersion != null && req.from !== this.getSessionId()) {
                wrap.style.display = 'block';
                wrap.innerHTML = 'Opponent requests undo &nbsp;<button onclick="chess.acceptUndo()">Accept</button> <button onclick="chess.declineUndo()">Decline</button>';
            } else {
                wrap.style.display = 'none';
            }
        });
    }

    async requestUndo() {
        if (!this.mySide || this.version <= 0) return;
        try {
            await fetch(`/games/${this.currentGame}/undo_request.json`, {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ from: this.getSessionId(), atVersion: this.version })
            });
            this.showMessage('Undo requested');
        } catch (e) {
            this.showMessage('Request failed');
        }
    }

    async acceptUndo() {
        try {
            const req = await this.fetchUndoRequest();
            if (!req.from || req.atVersion == null) return;
            const newVersion = Math.max(0, req.atVersion - 1);
            await fetch(`/games/${this.currentGame}/state.json`, {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ version: newVersion })
            });
            await fetch(`/games/${this.currentGame}/undo_request.json`, {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({})
            });
            await this.loadGame(true);
            this.render();
            this.updateMoveSelector();
            document.getElementById('undoRequestWrap').style.display = 'none';
            this.showMessage('Undo accepted');
        } catch (e) {
            this.showMessage('Failed');
        }
    }

    async declineUndo() {
        try {
            await fetch(`/games/${this.currentGame}/undo_request.json`, {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({})
            });
            document.getElementById('undoRequestWrap').style.display = 'none';
            this.showMessage('Undo declined');
        } catch (e) {}
    }

    moveFilename(n) {
        return String(n).padStart(4, '0') + '.json';
    }

    applyMoveData(data) {
        this.board = data.board;
        const v = data.version != null ? data.version : this.version;
        this.currentPlayer = data.nextPlayer || (v % 2 === 1 ? 'black' : 'white');
        this.castlingRights = data.castlingRights || { white: { kingSide: true, queenSide: true }, black: { kingSide: true, queenSide: true } };
        this.enPassantTarget = data.enPassantTarget || null;
    }

    async loadVersion(atVersion) {
        this.selectedSquare = null;
        this.validMoves = [];
        if (atVersion <= 0) {
            this.board = this.getInitialBoard();
            this.currentPlayer = 'white';
            this.castlingRights = { white: { kingSide: true, queenSide: true }, black: { kingSide: true, queenSide: true } };
            this.enPassantTarget = null;
            this.version = 0;
            this.saveGameToStorage();
            this.updateGameUI();
            this.render();
            this.updateMoveSelector();
            return;
        }
        if (this.moveCache[atVersion]) {
            this.applyMoveData(this.moveCache[atVersion]);
            this.version = atVersion;
            this.saveGameToStorage();
            this.updateGameUI();
            this.render();
            this.updateMoveSelector();
            return;
        }
        const moveFile = this.moveFilename(atVersion);
        try {
            const data = await fetch(`/games/${this.currentGame}/moves/${moveFile}`).then(r => {
                if (!r.ok) throw new Error('Move not found');
                return r.json();
            });
            this.moveCache[atVersion] = data;
            this.applyMoveData(data);
            this.version = atVersion;
        } catch (e) {
            console.warn('loadVersion failed:', e);
            this.showMessage('Could not load move ' + atVersion);
            return;
        }
        this.saveGameToStorage();
        this.updateGameUI();
        this.render();
        this.updateMoveSelector();
    }

    updateGameUI() {
        document.getElementById('gameId').textContent = this.currentGame;
        document.getElementById('gameVersion').textContent = this.version;
        const statusEl = document.getElementById('gameStatus');
        if (statusEl) {
            if (this.isSpectator) statusEl.textContent = 'Spectator (view only)';
            else if (this.mySide) {
                const turn = this.currentPlayer === this.mySide ? 'Your turn' : 'Waiting for opponent';
                statusEl.textContent = turn;
            } else statusEl.textContent = this.currentPlayer;
        }
        const sideEl = document.getElementById('mySide');
        if (sideEl) {
            if (this.isSpectator) sideEl.textContent = 'Spectator';
            else if (this.mySide) sideEl.textContent = this.mySide === 'white' ? 'White' : 'Black';
            else sideEl.textContent = '';
        }
        const reqUndoWrap = document.getElementById('requestUndoWrap');
        if (reqUndoWrap) {
            reqUndoWrap.style.display = (this.mySide && !this.canIMove() && this.version > 0) ? 'block' : 'none';
        }
        this.updateMoveSelector();
        this.updateUndoUI();
    }

    updateMoveSelector() {
        const sel = document.getElementById('moveSelector');
        const label = document.getElementById('moveSelectorLabel');
        if (!sel) return;
        const maxVer = Math.max(this.version, this.moves.length);
        sel.max = maxVer;
        sel.value = this.version;
        sel.disabled = maxVer <= 0;
        if (label) label.textContent = this.version;
    }

    setupMoveSelector() {
        const sel = document.getElementById('moveSelector');
        if (!sel) return;
        sel.addEventListener('input', () => {
            const v = parseInt(sel.value, 10);
            if (v !== this.version) this.loadVersion(v);
        });
    }

    async undo() {
        if (this.version <= 0) return;
        await this.loadVersion(this.version - 1);
    }

    getInitialBoard() {
        return [
            ['r','n','b','q','k','b','n','r'],
            ['p','p','p','p','p','p','p','p'],
            ['','','','','','','',''],
            ['','','','','','','',''],
            ['','','','','','','',''],
            ['','','','','','','',''],
            ['P','P','P','P','P','P','P','P'],
            ['R','N','B','Q','K','B','N','R']
        ];
    }

    getPieceSymbol(piece) {
        return this.pieceSymbols[piece] || piece;
    }

    render() {
        const boardElement = document.getElementById('chessboard');
        if (!boardElement) return;

        boardElement.innerHTML = '';
        boardElement.style.display = 'grid';
        boardElement.style.gridTemplateColumns = 'repeat(8, 70px)';
        boardElement.style.gridTemplateRows = 'repeat(8, 70px)';
        boardElement.style.border = '3px solid #34495e';

        const displayRows = this.mySide === 'black' ? [7,6,5,4,3,2,1,0] : [0,1,2,3,4,5,6,7];
        const displayCols = this.mySide === 'black' ? [7,6,5,4,3,2,1,0] : [0,1,2,3,4,5,6,7];
        for (let i = 0; i < 8; i++) {
            for (let j = 0; j < 8; j++) {
                const row = displayRows[i];
                const col = displayCols[j];
                const square = document.createElement('div');

                square.style.width = '70px';
                square.style.height = '70px';
                square.style.display = 'flex';
                square.style.alignItems = 'center';
                square.style.justifyContent = 'center';
                square.style.fontSize = '48px';
                square.style.cursor = 'pointer';
                square.style.fontFamily = 'Arial, "Segoe UI", sans-serif';
                square.style.backgroundColor = (row + col) % 2 === 0 ? '#f0d9b5' : '#b58863';

                square.dataset.row = row;
                square.dataset.col = col;

                if (this.selectedSquare && this.selectedSquare.row === row && this.selectedSquare.col === col) {
                    square.style.boxShadow = 'inset 0 0 0 4px #e74c3c';
                }
                if (this.validMoves.some(m => m.row === row && m.col === col)) {
                    square.style.boxShadow = 'inset 0 0 0 4px #2ecc71';
                }

                const piece = this.board[row][col];
                if (piece) square.innerHTML = this.getPieceSymbol(piece);

                boardElement.appendChild(square);
            }
        }

        this.updateGameUI();
        this.maybeShowGameEndOverlay();
    }

    setupEventListeners() {
        const boardElement = document.getElementById('chessboard');
        if (!boardElement) return;

        boardElement.addEventListener('click', (e) => {
            const square = e.target.closest('div[data-row]');
            if (!square) return;

            const row = parseInt(square.dataset.row);
            const col = parseInt(square.dataset.col);
            this.handleSquareClick(row, col);
        });
    }

    isPieceMine(piece) {
        if (!this.mySide || !piece) return false;
        return this.mySide === 'white' ? piece === piece.toUpperCase() : piece === piece.toLowerCase();
    }

    canIMove() {
        if (this.isSpectator || !this.mySide) return false;
        return this.currentPlayer === this.mySide;
    }

    handleSquareClick(row, col) {
        if (this.isSpectator) return;
        const piece = this.board[row][col];

        if (!this.selectedSquare) {
            if (!this.canIMove()) return;
            if (piece && this.isPieceMine(piece)) {
                this.selectedSquare = { row, col };
                this.validMoves = this.getValidMoves(row, col);
                this.render();
            }
            return;
        }


        if (this.selectedSquare.row === row && this.selectedSquare.col === col) {
            this.selectedSquare = null;
            this.validMoves = [];
            this.render();
            return;
        }

        const move = this.validMoves.find(m => m.row === row && m.col === col);
        if (move) {
            const special = {};
            if (move.castling) special.castling = move.castling;
            if (move.enPassant) special.enPassant = true;
            this.makeMove(this.selectedSquare.row, this.selectedSquare.col, row, col, special);
        }


        this.selectedSquare = null;
        this.validMoves = [];
        this.render();
    }

    isPieceCurrentPlayer(piece) {
        if (this.currentPlayer === 'white') {
            return piece === piece.toUpperCase();
        } else {
            return piece === piece.toLowerCase() && piece !== '';
        }
    }

    getValidMoves(row, col) {
        const piece = this.board[row][col];
        if (!piece) return [];

        const isWhite = piece === piece.toUpperCase();
        const pieceType = piece.toLowerCase();
        const moves = [];

        switch(pieceType) {
            case 'p':
                this.getPawnMoves(row, col, isWhite, moves);
                break;
            case 'n':
                this.getKnightMoves(row, col, isWhite, moves);
                break;
            case 'b':
                this.getBishopMoves(row, col, isWhite, moves);
                break;
            case 'r':
                this.getRookMoves(row, col, isWhite, moves);
                break;
            case 'q':
                this.getQueenMoves(row, col, isWhite, moves);
                break;
            case 'k':
                this.getKingMoves(row, col, isWhite, moves);
                break;
        }


        return moves.filter(move => {
            const newBoard = this.simulateMove(row, col, move.row, move.col);
            return !this.isInCheck(newBoard, isWhite);
        });
    }

    getPawnMoves(row, col, isWhite, moves) {
        const direction = isWhite ? -1 : 1;
        const startRow = isWhite ? 6 : 1;


        if (row + direction >= 0 && row + direction < 8 && !this.board[row + direction][col]) {
            moves.push({ row: row + direction, col });


            if (row === startRow && !this.board[row + 2 * direction][col]) {
                moves.push({ row: row + 2 * direction, col });
            }
        }


        if (col + 1 < 8 && row + direction >= 0 && row + direction < 8) {
            const targetPiece = this.board[row + direction][col + 1];
            if (targetPiece && !this.isPieceCurrentPlayer(targetPiece)) {
                moves.push({ row: row + direction, col: col + 1 });
            }

            if (this.enPassantTarget &&
                this.enPassantTarget.row === row + direction &&
                this.enPassantTarget.col === col + 1) {
                moves.push({ row: row + direction, col: col + 1, enPassant: true });
            }
        }


        if (col - 1 >= 0 && row + direction >= 0 && row + direction < 8) {
            const targetPiece = this.board[row + direction][col - 1];
            if (targetPiece && !this.isPieceCurrentPlayer(targetPiece)) {
                moves.push({ row: row + direction, col: col - 1 });
            }

            if (this.enPassantTarget &&
                this.enPassantTarget.row === row + direction &&
                this.enPassantTarget.col === col - 1) {
                moves.push({ row: row + direction, col: col - 1, enPassant: true });
            }
        }
    }

    getKnightMoves(row, col, isWhite, moves) {
        const knightMoves = [
            [-2, -1], [-2, 1], [-1, -2], [-1, 2],
            [1, -2], [1, 2], [2, -1], [2, 1]
        ];

        for (const [dr, dc] of knightMoves) {
            const newRow = row + dr;
            const newCol = col + dc;

            if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                const targetPiece = this.board[newRow][newCol];
                if (!targetPiece || !this.isPieceCurrentPlayer(targetPiece)) {
                    moves.push({ row: newRow, col: newCol });
                }
            }
        }
    }

    getBishopMoves(row, col, isWhite, moves) {
        const directions = [[-1, -1], [-1, 1], [1, -1], [1, 1]];
        this.getSlidingMoves(row, col, isWhite, directions, moves);
    }

    getRookMoves(row, col, isWhite, moves) {
        const directions = [[-1, 0], [1, 0], [0, -1], [0, 1]];
        this.getSlidingMoves(row, col, isWhite, directions, moves);
    }

    getQueenMoves(row, col, isWhite, moves) {
        const directions = [
            [-1, -1], [-1, 1], [1, -1], [1, 1],
            [-1, 0], [1, 0], [0, -1], [0, 1]
        ];
        this.getSlidingMoves(row, col, isWhite, directions, moves);
    }

    getKingMoves(row, col, isWhite, moves) {
        const kingMoves = [
            [-1, -1], [-1, 0], [-1, 1],
            [0, -1],           [0, 1],
            [1, -1],  [1, 0],  [1, 1]
        ];

        for (const [dr, dc] of kingMoves) {
            const newRow = row + dr;
            const newCol = col + dc;

            if (newRow >= 0 && newRow < 8 && newCol >= 0 && newCol < 8) {
                const targetPiece = this.board[newRow][newCol];
                if (!targetPiece || !this.isPieceCurrentPlayer(targetPiece)) {
                    moves.push({ row: newRow, col: newCol });
                }
            }
        }


        this.getCastlingMoves(row, col, isWhite, moves);
    }

    getSlidingMoves(row, col, isWhite, directions, moves) {
        for (const [dr, dc] of directions) {
            for (let i = 1; i < 8; i++) {
                const newRow = row + dr * i;
                const newCol = col + dc * i;

                if (newRow < 0 || newRow >= 8 || newCol < 0 || newCol >= 8) break;

                const targetPiece = this.board[newRow][newCol];
                if (!targetPiece) {
                    moves.push({ row: newRow, col: newCol });
                } else {
                    if (!this.isPieceCurrentPlayer(targetPiece)) {
                        moves.push({ row: newRow, col: newCol });
                    }
                    break;
                }
            }
        }
    }

    getCastlingMoves(row, col, isWhite, moves) {
        if (!isWhite && row !== 0) return;
        if (isWhite && row !== 7) return;

        const rights = isWhite ? this.castlingRights.white : this.castlingRights.black;

        const kingCol = 4;
        if (this.isSquareAttacked(row, kingCol, isWhite)) return;

        if (rights.kingSide) {
            if (!this.board[row][5] && !this.board[row][6]) {
                if (!this.isSquareAttacked(row, 5, isWhite) && !this.isSquareAttacked(row, 6, isWhite)) {
                    moves.push({ row, col: 6, castling: 'king' });
                }
            }
        }
        if (rights.queenSide) {
            if (!this.board[row][3] && !this.board[row][2] && !this.board[row][1]) {
                if (!this.isSquareAttacked(row, 3, isWhite) && !this.isSquareAttacked(row, 2, isWhite)) {
                    moves.push({ row, col: 2, castling: 'queen' });
                }
            }
        }
    }

    isSquareAttacked(row, col, isWhiteKing) {

        const opponentColor = isWhiteKing ? 'black' : 'white';

        for (let r = 0; r < 8; r++) {
            for (let c = 0; c < 8; c++) {
                const piece = this.board[r][c];
                if (!piece) continue;

                const isOpponent = opponentColor === 'white' ?
                    piece === piece.toUpperCase() :
                    piece === piece.toLowerCase();

                if (isOpponent) {
                    const moves = [];
                    const pieceType = piece.toLowerCase();


                    if (pieceType === 'p') {
                        const direction = isOpponent ? -1 : 1;
                        if (Math.abs(c - col) === 1 && r + direction === row) {
                            return true;
                        }
                    } else if (pieceType === 'n') {
                        if (Math.abs(r - row) === 2 && Math.abs(c - col) === 1 ||
                            Math.abs(r - row) === 1 && Math.abs(c - col) === 2) {
                            return true;
                        }
                    } else if (pieceType === 'k') {
                        if (Math.abs(r - row) <= 1 && Math.abs(c - col) <= 1) {
                            return true;
                        }
                    } else {

                        if (r === row || c === col || Math.abs(r - row) === Math.abs(c - col)) {

                            let dr = r === row ? 0 : (r < row ? 1 : -1);
                            let dc = c === col ? 0 : (c < col ? 1 : -1);
                            let blocked = false;

                            for (let i = 1; i < Math.max(Math.abs(r - row), Math.abs(c - col)); i++) {
                                const tr = r + dr * i;
                                const tc = c + dc * i;
                                if (tr === row && tc === col) break;
                                if (this.board[tr][tc]) {
                                    blocked = true;
                                    break;
                                }
                            }

                            if (!blocked) {
                                if ((pieceType === 'r' && (r === row || c === col)) ||
                                    (pieceType === 'b' && Math.abs(r - row) === Math.abs(c - col)) ||
                                    pieceType === 'q') {
                                    return true;
                                }
                            }
                        }
                    }
                }
            }
        }
        return false;
    }

    isInCheck(board, isWhite) {

        let kingRow, kingCol;
        for (let r = 0; r < 8; r++) {
            for (let c = 0; c < 8; c++) {
                const piece = board[r][c];
                if (piece && (isWhite ? piece === 'K' : piece === 'k')) {
                    kingRow = r;
                    kingCol = c;
                    break;
                }
            }
        }


        return this.isSquareAttackedByBoard(board, kingRow, kingCol, !isWhite);
    }

    isSquareAttackedByBoard(board, row, col, byWhite) {
        for (let r = 0; r < 8; r++) {
            for (let c = 0; c < 8; c++) {
                const piece = board[r][c];
                if (!piece) continue;

                const isAttacker = byWhite ?
                    piece === piece.toUpperCase() :
                    piece === piece.toLowerCase();

                if (isAttacker) {
                    const pieceType = piece.toLowerCase();


                    if (pieceType === 'p') {
                        const direction = byWhite ? -1 : 1;
                        if (r + direction === row && Math.abs(c - col) === 1) {
                            return true;
                        }
                    }


                    if (pieceType === 'n') {
                        if (Math.abs(r - row) === 2 && Math.abs(c - col) === 1 ||
                            Math.abs(r - row) === 1 && Math.abs(c - col) === 2) {
                            return true;
                        }
                    }


                    if (pieceType === 'k') {
                        if (Math.abs(r - row) <= 1 && Math.abs(c - col) <= 1) {
                            return true;
                        }
                    }


                    if (pieceType === 'b' || pieceType === 'r' || pieceType === 'q') {
                        if ((pieceType === 'r' || pieceType === 'q') && (r === row || c === col)) {

                            let dr = r === row ? 0 : (r < row ? 1 : -1);
                            let dc = c === col ? 0 : (c < col ? 1 : -1);
                            let blocked = false;

                            for (let i = 1; i < Math.max(Math.abs(r - row), Math.abs(c - col)); i++) {
                                const tr = r + dr * i;
                                const tc = c + dc * i;
                                if (tr === row && tc === col) break;
                                if (board[tr][tc]) {
                                    blocked = true;
                                    break;
                                }
                            }

                            if (!blocked) return true;
                        }

                        if ((pieceType === 'b' || pieceType === 'q') && Math.abs(r - row) === Math.abs(c - col)) {

                            let dr = r < row ? 1 : -1;
                            let dc = c < col ? 1 : -1;
                            let blocked = false;

                            for (let i = 1; i < Math.abs(r - row); i++) {
                                const tr = r + dr * i;
                                const tc = c + dc * i;
                                if (tr === row && tc === col) break;
                                if (board[tr][tc]) {
                                    blocked = true;
                                    break;
                                }
                            }

                            if (!blocked) return true;
                        }
                    }
                }
            }
        }
        return false;
    }

    simulateMove(fromRow, fromCol, toRow, toCol) {
        const newBoard = this.board.map(row => [...row]);
        newBoard[toRow][toCol] = newBoard[fromRow][fromCol];
        newBoard[fromRow][fromCol] = '';
        return newBoard;
    }

    async makeMove(fromRow, fromCol, toRow, toCol, special = {}) {
        const newBoard = this.board.map(row => [...row]);
        const piece = newBoard[fromRow][fromCol];
        const isWhite = piece === piece.toUpperCase();


        if (special.enPassant) {

            newBoard[fromRow][toCol] = '';
        } else if (special.castling) {

            if (special.castling === 'king') {
                newBoard[fromRow][5] = newBoard[fromRow][7];
                newBoard[fromRow][7] = '';
            } else if (special.castling === 'queen') {
                newBoard[fromRow][3] = newBoard[fromRow][0];
                newBoard[fromRow][0] = '';
            }
        }


        newBoard[toRow][toCol] = piece;
        newBoard[fromRow][fromCol] = '';


        if (piece.toLowerCase() === 'p' && (toRow === 0 || toRow === 7)) {
            newBoard[toRow][toCol] = isWhite ? 'Q' : 'q';
        }

        this.board = newBoard;
        this.version++;
        this.currentPlayer = this.currentPlayer === 'white' ? 'black' : 'white';


        if (piece.toLowerCase() === 'k') {
            if (isWhite) {
                this.castlingRights.white = { kingSide: false, queenSide: false };
            } else {
                this.castlingRights.black = { kingSide: false, queenSide: false };
            }
        }
        if (piece.toLowerCase() === 'r') {
            if (isWhite) {
                if (fromCol === 0) this.castlingRights.white.queenSide = false;
                if (fromCol === 7) this.castlingRights.white.kingSide = false;
            } else {
                if (fromCol === 0) this.castlingRights.black.queenSide = false;
                if (fromCol === 7) this.castlingRights.black.kingSide = false;
            }
        }


        this.enPassantTarget = null;
        if (piece.toLowerCase() === 'p' && Math.abs(fromRow - toRow) === 2) {
            this.enPassantTarget = {
                row: (fromRow + toRow) / 2,
                col: fromCol
            };
        }


        const inCheck = this.isInCheck(this.board, this.currentPlayer === 'white');
        const hasMoves = this.hasAnyMove(this.currentPlayer === 'white');

        if (inCheck && !hasMoves) {
            const loser = this.currentPlayer;
            this._gameEndShown = true;
            if (this.mySide) {
                if (this.mySide === loser) this.showGameOver('defeat', 'Checkmate. You lost.');
                else this.showGameOver('victory', 'Checkmate. You won!');
            } else {
                alert('Checkmate! ' + (loser === 'white' ? 'Black' : 'White') + ' wins!');
            }
        } else if (!inCheck && !hasMoves) {
            this._gameEndShown = true;
            if (this.mySide) this.showGameOver('draw', 'Stalemate. Draw.');
            else alert('Stalemate!');
        } else if (inCheck) {
            if (this.mySide) this.showMessage('Check!', 2000);
            else alert('Check! ' + this.currentPlayer + "'s king is in check");
        }

        const moveData = {
            board: this.board,
            from: `${fromRow},${fromCol}`,
            to: `${toRow},${toCol}`,
            piece: piece,
            special: special,
            nextPlayer: this.currentPlayer,
            castlingRights: this.castlingRights,
            enPassantTarget: this.enPassantTarget,
            version: this.version
        };
        this.moveCache[this.version] = moveData;
        try {
            const moveNumber = String(this.version).padStart(4, '0');
            await fetch(`/games/${this.currentGame}/moves/${moveNumber}.json`, {
                method: 'PUT',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify(moveData)
            });
            if (!this.moves.includes(moveNumber + '.json')) {
                this.moves.push(moveNumber + '.json');
                this.moves.sort();
            }
        } catch (e) {
            console.log('Move not saved to server');
        }
        if (this.mySide) {
            try {
                await fetch(`/games/${this.currentGame}/state.json`, {
                    method: 'PUT',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify({ version: this.version })
                });
            } catch (e) {}
            await this.loadGame(true);
        }
        this.saveGameToStorage();
        this.render();
    }

    hasAnyMove(isWhite) {
        for (let r = 0; r < 8; r++) {
            for (let c = 0; c < 8; c++) {
                const piece = this.board[r][c];
                if (piece && (isWhite ? piece === piece.toUpperCase() : piece === piece.toLowerCase())) {
                    const moves = this.getValidMoves(r, c);
                    if (moves.length > 0) return true;
                }
            }
        }
        return false;
    }

    newGame() {
        const newId = 'game_' + Date.now();
        window.location.href = `/?game=${encodeURIComponent(newId)}`;
    }

    async replicate() {
        const newGameId = 'game_' + Date.now();

        try {
            const moveData = [];
            for (let i = 1; i <= this.version; i++) {
                const moveNum = String(i).padStart(4, '0');
                try {
                    const move = await fetch(`/games/${this.currentGame}/moves/${moveNum}.json`)
                        .then(r => r.json());
                    moveData.push(move);
                } catch (e) {}
            }

            for (let i = 0; i < moveData.length; i++) {
                const moveNum = String(i + 1).padStart(4, '0');
                await fetch(`/games/${newGameId}/moves/${moveNum}.json`, {
                    method: 'PUT',
                    headers: { 'Content-Type': 'application/json' },
                    body: JSON.stringify(moveData[i])
                });
            }

            window.location.href = `/?game=${newGameId}`;
        } catch (e) {
            console.log('Replication failed, just navigating');
            window.location.href = `/?game=${newGameId}`;
        }
    }

    showMessage(msg, durationMs) {
        const el = document.getElementById('chessMessage');
        if (el) el.remove();
        const div = document.createElement('div');
        div.id = 'chessMessage';
        div.textContent = msg;
        div.style.cssText = 'position:fixed;bottom:20px;left:50%;transform:translateX(-50%);background:#2c3e50;color:#fff;padding:10px 16px;border-radius:8px;z-index:9999;font-size:14px;box-shadow:0 4px 12px rgba(0,0,0,0.3);';
        document.body.appendChild(div);
        setTimeout(() => div.remove(), durationMs || 2500);
    }

    showGameOver(kind, message) {
        const ov = document.getElementById('gameOverOverlay');
        const tx = document.getElementById('gameOverText');
        if (!ov || !tx) {
            alert(message);
            return;
        }
        tx.textContent = message;
        ov.classList.remove('game-over-defeat', 'game-over-victory', 'game-over-draw');
        if (kind === 'defeat') ov.classList.add('game-over-defeat');
        else if (kind === 'victory') ov.classList.add('game-over-victory');
        else ov.classList.add('game-over-draw');
        ov.style.display = 'flex';
    }

    maybeShowGameEndOverlay() {
        if (this.isSpectator || !this.mySide) return;
        const whiteToMove = this.currentPlayer === 'white';
        const inCheck = this.isInCheck(this.board, whiteToMove);
        const hasMoves = this.hasAnyMove(whiteToMove);
        const mate = inCheck && !hasMoves;
        const stale = !inCheck && !hasMoves;
        if (!mate && !stale) {
            this._gameEndShown = false;
            return;
        }
        if (this._gameEndShown) return;
        this._gameEndShown = true;
        if (mate) {
            const loser = this.currentPlayer;
            if (this.mySide === loser) this.showGameOver('defeat', 'Checkmate. You lost.');
            else this.showGameOver('victory', 'Checkmate. You won!');
        } else {
            this.showGameOver('draw', 'Stalemate. Draw.');
        }
    }

    async exportSnapshot() {
        const pieceSymbols = { 'r':'&#9820;','n':'&#9822;','b':'&#9821;','q':'&#9819;','k':'&#9818;','p':'&#9823;','R':'&#9814;','N':'&#9816;','B':'&#9815;','Q':'&#9813;','K':'&#9812;','P':'&#9817;' };
        let cells = '';
        for (let row = 0; row < 8; row++) {
            for (let col = 0; col < 8; col++) {
                const piece = this.board[row][col];
                const sym = piece ? pieceSymbols[piece] || piece : '';
                const bg = (row + col) % 2 === 0 ? '#f0d9b5' : '#b58863';
                cells += `<div style="width:50px;height:50px;display:flex;align-items:center;justify-content:center;font-size:36px;background:${bg}">${sym}</div>`;
            }
        }
        const html = `<!DOCTYPE html><html><head><meta charset="utf-8"><title>Chess</title></head><body style="margin:0;display:flex;justify-content:center;align-items:center;min-height:100vh;background:#ddd"><div style="display:grid;grid-template-columns:repeat(8,50px);border:2px solid #34495e;box-shadow:0 4px 12px rgba(0,0,0,0.2)">${cells}</div></body></html>`;
        const blob = new Blob([html], { type: 'text/html;charset=utf-8' });
        const a = document.createElement('a');
        a.href = URL.createObjectURL(blob);
        a.download = `chess_${this.currentGame}_v${this.version}.html`;
        a.click();
        URL.revokeObjectURL(a.href);
        this.showMessage('Snapshot saved');
    }

    async syncWithPeer() {
        try {
            await this.loadGame();
            this.render();
            this.showMessage('Refreshed from server. (Push requires Beagle backend.)');
        } catch (e) {
            this.showMessage('Refresh failed: ' + (e.message || 'network error'));
        }
    }
}

window.chess = new SelfReplicatingChess();