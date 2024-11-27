
#include <iostream>
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>
#include <limits>
#include <algorithm>
#include <cmath>
#include <curl/curl.h>
#include "json.hpp"
using namespace std;

enum EmergencySeverity {
    FIRE = 1,
    MEDICAL_EMERGENCY = 2,
    CRIME = 3,
    OTHER_EMERGENCY = 4
};

enum ResourceType {
    FIRE_BRIGADE,
    AMBULANCE,
    POLICE_VAN
};

// Graph Node structure
struct GraphNode {
    string id;
    double latitude;
    double longitude;
    ResourceType type;
    bool isAvailable;

    GraphNode(const string& nodeId, double lat, double lon, ResourceType resourceType)
        : id(nodeId), latitude(lat), longitude(lon), type(resourceType), isAvailable(true) {}
};

// Emergency Incident Structure
struct EmergencyIncident {
    string place;
    EmergencySeverity severity;
    double latitude;
    double longitude;

    EmergencyIncident(const string& p, EmergencySeverity s, double lat, double lon)
        : place(p), severity(s), latitude(lat), longitude(lon) {}
};

// Comparison function for priority queue based on severity
struct CompareIncident {
    bool operator()(const EmergencyIncident& a, const EmergencyIncident& b) {
        return a.severity > b.severity;
    }
};

// Write callback function for CURL response
size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ((string*)userp)->append((char*)contents, size * nmemb);
    return size * nmemb;
}

// Function to get route from OSRM API
string getRouteFromOSRM(double startLat, double startLon, double endLat, double endLon) {
    CURL *curl;
    CURLcode res;
    string response;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if (curl) {
        string url = "http://router.project-osrm.org/route/v1/driving/" +
                          to_string(startLon) + "," + to_string(startLat) + ";" +
                          to_string(endLon) + "," + to_string(endLat) +
                          "?overview=false&steps=true";
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());

        // Capture response in string
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        // Perform the request
        res = curl_easy_perform(curl);

        // Check for errors
        if (res != CURLE_OK) {
            cerr << "Request failed: " << curl_easy_strerror(res) << endl;
        }

        // Clean up
        curl_easy_cleanup(curl);
    }

    curl_global_cleanup();

    return response;
}


void printRouteTabFormat(const string& routeJson) {
    try {
        // Parse the JSON response
        auto jsonResponse = nlohmann::json::parse(routeJson);

        // Validate the structure of the response
        if (!jsonResponse.contains("routes") || jsonResponse["routes"].empty()) {
            cout << "No routes available in the response." << endl;
            return;
        }

        auto route = jsonResponse["routes"][0];
        if (!route.contains("legs") || route["legs"].empty()) {
            cout << "No legs available in the route." << endl;
            return;
        }

        auto legs = route["legs"][0]["steps"];
        if (legs.empty()) {
            cout << "No steps available in the route leg." << endl;
            return;
        }

        // Print the header of the table
        cout << "+--------+------------------------------+-------------------+" << endl;
        cout << "| Step   | Instruction                  | Distance (meters) |" << endl;
        cout << "+--------+------------------------------+-------------------+" << endl;

        // Print each step in the route
        int stepNumber = 1;
        for (auto& step : legs) {
            // Safely access instruction and distance
            string instruction = step.contains("maneuver") && step["maneuver"].contains("instruction")
                ? step["maneuver"]["instruction"].get<string>()
                : "FOLLOW THE ROAD";
            double distance = step.contains("distance") ? step["distance"].get<double>() : 0.0;

            cout << "| " << stepNumber << "      | " << instruction
                      << " | " << distance << "           |" <<endl;
            stepNumber++;
        }

       cout << "+--------+------------------------------+-------------------+" << endl;
    } catch (const exception& e) {
        cerr << "Error parsing route JSON: " << e.what() << endl;
    }
}
void printRouteInTabFormat2(const string& routeJson) {
    try {
        // Parse the JSON response
        auto jsonResponse = nlohmann::json::parse(routeJson);

        // Validate the structure of the response
        if (!jsonResponse.contains("routes") || jsonResponse["routes"].empty()) {
            cout << "No routes available in the response." << endl;
            return;
        }

        auto route = jsonResponse["routes"][0];
        if (!route.contains("legs") || route["legs"].empty()) {
            cout << "No legs available in the route." << endl;
            return;
        }

        auto legs = route["legs"][0]["steps"];
        if (legs.empty()) {
            cout << "No steps available in the route leg." << endl;
            return;
        }

        cout << "\n"
                  << "================================= ROUTE DETAILS =================================\n";
        cout << "+--------+-----------------------------------------+---------------------+--------------+\n";
        cout << "| Step   | Instruction                             | Distance (meters)   | Duration (s) |\n";
        cout << "+--------+-----------------------------------------+---------------------+--------------+\n";


        int stepNumber = 1;
        double totalDuration = 0.0; // Total duration in seconds
        for (auto& step : legs) {
            // Safely access instruction, distance, and duration
            string instruction = step.contains("maneuver") && step["maneuver"].contains("instruction")
                ? step["maneuver"]["instruction"].get<string>()
                : "Follow the road";
            double distance = step.contains("distance") ? step["distance"].get<double>() : 0.0;
            double duration = step.contains("duration") ? step["duration"].get<double>() : 0.0;

            totalDuration += duration; // Sum up the duration

            // Print each step
            cout << "| " << setw(6) << stepNumber << " | " << setw(39) << instruction.substr(0, 39)
                      << " | " << setw(19) << fixed << setprecision(1) << distance
                      << " | " << setw(12) << fixed << setprecision(1) << duration << " |\n";
            stepNumber++;
        }


        cout << "+--------+-----------------------------------------+---------------------+--------------+\n";

        // Calculate ETA (convert seconds to minutes and seconds)
        int etaMinutes = static_cast<int>(totalDuration) / 60;
        int etaSeconds = static_cast<int>(totalDuration) % 60;

        // Highlight ETA
        cout << "\n"
                  << "==================================== ETA ======================================\n";
        cout << "| Estimated Time of Arrival (ETA): " << etaMinutes << " minutes and " << etaSeconds << " seconds |\n";
        cout << "==============================================================================\n\n";
    } catch (const exception& e) {
        cerr << "Error parsing route JSON: " << e.what() << endl;
    }
}

