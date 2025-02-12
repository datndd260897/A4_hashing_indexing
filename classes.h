#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <unordered_map>
#include <cstring>
#include <cmath>
#include <cctype>

using namespace std;

const int PAGE_SIZE = 4096;

class Record {
public:
    int id, manager_id; // Employee ID and their manager's ID
    string bio, name; // Fixed length string to store employee name and biography

    Record(vector<string> &fields) {
        id = stoi(fields[0]);
        name = fields[1];
        bio = fields[2];
        manager_id = stoi(fields[3]);
    }

    Record() {};

    // Function to get the size of the record
    int get_size() {
        // sizeof(int) is for name/bio size() in serialize function
        return sizeof(id) + sizeof(manager_id) + sizeof(int) + name.size() + sizeof(int) + bio.size(); 
    }

    // Function to serialize the record for writing to file
    string serialize() const {
        ostringstream oss;
        oss.write(reinterpret_cast<const char *>(&id), sizeof(id));
        oss.write(reinterpret_cast<const char *>(&manager_id), sizeof(manager_id));
        int name_len = name.size();
        int bio_len = bio.size();
        oss.write(reinterpret_cast<const char *>(&name_len), sizeof(name_len));
        oss.write(name.c_str(), name.size());
        oss.write(reinterpret_cast<const char *>(&bio_len), sizeof(bio_len));
        oss.write(bio.c_str(), bio.size());
        return oss.str();
    }

    // Function to deserialize a record from a binary string
    bool deserialize(const string &data) {
        istringstream iss(data);

        // Read id
        iss.read(reinterpret_cast<char *>(&id), sizeof(id));
        if (!iss) return false;

        // Read manager_id
        iss.read(reinterpret_cast<char *>(&manager_id), sizeof(manager_id));
        if (!iss) return false;

        // Read name length and name
        int name_len;
        iss.read(reinterpret_cast<char *>(&name_len), sizeof(name_len));
        if (!iss || name_len < 0) return false;

        name.resize(name_len);
        iss.read(&name[0], name_len);
        if (!iss) return false;

        // Read bio length and bio
        int bio_len;
        iss.read(reinterpret_cast<char *>(&bio_len), sizeof(bio_len));
        if (!iss || bio_len < 0) return false;

        bio.resize(bio_len);
        iss.read(&bio[0], bio_len);
        if (!iss) return false;

        return true;
    }


    void print() {
        cout << "\tID: " << id << "\n";
        cout << "\tNAME: " << name << "\n";
        cout << "\tBIO: " << bio << "\n";
        cout << "\tMANAGER_ID: " << manager_id << "\n";
    }
};

class Page {
public:
    vector<Record> records; // Data_Area containing the records
    vector<pair<int, int> > slot_directory; // Slot directory containing offset and size of each record
    int cur_size = 0; // Current size of the page including the overflow page pointer. if you also write the length of slot directory change it accordingly.
    int overflowPointerIndex;  // Initially set to -1, indicating the page has no overflow page. 
                               // Update it to the position of the overflow page when one is created.
    int p_index = -1;


    // Constructor
    Page() : overflowPointerIndex(-1) {
        cur_size = 0;
        p_index = -1;
    }

    // Function to insert a record into the page
    bool insert_record_into_page(Record r) {
        int record_size = r.get_size();
        int slot_size = sizeof(int) * 2;
        int current_slot_directory = slot_directory.size() * slot_size;
        int records_num_size = sizeof(int);
        int overflow_pointer_size = sizeof(int);

        if (cur_size + record_size + slot_size + current_slot_directory + records_num_size + overflow_pointer_size > 4096) { //Check if page size limit exceeded, considering slot directory size
            return false; // Cannot insert the record into this page
        } else {
            records.push_back(r);
             // TO_DO: update slot directory information
            slot_directory.push_back(make_pair(cur_size, record_size));
            cur_size += record_size; //Updated
            return true;
        }
    }
    // Function to write the page to a binary output stream, i.e., EmployeeRelation.dat file
    void write_into_data_file(ostream &out) const {
        //Write the records and slot directory information into your data file. You are basically writing 4KB into the datafile.
        //You must maintain a fixed size of 4KB so there may be some unused empty spaces.

        char page_data[4096] = {0};
        // Let's write all the information of the page into this char array. So that we can write the page into the data file in one go.
        int offset = 0;

        int dir_offset = 4096; // Adding directory at the bottom
        int num_slots = 0; // The number of slots that we are using for the current page

        for (const auto &record: records) {
            string serialized = record.serialize();

            memcpy(page_data + offset, serialized.c_str(), serialized.size());

            offset += serialized.size();
        }
        // Write the overflowPointer index
        dir_offset -= sizeof(overflowPointerIndex);
        memcpy(page_data + dir_offset, &overflowPointerIndex, sizeof(overflowPointerIndex));
        num_slots += slot_directory.size();
        dir_offset -= sizeof(int);
        memcpy(page_data + dir_offset, &num_slots, sizeof(int));


        for (const auto &slots: slot_directory) {
            int record_offset = slots.first;
            int record_size = slots.second;
            dir_offset -= sizeof(int); //MO ves the pointer of the directory
            memcpy(page_data + dir_offset, &record_offset, sizeof(int));
            dir_offset -= sizeof(int);
            memcpy(page_data + dir_offset, &record_size, sizeof(int));
        }
        out.seekp(p_index * PAGE_SIZE, ios::beg);
        out.write(page_data, 4096); // Always write exactly 4KB
        out.flush();
    }

