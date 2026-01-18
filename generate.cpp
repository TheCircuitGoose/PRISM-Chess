#include <iostream>
#include <fstream>
#include <string>
#include <random>

using namespace std;

// Generate random bot
void generateRandomBot(const string& filename) {
    ofstream outFile(filename);
    if (!outFile) {
        cout << "Error: Could not create file " << filename << "\n";
        return;
    }
    
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> matDis(5, 200);      // Material values range
    uniform_int_distribution<> pstDis(-100, 100);   // Position table values range
    
    // Generate material values (6 values)
    for (int i = 0; i < 6; i++) {
        outFile << matDis(gen);
        if (i < 5) outFile << " ";
    }
    outFile << "\n";
    
    // Generate position PST (48 groups of 8 = 384 values)
    int count = 0;
    for (int piece = 0; piece < 6; piece++) {
        for (int rank = 0; rank < 8; rank++) {
            for (int file = 0; file < 8; file++) {
                outFile << pstDis(gen);
                count++;
                if (count % 8 == 0) {
                    outFile << "\n";
                } else {
                    outFile << " ";
                }
            }
        }
    }
    
    // Generate neighbor PST (54 groups of 6 = 324 values)
    count = 0;
    for (int piece = 0; piece < 6; piece++) {
        for (int neighbor = 0; neighbor < 6; neighbor++) {
            for (int row = 0; row < 3; row++) {
                for (int col = 0; col < 3; col++) {
                    outFile << pstDis(gen);
                    count++;
                    if (count % 6 == 0) {
                        outFile << "\n";
                    } else {
                        outFile << " ";
                    }
                }
            }
        }
    }

    outFile.close();
    cout << "Created: " << filename << "\n";
}

int main(int argc, char* argv[]) {
    cout.setf(ios::unitbuf); // Enable unbuffered output
    
    if (argc != 3) {
        cout << "Usage: " << argv[0] << " <quantity> <output_directory>\n";
        return 1;
    }
    
    int quantity = stoi(argv[1]);
    string outputDir = argv[2];
    
    // Remove trailing slash if present
    if (outputDir.back() == '/') {
        outputDir.pop_back();
    }
    
    cout << "Generating " << quantity << " random bots...\n";
    cout.flush();
    
    for (int i = 0; i < quantity; i++) {
        string filename = outputDir + "/bot_" + to_string(i) + ".txt";
        generateRandomBot(filename);
    }
    
    cout << "Successfully generated " << quantity << " bots in " << outputDir << "/\n";
    cout.flush();
    
    return 0;
}
