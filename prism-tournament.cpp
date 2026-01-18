/*
 * PRISM Engine V0.7
 * Template for AI Powered Tournament
 *
 * (C) 2025 Tommy Ciccone All Rights Reserved.
*/

#include <chrono>
#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>

using namespace std;

class Timer {
    public:
        void start() {
            startTime = chrono::high_resolution_clock::now();
        }
        void stop() {
            endTime = chrono::high_resolution_clock::now();
        }
        string getTime() {
            return to_string(chrono::duration_cast<std::chrono::seconds>(endTime - startTime).count());
        }
    private:
        chrono::high_resolution_clock::time_point startTime;
        chrono::high_resolution_clock::time_point endTime;
};

int engineDepth = 5;

bool whiteKingMoved = 0, blackKingMoved = 0, whiteLeftRookMoved = 0, 
    whiteRightRookMoved = 0, blackLeftRookMoved = 0, blackRightRookMoved = 0;

bool whiteCastled = false;
bool blackCastled = false;

int positionsEvaluated = 0;
char board[8][8]; // 8x8 chess board

void initializeBoard() { // place default pieces on board
    string blackPieces = "rnbqkbnr";
    string whitePieces = "RNBQKBNR";

    for (int i = 0; i < 8; i++) {
        board[0][i] = blackPieces[i];
        board[1][i] = 'p';
        board[6][i] = 'P';
        board[7][i] = whitePieces[i];
        for (int j = 2; j < 6; j++) {
            board[j][i] = '.';
        }
    }
}

void printBoard() { // print board to console
    for (int i = 0; i < 8; i++) {
        cout << "\033[90m" << 8 - i << " \033[0m";  // rank
        for (int j = 0; j < 8; j++) {
            char piece = board[i][j];
            string unicodePiece = ".";
            
            // Unicode chess pieces
            switch (piece) { 
                case 'K': unicodePiece = "♚"; break;
                case 'Q': unicodePiece = "♛"; break;
                case 'R': unicodePiece = "♜"; break;
                case 'B': unicodePiece = "♝"; break;
                case 'N': unicodePiece = "♞"; break;
                case 'P': unicodePiece = "♟"; break;
                case 'k': unicodePiece = "♔"; break;
                case 'q': unicodePiece = "♕"; break;
                case 'r': unicodePiece = "♖"; break;
                case 'b': unicodePiece = "♗"; break;
                case 'n': unicodePiece = "♘"; break;
                case 'p': unicodePiece = "♙"; break;
            }
            
            cout << unicodePiece << ' ';
        }
        cout << "\n";
    }
    cout << "\033[90m  a b c d e f g h\033[0m\n"; // file
}

// Move encoding: (flag << 16) | (r << 12) | (f << 8) | (tr << 4) | tf
// flag: 0 = normal, 1 = kingside castle, 2 = queenside castle
inline int encodeMove(int r, int f, int tr, int tf, int flag = 0) {
    return (flag << 16) | (r << 12) | (f << 8) | (tr << 4) | tf;
}

inline int getFromRank(int move) {
    return (move >> 12) & 0xF;
}

inline int getFromFile(int move) {
    return (move >> 8) & 0xF;
}

inline int getToRank(int move) {
    return (move >> 4) & 0xF;
}

inline int getToFile(int move) {
    return move & 0xF;
}

inline int getMoveFlag(int move) {
    return (move >> 16) & 0xF;
}

// Positional piece square table
// types 0 = P, 1 = N, 2 = B, 3 = R, 4 = Q, 5 = K
int positionPST[6][8][8];

// Neighbor piece square table
int neighborPST[6][6][3][3];

// Material values
int materialValues[6];

// Convert piece character to index (0-5)
inline int pieceToIndex(char piece) {
    switch (tolower(piece)) {
        case 'p': return 0;
        case 'n': return 1;
        case 'b': return 2;
        case 'r': return 3;
        case 'q': return 4;
        case 'k': return 5;
        default: return -1;
    }
}

void importPieceSquareTables(const string& botFile) {
    ifstream file(botFile);
    if (!file.is_open()) {
        cout << "Error: Could not open " << botFile << "\n";
        exit(1);
    }

    // Read material values (first 6 values)
    for (int i = 0; i < 6; i++) {
        file >> materialValues[i];
    }

    // Read position PST (next 384 values: 48 groups of 8)
    for (int piece = 0; piece < 6; piece++) {
        for (int rank = 0; rank < 8; rank++) {
            for (int f = 0; f < 8; f++) {
                file >> positionPST[piece][rank][f];
            }
        }
    }

    // Read neighbor PST (next 324 values: 54 groups of 6)
    for (int piece = 0; piece < 6; piece++) {
        for (int neighbor = 0; neighbor < 6; neighbor++) {
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++) {
                    file >> neighborPST[piece][neighbor][row][col];
                }
            }
        }
    }

    file.close();
    cout << "Loaded from " << botFile << "\n";
}

