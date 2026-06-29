#pragma once
#include "Sensor.h"

class TemperaturSensor : public Sensor {
private:
    double basisTemperatur;
    double amplitude;

public:
    TemperaturSensor(string name, double basis, double amp, double offset = 0.0);

    Messwert messen(int tick) const override;
    string   getTyp() const override;
};

class LichtSensor : public Sensor {
private:
    double maxLux;

public:
    LichtSensor(string name, double maxL, double offset = 0.0);

    Messwert messen(int tick) const override;
    string   getTyp() const override;
};

class LuftfeuchtigkeitsSensor : public Sensor {
private:
    double basisFeuchtigkeit;
    double schwankung;

public:
    LuftfeuchtigkeitsSensor(string name, double basis, double schwank, double offset = 0.0);

    Messwert messen(int tick) const override;
    string   getTyp() const override;
};

class CO2Sensor : public Sensor {
private:
    double basisPPM;
    bool   fensterOffen;

public:
    CO2Sensor(string name, double basis, double offset = 0.0);

    void setFensterOffen(bool offen);
    bool isFensterOffen() const;

    Messwert messen(int tick) const override;
    string   getTyp() const override;
};