    void clear() {
        records.clear();
        slot_directory.clear();
        cur_size = 0;
    }

    // Read a page from a binary input stream, i.e., EmployeeRelation.dat file to populate a page object
    bool read_from_data_file(istream &in, size_t page_position) {
        if (page_position == -1) {
            return false;
        }
        // Read all the records and slot_directory information from your .dat file
        // Remember you are reading a chunk of 4098 byte / 4 KB from the data file to your main memory.
        char page_data[4096] = {0};
        in.seekg(page_position * 4096, ios::beg);
        in.read(page_data, 4096);

        streamsize bytes_read = in.gcount();
        // You may populate the records and slot_directory from the 4 KB data you just read.
        if (bytes_read == 4096) {
            // Process data to fill the slot directory and the records to handle it according to the structure
            // Assuming slot directory is processed here or elsewhere depending on your serialization method
            // Reconstruct the slot directory
            // Start rebuilding the slot_directory from the bottom of the page
            int dir_offset = 4096;
            p_index = page_position;

            clear();
            // Read the overflowpointer
            dir_offset -= sizeof(int);
            int overflow_pointer;
            memcpy(&overflow_pointer, page_data + dir_offset, sizeof(overflow_pointer));

            // Read the number of slots
            dir_offset -= sizeof(int);
            int num_slots;
            memcpy(&num_slots, page_data + dir_offset, sizeof(int));
            if (num_slots == 0) {
                return false;
            }
            overflowPointerIndex = overflow_pointer;

            // Rebuild the slot directory
            for (int i = 0; i < num_slots; ++i) {
                dir_offset -= sizeof(int);
                int record_offset;
                memcpy(&record_offset, page_data + dir_offset, sizeof(int));

                dir_offset -= sizeof(int);
                int record_size;
                memcpy(&record_size, page_data + dir_offset, sizeof(int));

                slot_directory.push_back(make_pair(record_offset, record_size));
            }

            for (const auto &slots: slot_directory) {
                // Reconstruct the records
                int record_offset = slots.first;
                int record_size = slots.second;
                // Extract the serialized record data
                string record_string(page_data + record_offset, record_size);
                Record r;
                r.deserialize(record_string);
                records.push_back(r);
                cur_size += record_size;
            }

            return true;
        }

        if (bytes_read > 0) {
            cerr << "Incomplete read: Expected " << 4096 << " bytes, but only read " << bytes_read << " bytes." << endl;
        }

        // Reset the stream state for subsequent reads
        if (in.eof() || in.fail()) {
            in.clear(); // Clear EOF or error flags
            in.seekg(0, ios::end); // Move to the end of the file (optional reset position)
        }

        return false;
    }

};

class LinearHashIndex {
private:
    const size_t maxCacheSize = 1; // Maximum number of pages in the buffer
    const int Page_SIZE = 4096; // Size of each page in bytes
    int n;  // The number of indexes (pages) being used
    int i;	// The number of least-significant-bits of h(id) to check. Will need to increase i once n > 2^i
    int numRecords;    // Records currently in index. Used to test whether to increase n
    string fileName;
    const int RECORD_SIZE = 708;
    int over_flow_page_num = 0;
    long total_records_length = 0;

    // Function to compute hash value for a given ID
    int compute_hash_value(int id) {
        int max_bit_nums = (int) floor(log2(4096));
        // TODO: Implement the hash function h = id mod 2^5.
        return id & ((1 << max_bit_nums) - 1);
    }

    int flipFirstBit(int value) {
        return value ^ (1 << (i - 1)); // XOR to flip the first bit
    }