inline bool inBounds(int r, int f) {
    return r >= 0 && r < 8 && f >= 0 && f < 8;
}

int immediateEvaluation() {
    int evaluation = 0;
    bool whiteWins = true;
    bool blackWins = true;
    char piece;

    for (int i = 0; i < 8; i++) {
        for (int j = 0; j < 8; j++) {
            piece = board[i][j];
            if (piece == '.') continue;

            int pieceIdx = pieceToIndex(piece);
            if (pieceIdx == -1) continue;

            bool isWhite = isupper(piece);
            int multiplier;
            if (isWhite) {
                multiplier = 1;
            } else {
                multiplier = -1;
            }

            // Material value
            evaluation += multiplier * materialValues[pieceIdx];

            // Position PST
            int rank;
            if (isWhite) {
                rank = i;
            } else {
                rank = 7 - i;
            }
            evaluation += multiplier * positionPST[pieceIdx][rank][j];

            // Neighbor PST
            for (int dr = -1; dr <= 1; dr++) {
                for (int df = -1; df <= 1; df++) {
                    if (dr == 0 && df == 0) continue; // skip center
                    
                    int nr = i + dr;
                    int nf = j + df;
                    
                    if (inBounds(nr, nf) && board[nr][nf] != '.') {
                        int neighborIdx = pieceToIndex(board[nr][nf]);
                        if (neighborIdx != -1) {
                            int gridRow = dr + 1;
                            int gridCol = df + 1;
                            evaluation += multiplier * neighborPST[pieceIdx][neighborIdx][gridRow][gridCol];
                        }
                    }
                }
            }

            // Track if kings are present
            if (piece == 'K') whiteWins = false;
            if (piece == 'k') blackWins = false;
        }
    }

    // Castling bonus
    if (whiteCastled) {
        evaluation += 10;
    }
    if (blackCastled) {
        evaluation -= 10;
    }

    positionsEvaluated++;
    return evaluation;
}

vector<int> enumeratePawnMoves(int r, int f, char piece) { // list all possible pawn moves for a given pawn
    vector<int> moves;
    moves.reserve(4);
    int direction = 0;
    switch (piece) { // get forwards direction of pawn
        case 'P': direction = -1; break;
        case 'p': direction = 1; break;
    }

    if (inBounds(r + direction, f) && board[r + direction][f] == '.') { // check if square in front is empty
        moves.push_back(encodeMove(r, f, r + direction, f));
        if ((piece == 'P' && r == 6) || (piece == 'p' && r == 1)) { // check if pawn is on starting square
            if (inBounds(r + 2 * direction, f) && board[r + 2 * direction][f] == '.') { // then check two ahead
                moves.push_back(encodeMove(r, f, r + 2*direction, f));
            }
        }
    }

    // captures
    if (inBounds(r + direction, f - 1) && islower(board[r + direction][f - 1]) != islower(piece) && board[r + direction][f - 1] != '.') {
        moves.push_back(encodeMove(r, f, r + direction, f - 1));
    }

    if (inBounds(r + direction, f + 1) && islower(board[r + direction][f + 1]) != islower(piece) && board[r + direction][f + 1] != '.') {
        moves.push_back(encodeMove(r, f, r + direction, f + 1));
    }
    return moves;
}

vector<int> enumerateKnightMoves(int r, int f, char piece) { // list all possible knight moves for a given knight
    vector<int> moves;
    moves.reserve(8);
    int knightMoves[8][2] = {{2, -1}, {2, 1}, {-2, -1}, {-2, 1}, {1, -2}, {1, 2}, {-1, -2}, {-1, 2}}; // knight move patterns

    for (int i = 0; i < 8; i++) { // check each knight move pattern, capture or open square
        int tr = r + knightMoves[i][0];
        int tf = f + knightMoves[i][1];
        if (inBounds(tr, tf)) {
            if (board[tr][tf] == '.' || islower(board[tr][tf]) != islower(piece)) {
                moves.push_back(encodeMove(r, f, tr, tf));
            }
        }
    }
    return moves;
}

