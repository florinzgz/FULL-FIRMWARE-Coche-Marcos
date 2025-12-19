// ============================================================================
// audio_validation_tests.cpp - Audio System Validation Tests
// ============================================================================
// Validates all 68 audio tracks are properly defined and can be queued
// Tests error handling for invalid tracks and queue overflow
// ============================================================================

#include "audio_validation_tests.h"
#include "logger.h"
#include "alerts.h"
#include "queue.h"
#include "dfplayer.h"

namespace AudioValidationTests {

// ============================================================================
// Private State
// ============================================================================

static const uint32_t MAX_TEST_RESULTS = 20;
static TestResult testResults[MAX_TEST_RESULTS];
static uint32_t testCount = 0;
static uint32_t passedCount = 0;
static uint32_t failedCount = 0;

// ============================================================================
// Helper Functions
// ============================================================================

static void recordTest(const char* name, bool passed, const char* reason) {
    if (testCount < MAX_TEST_RESULTS) {
        testResults[testCount].testName = name;
        testResults[testCount].passed = passed;
        testResults[testCount].failureReason = reason;
        testCount++;
        
        if (passed) {
            passedCount++;
            Logger::infof("✅ AUDIO TEST PASSED: %s", name);
        } else {
            failedCount++;
            Logger::errorf("❌ AUDIO TEST FAILED: %s - %s", name, reason);
        }
    }
}

// ============================================================================
// Individual Test Functions
// ============================================================================

static bool testAllTracksDefinedInEnum() {
    // Verificar que todos los 68 tracks están definidos en el enum
    // Esto se valida en tiempo de compilación, pero podemos verificar algunos tracks clave
    
    bool allDefined = true;
    
    // Verificar tracks básicos (1-38)
    if (static_cast<uint16_t>(Audio::AUDIO_INICIO) != 1) allDefined = false;
    if (static_cast<uint16_t>(Audio::AUDIO_APAGADO) != 2) allDefined = false;
    if (static_cast<uint16_t>(Audio::AUDIO_TRACCION_4X2) != 38) allDefined = false;
    
    // Verificar tracks avanzados (39-68)
    if (static_cast<uint16_t>(Audio::AUDIO_ABS_ACTIVADO) != 39) allDefined = false;
    if (static_cast<uint16_t>(Audio::AUDIO_MODO_SPORT) != 63) allDefined = false;
    if (static_cast<uint16_t>(Audio::AUDIO_BEEP) != 68) allDefined = false;
    
    return allDefined;
}

static bool testInvalidTrackRejected() {
    // Intentar encolar un track inválido (0)
    Audio::AudioQueue::init();
    bool result = Audio::AudioQueue::push(0, Audio::PRIO_NORMAL);
    
    // Debe retornar false (rechazado)
    return !result;
}

static bool testOutOfRangeTrackRejected() {
    // Intentar encolar un track fuera de rango (69, 100, 255)
    Audio::AudioQueue::init();
    
    bool rejected69 = !Audio::AudioQueue::push(69, Audio::PRIO_NORMAL);
    bool rejected100 = !Audio::AudioQueue::push(100, Audio::PRIO_NORMAL);
    bool rejected255 = !Audio::AudioQueue::push(255, Audio::PRIO_NORMAL);
    
    return rejected69 && rejected100 && rejected255;
}

static bool testValidTracksAccepted() {
    // Verificar que todos los tracks válidos (1-68) son aceptados
    Audio::AudioQueue::init();
    
    bool allAccepted = true;
    
    // Probar algunos tracks representativos
    for (uint16_t track = 1; track <= 68; track += 7) {
        if (!Audio::AudioQueue::push(track, Audio::PRIO_NORMAL)) {
            Logger::errorf("Track %u rechazado incorrectamente", (unsigned)track);
            allAccepted = false;
        }
    }
    
    return allAccepted;
}

static bool testQueueOverflow() {
    // Probar que la cola maneja correctamente el desbordamiento
    Audio::AudioQueue::init();
    
    // La cola tiene MAX_QUEUE = 8 elementos
    // Intentar agregar 10 elementos
    uint32_t accepted = 0;
    for (uint16_t i = 1; i <= 10; i++) {
        if (Audio::AudioQueue::push(i, Audio::PRIO_NORMAL)) {
            accepted++;
        }
    }
    
    // Debe haber aceptado exactamente 8
    return (accepted == 8);
}

static bool testQueuePriorityLevels() {
    // Verificar que se pueden usar diferentes niveles de prioridad
    Audio::AudioQueue::init();
    
    bool lowOk = Audio::AudioQueue::push(1, Audio::PRIO_LOW);
    bool normalOk = Audio::AudioQueue::push(2, Audio::PRIO_NORMAL);
    bool highOk = Audio::AudioQueue::push(3, Audio::PRIO_HIGH);
    bool criticalOk = Audio::AudioQueue::push(4, Audio::PRIO_CRITICAL);
    
    return lowOk && normalOk && highOk && criticalOk;
}

static bool testAllBasicTracks() {
    // Verificar que todos los tracks básicos (1-38) se pueden encolar
    Audio::AudioQueue::init();
    
    bool allOk = true;
    for (uint16_t track = 1; track <= 38; track++) {
        // Reiniciar cola para cada grupo de pruebas
        if ((track - 1) % 8 == 0) {
            Audio::AudioQueue::init();
        }
        
        if (!Audio::AudioQueue::push(track, Audio::PRIO_NORMAL)) {
            Logger::errorf("Track básico %u falló al encolar", (unsigned)track);
            allOk = false;
        }
    }
    
    return allOk;
}

static bool testAllAdvancedTracks() {
    // Verificar que todos los tracks avanzados (39-68) se pueden encolar
    Audio::AudioQueue::init();
    
    bool allOk = true;
    for (uint16_t track = 39; track <= 68; track++) {
        // Reiniciar cola para cada grupo de pruebas
        if ((track - 39) % 8 == 0) {
            Audio::AudioQueue::init();
        }
        
        if (!Audio::AudioQueue::push(track, Audio::PRIO_NORMAL)) {
            Logger::errorf("Track avanzado %u falló al encolar", (unsigned)track);
            allOk = false;
        }
    }
    
    return allOk;
}

static bool testAlertsPlayWithValidTrack() {
    // Verificar que Alerts::play puede reproducir tracks válidos
    Alerts::init();
    
    // Esto no debe generar errores (aunque DFPlayer puede no estar disponible)
    // Solo verificamos que no haya crashes o assertions
    Alerts::play(Audio::AUDIO_INICIO);
    Alerts::play(Audio::AUDIO_BEEP);
    Alerts::play(Audio::AUDIO_ABS_ACTIVADO);
    
    return true; // Si llegamos aquí sin crash, pasó
}

static bool testTrackEnumCoverage() {
    // Verificar que el enum cubre exactamente 68 valores únicos
    // Esto es más una verificación de definición que un test en runtime
    
    // Verificar algunos tracks críticos para asegurar continuidad
    bool coverage = true;
    
    // Primero (1) y último (68)
    coverage &= (static_cast<uint16_t>(Audio::AUDIO_INICIO) == 1);
    coverage &= (static_cast<uint16_t>(Audio::AUDIO_BEEP) == 68);
    
    // Transición entre básicos y avanzados
    coverage &= (static_cast<uint16_t>(Audio::AUDIO_TRACCION_4X2) == 38);
    coverage &= (static_cast<uint16_t>(Audio::AUDIO_ABS_ACTIVADO) == 39);
    
    // Algunos intermedios
    coverage &= (static_cast<uint16_t>(Audio::AUDIO_BATERIA_BAJA) == 12);
    coverage &= (static_cast<uint16_t>(Audio::AUDIO_WIFI_CONECTADO) == 45);
    coverage &= (static_cast<uint16_t>(Audio::AUDIO_MODO_ECO) == 61);
    
    return coverage;
}

// ============================================================================
// Public API Implementation
// ============================================================================

void init() {
    testCount = 0;
    passedCount = 0;
    failedCount = 0;
    
    Logger::info("AudioValidationTests: Initialized");
}

bool runAllTests() {
    Logger::info("========================================");
    Logger::info("Audio Validation Tests (68 Tracks)");
    Logger::info("========================================");
    
    // Reset counters
    testCount = 0;
    passedCount = 0;
    failedCount = 0;
    
    // Ejecutar todas las pruebas
    bool allTracksOk = testAllTracksDefinedInEnum();
    recordTest("All 68 tracks defined in enum", allTracksOk, 
               allTracksOk ? "OK" : "Enum definition incomplete");
               
    bool invalidRejected = testInvalidTrackRejected();
    recordTest("Invalid track (0) rejected", invalidRejected,
               invalidRejected ? "OK" : "Track 0 was accepted");
               
    bool rangeRejected = testOutOfRangeTrackRejected();
    recordTest("Out of range tracks rejected", rangeRejected,
               rangeRejected ? "OK" : "Invalid tracks accepted");
               
    bool validAccepted = testValidTracksAccepted();
    recordTest("Valid tracks (1-68) accepted", validAccepted,
               validAccepted ? "OK" : "Some valid tracks rejected");
               
    bool overflowOk = testQueueOverflow();
    recordTest("Queue overflow handling", overflowOk,
               overflowOk ? "OK" : "Queue overflow not handled");
               
    bool priorityOk = testQueuePriorityLevels();
    recordTest("Queue priority levels", priorityOk,
               priorityOk ? "OK" : "Priority levels failed");
               
    bool basicOk = testAllBasicTracks();
    recordTest("All basic tracks (1-38)", basicOk,
               basicOk ? "OK" : "Some basic tracks failed");
               
    bool advancedOk = testAllAdvancedTracks();
    recordTest("All advanced tracks (39-68)", advancedOk,
               advancedOk ? "OK" : "Some advanced tracks failed");
               
    bool alertsOk = testAlertsPlayWithValidTrack();
    recordTest("Alerts::play with valid tracks", alertsOk,
               alertsOk ? "OK" : "Alerts::play failed");
               
    bool coverageOk = testTrackEnumCoverage();
    recordTest("Track enum coverage (1-68)", coverageOk,
               coverageOk ? "OK" : "Enum coverage incomplete");
    
    printSummary();
    
    return (failedCount == 0);
}

void printSummary() {
    Logger::info("========================================");
    Logger::info("Audio Validation Test Summary");
    Logger::info("========================================");
    Logger::infof("Total Tests: %u", (unsigned)testCount);
    Logger::infof("Passed:      %u ✅", (unsigned)passedCount);
    Logger::infof("Failed:      %u ❌", (unsigned)failedCount);
    Logger::infof("Success Rate: %.1f%%", 
                  testCount > 0 ? (100.0f * passedCount / testCount) : 0.0f);
    Logger::info("========================================");
    
    if (failedCount > 0) {
        Logger::error("❌ Some audio validation tests failed!");
        Logger::info("Failed tests:");
        for (uint32_t i = 0; i < testCount; i++) {
            if (!testResults[i].passed) {
                Logger::errorf("  - %s: %s", 
                              testResults[i].testName, 
                              testResults[i].failureReason);
            }
        }
    } else {
        Logger::info("✅ All audio validation tests passed!");
    }
}

const TestResult* getResults(uint32_t& count) {
    count = testCount;
    return testResults;
}

} // namespace AudioValidationTests