void printRouteInTabularFormatWithTraffic(const string& routeJson, const vector<double>& trafficFactors) {
    try {
        // Parse the JSON response
        auto jsonResponse = nlohmann::json::parse(routeJson);

        // Validate the structure of the response
        if (!jsonResponse.contains("routes") || jsonResponse["routes"].empty()) {
            cout << "No routes available in the response." << endl;
            return;
        }

        auto route = jsonResponse["routes"][0];
        if (!route.contains("legs") || route["legs"].empty()) {
            cout << "No legs available in the route." << endl;
            return;
        }

        auto legs = route["legs"][0]["steps"];
        if (legs.empty()) {
            cout << "No steps available in the route leg." << endl;
            return;
        }

        // Ensure traffic factors match the number of steps
        if (trafficFactors.size() != legs.size()) {
            cerr << "Traffic factor size does not match the number of route steps!" << endl;
            return;
        }

        // Header and Border
        cout << "\n"
                  << "=============================== ROUTE DETAILS WITH TRAFFIC ===============================\n";
        cout << "+--------+-----------------------------------------+---------------------+--------------+-------------------+\n";
        cout << "| Step   | Instruction                             | Distance (meters)   | Duration (s) | Traffic Factor    |\n";
        cout << "+--------+-----------------------------------------+---------------------+--------------+-------------------+\n";

        // Print each step in the route and calculate total traffic-adjusted duration
        int stepNumber = 1;
        double totalOriginalDuration = 0.0;   // Total original duration in seconds
        double totalTrafficDuration = 0.0;   // Total duration adjusted for traffic
        for (size_t i = 0; i < legs.size(); ++i) {
            auto& step = legs[i];

            // Safely access instruction, distance, and duration
            string instruction = step.contains("maneuver") && step["maneuver"].contains("instruction")
                ? step["maneuver"]["instruction"].get<string>()
                : "Follow the road";
            double distance = step.contains("distance") ? step["distance"].get<double>() : 0.0;
            double duration = step.contains("duration") ? step["duration"].get<double>() : 0.0;

            double trafficFactor = trafficFactors[i];
            double trafficAdjustedDuration = duration * trafficFactor;

            totalOriginalDuration += duration;            // Sum up the original duration
            totalTrafficDuration += trafficAdjustedDuration; // Sum up the traffic-adjusted duration

            // Print each step
            cout << "| " << setw(6) << stepNumber << " | " << setw(39) << instruction.substr(0, 39)
                      << " | " << setw(19) << fixed << setprecision(1) << distance
                      << " | " << setw(12) << fixed << setprecision(1) << duration
                      << " | " << setw(17) << fixed << setprecision(2) << trafficFactor << " |\n";
            stepNumber++;
        }

        // Footer
        cout << "+--------+-----------------------------------------+---------------------+--------------+-------------------+\n";

        // Calculate original and traffic-adjusted ETA
        int originalEtaMinutes = static_cast<int>(totalOriginalDuration) / 60;
        int originalEtaSeconds = static_cast<int>(totalOriginalDuration) % 60;
        int trafficEtaMinutes = static_cast<int>(totalTrafficDuration) / 60;
        int trafficEtaSeconds = static_cast<int>(totalTrafficDuration) % 60;

        // Highlight ETA
        cout << "\n"
                  << "============================= ESTIMATED TIME OF ARRIVAL =============================\n";
        cout << "| Original ETA: " << originalEtaMinutes << " minutes and " << originalEtaSeconds << " seconds                             |\n";
        cout << "| Traffic-Adjusted ETA: " << trafficEtaMinutes << " minutes and " << trafficEtaSeconds << " seconds                        |\n";
        cout << "===================================================================================\n\n";
    } catch (const exception& e) {
        cerr << "Error parsing route JSON: " << e.what() << endl;
    }
}



