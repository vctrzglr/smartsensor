#pragma once
#include "Sensor.h"
#include <vector>
#include <map>
#include <string>
using namespace std;

class SensorStation {
private:
    string name;
    vector<Sensor*> sensoren;
    map<string, Messverlauf> verlaeufe;

public:
    SensorStation(string n);
    ~SensorStation();

    void sensorHinzufuegen(Sensor* s);

    // overloaded: measure all, or only active sensors
    void allesMessen(int tick);
    void allesMessen(int tick, bool nurAktive);

    void statusAnzeigen() const;
    void verlaufAnzeigen() const;
    void statistikAnzeigen() const;

    int getAnzahlSensoren() const;
};
