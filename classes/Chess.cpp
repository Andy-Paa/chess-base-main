#include "Chess.h"
#include <limits>
#include <cmath>


Chess::Chess()
{
    _grid = new Grid(8, 8);
}

Chess::~Chess()
{
    delete _grid;
}

char Chess::pieceNotation(int x, int y) const
{
    const char *wpieces = { "0PNBRQK" };
    const char *bpieces = { "0pnbrqk" };
    Bit *bit = _grid->getSquare(x, y)->bit();
    char notation = '0';
    if (bit) {
        notation = bit->gameTag() < 128 ? wpieces[bit->gameTag()] : bpieces[bit->gameTag()-128];
    }
    return notation;
}

Bit* Chess::PieceForPlayer(const int playerNumber, ChessPiece piece)
{
    const char* pieces[] = { "pawn.png", "knight.png", "bishop.png", "rook.png", "queen.png", "king.png" };

    Bit* bit = new Bit();
    // should possibly be cached from player class?
    const char* pieceName = pieces[piece - 1];
    std::string spritePath = std::string("") + (playerNumber == 0 ? "w_" : "b_") + pieceName;
    bit->LoadTextureFromFile(spritePath.c_str());
    bit->setOwner(getPlayerAt(playerNumber));
    bit->setSize(pieceSize, pieceSize);

    return bit;
}

void Chess::setUpBoard()
{
    setNumberOfPlayers(2);
    _gameOptions.rowX = 8;
    _gameOptions.rowY = 8;

    _currentPlayer = 1;
    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

    _gameState.init(stateString().c_str(), WHITE);
    _moves = _gameState.generateAllMoves();
    std::cout << "[DEBUG] moves at start: " << _moves.size() << std::endl;
    std::string s = stateString();
    for (int i = 0; i < (int)_moves.size(); ++i) {
    auto &m = _moves[i];
    char pieceChar = s[m.from];
    std::cout << "  move " << i
              << " from=" << (int)m.from
              << " to="   << (int)m.to
              << " piece=" << pieceChar
              << std::endl;
    }
    startGame();
}

void Chess::FENtoBoard(const std::string& fen) {
    // convert a FEN string to a board
    // FEN is a space delimited string with 6 fields
    // 1: piece placement (from white's perspective)
    // NOT PART OF THIS ASSIGNMENT BUT OTHER THINGS THAT CAN BE IN A FEN STRING
    // ARE BELOW
    // 2: active color (W or B)
    // 3: castling availability (KQkq or -)
    // 4: en passant target square (in algebraic notation, or -)
    // 5: halfmove clock (number of halfmoves since the last capture or pawn advance)
    if (fen.empty()) throw std::invalid_argument("Empty FEN string");

    // 清空棋盘
    for (int yy = 0; yy < 8; ++yy)
        for (int xx = 0; xx < 8; ++xx)
            _grid->getSquare(xx, yy)->setBit(nullptr);

    // 只取布局字段
    const std::string placement = fen.substr(0, fen.find(' '));

    int x = 0, y = 7;

    auto put = [this, &x, &y](char c) {
        if (x >= 8 || y >= 8)  // 二次保险
            throw std::invalid_argument("Board index out of range");

        struct Item { int player; ChessPiece piece; int tag; } it;
        switch (c) {
            case 'P': it = {0, ChessPiece::Pawn,   1}; break;
            case 'N': it = {0, ChessPiece::Knight, 2}; break;
            case 'B': it = {0, ChessPiece::Bishop, 3}; break;
            case 'R': it = {0, ChessPiece::Rook,   4}; break;
            case 'Q': it = {0, ChessPiece::Queen,  5}; break;
            case 'K': it = {0, ChessPiece::King,   6}; break;
            case 'p': it = {1, ChessPiece::Pawn,   1+128}; break;
            case 'n': it = {1, ChessPiece::Knight, 2+128}; break;
            case 'b': it = {1, ChessPiece::Bishop, 3+128}; break;
            case 'r': it = {1, ChessPiece::Rook,   4+128}; break;
            case 'q': it = {1, ChessPiece::Queen,  5+128}; break;
            case 'k': it = {1, ChessPiece::King,   6+128}; break;
            default:  throw std::invalid_argument("Invalid piece char in FEN");
        }

        Bit* bit = PieceForPlayer(it.player, it.piece);
        ChessSquare* square = _grid->getSquare(x, y);
        bit->setPosition(square->getPosition());
        bit->setParent(square);
        bit->setGameTag(it.tag);
        square->setBit(bit);
        ++x;
    };

    for (char c : placement) {
        if (c == '/') {
            if (x != 8) throw std::invalid_argument("FEN rank does not have 8 files");
            x = 0;
            --y;
            if (y < 0) throw std::invalid_argument("Too many ranks in FEN");
        } else if (c >= '1' && c <= '8') {
            int n = c - '0';
            if (x + n > 8) throw std::invalid_argument("Rank overflow in FEN");
            x += n; // 空格跳过
        } else {
            if (x >= 8) throw std::invalid_argument("Too many files in rank");
            put(c);
        }
    }

    // 最终必须刚好填满 8x8
    if (!(y == 0 && x == 8))
        throw std::invalid_argument("FEN must describe exactly 8 ranks of 8 files");
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    ChessSquare* srcsquare = (ChessSquare *)&src;
    ChessSquare* square = (ChessSquare *)&dst;
    if (square) {
        int squareIndex = square->getSquareIndex();
        for(auto move : _moves) {
            if (move.to == squareIndex && move.from == srcsquare->getSquareIndex()) {
                return true;
            }
        }
    }
    return false;
}

