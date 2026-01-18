#include <iostream>
#include <fstream>
#include <string>
#include <random>
#include <vector>

using namespace std;

// Read bot file and return all values
vector<int> readBotFile(const string& filename) {
    vector<int> values;
    ifstream inFile(filename);
    if (!inFile) {
        cout << "Error: Could not open file " << filename << "\n";
        return values;
    }
    
    int value;
    while (inFile >> value) {
        values.push_back(value);
    }
    
    inFile.close();
    return values;
}

// Write bot file with given values
void writeBotFile(const string& filename, const vector<int>& values) {
    ofstream outFile(filename);
    if (!outFile) {
        cout << "Error: Could not create file " << filename << "\n";
        return;
    }
    
    // Write material values (6 values)
    for (int i = 0; i < 6 && i < values.size(); i++) {
        outFile << values[i];
        if (i < 5) outFile << " ";
    }
    outFile << "\n";
    
    // Write position PST (next 384 values: 48 groups of 8)
    int count = 0;
    for (int i = 6; i < 390 && i < values.size(); i++) {
        outFile << values[i];
        count++;
        if (count % 8 == 0) {
            outFile << "\n";
        } else {
            outFile << " ";
        }
    }
    
    // Write neighbor PST (next 324 values: 54 groups of 6)
    count = 0;
    for (int i = 390; i < values.size(); i++) {
        outFile << values[i];
        count++;
        if (count % 6 == 0) {
            outFile << "\n";
        } else {
            outFile << " ";
        }
    }
    
    outFile.close();
}

// Generate mutated bot
void generateMutatedBot(const string& inputFile, const string& outputFile, double mutationFactor) {
    vector<int> originalValues = readBotFile(inputFile);
    if (originalValues.empty()) {
        return;
    }
    
    random_device rd;
    mt19937 gen(rd());
    
    vector<int> mutatedValues = originalValues;
    
    for (int i = 0; i < mutatedValues.size(); i++) {
        uniform_real_distribution<> mutateChance(0.0, 1.0);
        
        if (mutateChance(gen) < mutationFactor) {
            // Apply mutation: add/subtract a random value
            uniform_int_distribution<> mutationAmount(-50, 50);
            mutatedValues[i] += mutationAmount(gen);
            
            // Clamp values in range
            if (i < 6) {
                // Material values: keep between 1 and 1000
                mutatedValues[i] = max(1, min(1000, mutatedValues[i]));
            } else {
                // PST values: keep between -200 and 200
                mutatedValues[i] = max(-200, min(200, mutatedValues[i]));
            }
        }
    }
    
    writeBotFile(outputFile, mutatedValues);
    cout << "Created: " << outputFile << "\n";
}

int main(int argc, char* argv[]) {
    cout.setf(ios::unitbuf); // Enable unbuffered output
    
    if (argc != 5) {
        cout << "Usage: " << argv[0] << " <input_bot> <quantity> <mutation_factor> <output_directory>\n";
        return 1;
    }
    
    string inputBot = argv[1];
    int quantity = stoi(argv[2]);
    double mutationFactor = stod(argv[3]);
    string outputDir = argv[4];
    
    // check factor
    if (mutationFactor < 0.0 || mutationFactor > 1.0) {
        cout << "Error: Mutation factor must be between 0.0 and 1.0\n";
        return 1;
    }
    
    // Remove trailing slash if present
    if (outputDir.back() == '/') {
        outputDir.pop_back();
    }
    
    cout << "Generating " << quantity << " mutated bots from " << inputBot << " with factor " << mutationFactor << "\n";
    cout.flush();
    
    for (int i = 1; i < quantity; i++) {
        string filename = outputDir + "/bot_" + to_string(i) + ".txt";
        generateMutatedBot(inputBot, filename, mutationFactor);
    }
    
    cout << "Successfully generated " << quantity - 1 << " mutated bots in " << outputDir << "/\n";
    cout.flush();
    
    return 0;
}