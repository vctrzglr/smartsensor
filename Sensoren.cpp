#include "Sensoren.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>

// --- TemperaturSensor ---

TemperaturSensor::TemperaturSensor(string name, double basis, double amp, double offset)
    : Sensor(name, "°C", offset), basisTemperatur(basis), amplitude(amp) {}

Messwert TemperaturSensor::messen(int tick) const {
    // sine wave: coldest at midnight (tick 0), warmest at noon (tick 12)
    const double PI = 3.14159265358979;
    double wert = basisTemperatur + amplitude * sin((tick / 24.0) * 2 * PI - PI / 2.0);
    wert = round((wert + getOffset()) * 100.0) / 100.0;
    return Messwert(wert, getEinheit(), tick);
}

string TemperaturSensor::getTyp() const { return "TemperaturSensor"; }

// --- LichtSensor ---

LichtSensor::LichtSensor(string name, double maxL, double offset)
    : Sensor(name, "lux", offset), maxLux(maxL) {}

Messwert LichtSensor::messen(int tick) const {
    // sunlight: half-sine from sunrise (tick 6) to sunset (tick 20)
    const double PI = 3.14159265358979;
    double wert = 0.0;
    if (tick >= 6 && tick <= 20)
        wert = maxLux * sin(((tick - 6) / 14.0) * PI);
    wert = max(0.0, wert + getOffset());
    wert = round(wert * 10.0) / 10.0;
    return Messwert(wert, getEinheit(), tick);
}

string LichtSensor::getTyp() const { return "LichtSensor"; }

// --- LuftfeuchtigkeitsSensor ---

LuftfeuchtigkeitsSensor::LuftfeuchtigkeitsSensor(string name, double basis, double schwank, double offset)
    : Sensor(name, "%", offset), basisFeuchtigkeit(basis), schwankung(schwank) {}

Messwert LuftfeuchtigkeitsSensor::messen(int tick) const {
    // humidity is inversely correlated with temperature: higher at night, lower at noon
    const double PI = 3.14159265358979;
    double variation = schwankung * sin((tick / 24.0) * 2 * PI + PI / 2.0);
    double rauschen  = (static_cast<double>(rand()) / RAND_MAX - 0.5) * 2.0;
    double wert = basisFeuchtigkeit + variation + rauschen + getOffset();
    wert = max(0.0, min(100.0, wert));
    wert = round(wert * 100.0) / 100.0;
    return Messwert(wert, getEinheit(), tick);
}

string LuftfeuchtigkeitsSensor::getTyp() const { return "LuftfeuchtigkeitsSensor"; }

// --- CO2Sensor ---

CO2Sensor::CO2Sensor(string name, double basis, double offset)
    : Sensor(name, "ppm", offset), basisPPM(basis), fensterOffen(true) {}

void CO2Sensor::setFensterOffen(bool offen) { fensterOffen = offen; }
bool CO2Sensor::isFensterOffen() const      { return fensterOffen; }

Messwert CO2Sensor::messen(int tick) const {
    double wert = basisPPM;
    if (!fensterOffen) {
        // CO2 builds up: peaks in the morning when people are active indoors
        const double PI = 3.14159265358979;
        double anstieg = 350.0 * max(0.0, sin(((tick - 6) / 10.0) * PI));
        wert += anstieg;
    }
    double rauschen = (static_cast<double>(rand()) / RAND_MAX - 0.5) * 8.0;
    wert += rauschen + getOffset();
    wert = max(300.0, round(wert * 10.0) / 10.0);
    return Messwert(wert, getEinheit(), tick);
}

string CO2Sensor::getTyp() const { return "CO2Sensor"; }