void Chess::clearBoardHighlights() {
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
            square->setHighlighted(false);
    });
}

void Chess::bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) {
    _currentPlayer = (_currentPlayer == WHITE ? BLACK : WHITE);
    _gameState.init( stateString().c_str(), _currentPlayer);
    _moves = _gameState.generateAllMoves();
    clearBoardHighlights();
    std::cout << "[DEBUG] moves after bit moved: " << _moves.size() << std::endl;
    endTurn();

    if (_currentPlayer == BLACK) {
        makeAIMove(4);
    }
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
        std::cout << "canBitMoveFrom called, gameTag=" << bit.gameTag()
              << " playerNumber=" << getCurrentPlayer()->playerNumber() << std::endl;
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor != currentPlayer) return false;

    bool ret = false;
    ChessSquare* square = (ChessSquare *)&src;
    if (square) {
        int squareIndex = square->getSquareIndex();
        for(auto move : _moves) {
            if (move.from == squareIndex) {
                ret = true;
                auto dest = _grid->getSquareByIndex(move.to);
                dest->setHighlighted(true);
            }
        }
    }
    return ret;
}

void Chess::stopGame()
{
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
}

Player* Chess::ownerAt(int x, int y) const
{
    if (x < 0 || x >= 8 || y < 0 || y >= 8) {
        return nullptr;
    }

    auto square = _grid->getSquare(x, y);
    if (!square || !square->bit()) {
        return nullptr;
    }
    return square->bit()->getOwner();
}

Player* Chess::checkForWinner()
{
    
    return nullptr;
}

bool Chess::checkForDraw()
{
    return false;
}

std::string Chess::initialStateString()
{
    return stateString();
}

std::string Chess::stateString()
{
    std::string s;
    s.reserve(64);
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
            s += pieceNotation(x, y);
        });
    return s;
}

void Chess::setStateString(const std::string &s)
{
    _grid->forEachSquare([&](ChessSquare* square, int x, int y) {
        int index = y * 8 + x;
        char playerNumber = s[index] - '0';
        if (playerNumber) {
            square->setBit(PieceForPlayer(playerNumber - 1, Pawn));
        } else {
            square->setBit(nullptr);
        }
    });
}

static int pieceValue(char c) {
    switch (c) {
        case 'P': case 'p': return 100;
        case 'N': case 'n': return 320;
        case 'B': case 'b': return 330;
        case 'R': case 'r': return 500;
        case 'Q': case 'q': return 900;
        case 'K': case 'k': return 10000;
        default: return 0;
    }
}

