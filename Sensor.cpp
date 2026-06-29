#include "Sensor.h"
#include <iostream>
#include <iomanip>
#include <algorithm>

// --- Messwert ---

Messwert::Messwert() : wert(0.0), einheit("?"), tick(0) {}
Messwert::Messwert(double w, string e, int t) : wert(w), einheit(e), tick(t) {}

double Messwert::getWert() const    { return wert; }
string Messwert::getEinheit() const { return einheit; }
int    Messwert::getTick() const    { return tick; }

void Messwert::anzeigen() const {
    cout << fixed << setprecision(2) << wert << " " << einheit
         << "  (Tick " << setw(2) << tick << ")";
}

// --- Messverlauf ---

Messverlauf::Messverlauf() : sensorName("unbekannt") {}
Messverlauf::Messverlauf(string name) : sensorName(name) {}

void Messverlauf::hinzufuegen(const Messwert& mw) {
    messungen.push_back(mw);
}

void Messverlauf::anzeigen() const {
    anzeigen(static_cast<int>(messungen.size()));
}

void Messverlauf::anzeigen(int letzteN) const {
    cout << "  [" << sensorName << "]" << endl;
    int start = max(0, static_cast<int>(messungen.size()) - letzteN);
    for (int i = start; i < static_cast<int>(messungen.size()); i++) {
        cout << "    Tick " << setw(2) << messungen[i].getTick() << ":  ";
        messungen[i].anzeigen();
        cout << endl;
    }
}

double Messverlauf::getDurchschnitt() const {
    if (messungen.empty()) return 0.0;
    double summe = 0.0;
    for (const Messwert& mw : messungen) summe += mw.getWert();
    return summe / messungen.size();
}

double Messverlauf::getMin() const {
    if (messungen.empty()) return 0.0;
    double minWert = messungen[0].getWert();
    for (const Messwert& mw : messungen)
        if (mw.getWert() < minWert) minWert = mw.getWert();
    return minWert;
}

double Messverlauf::getMax() const {
    if (messungen.empty()) return 0.0;
    double maxWert = messungen[0].getWert();
    for (const Messwert& mw : messungen)
        if (mw.getWert() > maxWert) maxWert = mw.getWert();
    return maxWert;
}

int    Messverlauf::getAnzahl() const    { return static_cast<int>(messungen.size()); }
string Messverlauf::getSensorName() const { return sensorName; }

// --- Sensor ---

Sensor::Sensor(string n, string e, double o)
    : name(n), einheit(e), aktiv(true), offset(o) {}

void   Sensor::aktivieren()       { aktiv = true; }
void   Sensor::deaktivieren()     { aktiv = false; }
bool   Sensor::isAktiv() const    { return aktiv; }
string Sensor::getName() const    { return name; }
string Sensor::getEinheit() const { return einheit; }
double Sensor::getOffset() const  { return offset; }

void Sensor::anzeigen() const {
    cout << "  [" << getTyp() << "] " << name
         << "  |  Einheit: " << einheit
         << "  |  Status: "  << (aktiv ? "aktiv" : "inaktiv")
         << "  |  Offset: "  << offset << endl;
}
