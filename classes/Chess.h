#pragma once
#include <vector>
#include <string>
#include "Game.h"
#include "Grid.h"
#include "GameState.h"

constexpr int pieceSize = 80;


class Chess : public Game
{
public:
    Chess();
    ~Chess();

    void setUpBoard() override;

    bool canBitMoveFrom(Bit &bit, BitHolder &src) override;
    bool canBitMoveFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;
    bool actionForEmptyHolder(BitHolder &holder) override;

    void stopGame() override;

    void bitMovedFromTo(Bit &bit, BitHolder &src, BitHolder &dst) override;

    Player *checkForWinner() override;
    bool checkForDraw() override;

    std::string initialStateString() override;
    std::string stateString() override;
    void setStateString(const std::string &s) override;

    Grid* getGrid() override { return _grid; }

    void makeAIMove(int depth = 4);

private:
    Bit* PieceForPlayer(const int playerNumber, ChessPiece piece);
    Player* ownerAt(int x, int y) const;
    void FENtoBoard(const std::string& fen);
    char pieceNotation(int x, int y) const;

    void clearBoardHighlights();
    Grid* _grid;
    GameState _gameState;
    std::vector<BitMove> _moves;

    int evaluateBoard(const GameState& gamestate) const;
    int negamax(GameState& gamestate, int depth, int alpha, int beta);

    BitMove findBestMove(int depth);

    void printBoard(const GameState& gamestate) const;
    std::string gameStateToFEN(const GameState& gamestate) const;

    int _currentPlayer;


};