vector<int> enumerateBishopMoves(int r, int f, char piece) { // list all possible bishop moves for a given bishop
    vector<int> moves;
    moves.reserve(13);
    int bishopDirections[4][2] = {{1, 1}, {1, -1}, {-1, 1}, {-1, -1}}; // bishop move directions
    for (int i = 0; i < 4; i++) {
        int dr = bishopDirections[i][0];
        int df = bishopDirections[i][1];
        int tr = r + dr;
        int tf = f + df;
        while (inBounds(tr, tf)) { // keep moving in each direction until edge of board or blocked
            if (board[tr][tf] == '.') {
                moves.push_back(encodeMove(r, f, tr, tf));
            } else {
                if (islower(board[tr][tf]) != islower(piece)) { // capture if blocked by other color
                    moves.push_back(encodeMove(r, f, tr, tf));
                }
                break;
            }
            tr += dr;
            tf += df;
        }
    }
    return moves;
}

vector<int> enumerateRookMoves(int r, int f, char piece) { // list all possible rook moves for a given rook
    vector<int> moves;
    moves.reserve(14);
    int rookDirections[4][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}}; // rook move directions
    for (int i = 0; i < 4; i++) {
        int dr = rookDirections[i][0];
        int df = rookDirections[i][1];
        int tr = r + dr;
        int tf = f + df;
        while (inBounds(tr, tf)) { // keep moving in each direction until edge of board or blocked
            if (board[tr][tf] == '.') {
                moves.push_back(encodeMove(r, f, tr, tf));
            } else {
                if (islower(board[tr][tf]) != islower(piece)) { // capture if blocked by other color
                    moves.push_back(encodeMove(r, f, tr, tf));
                }
                break;
            }
            tr += dr;
            tf += df;
        }
    }
    return moves;
}

vector<int> enumerateQueenMoves(int r, int f, char piece) { // list all possible queen moves for a given queen
    vector<int> moves;
    moves.reserve(27);
    int queenDirections[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}}; // queen move directions
    for (int i = 0; i < 8; i++) {
        int dr = queenDirections[i][0];
        int df = queenDirections[i][1];
        int tr = r + dr;
        int tf = f + df;
        while (inBounds(tr, tf)) { // keep moving in each direction until edge of board or blocked
            if (board[tr][tf] == '.') {
                moves.push_back(encodeMove(r, f, tr, tf));
            } else {
                if (islower(board[tr][tf]) != islower(piece)) { // capture if blocked by other color
                    moves.push_back(encodeMove(r, f, tr, tf));
                }
                break;
            }
            tr += dr;
            tf += df;
        }
    }
    return moves;
}

vector<int> enumerateKingMoves(int r, int f, char piece) { // list all possible king moves for a given king
    vector<int> moves;
    moves.reserve(10);
    int kingDirections[8][2] = {{1, 0}, {-1, 0}, {0, 1}, {0, -1}, {1, 1}, {1, -1}, {-1, 1}, {-1, -1}}; // king move directions
    for (int i = 0; i < 8; i++) {
        int tr = r + kingDirections[i][0];
        int tf = f + kingDirections[i][1];
        if (inBounds(tr, tf)) {
            if (board[tr][tf] == '.' || islower(board[tr][tf]) != islower(piece)) {
                moves.push_back(encodeMove(r, f, tr, tf));
            }
        }
    }
  if (piece == 'K' && !whiteKingMoved) { // white castling
        if (!whiteLeftRookMoved && board[7][1] == '.' && board[7][2] == '.' && board[7][3] == '.') {
            moves.push_back(encodeMove(7, 4, 7, 2, 2)); // queenside, flag=2
        }
        if (!whiteRightRookMoved && board[7][5] == '.' && board[7][6] == '.') {
            moves.push_back(encodeMove(7, 4, 7, 6, 1)); // kingside, flag=1
        }
    }
    if (piece == 'k' && !blackKingMoved) { // black castling
        if (!blackLeftRookMoved && board[0][1] == '.' && board[0][2] == '.' && board[0][3] == '.') {
            moves.push_back(encodeMove(0, 4, 0, 2, 2)); // queenside, flag=2
        }
        if (!blackRightRookMoved && board[0][5] == '.' && board[0][6] == '.') {
            moves.push_back(encodeMove(0, 4, 0, 6, 1)); // kingside, flag=1
        }
    }
    return moves;
}

