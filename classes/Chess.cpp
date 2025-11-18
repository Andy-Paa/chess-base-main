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

    _grid->initializeChessSquares(pieceSize, "boardsquare.png");
    FENtoBoard("rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR");

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

    int x = 0, y = 0;

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
            ++y;
            if (y >= 8) throw std::invalid_argument("Too many ranks in FEN");
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
    if (!(y == 7 && x == 8))
        throw std::invalid_argument("FEN must describe exactly 8 ranks of 8 files");
}

bool Chess::actionForEmptyHolder(BitHolder &holder)
{
    return false;
}

bool Chess::canBitMoveFrom(Bit &bit, BitHolder &src)
{
    // need to implement friendly/unfriendly in bit so for now this hack
    int currentPlayer = getCurrentPlayer()->playerNumber() * 128;
    int pieceColor = bit.gameTag() & 128;
    if (pieceColor == currentPlayer) return true;
    return false;
}

bool Chess::canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst)
{
    return true;
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