int Chess::evaluateBoard(const GameState& gamestate) const
{
    int value = 0;
    for (int i = 0; i < 64; ++i) {
        char c = gamestate.state[i];
        switch (c) {
            // 白子加分
            case 'P': case 'N': case 'B': case 'R': case 'Q': case 'K':
                value += pieceValue(c);
                break;
            // 黑子减分
            case 'p': case 'n': case 'b': case 'r': case 'q': case 'k':
                value -= pieceValue(c);
                break;
            default:
                break;
        }
    }
    return value * gamestate.color;
}

int Chess::negamax(GameState& gamestate, int depth, int alpha, int beta)
{
    std::vector<BitMove> moves = gamestate.generateAllMoves();

    if (depth == 0 || _moves.empty()) {
        return evaluateBoard(gamestate);
    }

    int best = std::numeric_limits<int>::min();

    for (const BitMove& mv : _moves) {
        gamestate.pushMove(mv);

        int score = -negamax(gamestate, depth - 1, -beta, -alpha);

        gamestate.popState();

        if (score > best) best = score;
        if (score > alpha) alpha = score;
        if (alpha >= beta) {
            break;
        }
    }

    return best;
}

BitMove Chess::findBestMove(int depth)
{
    std::vector<BitMove> moves = _gameState.generateAllMoves();
    if (moves.empty()) {
        return BitMove();
    }

    int alpha = std::numeric_limits<int>::min() + 1;
    int beta  = std::numeric_limits<int>::max();
    int bestScore = std::numeric_limits<int>::min();

    BitMove bestMove = moves[0];

    for (const BitMove& mv : moves) {
        _gameState.pushMove(mv);
        int score = -negamax(_gameState, depth - 1, -beta, -alpha);
        _gameState.popState();

        if (score > bestScore) {
            bestScore = score;
            bestMove = mv;
        }
        if (score > alpha) {
            alpha = score;
        }
    }

    std::cout << "Eval Result: " << bestScore << std::endl;
    return bestMove;
}

void Chess::printBoard(const GameState& gamestate) const
{
    std::cout << "  +-----------------+\n";
    for (int rank = 7; rank >= 0; --rank) {
        std::cout << (rank + 1) << " | ";
        for (int file = 0; file < 8; ++file) {
            int idx = rank * 8 + file;
            char c = gamestate.state[idx];
            if (c == '0') c = '.';
            std::cout << c << ' ';
        }
        std::cout << "|\n";
    }
    std::cout << "  +-----------------+\n";
    std::cout << "    a b c d e f g h\n";
}

void Chess::makeAIMove(int depth)
{
    if (_currentPlayer != BLACK) return;

    BitMove best = findBestMove(depth);

    _gameState.pushMove(best);

    std::cout << "AI move: from " << (int)best.from
              << " to " << (int)best.to << std::endl;
    printBoard(_gameState);

    std::string fenPlacement = gameStateToFEN(_gameState);
    FENtoBoard(fenPlacement);

    _currentPlayer = _gameState.color;

    _moves = _gameState.generateAllMoves();
    clearBoardHighlights();
    std::cout << "[DEBUG] moves after AI move: " << _moves.size() << std::endl;

    endTurn();
}

std::string Chess::gameStateToFEN(const GameState& gamestate) const
{
    std::string fen;
    fen.reserve(64 + 7);

    for (int rank = 7; rank >= 0; --rank) {
        int empty = 0;
        for (int file = 0; file < 8; ++file) {
            int idx = rank * 8 + file;
            char c = gamestate.state[idx];

            if (c == '0') {
                ++empty;
            } else {
                if (empty > 0) {
                    fen.push_back('0' + empty);
                    empty = 0;
                }
                fen.push_back(c); // 直接用 'PNBRQKpnbrqk'
            }
        }
        if (empty > 0) {
            fen.push_back('0' + empty);
        }
        if (rank > 0) fen.push_back('/');
    }

    return fen;
}