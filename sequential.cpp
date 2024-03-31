#include <iostream>
#include <vector>
#include <string>
#include <queue>
#include <fstream>
#include <map>
#include <algorithm>
#include <chrono>
#include <unistd.h>

using namespace std;

// Structure to represent traffic signal data
struct traffic_data {
    std::string timestamp;
    int light_id;
    int num_cars;
};

queue<traffic_data> signal_queue; // Queue for storing traffic light data

// Map to store traffic data for each timestamp
map<string, vector<traffic_data>> timestamp_data;

// Comparison function used for sorting traffic data based on the number of cars
bool sort_method(const traffic_data& first, const traffic_data& second) {
    return first.num_cars > second.num_cars;
}

// Producer function
void producer() {
    ifstream infile("traffic_data.txt");
    if (!infile.is_open()) {
        cout << "Error: Unable to open file." << endl;
        return;
    }

    std::string id, times, lightID, Cars_No;
    while (getline(infile, id, ',') &&
           getline(infile, times, ',') &&
           getline(infile, lightID, ',') &&
           getline(infile, Cars_No, '\n')) {
        traffic_data td;
        td.timestamp = times;
        td.light_id = stoi(lightID);
        td.num_cars = stoi(Cars_No);

        signal_queue.push(td);

        sleep(rand() % 5);
    }
    infile.close();
}

// Consumer function
void consumer() {
    while (!signal_queue.empty()) {
        traffic_data td = signal_queue.front();
        signal_queue.pop();

        // Store traffic data for each timestamp
        timestamp_data[td.timestamp].push_back(td);

        sleep(rand() % 5); // Simulate some processing time
    }

    // Print summary
    for (auto& entry : timestamp_data) {
        cout << "Timestamp: " << entry.first << endl;
        cout << "Top 5 Entries:" << endl;
        // Sort traffic data for this timestamp
        sort(entry.second.begin(), entry.second.end(), sort_method);
        // Print top 5 entries
        int count = 0;
        for (int i = 0; i < entry.second.size() && count < 5; ++i) {
            if (entry.second[i].light_id != entry.second[i + 1].light_id) {
                cout << "Traffic Light ID: " << entry.second[i].light_id << " | Number of Cars: " << entry.second[i].num_cars << endl;
                count++;
            }
        }
        cout << endl;
    }
}

int main() {
    auto start = chrono::steady_clock::now();

    // Producer
    producer();

    // Consumer
    consumer();

    auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "Total execution time: " << duration << " milliseconds" << endl;

    return 0;
}