// Emergency Response System class
class EmergencyResponseSystem {
private:
    vector<GraphNode> resourceGraph;
    unordered_map<string, vector<pair<string, double>>> adjacencyList;
    priority_queue<EmergencyIncident, vector<EmergencyIncident>, CompareIncident> incidentQueue;

    double haversineDistance(double lat1, double lon1, double lat2, double lon2) {
        const double R = 6371; // Earth's radius in kilometers
        double dLat = (lat2 - lat1) * M_PI / 180.0;
        double dLon = (lon2 - lon1) * M_PI / 180.0;
        double a = sin(dLat / 2) * sin(dLat / 2) +
                   cos(lat1 * M_PI / 180.0) * cos(lat2 * M_PI / 180.0) *
                   sin(dLon / 2) * sin(dLon / 2);
        double c = 2 * atan2(sqrt(a), sqrt(1 - a));
        return R * c;
    }

    void buildGraphConnections() {
        for (size_t i = 0; i < resourceGraph.size(); ++i) {
            for (size_t j = i + 1; j < resourceGraph.size(); ++j) {
                double distance = haversineDistance(
                    resourceGraph[i].latitude, resourceGraph[i].longitude,
                    resourceGraph[j].latitude, resourceGraph[j].longitude
                );

                if (distance <= 20.0) {
                    adjacencyList[resourceGraph[i].id].push_back({resourceGraph[j].id, distance});
                    adjacencyList[resourceGraph[j].id].push_back({resourceGraph[i].id, distance});
                }
            }
        }
    }

    GraphNode* findBestResource(const EmergencyIncident& incident) {
        GraphNode* bestResource = nullptr;
        double minDistance = numeric_limits<double>::max();

        for (auto& node : resourceGraph) {
            if (node.type == getResourceTypeForSeverity(incident.severity) && node.isAvailable) {
                double distance = haversineDistance(
                    incident.latitude, incident.longitude,
                    node.latitude, node.longitude
                );

                if (distance < minDistance) {
                    minDistance = distance;
                    bestResource = &node;
                }
            }
        }

        return bestResource;
    }

    ResourceType getResourceTypeForSeverity(EmergencySeverity severity) {
        switch (severity) {
            case FIRE: return FIRE_BRIGADE;
            case MEDICAL_EMERGENCY: return AMBULANCE;
            case CRIME: return POLICE_VAN;
            default: return POLICE_VAN;
        }
    }

public:
    EmergencyResponseSystem() {
        resourceGraph = {
            {"Fire_Connaught", 28.6304, 77.2177, FIRE_BRIGADE},
            {"Fire_Karol", 28.6487, 77.1900, FIRE_BRIGADE},
            {"Fire_Dwarka", 28.5595, 77.0553, FIRE_BRIGADE},
            {"Ambulance_Moti", 28.5916, 77.2022, AMBULANCE},
            {"Ambulance_Sarai", 28.6478, 77.1945, AMBULANCE},
            {"Ambulance_Khichdi", 28.5398, 77.0146, AMBULANCE},
            {"Police_Kashmiri", 28.6253, 77.2192, POLICE_VAN},
            {"Police_Alaknanda", 28.5541, 77.2483, POLICE_VAN},
            {"Police_Ashok", 28.5839, 77.2189, POLICE_VAN}
        };

        buildGraphConnections();
    }

