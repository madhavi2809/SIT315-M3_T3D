#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <string>
#include <queue>
#include <fstream>
#include <map>
#include <algorithm>
#include <chrono>
#include <unistd.h>

using namespace std;

mutex MUTEX; // Mutex for protecting shared resources

int PThreads = 4;      // Number of producer threads
int CThreads = 4;      // Number of consumer threads
int Hours = 12;        // Duration after which to print summary
int ProducerCount = 0; // Counter for producer threads
int consumerCount = 0; // Counter for consumer threads
int m = 0;             // Number of rows in traffic data

condition_variable producer_cv, consumer_cv;

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

// Producer thread function
void *producer(void *args) {
    ifstream infile("traffic_data.txt");
    if (!infile.is_open()) {
        cout << "Error: Unable to open file." << endl;
        return nullptr;
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

        unique_lock<mutex> lock(MUTEX);
        signal_queue.push(td);
        lock.unlock();
        producer_cv.notify_all();

        ProducerCount++;
        sleep(rand() % 5);
    }
    infile.close();
    return nullptr;
}

// Consumer thread function
void *consumer(void *args) {
    while (consumerCount < m) {
        unique_lock<mutex> lock(MUTEX);

        if (!signal_queue.empty()) {
            traffic_data td = signal_queue.front();
            signal_queue.pop();

            // Process traffic data
            // Update totals of each traffic light here
            // This part needs modification according to the structure of your data
            cout << "Processing: " << td.timestamp << ", " << td.light_id << ", " << td.num_cars << endl;

            // Store traffic data for each timestamp
            timestamp_data[td.timestamp].push_back(td);

            consumerCount++;
        } else {
            consumer_cv.wait(lock, [] { return !signal_queue.empty(); });
        }

        // Print summary if necessary
        if (consumerCount % (Hours * PThreads) == 0 && consumerCount != 0) {
            // Print top 5 sorted entries for each timestamp
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

        lock.unlock();
        sleep(rand() % 5); // Simulate some processing time
    }
    return nullptr;
}

int main() {
    auto start = chrono::steady_clock::now();

    pthread_t producers[PThreads];
    pthread_t consumers[CThreads];

    // Count the number of rows in traffic data
    ifstream infile("traffic_data.txt");
    string line;
    while (getline(infile, line)) {
        m++;
    }
    infile.close();

    // Create producer threads
    for (long i = 0; i < PThreads; i++)
        pthread_create(&producers[i], NULL, producer, (void *)i);

    // Create consumer threads
    for (long i = 0; i < CThreads; i++)
        pthread_create(&consumers[i], NULL, consumer, NULL);

    // Join producer threads
    for (long i = 0; i < PThreads; i++)
        pthread_join(producers[i], NULL);

    // Join consumer threads
    for (long i = 0; i < CThreads; i++)
        pthread_join(consumers[i], NULL);

    auto end = chrono::steady_clock::now();
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start).count();
    cout << "Total execution time: " << duration << " milliseconds" << endl;

    return 0;
}
