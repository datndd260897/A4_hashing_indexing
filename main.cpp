
#include <string>
#include <iostream>
#include <stdexcept>
#include "classes.h"

using namespace std;


int main(int argc, char* const argv[]) {

    // Create the index
    LinearHashIndex hashIndex("EmployeeIndex");
    hashIndex.createFromFile("Employee (1).csv");

    for (int i = 1; i < argc; ++i) {
        int searchId = stoi(argv[i]);
        cout << "\nSearching Employee ID " << searchId << ": ";
        hashIndex.findAndPrintEmployee(searchId);
    }


    // TODO: You'll receive employee IDs as arguments, process them to retrieve the record, or display a message if not found. 

    return 0;
}

