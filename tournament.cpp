#include <iostream>
#include <vector>
#include <string>
#include <map>
#include <algorithm>
#include <random>
#include <cstdlib>
#include <cstdio>
#include <fstream>
#include <sstream>

using namespace std;

// Get all bot files from a directory
vector<string> getBotFiles(const string& directory) {
    vector<string> bots;
    
    string command = "ls -1 " + directory + "/*.txt 2>/dev/null";
    FILE* pipe = popen(command.c_str(), "r");
    
    if (!pipe) {
        cout << "Error: Could not read directory\n";
        return bots;
    }
    
    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        string line(buffer);
        // Remove newline
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();
        }
        if (!line.empty()) {
            bots.push_back(line);
        }
    }
    
    pclose(pipe);
    
    // Sort
    sort(bots.begin(), bots.end());
    return bots;
}

// Shuffle bots
void shuffleBots(vector<string>& bots) {
    random_device rd;
    mt19937 gen(rd());
    shuffle(bots.begin(), bots.end(), gen);
}

// Extract filename
string getFilename(const string& path) {
    size_t lastSlash = path.find_last_of("/");
    if (lastSlash == string::npos) {
        return path;
    }
    return path.substr(lastSlash + 1);
}

// Run a match between two bots, returns result and sets finalEval for draws
int runMatch(const string& whiteBot, const string& blackBot, int& finalEval) {
    // Put two bots into the folder tournament reads from
    system("mkdir -p ./match_temp");
    string cpWhite = "cp \"" + whiteBot + "\" ./match_temp/white_bot.txt";
    string cpBlack = "cp \"" + blackBot + "\" ./match_temp/black_bot.txt";
    system(cpWhite.c_str());
    system(cpBlack.c_str());
    
    int result = system("./prism-tournament ./match_temp");
    
    // get previous evaluation if draw (to prevent repetitive draws)
    finalEval = 0;
    ifstream evalFile("./match_temp/final_eval.txt");
    if (evalFile.is_open()) {
        evalFile >> finalEval;
        evalFile.close();
    }
    
    // Clean up files
    system("rm -rf ./match_temp");
    
    // system returns 256 for some reason
    if (result == 256) {
        return 1; // White wins
    } else if (result == 0) {
        return 0; // Draw
    } else {
        return -1; // Black wins
    }
}

int main(int argc, char* argv[]) {
    cout.setf(ios::unitbuf); // Enable unbuffered output
    
    if (argc != 2) {
        cout << "Usage: " << argv[0] << " <bots_directory>\n";
        return 1;
    }
    
    string botsDir = argv[1];
    
    // Remove trailing slash
    if (botsDir.back() == '/') {
        botsDir.pop_back();
    }
    
    cout << "Starting tournament.\n";
    cout.flush();
    
    vector<string> currentRound = getBotFiles(botsDir);
    
    if (currentRound.empty()) {
        cout << "Error: No bots found in " << botsDir << "\n";
        return 1;
    }
    
    cout << "Found " << currentRound.size() << " bots\n";
    cout.flush();
    
    int roundNumber = 1;
    map<string, int> consecutiveTies;
    map<string, vector<int>> tieEvaluations; // track evaluations for repetitive ties
    
    // keep running until only 1 remains
    while (currentRound.size() > 1) {
        cout << "\nRound " << roundNumber << "\n";
        cout << "Bots remaining: " << currentRound.size() << "\n";
        
        // shuffle for random pairings
        shuffleBots(currentRound);
        
        vector<string> nextRound;
        bool repairRound = false; // if need to replay due to repetitive ties with no clear victor
        
        // pair bots and run matches
        for (size_t i = 0; i < currentRound.size(); i += 2) {
            if (i + 1 < currentRound.size()) {
                string bot1 = currentRound[i];
                string bot2 = currentRound[i + 1];
                
                // bot names for display
                string name1 = getFilename(bot1);
                string name2 = getFilename(bot2);
                
                cout << "\nMatch: " << name1 << " (white) vs " << name2 << " (black)\n";
                cout.flush();
                
                int finalEval = 0;
                int result = runMatch(bot1, bot2, finalEval);
                
                if (result == 1) {
                    cout << "Winner: " << name1 << " (white)\n";
                    cout.flush();
                    nextRound.push_back(bot1);
                    consecutiveTies[bot1] = 0;
                    consecutiveTies[bot2] = 0;
                    tieEvaluations.erase(bot1);
                    tieEvaluations.erase(bot2);
                    // Delete loser
                    string deleteCmd = "rm \"" + bot2 + "\"";
                    system(deleteCmd.c_str());
                } else if (result == -1) {
                    cout << "Winner: " << name2 << " (black)\n";
                    cout.flush();
                    nextRound.push_back(bot2);
                    consecutiveTies[bot1] = 0;
                    consecutiveTies[bot2] = 0;
                    tieEvaluations.erase(bot1);
                    tieEvaluations.erase(bot2);
                    // Delete loser
                    string deleteCmd = "rm \"" + bot1 + "\"";
                    system(deleteCmd.c_str());
                } else {
                    cout << "Draw (Eval: " << finalEval << ")\n";
                    cout.flush();
                    
                    // ties: take the higher eval, or randomly select if eval is 0
                    if (finalEval > 0) {
                        cout << "White (" << name1 << ") has higher eval, advances\n";
                        cout.flush();
                        nextRound.push_back(bot1);
                        // Delete loser
                        string deleteCmd = "rm \"" + bot2 + "\"";
                        system(deleteCmd.c_str());
                    } else if (finalEval < 0) {
                        cout << "Black (" << name2 << ") has higher eval, advances\n";
                        cout.flush();
                        nextRound.push_back(bot2);
                        // Delete loser
                        string deleteCmd = "rm \"" + bot1 + "\"";
                        system(deleteCmd.c_str());
                    } else {
                        // Evaluation is exactly 0. pick winner randomly
                        random_device rd;
                        mt19937 gen(rd());
                        uniform_int_distribution<> dis(0, 1);
                        int randomWinner = dis(gen);
                        
                        if (randomWinner == 0) {
                            cout << "Eval is 0. " << name1 << " wins by random selection\n";
                            cout.flush();
                            nextRound.push_back(bot1);
                            // Delete loser
                            string deleteCmd = "rm \"" + bot2 + "\"";
                            system(deleteCmd.c_str());
                        } else {
                            cout << "Eval is 0. " << name2 << " wins by random selection\n";
                            cout.flush();
                            nextRound.push_back(bot2);
                            // Delete loser
                            string deleteCmd = "rm \"" + bot1 + "\"";
                            system(deleteCmd.c_str());
                        }
                    }
                }
            } else {
                // byes
                string botName = getFilename(currentRound[i]);
                cout << "\nBye: " << botName << " advances without playing\n";
                nextRound.push_back(currentRound[i]);
            }
        }
        
        currentRound = nextRound;
        roundNumber++;
    }
    
    // tournament complete
    cout << "\nTournament Complete.\n";
    string winner = getFilename(currentRound[0]);
    cout << "Champion: " << winner << "\n";
    cout.flush();
    
    return 0;
}