    // Function to add a new record to an existing page in the index file
    void addRecordToIndex(int pageIndex, Page &page, Record &record, bool isRehashing = false) {
        // Open index file in binary mode for updating
        fstream indexFile(fileName, ios::binary | ios::in | ios::out);
        if (!indexFile) {
            // If the file does not exist, create it
            indexFile.open(fileName, ios::binary | ios::in | ios::out | ios::trunc);
        }

        // TODO:
        // Add record to the index in the correct page, creating a overflow page if necessary

        // If record does not belong to the current page, so we have to write the current page to .dat
        // Read the page from .dat
        if (page.p_index != pageIndex) {
            // Write current page to .dat file
            if (page.p_index != -1) {
                page.write_into_data_file(indexFile);
            }
            // Read the new page to buffer and add record into it
            if (!page.read_from_data_file(indexFile, pageIndex)) {
                page = Page(); // Initialize the page
            }
            page.p_index = pageIndex; // Ensure page index is correctly set
        }
        // After staying at the correct index page
        // Insert record to the index page
        if (!page.insert_record_into_page(record)) {
            // If page is dirty Go to the last overflow_page
            page.write_into_data_file(indexFile);
            // Move to last overflowpage
            while (page.overflowPointerIndex != -1) {
                page.read_from_data_file(indexFile, page.overflowPointerIndex);
            }

            if (!page.insert_record_into_page(record)) {
                // current page or overflow page is full
                int overflow_pointer_index = pow(2, 12) + over_flow_page_num;
                page.overflowPointerIndex = overflow_pointer_index;
                page.write_into_data_file(indexFile);
                // set overflow pointer to the overflow region
                page = Page();
                page.p_index = overflow_pointer_index;
                over_flow_page_num ++;
                page.insert_record_into_page(record);
            }
        }

        if (!indexFile) {
            cerr << "Error: Unable to open index file for adding record." << endl;
            return;
        }


        numRecords++;
        total_records_length += record.get_size();
        // Check and Take neccessary steps if capacity is reached:
        if (!isRehashing) {
            OverflowHandler(indexFile, record);
        }
		// increase n; increase i (if necessary); place records in the new bucket that may have been originally misplaced due to a bit flip

        // Close the index file
        indexFile.close();
    }

    void rehashRecords(fstream &indexFile) {
        indexFile.seekg(0, ios::beg); // Rewind the data_file to the beginning for reading
        int page_number = 0;
        Page page;
        Page insert_page;
        while (page.read_from_data_file(indexFile, page_number)) {
            // Now process the current page using the slot directory to find the desired id
            // Process logic goes here
            for (Record& record: page.records) {
                int page_index = compute_hash_value(record.id) % (int)pow(2, i);
                if (page_index > n - 1) {
                    page_index = flipFirstBit(page_index);
                }
                addRecordToIndex(page_index, insert_page, record, true);
            }
            page_number++;
            if (indexFile.eof()) {
                break;
            }
        }
    }

    void relocateBitFlippedRecords(fstream &indexFile) {
        int oldBucket = flipFirstBit(n - 1); // Bucket affected by bit flip
        Page oldPage;
        Page newPage;
        vector<Record> toReinsert;
        vector<Record> retained;
        while (oldPage.read_from_data_file(indexFile, oldBucket)) {
            for (const auto &record : oldPage.records) {
                int newBucket = compute_hash_value(record.id) % (1 << i);
                if (newBucket == n - 1) {
                    toReinsert.push_back(record);
                } else {
                    retained.push_back(record);
                }
            }
            oldBucket = oldPage.overflowPointerIndex;
        }
        // Separate records into those that should stay and those to be moved

        // If there are records to be moved, reinsert them in the correct bucket
        if (!toReinsert.empty()) {
            for (auto &record : toReinsert) {
                addRecordToIndex(n - 1, newPage, record, true);
            }
            newPage.write_into_data_file(indexFile);
        }
    }


    void OverflowHandler(fstream &indexFile, Record &record) {
        // Compute average records per page
        float avg_rec_size = (total_records_length / numRecords) ;
        float total_page_size = n * PAGE_SIZE;
        float avg_records_per_page = (avg_rec_size * numRecords) / total_page_size;
        // Check if expansion is needed
        if (avg_records_per_page > 0.7 && n < 4096) {
            // Increase number of buckets
            n++;
            if (n > (1 << i)) {
                i++;
            }
            relocateBitFlippedRecords(indexFile);
            // if (n > (1 << i)) {  // If n exceeds 2^i, increase i and rehash
            //     i++;
            //     rehashRecords(indexFile);  // Full rehash and redistribution
            // } else {
            //     relocateBitFlippedRecords(indexFile);  // Only fix misplaced records
            // }
        }
    }

