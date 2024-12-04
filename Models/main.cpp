#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <sstream>
#include "model_ethanol.h"
// #include "model_shift.h"

using namespace std;

// to compile: g++ -std=c++11 -o main main.cpp

Eloquent::ML::Port::RandomForest classifier;

float X[6] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0};

int main() {

    bool skip = true;
    int count = 0;
    int outlier_detection = 0;
    std::string filename = "./Datasets/X_test.csv"; // Nome do seu arquivo CSV
    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cerr << "Erro ao abrir o arquivo." << std::endl;
        return 1;
    }

    std::vector<std::vector<double>> data;
    std::string line;

    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::string token;
        std::vector<double> values;

        while (std::getline(iss, token, ',')) {
            try {
                double value = std::stod(token);
                values.push_back(value);
            } catch (const std::exception& e) {
                std::cerr << "Erro ao converter valor: " << token << std::endl;
            }
        }
        
        if (!values.empty()) {
            data.push_back(values);
        }
    }

    file.close();

    // print len of data
    std::cout << "Data size: " << data.size() << std::endl;

    
    // iterate over the data
    for (auto& values : data) {

        X[0] = values[0];
        X[1] = values[1];
        X[2] = values[2];
        X[3] = values[3];
        X[4] = values[4];
        X[5] = values[5];

        // Predict the class
        int prediction = classifier.predict(X);

        // create a new file with the prediction
        std::ofstream output_file;
        output_file.open("output_test.csv", std::ios_base::app);
        output_file << prediction << std::endl;
        output_file.close();
        
    }

    return 0;
}
