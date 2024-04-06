#include <iostream>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <string.h>
#include <queue>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <chrono>

using namespace std;

mutex mutex1;

// Struct for traffic data row
struct tr_signal
{
    int ind;
    std::string t_stamp;
    int tr_id;
    int num_cars;
};

// Global variables
vector<int> in;
vector<int> tr_light;
vector<int> no_cars;
vector<string> tstamp;
int m = 0; // Number of rows
int hour_ind = 12; // Number of rows representing 5 minutes (assuming 12 rows per 5 minutes)

// Function to sort traffic light data
bool sort_method(struct tr_signal first, struct tr_signal second)
{
    if (first.num_cars > second.num_cars)
        return true;
    return false;
}

// Function to print sorted traffic light data
void print_sorted_traffic(const tr_signal* tlSorter, const std::string& timestamp)
{
    cout << "Traffic lights sorted according to most busy | Time: " << timestamp << endl;
    cout << "Traf Lgt" << setw(20) << "Number of Cars" << endl;
    for (int i = 0; i < 4; ++i)
    {
        cout << setw(3) << tlSorter[i].tr_id << setw(20) << tlSorter[i].num_cars << endl;
    }
}

// Function to process traffic data
void process_traffic_data(int p_num_threads, int c_num_threads)
{
    // Initialize counters
    int ccount = 0;
    int con_count = 0;
    string last_timestamp = "";

    // Initialize queue to store traffic light data
    queue<tr_signal> tr_sig_queue;

    // Array to hold the totals of each of the 4 traffic lights
    tr_signal tlSorter[4] = {{0, "", 1, 0}, {0, "", 2, 0}, {0, "", 3, 0}, {0, "", 4, 0}};

    condition_variable producer_cv, consumer_cv; // Initializing condition variables for prod and cons

    // Producer function
    auto produce = [&]() {
        while (ccount < m) {
            unique_lock<mutex> lk(mutex1); // Locking until producer finishes processing 

            if (ccount < m) { // If count is less than the number of rows in the dataset 
                tr_sig_queue.push(tr_signal{in[ccount], tstamp[ccount], tr_light[ccount], no_cars[ccount]}); // Push into queue
                consumer_cv.notify_all(); // Notifying consumer threads
                ccount++;
            } else {
                producer_cv.wait(lk, [&]{ return ccount < m; }); // If count is greater than the number of rows in the data set wait
            }

            lk.unlock(); // Unlock after processing
            sleep(rand()%3);
        }
    };

    // Consumer function
    auto consume = [&]() {
        while (con_count < m) {
            unique_lock<mutex> lk(mutex1); // Lock until processing

            if (!tr_sig_queue.empty()) {
                tr_signal sig = tr_sig_queue.front(); // Getting the front elements of the queue

                // Add the number of cars into the respective traffic light id
                if (sig.tr_id == 1) {
                    tlSorter[0].num_cars += sig.num_cars;
                } else if (sig.tr_id == 2) {
                    tlSorter[1].num_cars += sig.num_cars;
                } else if (sig.tr_id == 3) {
                    tlSorter[2].num_cars += sig.num_cars;
                } else if (sig.tr_id == 4) {
                    tlSorter[3].num_cars += sig.num_cars;
                }

                tr_sig_queue.pop(); // Pop the data
                producer_cv.notify_all(); // Notify producer
                con_count++;

                if (last_timestamp != "" && sig.t_stamp != last_timestamp) {
                    sort(tlSorter, tlSorter + 4, sort_method);
                    print_sorted_traffic(tlSorter, sig.t_stamp);
                }
                last_timestamp = sig.t_stamp;
            } else {
                consumer_cv.wait(lk, [&]{ return !tr_sig_queue.empty(); }); // If queue empty, wait until producer produces
            }

            lk.unlock();
            sleep(rand()%3);
        }
    };

    // Start time
    auto start = chrono::high_resolution_clock::now();

    // Create producer threads
    vector<thread> producer_threads(p_num_threads);
    for (auto& t : producer_threads) {
        t = thread(produce);
    }

    // Create consumer threads
    vector<thread> consumer_threads(c_num_threads);
    for (auto& t : consumer_threads) {
        t = thread(consume);
    }

    // Join producer threads
    for (auto& t : producer_threads) {
        t.join();
    }

    // Join consumer threads
    for (auto& t : consumer_threads) {
        t.join();
    }

    // End time
    auto end = chrono::high_resolution_clock::now();

    // Calculate elapsed time
    auto elapsed_seconds = chrono::duration_cast<chrono::seconds>(end - start);

    cout << "Total Execution Time: " << elapsed_seconds.count() << " seconds" << endl;
}

// Function to get data from file
void get_traffic_data()
{
    ifstream infile("data.txt");

    if (infile.is_open())
    {
        std::string line;
        getline(infile, line); // Skipping header line

        while (getline(infile, line))
        {
            istringstream iss(line);
            string ind, t_stamp, tr_light_id, no_of_cars;
            getline(iss, ind, ',');
            getline(iss, t_stamp, ',');
            getline(iss, tr_light_id, ',');
            getline(iss, no_of_cars, '\n');

            in.push_back(stoi(ind));
            tstamp.push_back(t_stamp);
            tr_light.push_back(stoi(tr_light_id));
            no_cars.push_back(stoi(no_of_cars));

            m += 1; // Increment row count
        }
        infile.close();
    }
    else
    {
        cerr << "Could not open file." << endl;
        exit(1);
    }
}

int main()
{
    // Read traffic data from file
    get_traffic_data();

    // Run scenario: Using 12 threads (6 for producer, 6 for consumer)
    cout << "Scenario: Using 12 threads (6 for producer, 6 for consumer)" << endl;
    process_traffic_data(6, 6);

    return 0;
}
