#include "queue.h"
#include "dfplayer.h"
#include "logger.h"
#include "system.h" // para logError()

using namespace Audio;

Item AudioQueue::queue_[AudioQueue::MAX_QUEUE];
size_t AudioQueue::head_ = 0;
size_t AudioQueue::tail_ = 0;
size_t AudioQueue::count_ = 0;

void AudioQueue::init() noexcept {
  head_ = tail_ = count_ = 0;
  Logger::info("AudioQueue init OK");
}

bool AudioQueue::push(uint16_t track, Priority prio) noexcept {
  // Validar rango de tracks (1-68)
  if (track == 0 || track > 68) {
    Logger::warnf("AudioQueue: track inválido (%u). Rango válido: 1-68",
                  (unsigned)track);
    System::logError(730); // código reservado: track inválido
    return false;
  }

  if (count_ >= MAX_QUEUE) {
    Logger::errorf("AudioQueue FULL, dropping track %u", (unsigned)track);
    System::logError(731); // código reservado: cola llena
    return false;
  }

  queue_[tail_] = {track, prio};
  tail_ = (tail_ + 1) % MAX_QUEUE;
  ++count_;
  Logger::infof("AudioQueue push track %u prio %u", (unsigned)track,
                (unsigned)prio);
  return true;
}

bool AudioQueue::pop(Item &out) noexcept {
  if (count_ == 0) return false;
  out = queue_[head_];
  head_ = (head_ + 1) % MAX_QUEUE;
  --count_;
  return true;
}

bool AudioQueue::empty() noexcept { return (count_ == 0); }

void AudioQueue::update() noexcept {
  // Punto de extensión: despachar a DFPlayer si procede.
  Item it;
  if (pop(it)) {
    Logger::infof("AudioQueue dispatch track %u", (unsigned)it.track);
    // 🔎 Si DFPlayer está inicializado, reproducir
    if (Audio::DFPlayer::initOK()) {
      Audio::DFPlayer::play(it.track);
    } else {
      Logger::warn(
          "AudioQueue: DFPlayer no inicializado, no se puede reproducir");
      System::logError(732); // código reservado: DFPlayer no listo
    }
  }
}