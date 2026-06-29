#include "SensorStation.h"
#include <iostream>
#include <iomanip>

SensorStation::SensorStation(string n) : name(n) {}

SensorStation::~SensorStation() {
    for (Sensor* s : sensoren) delete s;
}

void SensorStation::sensorHinzufuegen(Sensor* s) {
    sensoren.push_back(s);
    verlaeufe[s->getName()] = Messverlauf(s->getName());
}

void SensorStation::allesMessen(int tick) {
    allesMessen(tick, false);
}

void SensorStation::allesMessen(int tick, bool nurAktive) {
    cout << "\n--- Tick " << setw(2) << tick
         << " Uhr  [" << name << "] ---" << endl;

    for (Sensor* s : sensoren) {
        if (nurAktive && !s->isAktiv()) continue;

        if (!s->isAktiv()) {
            cout << "  " << setw(26) << left << s->getName()
                 << ": [inaktiv]" << endl;
            continue;
        }

        Messwert mw = s->messen(tick);
        cout << "  " << setw(26) << left << s->getName() << ": ";
        mw.anzeigen();
        cout << endl;
        verlaeufe[s->getName()].hinzufuegen(mw);
    }
}

void SensorStation::statusAnzeigen() const {
    cout << "\n========================================" << endl;
    cout << "  Station: " << name << endl;
    cout << "  Sensoren: " << sensoren.size() << " gesamt" << endl;
    cout << "========================================" << endl;
    for (Sensor* s : sensoren) s->anzeigen();
}

void SensorStation::verlaufAnzeigen() const {
    cout << "\n=== Messverlauf ===" << endl;
    for (const auto& eintrag : verlaeufe) {
        eintrag.second.anzeigen();
        cout << endl;
    }
}

void SensorStation::statistikAnzeigen() const {
    cout << "\n========================================" << endl;
    cout << "  Statistik: " << name << endl;
    cout << "========================================" << endl;
    cout << fixed << setprecision(2);

    for (const auto& eintrag : verlaeufe) {
        const Messverlauf& mv = eintrag.second;
        if (mv.getAnzahl() == 0) continue;

        cout << "  " << mv.getSensorName() << ":" << endl;
        cout << "    Messungen : " << mv.getAnzahl() << endl;
        cout << "    Minimum   : " << mv.getMin()   << endl;
        cout << "    Maximum   : " << mv.getMax()   << endl;
        cout << "    Durchschn.: " << mv.getDurchschnitt() << endl;
    }
}

int SensorStation::getAnzahlSensoren() const {
    return static_cast<int>(sensoren.size());
}
