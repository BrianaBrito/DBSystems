#include "Buzzer.h"

namespace {

const uint8_t kChannel = 14;  // canal LEDC reservado para el buzzer
const uint8_t kResolutionBits = 8;
const uint32_t kBaseFreqHz = 2000;

}  // namespace

// Patrones: {frecuencia en Hz, duración en ms}; frecuencia 0 = silencio.
static const Buzzer::Note kClick[] = {{2000, 40}};

static const Buzzer::Note kAccept[] = {{1800, 120}, {0, 50}, {2400, 180}};

static const Buzzer::Note kDeny[] = {{400, 250}, {0, 80}, {400, 250}};

static const Buzzer::Note kAlert[] = {{2200, 200}, {0, 120}};

Buzzer::Buzzer(uint8_t pin)
    : pin_(pin), notes_(nullptr), noteCount_(0), index_(0), loop_(false), playing_(false), noteStartMs_(0) {}

void Buzzer::begin() {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcAttachChannel(pin_, kBaseFreqHz, kResolutionBits, kChannel);
#else
  ledcSetup(kChannel, kBaseFreqHz, kResolutionBits);
  ledcAttachPin(pin_, kChannel);
#endif
  setTone(0);
}

void Buzzer::setTone(uint32_t freqHz) {
#if defined(ESP_ARDUINO_VERSION_MAJOR) && ESP_ARDUINO_VERSION_MAJOR >= 3
  ledcWriteTone(pin_, freqHz);
#else
  ledcWriteTone(kChannel, freqHz);
#endif
}

void Buzzer::play(Pattern pattern, bool loop) {
  switch (pattern) {
    case Click:
      notes_ = kClick;
      noteCount_ = sizeof(kClick) / sizeof(kClick[0]);
      break;
    case Accept:
      notes_ = kAccept;
      noteCount_ = sizeof(kAccept) / sizeof(kAccept[0]);
      break;
    case Deny:
      notes_ = kDeny;
      noteCount_ = sizeof(kDeny) / sizeof(kDeny[0]);
      break;
    case Alert:
      notes_ = kAlert;
      noteCount_ = sizeof(kAlert) / sizeof(kAlert[0]);
      break;
  }
  loop_ = loop;
  index_ = 0;
  playing_ = true;
  startNote(millis());
}

void Buzzer::stop() {
  playing_ = false;
  setTone(0);
}

void Buzzer::update(uint32_t nowMs) {
  if (!playing_) return;
  if (nowMs - noteStartMs_ < notes_[index_].ms) return;

  index_++;
  if (index_ >= noteCount_) {
    if (!loop_) {
      stop();
      return;
    }
    index_ = 0;
  }
  startNote(nowMs);
}

void Buzzer::startNote(uint32_t nowMs) {
  noteStartMs_ = nowMs;
  setTone(notes_[index_].freqHz);
}
