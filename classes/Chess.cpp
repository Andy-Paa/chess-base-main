#include "Chess.h"
#include <limits>
#include <cmath>
#include <sstream>   


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

    if (depth == 0 || moves.empty()) {
        return evaluateBoard(gamestate);
    }

    int best = std::numeric_limits<int>::min();

    for (const BitMove& mv : moves) {
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

void Chess::updateAI()
{
    // 比赛模式下，TournamentClient 每次都会先调用 setBoardFromFEN，
    // 所以这里直接用 _gameState 来搜索就行。

    _lastAIMove = BitMove();  // 重置，确保没有旧数据

    // 防御一下：没有合法走法就不要再算了
    std::vector<BitMove> moves = _gameState.generateAllMoves();
    if (moves.empty()) {
        std::cout << "[AI] No legal moves in current position\n";
        return;
    }

    // 用你已经写好的 negamax 搜索
    int searchDepth = 4; // 可调
    BitMove best = findBestMove(searchDepth);

    _lastAIMove = best;  // 保存给 TournamentClient 用

    std::cout << "[AI] Computed move (tournament): from "
              << (int)best.from << " to " << (int)best.to << std::endl;
}

// Tournament support: Set board from FEN and reinitialize game state for AI
void Chess::setBoardFromFEN(const std::string& fen) {
    _grid->forEachSquare([](ChessSquare* square, int x, int y) {
        square->destroyBit();
    });
    // 允许传完整 FEN，也允许只传棋子布局部分
    std::string piecePlacement = fen;
    std::string activeColor = "w";
    std::string castling = "-";
    std::string enPassant = "-";

    // 如果是完整 FEN（包含空格）
    size_t spacePos = fen.find(' ');
    if (spacePos != std::string::npos) {
        std::istringstream fenStream(fen);
        fenStream >> piecePlacement >> activeColor >> castling >> enPassant;
    }

    // 1) 用布局部分更新 UI 棋盘
    FENtoBoard(piecePlacement);

    // 2) 从 FEN 判断轮到谁走
    _currentPlayer = (activeColor == "w" || activeColor == "W") ? WHITE : BLACK;

    // 3) 用当前 UI 盘面字符串初始化 GameState（注意是 _gameState，不是 _gamestate）
    _gameState.init(stateString().c_str(), _currentPlayer);

    // 4) 重新生成合法走法给 negamax 用
    _moves = _gameState.generateAllMoves();

    std::cout << "[Tournament] Board set from FEN. Player: "
              << (_currentPlayer == WHITE ? "White" : "Black")
              << ", Legal moves: " << _moves.size() << std::endl;
}

// Tournament support: Generate FEN string from current board
std::string Chess::getFEN() const {
    std::string fen;
    fen.reserve(90);

    // 1) Piece placement (rank 8 -> rank 1)
    for (int rank = 7; rank >= 0; --rank) {
        int emptyCount = 0;
        for (int file = 0; file < 8; ++file) {
            char piece = pieceNotation(file, rank);
            if (piece == '0') {
                emptyCount++;
            } else {
                if (emptyCount > 0) {
                    fen += std::to_string(emptyCount);
                    emptyCount = 0;
                }
                fen += piece;
            }
        }
        if (emptyCount > 0) {
            fen += std::to_string(emptyCount);
        }
        if (rank > 0) {
            fen += '/';
        }
    }

    // 2) Active color
    fen += ' ';
    fen += (_currentPlayer == WHITE) ? 'w' : 'b';

    // 3) Castling availability（简化版，根据王和车的位置判断）
    fen += ' ';
    std::string castling;

    // 白方
    char e1 = pieceNotation(4, 0);
    char a1 = pieceNotation(0, 0);
    char h1 = pieceNotation(7, 0);
    if (e1 == 'K') {
        if (h1 == 'R') castling += 'K';
        if (a1 == 'R') castling += 'Q';
    }

    // 黑方
    char e8 = pieceNotation(4, 7);
    char a8 = pieceNotation(0, 7);
    char h8 = pieceNotation(7, 7);
    if (e8 == 'k') {
        if (h8 == 'r') castling += 'k';
        if (a8 == 'r') castling += 'q';
    }

    fen += castling.empty() ? "-" : castling;

    // 4) En passant（简化：不处理，写 '-'）
    fen += " -";

    // 5) Halfmove clock（简化）
    fen += " 0";

    // 6) Fullmove number（简化）
    fen += " 1";

    return fen;
}