vector<int> enumeratePieceMoves(int r, int f) {
    vector<int> moves;
    char piece = board[r][f];
    if (piece == '.') return moves; // no piece

    switch (tolower(piece)) { // get moves for piece type
        case 'p': moves = enumeratePawnMoves(r, f, piece); break;
        case 'n': moves = enumerateKnightMoves(r, f, piece); break;
        case 'b': moves = enumerateBishopMoves(r, f, piece); break;
        case 'r': moves = enumerateRookMoves(r, f, piece); break;
        case 'q': moves = enumerateQueenMoves(r, f, piece); break;
        case 'k': moves = enumerateKingMoves(r, f, piece); break;
    }
    return moves; // return list of moves
}

vector<int> enumerateAllMoves(bool whiteToMove) {
    vector<int> moves;
    moves.reserve(50); // typical position has 30-40 legal moves

    bool whiteKingFound = false;
    bool blackKingFound = false;
    for (int r = 0; r < 8; r++) {
        for (int f = 0; f < 8; f++) {
            if (board[r][f] == 'K') {
                whiteKingFound = true;
            }
            if (board[r][f] == 'k') {
                blackKingFound = true;
            }
        }
    }

    if (!whiteKingFound || !blackKingFound) {
        return moves; // no legal moves if a king is dead
    }

    for (int r = 0; r < 8; r++) { // make every move for every piece of color
        for (int f = 0; f < 8; f++) {
            if (board[r][f] != '.' && (isupper(board[r][f]) == whiteToMove)) {
                vector<int> pieceMoves = enumeratePieceMoves(r, f);
                moves.insert(moves.end(), pieceMoves.begin(), pieceMoves.end());
            }
        }
    }
    return moves;
}

int getMoveScore(int move) {
    // Rank moves for better alpha-beta pruning
    int score = 0;
    int r = getFromRank(move);
    int f = getFromFile(move);
    int tr = getToRank(move);
    int tf = getToFile(move);
    char piece = board[r][f];
    char captured = board[tr][tf];

    // rank captures with MVV/LVA
    if (captured != '.') {
        int victimValue = 0;
        switch (tolower(captured)) {
            case 'p': victimValue = 1; break;
            case 'n': victimValue = 3; break;
            case 'b': victimValue = 3; break;
            case 'r': victimValue = 5; break;
            case 'q': victimValue = 9; break;
            case 'k': victimValue = 100; break;
        }
        
        int attackerValue = 0;
        switch (tolower(piece)) {
            case 'p': attackerValue = 1; break;
            case 'n': attackerValue = 3; break;
            case 'b': attackerValue = 3; break;
            case 'r': attackerValue = 5; break;
            case 'q': attackerValue = 9; break;
            case 'k': attackerValue = 100; break;
        }
        
        score = 1000 + (victimValue * 10) - attackerValue;
    }

    return score;
}

void orderMoves(vector<int>& moves) {
    // Sort moves by score descending (in-place, no copy)
    sort(moves.begin(), moves.end(), [](int a, int b) {
        return getMoveScore(a) > getMoveScore(b);
    });
}

int enumerateMoveTree(int depth, bool whiteToMove, int currentEval, int alpha = -10000000, int beta = 10000000) { // recursive evaluation with alpha-beta pruning
    if (depth == 0) return immediateEvaluation(); // base case

    vector<int> moves = enumerateAllMoves(whiteToMove); // get moves
    orderMoves(moves); // order moves for better time (in-place)
    
    if (whiteToMove) { // for white (maximizing player)
        int te = -10000000; // initial value
        for (int move : moves) {
            int r = getFromRank(move);
            int f = getFromFile(move);
            int tr = getToRank(move);
            int tf = getToFile(move);
            int flag = getMoveFlag(move);
            char movingPiece = board[r][f];
            char captured = board[tr][tf];
            board[tr][tf] = movingPiece;
            board[r][f] = '.';
            if (flag == 1) { // kingside castle
                board[7][5] = 'R';
                board[7][7] = '.';
                whiteCastled = true;
            } else if (flag == 2) { // queenside castle
                board[7][3] = 'R';
                board[7][0] = '.';
                whiteCastled = true;
            }
            int evaluation = enumerateMoveTree(depth - 1, false, currentEval, alpha, beta);
            board[r][f] = movingPiece; // undo move
            board[tr][tf] = captured;
            if (flag == 1) {
                board[7][7] = 'R';
                board[7][5] = '.';
                whiteCastled = false;

            } else if (flag == 2) {
                board[7][0] = 'R';
                board[7][3] = '.';
                whiteCastled = false;
            }
            te = max(te, evaluation);
            alpha = max(alpha, te); // update alpha
            if (beta <= alpha) break; // prune remaining branches
        }
        return te; // return evaluation
    } else { // for black (minimizing player)
        int te = 10000000; 
        for (int move : moves) {
            int r = getFromRank(move);
            int f = getFromFile(move);
            int tr = getToRank(move);
            int tf = getToFile(move);
            int flag = getMoveFlag(move);
            char movingPiece = board[r][f];
            char captured = board[tr][tf];
            board[tr][tf] = movingPiece;
            board[r][f] = '.';
            if (flag == 1) { // kingside castle
                board[0][5] = 'r';
                board[0][7] = '.';
                blackCastled = true;
            } else if (flag == 2) { // queenside castle
                board[0][3] = 'r';
                board[0][0] = '.';
                blackCastled = true;
            }
            int evaluation = enumerateMoveTree(depth - 1, true, currentEval, alpha, beta);
            board[r][f] = movingPiece;
            board[tr][tf] = captured;
            if (flag == 1) { // undo castling move
                board[0][7] = 'r';
                board[0][5] = '.';
                blackCastled = false;
            } else if (flag == 2) {
                board[0][0] = 'r';
                board[0][3] = '.';
                blackCastled = false;

            }
            te = min(te, evaluation);
            beta = min(beta, te); // update beta
            if (beta <= alpha) break; // prune remaining branches
        }
        return te;
    }
}