    // Function to search for a record by ID in a given page of the index file
    void searchRecordByIdInPage(int pageIndex, int id) {
        // Open index file in binary mode for reading
        ifstream indexFile(fileName, ios::binary | ios::in);
        if (!indexFile) {
            cerr << "Error: Unable to open index file." << endl;
            return;
        }
        bool found = false;
        while (true) {
            // Seek to the appropriate position in the index file
            //indexFile.seekg(pageIndex * Page_SIZE, ios::beg); in read+from_file

            // Read the page from the index file
            Page page;
            page.read_from_data_file(indexFile, pageIndex);
            for (Record& record: page.records) {
                if (record.id == id) {
                    found = true;
                    record.print();
                    return;}
            }
            if (found){
                break;
            }

            if (page.overflowPointerIndex == -1) {
                break;
            }
            else{
                pageIndex=page.overflowPointerIndex;
            }
        }
        if (!found){
            cerr << "Error: Employee with Id.: "<<id<< "Not found in page or overflow" << endl;
        }

        // TODO:
        //  - Search for the record by ID in the page
        //  - Check for overflow pages and report if record with given ID is not found
    }


public:
LinearHashIndex(string indexFileName) : numRecords(0), fileName(indexFileName) {
        n = 4; // Start with 4 buckets in index
        i = 2; // Need 2 bits to address 4 buckets
    }

    bool isHeaderLine(const std::string& line) {
        string substr = line.substr(0, 2);
        transform(substr.begin(), substr.end(), substr.begin(), ::toupper);
        return substr == "ID";  // Extract first 2 characters and compare
    }

    vector<string> get_fields(ifstream& csvFile, string& line, bool& insideQuotes, ostringstream& quotedField, string& item) {
        vector<string> fields;
        while (getline(csvFile, line)) {
            // Read the entire line
            stringstream ss(line);
            while (getline(ss, item, ',')) {
                // Process CSV fields
                if (insideQuotes) {
                    quotedField << "\n" << item;
                    if (!item.empty() && item.back() == '"') {
                        // Closing quote found
                        fields.push_back(quotedField.str());
                        insideQuotes = false;
                    }
                } else if (!item.empty() && item.front() == '"' && item.back() != '"') {
                    quotedField.str("");
                    quotedField << item;
                    insideQuotes = true;
                } else {
                    fields.push_back(item); // Normal field
                }
            }
            if (!insideQuotes) break; // Stop reading if a complete record is formed
        }
        return fields;
    }

    Record create_record(ifstream& csvFile, string& line) {
        bool insideQuotes = false;
        ostringstream quotedField;
        string item;
        Record result;
        try {
            vector<string> fields = get_fields(csvFile, line, insideQuotes, quotedField, item);
            if (!fields.empty()) {
                // **Try to create a Record object**
                try {
                    result = Record(fields);  // Constructor may throw an exception if invalid
                } catch (const std::exception& e) {
                    cout << "Error creating record: " << e.what() << endl;
                }
            }
        } catch (const exception& e) {
            cout << "Error processing CSV row: " << e.what() << endl;
        }
        return result;
    }

    // Function to create hash index from Employee CSV file
    void createFromFile(std::string csvFileName) {
        // Open the CSV file for reading
        ifstream csvFile(csvFileName);
        if (!csvFile.is_open()) {
            cout << "Error: Unable to open CSV file: " << csvFileName << endl;
            return;
        }

        string line;
        Page page;

        // **Peek at the first line to check if it's a header**
        if (getline(csvFile, line)) {
            if (isHeaderLine(line)) {
                cout << "Detected header, skipping: " << line << endl;
            } else {
                csvFile.seekg(0);  // Reset file stream if no header detected
            }
        }

        // Read each line from the CSV file
        while (csvFile.peek() != EOF) {  // Ensure proper handling of multi-line records
            Record record = create_record(csvFile, line);
            // TODO:
            //   - Compute hash value for the record's ID using compute_hash_value() function.
            //   - Insert the record into the appropriate page in the index file using addRecordToIndex() function.
            int page_index = compute_hash_value(record.id) % (int)pow(2, i);
            if (page_index > n-1) {
                page_index = flipFirstBit(page_index);
            }
            addRecordToIndex(page_index, page, record);
        }
        // Write the last record to page
        fstream indexFile(fileName, ios::binary | ios::in | ios::out);
        page.write_into_data_file(indexFile);
        cout << "Number of n: " << n << endl;
        // Close the CSV file
        csvFile.close();
    }

    void findAndPrintEmployee(int id) {
        // Open index file in binary mode for reading
        ifstream indexFile(fileName, ios::binary | ios::in);
        // TODO:

        int pageIndex = compute_hash_value(id) % (int)pow(2, i); //  - Compute hash value for the given ID using compute_hash_value() function
        if (pageIndex > n-1) {
          pageIndex = flipFirstBit(pageIndex);
        }
        Record record;
        searchRecordByIdInPage(pageIndex, id); //  - Search for the record in the page corresponding to the hash value using searchRecordByIdInPage() function
        // Close the index file
        indexFile.close();
    }
};

