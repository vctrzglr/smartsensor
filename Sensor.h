#pragma once
#include <string>
#include <vector>
using namespace std;

// --- Messwert ---

class Messwert {
private:
    double wert;
    string einheit;
    int tick;

public:
    Messwert();
    Messwert(double w, string e, int t);

    double getWert() const;
    string getEinheit() const;
    int    getTick() const;

    void anzeigen() const;
};

// --- Messverlauf ---

class Messverlauf {
private:
    string sensorName;
    vector<Messwert> messungen;

public:
    Messverlauf();
    Messverlauf(string name);

    void hinzufuegen(const Messwert& mw);

    void anzeigen() const;
    void anzeigen(int letzteN) const;

    double getDurchschnitt() const;
    double getMin() const;
    double getMax() const;
    int    getAnzahl() const;
    string getSensorName() const;
};

// --- Sensor (Basisklasse) ---

class Sensor {
private:
    string name;
    string einheit;
    bool   aktiv;
    double offset;

public:
    Sensor(string n, string e, double offset = 0.0);
    virtual ~Sensor() = default;

    virtual Messwert messen(int tick) const = 0;
    virtual string   getTyp() const = 0;

    void   aktivieren();
    void   deaktivieren();
    bool   isAktiv() const;
    string getName() const;
    string getEinheit() const;
    double getOffset() const;

    void anzeigen() const;
};
