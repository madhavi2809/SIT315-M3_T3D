#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <algorithm>
#include <iomanip>
#include <mpi.h>

// Structure to represent traffic signal data
struct TrafficSignal
{
    int row_index;
    std::string time_stamp;
    int light_id;
    int car_count;
};

// Function to read traffic data from file
void read_traffic_data(const std::string &filename, std::vector<TrafficSignal> &traffic_signals)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Error: Unable to open file: " << filename << std::endl;
        return;
    }

    std::string line;
    while (std::getline(file, line))
    {
        std::istringstream iss(line);
        TrafficSignal signal;
        if (iss >> signal.time_stamp >> signal.light_id >> signal.car_count)
        {
            traffic_signals.push_back(signal);
        }
        else
        {
            std::cerr << "Error: Invalid data format in file: " << filename << std::endl;
            traffic_signals.clear();
            break;
        }
    }
    file.close();
}

// Function to find top congested traffic lights
void find_top_congested_lights(const std::string &timestamp, const std::vector<TrafficSignal> &traffic_signals)
{
    std::vector<TrafficSignal> signals_for_timestamp;
    for (const auto &signal : traffic_signals)
    {
        if (signal.time_stamp == timestamp)
        {
            signals_for_timestamp.push_back(signal);
        }
    }

    if (!signals_for_timestamp.empty())
    {
        // Sort signals based on car count
        std::sort(signals_for_timestamp.begin(), signals_for_timestamp.end(),
                  [](const TrafficSignal &a, const TrafficSignal &b)
                  { return a.car_count > b.car_count; });

        std::cout << "Top 5 congested traffic lights for timestamp " << timestamp << ":" << std::endl;
        for (size_t i = 0; i < std::min<size_t>(5, signals_for_timestamp.size()); ++i)
        {
            std::cout << "Light ID: " << signals_for_timestamp[i].light_id
                      << ", Number of Cars: " << signals_for_timestamp[i].car_count << std::endl;
        }
        std::cout << std::endl;
    }
    else
    {
        std::cout << "No data available for timestamp " << timestamp << std::endl;
    }
}

int main(int argc, char **argv)
{
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    const std::string filename = "traffic_data.txt";

    std::vector<TrafficSignal> traffic_signals;
    std::vector<TrafficSignal> process_traffic_signals;

    if (rank == 0)
    {
        read_traffic_data(filename, traffic_signals);

        // Scatter data to all processes
        int num_items_per_process = traffic_signals.size() / size;
        int remainder = traffic_signals.size() % size;
        std::vector<int> send_counts(size, num_items_per_process);
        for (int i = 0; i < remainder; ++i)
        {
            send_counts[i]++;
        }
        std::vector<int> displacements(size, 0);
        for (int i = 1; i < size; ++i)
        {
            displacements[i] = displacements[i - 1] + send_counts[i - 1];
        }

        process_traffic_signals.resize(send_counts[0]); // Resize to hold the data for the master process
        MPI_Scatterv(traffic_signals.data(), send_counts.data(), displacements.data(), MPI_BYTE,
                     process_traffic_signals.data(), send_counts[0] * sizeof(TrafficSignal), MPI_BYTE,
                     0, MPI_COMM_WORLD);
    }
    else
    {
        // Slave processes receive data from master
        int num_items;
        MPI_Status status;
        MPI_Probe(0, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
        MPI_Get_count(&status, MPI_BYTE, &num_items);
        process_traffic_signals.resize(num_items / sizeof(TrafficSignal));
        MPI_Recv(process_traffic_signals.data(), num_items, MPI_BYTE, 0, MPI_ANY_TAG, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    // Synchronize all processes
    MPI_Barrier(MPI_COMM_WORLD);

    double start_time = MPI_Wtime();

    // Process data
    for (int hour = 0; hour < 8; ++hour)
    {
        std::ostringstream oss;
        oss << std::setw(2) << std::setfill('0') << hour << ":00:00";
        std::string timestamp = oss.str();
        find_top_congested_lights(timestamp, process_traffic_signals);
    }

    // Gather results from all processes to master
    std::vector<TrafficSignal> all_traffic_signals(traffic_signals.size());
    MPI_Gatherv(process_traffic_signals.data(), process_traffic_signals.size() * sizeof(TrafficSignal), MPI_BYTE,
                all_traffic_signals.data(), process_traffic_signals.size() * sizeof(TrafficSignal), MPI_BYTE,
                0, MPI_COMM_WORLD);

    double end_time = MPI_Wtime();
    double execution_time = end_time - start_time;

    if (rank == 0)
    {
        std::cout << "Total execution time: " << execution_time << " seconds.\n"
                  << std::endl;
    }

    MPI_Finalize();
    return 0;
}
