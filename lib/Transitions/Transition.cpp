#include <Transition.h>
#include <Arduino.h>
#include <cmath>

// transition commands are in seconds, convert to ms.
const uint16_t Transition::DURATION_UNIT_MULTIPLIER = 1000;

// If period goes lower than this, throttle other parameters up to adjust.
const size_t Transition::MIN_PERIOD = 150;
const size_t Transition::DEFAULT_DURATION = 10000;

Transition::Builder::Builder(const size_t id,
                             const uint16_t defaultPeriod,
                             const BulbId &bulbId,
                             const TransitionFn &callback,
                             const size_t maxSteps)
  : id(id)
  , defaultPeriod(defaultPeriod)
  , bulbId(bulbId)
  , callback(callback)
  , duration(0)
  , period(0)
  , numPeriods(0)
  , maxSteps(maxSteps)
{ }

Transition::Builder& Transition::Builder::setDuration(const float duration) {
  this->duration = duration * DURATION_UNIT_MULTIPLIER;
  return *this;
}

void Transition::Builder::setDurationRaw(const size_t duration) {
  this->duration = duration;
}

Transition::Builder& Transition::Builder::setPeriod(const size_t period) {
  this->period = period;
  return *this;
}

Transition::Builder& Transition::Builder::setDurationAwarePeriod(const size_t period, const size_t duration, const size_t maxSteps) {
  if ((period * maxSteps) < duration) {
    setPeriod(std::ceil(duration / static_cast<float>(maxSteps)));
  } else {
    setPeriod(period);
  }
  return *this;
}

size_t Transition::Builder::getNumPeriods() const {
  return this->numPeriods;
}

size_t Transition::Builder::getDuration() const {
  return this->duration;
}

size_t Transition::Builder::getPeriod() const {
  return this->period;
}

size_t Transition::Builder::getMaxSteps() const {
  return this->maxSteps;
}

bool Transition::Builder::isSetDuration() const {
  return this->duration > 0;
}

bool Transition::Builder::isSetPeriod() const {
  return this->period > 0;
}

bool Transition::Builder::isSetNumPeriods() const {
  return this->numPeriods > 0;
}

size_t Transition::Builder::numSetParams() const {
  size_t setCount = 0;

  if (isSetDuration()) { ++setCount; }
  if (isSetPeriod()) { ++setCount; }
  if (isSetNumPeriods()) { ++setCount; }

  return setCount;
}

size_t Transition::Builder::getOrComputePeriod() const {
  if (period > 0) {
    return period;
  }
  if (duration > 0 && numPeriods > 0) {
    const size_t computed = floor(duration / static_cast<float>(numPeriods));
    return max(MIN_PERIOD, computed);
  }
  return 0;
}

size_t Transition::Builder::getOrComputeDuration() const {
  if (duration > 0) {
    return duration;
  }
  if (period > 0 && numPeriods > 0) {
    return period * numPeriods;
  }
  return 0;
}

size_t Transition::Builder::getOrComputeNumPeriods() const {
  if (numPeriods > 0) {
    return numPeriods;
  }
  if (period > 0 && duration > 0) {
    const size_t _numPeriods = ceil(duration / static_cast<float>(period));
    return max(static_cast<size_t>(1), _numPeriods);
  }
  return 0;
}

std::shared_ptr<Transition> Transition::Builder::build() {
  // Set defaults for underspecified transitions
  const size_t numSet = numSetParams();

  if (numSet == 0) {
    setDuration(DEFAULT_DURATION);
    setDurationAwarePeriod(defaultPeriod, duration, maxSteps);
  } else if (numSet == 1) {
    // If duration is unbound, bind it
    if (! isSetDuration()) {
      setDurationRaw(DEFAULT_DURATION);
    // Otherwise, bind the period
    } else {
      setDurationAwarePeriod(defaultPeriod, duration, maxSteps);
    }
  }

  return _build();
}

Transition::Transition(
  const size_t id,
  const BulbId& bulbId,
  const size_t period,
  const TransitionFn &callback
) : id(id)
  , bulbId(bulbId)
  , callback(callback)
  , period(period)
  , lastSent(0)
{ }

void Transition::tick() {
  if (
    const unsigned long now = millis(); (lastSent + period) <= now &&
    ((!isFinished() || lastSent == 0))
  ) { // always send at least once
    step();
    lastSent = now;
  }
}

size_t Transition::calculatePeriod(const int16_t distance, const size_t stepSize, const size_t duration) {
  const float fPeriod =
    distance != 0
      ? (duration / (distance / static_cast<float>(stepSize)))
      : 0;

  return static_cast<size_t>(round(fPeriod));
}

void Transition::stepValue(int16_t& current, const int16_t end, const int16_t stepSize) {
  if (const int16_t delta = end - current; std::abs(delta) < std::abs(stepSize)) {
    current += delta;
  } else {
    current += stepSize;
  }
}

void Transition::serialize(JsonObject& json) {
  json[F("id")] = id;
  json[F("period")] = period;
  json[F("last_sent")] = lastSent;

  const JsonObject bulbParams = json.createNestedObject("bulb");
  bulbId.serialize(bulbParams);

  childSerialize(json);
}
