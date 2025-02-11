
#include <string>
#include <iostream>
#include <stdexcept>
#include "classes.h"



void removeExistingFiles() {
    for (int i = 1; i <= 12; ++i) {
        std::string filename = "EmployeeIndex" + std::to_string(i) + ".dat";
        if (std::FILE *file = std::fopen(filename.c_str(), "r")) {
            std::fclose(file);
            std::remove(filename.c_str());
        }
    }
}

int main(int argc, char* const argv[]) {
    removeExistingFiles();

    // Create the index
    LinearHashIndex hashIndex("EmployeeIndex");
    hashIndex.createFromFile("Employee.csv");

    for (int i = 1; i < argc; ++i) {
        int searchId = stoi(argv[i]);
        cout << "\nSearching Employee ID " << searchId << ": ";
        hashIndex.findAndPrintEmployee(searchId);
    }


    // TODO: You'll receive employee IDs as arguments, process them to retrieve the record, or display a message if not found. 

    return 0;
}