int selector(int depth, bool whiteToMove, int currentEval) { // select best move for either side
    vector<int> moves = enumerateAllMoves(whiteToMove);
    orderMoves(moves); // order moves for better time (in-place)

    int bestMove = 0;
    int te;
    if (whiteToMove) {
        te = -10000000;
    } else {
        te = 10000000;
    }
    
    for (int i = 0; i < moves.size(); i++) {
        int r = getFromRank(moves[i]);
        int f = getFromFile(moves[i]);
        int tr = getToRank(moves[i]);
        int tf = getToFile(moves[i]);
        int flag = getMoveFlag(moves[i]);
        
        char movingPiece = board[r][f];
        char captured = board[tr][tf];
        board[tr][tf] = movingPiece;
        board[r][f] = '.';
        
        // Handle castling
        if (whiteToMove) {
            if (flag == 1) { // kingside castle
                board[7][5] = 'R';
                board[7][7] = '.';
                whiteCastled = true;
            } else if (flag == 2) { // queenside castle
                board[7][3] = 'R';
                board[7][0] = '.';
                whiteCastled = true;
            }
        } else {
            if (flag == 1) { // kingside castle
                board[0][5] = 'r';
                board[0][7] = '.';
                blackCastled = true;
            } else if (flag == 2) { // queenside castle
                board[0][3] = 'r';
                board[0][0] = '.';
                blackCastled = true;
            }
        }
        
        int evaluation = enumerateMoveTree(depth - 1, !whiteToMove, currentEval);
        
        // Undo the move immediately
        board[r][f] = movingPiece;
        board[tr][tf] = captured;
        
        if (whiteToMove) {
            if (flag == 1) {
                board[7][7] = 'R';
                board[7][5] = '.';
                whiteCastled = false;
            } else if (flag == 2) {
                board[7][0] = 'R';
                board[7][3] = '.';
                whiteCastled = false;
            }
        } else {
            if (flag == 1) {
                board[0][7] = 'r';
                board[0][5] = '.';
                blackCastled = false;
            } else if (flag == 2) {
                board[0][0] = 'r';
                board[0][3] = '.';
                blackCastled = false;
            }
        }
        
        if (whiteToMove) {
            if (evaluation > te) {
                te = evaluation;
                bestMove = moves[i];
            }
        } else {
            if (evaluation < te) {
                te = evaluation;
                bestMove = moves[i];
            }
        }
    }
    
    return bestMove; // return best move
}

string convertToCoordinates(string algebraic) { // convert lan to coordinates
    string files = "abcdefgh";
    string ranks = "87654321";

    int fromRow = ranks.find(algebraic[1]);
    int fromCol = files.find(algebraic[0]); 
    int toRow = ranks.find(algebraic[3]);
    int toCol = files.find(algebraic[2]);

    return to_string(fromRow) + to_string(fromCol) + to_string(toRow) + to_string(toCol);
}

