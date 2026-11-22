/**
 * Pruebas unitarias de la lógica pura (sin hardware).
 * Se ejecutan en la PC con:  pio test -e native
 */
#include <GateLogic.h>
#include <PresenceFilter.h>
#include <unity.h>

namespace {

GateLogicConfig makeConfig() {
  GateLogicConfig c;
  c.travelMs = 1000;
  c.waitVehicleMs = 10000;
  c.clearDelayMs = 1500;
  return c;
}

// Compara por nombre para que un fallo se lea bien ("expected closing, was closed").
void assertState(GateState expected, const GateLogic& logic) {
  TEST_ASSERT_EQUAL_STRING(GateLogic::stateName(expected), GateLogic::stateName(logic.state()));
}

// Lleva una caseta nueva hasta WaitingVehicle (abierta, esperando). Termina en t = 1000.
void openAndWait(GateLogic& logic) {
  logic.requestOpen(0);
  logic.update(1000, false);
  assertState(GateState::WaitingVehicle, logic);
}

// Lleva la caseta hasta VehiclePassing. Termina en t = 2000.
void openAndVehicleArrives(GateLogic& logic) {
  openAndWait(logic);
  logic.update(2000, true);
  assertState(GateState::VehiclePassing, logic);
}

}  // namespace

void setUp(void) {}
void tearDown(void) {}

// ---------------------------------------------------------------- GateLogic

void test_starts_closed() {
  GateLogic logic(makeConfig());
  assertState(GateState::Closed, logic);
  TEST_ASSERT_FALSE(logic.servoShouldBeOpen());
}

void test_open_request_opens_then_waits() {
  GateLogic logic(makeConfig());
  logic.requestOpen(0);
  assertState(GateState::Opening, logic);
  TEST_ASSERT_TRUE(logic.servoShouldBeOpen());
  logic.update(999, false);
  assertState(GateState::Opening, logic);
  logic.update(1000, false);
  assertState(GateState::WaitingVehicle, logic);
}

void test_normal_pass_closes_after_clear_delay() {
  GateLogic logic(makeConfig());
  openAndVehicleArrives(logic);

  logic.update(3000, false);  // el sensor se libera: empieza la cuenta
  assertState(GateState::VehiclePassing, logic);
  logic.update(4499, false);
  assertState(GateState::VehiclePassing, logic);
  logic.update(4500, false);
  assertState(GateState::Closing, logic);
  TEST_ASSERT_EQUAL_STRING("vehicle_passed", GateLogic::reasonName(logic.lastCloseReason()));
  TEST_ASSERT_FALSE(logic.servoShouldBeOpen());

  logic.update(5499, false);
  assertState(GateState::Closing, logic);
  logic.update(5500, false);
  assertState(GateState::Closed, logic);
}

void test_vehicle_flicker_restarts_clear_timer() {
  GateLogic logic(makeConfig());
  openAndVehicleArrives(logic);

  logic.update(3000, false);  // libre
  logic.update(3500, true);   // reaparece: reinicia la cuenta
  logic.update(4000, false);  // libre otra vez: cuenta desde 4000
  logic.update(5499, false);
  assertState(GateState::VehiclePassing, logic);
  logic.update(5500, false);
  assertState(GateState::Closing, logic);
}

void test_closes_by_timeout_if_no_vehicle_arrives() {
  GateLogic logic(makeConfig());
  openAndWait(logic);  // espera desde t = 1000

  logic.update(10999, false);
  assertState(GateState::WaitingVehicle, logic);
  logic.update(11000, false);
  assertState(GateState::Closing, logic);
  TEST_ASSERT_EQUAL_STRING("timeout", GateLogic::reasonName(logic.lastCloseReason()));
}

void test_obstacle_while_closing_reopens() {
  GateLogic logic(makeConfig());
  openAndVehicleArrives(logic);
  logic.update(3000, false);
  logic.update(4500, false);
  assertState(GateState::Closing, logic);

  logic.update(4600, true);  // aparece algo mientras baja la pluma
  assertState(GateState::Opening, logic);
  TEST_ASSERT_EQUAL_UINT32(1, logic.obstacleReopens());
  TEST_ASSERT_EQUAL_STRING("none", GateLogic::reasonName(logic.lastCloseReason()));
  TEST_ASSERT_TRUE(logic.servoShouldBeOpen());

  logic.update(5600, false);
  assertState(GateState::WaitingVehicle, logic);
}

void test_open_request_while_closing_reopens() {
  GateLogic logic(makeConfig());
  openAndWait(logic);
  logic.update(11000, false);  // timeout: empieza a cerrar
  assertState(GateState::Closing, logic);
  logic.requestOpen(11100);
  assertState(GateState::Opening, logic);
}

