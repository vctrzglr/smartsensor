#include <iostream>
#include <cstdlib>
#include <ctime>
#include "SensorStation.h"
#include "Sensoren.h"

using namespace std;

int main() {
    srand(static_cast<unsigned int>(time(0)));

    cout << "==========================================" << endl;
    cout << "   Smart Home Sensor Simulation" << endl;
    cout << "   Simuliert 24 Stunden (Tick = Stunde)" << endl;
    cout << "==========================================" << endl;

    SensorStation station("Wohnzimmer");

    // two temperature sensors: different base values and offsets
    station.sensorHinzufuegen(new TemperaturSensor("Innentemperatur",  21.0, 3.0));
    station.sensorHinzufuegen(new TemperaturSensor("Aussentemperatur", 14.0, 7.0, -1.5));

    // keep pointer to control licht and co2 during simulation
    LichtSensor* licht = new LichtSensor("Lichtsensor", 50000.0);
    station.sensorHinzufuegen(licht);

    station.sensorHinzufuegen(new LuftfeuchtigkeitsSensor("Luftfeuchtigkeit", 55.0, 12.0));

    CO2Sensor* co2 = new CO2Sensor("CO2-Sensor", 420.0);
    co2->setFensterOffen(false);
    station.sensorHinzufuegen(co2);

    station.statusAnzeigen();

    cout << "\n--- Starte Simulation ---" << endl;

    for (int tick = 0; tick < 24; tick++) {
        // light sensor is inactive at night
        if (tick == 6)  licht->aktivieren();
        if (tick == 22) licht->deaktivieren();

        // open window at 8:00, close again at 18:00
        if (tick == 8) {
            cout << "\n  >> Fenster wird geoeffnet" << endl;
            co2->setFensterOffen(true);
        }
        if (tick == 18) {
            cout << "\n  >> Fenster wird geschlossen" << endl;
            co2->setFensterOffen(false);
        }

        station.allesMessen(tick);
    }

    station.statistikAnzeigen();

    return 0;
}