    void addIncident(const EmergencyIncident& incident) {
        incidentQueue.push(incident);
    }

    /*void dispatchResources() {
        while (!incidentQueue.empty()) {
            EmergencyIncident incident = incidentQueue.top();
            incidentQueue.pop();

            GraphNode* bestResource = findBestResource(incident);
            if (bestResource) {
                bestResource->isAvailable = false;

                cout << "Dispatching resource " << bestResource->id << " to incident at " << incident.place << endl;

                // Get the route from OSRM
                string routeJson = getRouteFromOSRM(
                    bestResource->latitude, bestResource->longitude,
                    incident.latitude, incident.longitude
                );

                // Print the route in tabular format
                printRouteInTabularFormatWithTraffic(routeJson);

            } else {
                cout << "No available resources for incident at " << incident.place << endl;
            }
        }
    }*/
    void dispatchResources() {
    while (!incidentQueue.empty()) {
        EmergencyIncident incident = incidentQueue.top();
        incidentQueue.pop();

        GraphNode* bestResource = findBestResource(incident);
        if (bestResource) {
            bestResource->isAvailable = false;

            cout << "Dispatching resource " << bestResource->id << " to incident at " << incident.place << endl;

            // Get the route from OSRM
            string routeJson = getRouteFromOSRM(
                bestResource->latitude, bestResource->longitude,
                incident.latitude, incident.longitude
            );

            // Generate mock traffic factors (e.g., random factors between 0.8 and 1.2)
            vector<double> trafficFactors;
            auto jsonResponse = nlohmann::json::parse(routeJson);
            if (jsonResponse.contains("routes") && !jsonResponse["routes"].empty()) {
                auto route = jsonResponse["routes"][0];
                if (route.contains("legs") && !route["legs"].empty()) {
                    auto legs = route["legs"][0]["steps"];
                    for (size_t i = 0; i < legs.size(); ++i) {
                        // Get a random traffic factor between 0.8 and 1.2
                        trafficFactors.push_back(0.8 + static_cast<double>(rand()) / RAND_MAX * 0.4);
                    }
                }
            }

            // Print the route in tabular format with traffic factors
            printRouteInTabularFormatWithTraffic(routeJson, trafficFactors);

        } else {
            cout << "No available resources for incident at " << incident.place << endl;
        }
    }
}

};


int main() {
    cout<<"                           --------------------EMERGENCY RESPONSE SYSTEM---------------------"<<endl;
    cout<<"   The Emergency Response System (ERS) is a software designed to assist individuals and organizations in responding"<< endl;
    cout<<"   effectively to emergency situations. It aims to provide timely alerts, location tracking, and resource management"<<endl;
    cout<<"                                to minimize the impact of disasters and emergency events"<<endl<<endl;
    EmergencyResponseSystem system;
    int ch=1,code;
    string place;
    float c1,c2;
    EmergencySeverity es;
    while(ch){
        cout << "Enter the place: ";
        getline(std::cin, place);
        cout << "Entered place: " << place <<endl;
        cout << "Enter Emergency: "<< endl<<"1. Fire"<< endl << "2. Medical" << endl <<"3. Crime" << endl <<"4. Other"<< endl;
        cin>>code;
        cin.ignore(numeric_limits<std::streamsize>::max(), '\n');
        if(code>4 || code<1){
            cout<<"Enter valid code!!!!";
            continue;
        }
        cout<<endl<<"Enter the coordinates: ";
        cin>>c1>>c2;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
        if(code==1){
            es=FIRE;
        }
        else if(code==2){
            es=MEDICAL_EMERGENCY;
        }
        else if(code==3){
            es=CRIME;
        }
        else{
            es=OTHER_EMERGENCY;
        }
        system.addIncident({place,es,c1,c2});
        cout<<"Any Other Assistance Required: 1/0    ";
        cin>>ch;
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

       system.dispatchResources();


    return 0;
}
/*system.addIncident({"Connaught Place", FIRE, 28.6300, 77.2170});
    system.addIncident({"Karol Bagh", MEDICAL_EMERGENCY, 28.6517, 77.1910});
    system.addIncident({"Dwarka", CRIME, 28.5971, 77.0582});



    system.dispatchResources();
    system.addIncident({"Rohini", FIRE, 30.5968, 78.0582});
*/
