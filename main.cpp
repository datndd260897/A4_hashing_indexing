#include <string>
#include <iostream>
#include <stdexcept>
#include <chrono>  // Include chrono for timing
#include "classes.h"

using namespace std;
using namespace std::chrono;

int main(int argc, char* const argv[]) {
    std::string filename = "EmployeeIndex.dat";
    if (std::FILE *file = std::fopen(filename.c_str(), "r")) {
        std::fclose(file);
        std::remove(filename.c_str());
    }
    // Start measuring time
    auto start = high_resolution_clock::now();

    // Create the index
    LinearHashIndex hashIndex("EmployeeIndex.dat");
    hashIndex.createFromFile("Employee.csv");

    for (int i = 1; i < argc; ++i) {
        int searchId = stoi(argv[i]);
        cout << "\nSearching Employee ID " << searchId << ": ";
        hashIndex.findAndPrintEmployee(searchId);
    }

    // Stop measuring time
    auto end = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(end - start);

    // Print execution time
    cout << "\nExecution Time: " << duration.count() << " ms\n";
    hashIndex.print();
    return 0;
}