void test_manual_close_only_while_waiting() {
  GateLogic logic(makeConfig());

  logic.requestClose(0);  // cerrada: se ignora
  assertState(GateState::Closed, logic);

  logic.requestOpen(0);
  logic.requestClose(500);  // abriendo: se ignora
  assertState(GateState::Opening, logic);

  logic.update(1000, false);
  logic.requestClose(2000);  // esperando: se acepta
  assertState(GateState::Closing, logic);
  TEST_ASSERT_EQUAL_STRING("manual", GateLogic::reasonName(logic.lastCloseReason()));
}

void test_manual_close_ignored_when_vehicle_is_passing() {
  GateLogic logic(makeConfig());
  openAndVehicleArrives(logic);
  logic.requestClose(2500);
  assertState(GateState::VehiclePassing, logic);
}

void test_second_card_extends_wait() {
  GateLogic logic(makeConfig());
  openAndWait(logic);          // espera desde 1000
  logic.requestOpen(8000);     // otra tarjeta: la espera se reinicia
  logic.update(17999, false);
  assertState(GateState::WaitingVehicle, logic);
  logic.update(18000, false);
  assertState(GateState::Closing, logic);
}

void test_vehicle_present_during_opening_is_noticed_after_travel() {
  GateLogic logic(makeConfig());
  logic.requestOpen(0);
  logic.update(500, true);  // todavía subiendo: no cambia de estado
  assertState(GateState::Opening, logic);
  logic.update(1000, true);
  assertState(GateState::WaitingVehicle, logic);
  logic.update(1001, true);
  assertState(GateState::VehiclePassing, logic);
}

void test_survives_millis_wraparound() {
  GateLogic logic(makeConfig());
  const uint32_t t0 = 0xFFFFFF00u;  // a 256 ms de que millis() dé la vuelta
  logic.requestOpen(t0);
  logic.update(t0 + 999u, false);
  assertState(GateState::Opening, logic);
  logic.update(t0 + 1000u, false);  // aquí el contador ya dio la vuelta
  assertState(GateState::WaitingVehicle, logic);
  logic.update(t0 + 1000u + 10000u, false);
  assertState(GateState::Closing, logic);
}

// ----------------------------------------------------------- PresenceFilter

void test_filter_starts_absent() {
  PresenceFilter filter(15, 20, 3);
  TEST_ASSERT_FALSE(filter.present());
}

void test_filter_needs_consecutive_readings_to_detect() {
  PresenceFilter filter(15, 20, 3);
  filter.addReading(10);
  filter.addReading(10);
  TEST_ASSERT_FALSE(filter.present());
  filter.addReading(10);
  TEST_ASSERT_TRUE(filter.present());
}

void test_filter_ignores_isolated_spike() {
  PresenceFilter filter(15, 20, 3);
  filter.addReading(10);
  filter.addReading(10);
  filter.addReading(100);  // lectura lejana: rompe la racha
  filter.addReading(10);
  filter.addReading(10);
  TEST_ASSERT_FALSE(filter.present());
}

void test_filter_hysteresis_keeps_state_between_thresholds() {
  PresenceFilter filter(15, 20, 2);
  filter.addReading(10);
  filter.addReading(10);
  TEST_ASSERT_TRUE(filter.present());

  filter.addReading(18);  // entre 15 y 20: no libera
  filter.addReading(18);
  filter.addReading(18);
  TEST_ASSERT_TRUE(filter.present());

  filter.addReading(25);
  filter.addReading(25);
  TEST_ASSERT_FALSE(filter.present());
}

void test_filter_no_echo_counts_as_absent() {
  PresenceFilter filter(15, 20, 2);
  filter.addReading(10);
  filter.addReading(10);
  TEST_ASSERT_TRUE(filter.present());
  filter.addReading(-1);
  filter.addReading(-1);
  TEST_ASSERT_FALSE(filter.present());
}

int main(int, char**) {
  UNITY_BEGIN();
  RUN_TEST(test_starts_closed);
  RUN_TEST(test_open_request_opens_then_waits);
  RUN_TEST(test_normal_pass_closes_after_clear_delay);
  RUN_TEST(test_vehicle_flicker_restarts_clear_timer);
  RUN_TEST(test_closes_by_timeout_if_no_vehicle_arrives);
  RUN_TEST(test_obstacle_while_closing_reopens);
  RUN_TEST(test_open_request_while_closing_reopens);
  RUN_TEST(test_manual_close_only_while_waiting);
  RUN_TEST(test_manual_close_ignored_when_vehicle_is_passing);
  RUN_TEST(test_second_card_extends_wait);
  RUN_TEST(test_vehicle_present_during_opening_is_noticed_after_travel);
  RUN_TEST(test_survives_millis_wraparound);
  RUN_TEST(test_filter_starts_absent);
  RUN_TEST(test_filter_needs_consecutive_readings_to_detect);
  RUN_TEST(test_filter_ignores_isolated_spike);
  RUN_TEST(test_filter_hysteresis_keeps_state_between_thresholds);
  RUN_TEST(test_filter_no_echo_counts_as_absent);
  return UNITY_END();
}
