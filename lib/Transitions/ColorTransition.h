#pragma once

#include <Transition.h>
#include <ParsedColor.h>

class ColorTransition final : public Transition {
public:
  struct RgbColor {
    RgbColor();
    RgbColor(const ParsedColor& color);
    RgbColor(int16_t r, int16_t g, int16_t b);
    bool operator==(const RgbColor& other) const;

    int16_t r, g, b;
  };

  class Builder final : public Transition::Builder {
  public:
    Builder(size_t id, uint16_t defaultPeriod, const BulbId& bulbId, const TransitionFn &callback, const ParsedColor& start, const ParsedColor& end);

    std::shared_ptr<Transition> _build() const override;

  private:
    RgbColor start;
    RgbColor end;
  };

  ColorTransition(
    size_t id,
    const BulbId& bulbId,
    const RgbColor& startColor,
    const RgbColor& endColor,
    size_t duration,
    size_t period,
    size_t numPeriods,
    TransitionFn callback
  );

  static size_t calculateColorPeriod(ColorTransition* t, const RgbColor& start, const RgbColor& end, size_t stepSize, size_t duration);
  inline static size_t calculateMaxDistance(const RgbColor& start, const RgbColor& end);
  inline static int16_t calculateStepSizePart(int16_t distance, size_t duration, size_t period);
  bool isFinished() override;

protected:
  const RgbColor endColor;
  RgbColor currentColor;
  RgbColor stepSizes;

  // Store these to avoid wasted packets
  uint16_t lastHue;
  uint16_t lastSaturation;
  bool sentFinalColor;

  void step() override;
  void childSerialize(JsonObject& json) override;
  static inline void stepPart(uint16_t& current, uint16_t end, int16_t step);
};