string convertToAlgebraic(string coordinates) { // convert coordinates to lan
    string files = "abcdefgh";
    string ranks = "87654321";

    int fromRow = coordinates[0] - '0';
    int fromCol = coordinates[1] - '0';
    int toRow = coordinates[2] - '0';
    int toCol = coordinates[3] - '0';

    return string(1, files[fromCol]) + string(1, ranks[fromRow]) + string(1, files[toCol]) + string(1, ranks[toRow]);
}

void whiteCastleCheck(int r, int f) {
    if (r == 7 && f == 4) whiteKingMoved = true;
    if (r == 7 && f == 0) whiteLeftRookMoved = true;
    if (r == 7 && f == 7) whiteRightRookMoved = true;
}

void blackCastleCheck(int r, int f) {
    if (r == 0 && f == 4) blackKingMoved = true;
    if (r == 0 && f == 0) blackLeftRookMoved = true;
    if (r == 0 && f == 7) blackRightRookMoved = true;
}

int main(int argc, char* argv[]) {
    cout.setf(ios::unitbuf); // Enable unbuffered output

    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <bots_directory>\n";
        return 1;
    }

    string botsDirectory = argv[1];

    cout << "Welcome to \033[1mPRISM Engine V0.7\033[0m\n";
    cout << "(C) 2025 Tommy Ciccone All Rights Reserved.\n";

    initializeBoard();

    cout << "Running in tournament mode\n";
    cout.flush();

    string whiteBot = botsDirectory + "/white_bot.txt";
    string blackBot = botsDirectory + "/black_bot.txt";
    
    cout << "Loading white bot from " << whiteBot << ".\n";
    cout << "Loading black bot from " << blackBot << ".\n";
    cout.flush();
    
    importPieceSquareTables(whiteBot);
    importPieceSquareTables(blackBot);
    
    bool whiteToMove = true;
    int moveCount = 0;
    const int maxMoves = 100; // prevent infinite games
    
    while (moveCount < maxMoves) {
        vector<int> moves = enumerateAllMoves(whiteToMove);
        
        if (moves.empty()) {
            // No legal moves: checkmate or stalemate
            int eval = immediateEvaluation();
            if (eval > 50000) {
                cout << "White wins by checkmate\n";
                cout.flush();
                return 1; // White wins
            } else if (eval < -50000) {
                cout << "Black wins by checkmate\n";
                cout.flush();
                return -1; // Black wins
            } else {
                cout << "Stalemate\n";
                int finalEval = immediateEvaluation();
                cout << "Final evaluation: " << finalEval << "\n";
                cout.flush();
                
                // Write final evaluation to file for tournament, to prevent repetitive draws
                ofstream evalFile("./match_temp/final_eval.txt");
                evalFile << finalEval;
                evalFile.close();
                
                return 0; // Draw
            }
        }
        
        int bestMove = selector(engineDepth, whiteToMove, immediateEvaluation());
        
        // Execute move
        int r = getFromRank(bestMove);
        int f = getFromFile(bestMove);
        int tr = getToRank(bestMove);
        int tf = getToFile(bestMove);
        int flag = getMoveFlag(bestMove);
        
        board[tr][tf] = board[r][f];
        board[r][f] = '.';
        
        if (whiteToMove) {
            whiteCastleCheck(r, f);
            if (flag == 1) { // kingside castle
                board[7][5] = 'R';
                board[7][7] = '.';
                whiteCastled = true;
            } else if (flag == 2) { // queenside castle
                board[7][3] = 'R';
                board[7][0] = '.';
                whiteCastled = true;
            }
        } else {
            blackCastleCheck(r, f);
            if (flag == 1) { // kingside castle
                board[0][5] = 'r';
                board[0][7] = '.';
                blackCastled = true;
            } else if (flag == 2) { // queenside castle
                board[0][3] = 'r';
                board[0][0] = '.';
                blackCastled = true;
            }
        }
        
        printBoard();
        int eval = immediateEvaluation();
        cout << "Evaluation: " << eval << "\n\n";
        cout.flush();
        
        // Check for checkmate
        if (eval > 50000) {
            cout << "White wins by checkmate\n";
            cout.flush();
            return 1; // White wins
        } else if (eval < -50000) {
            cout << "Black wins by checkmate\n";
            cout.flush();
            return -1; // Black wins
        }
        
        whiteToMove = !whiteToMove;
        moveCount++;
    }
    
    cout << "Game ended in draw by move limit\n";
    int finalEval = immediateEvaluation();
    cout << "Evaluation: " << finalEval << "\n";
    cout.flush();
    
    ofstream evalFile("./match_temp/final_eval.txt");
    evalFile << finalEval;
    evalFile.close();
    
    return 0;
}