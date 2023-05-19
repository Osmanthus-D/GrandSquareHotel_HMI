/**
  * This file is part of the energystoragehmi project.
  *
  * Copyright (C) 2022 Haozhi Lu <dyyfb25@xes.com>
  */

#ifndef SHAREDDATA_H
#define SHAREDDATA_H

#include <stdint.h>

#define PACKS_NUM                           (18)
#define MAX_VOLTAGE_NUM_IN_PACKS            (400)
#define MAX_TEMP_NUM_IN_PACKS               (MAX_VOLTAGE_NUM_IN_PACKS / 2)

typedef struct _battery_packs_data_t_ {
    uint16_t main_positive_contactor_feedback;
    uint16_t main_negative_contactor_feedback;
    uint16_t packs_voltage;
    uint16_t packs_current;
    uint16_t packs_soc;
    uint16_t packs_soh;
    uint16_t packs_soe;
    int16_t packs_temp;
    uint16_t packs_insulation_resistance;
    uint16_t packs_average_cell_voltage;
    int16_t packs_average_cell_temp;
    uint16_t max_cell_voltage;
    uint16_t max_cell_voltage_id;
    uint16_t min_cell_voltage;
    uint16_t min_cell_voltage_id;
    int16_t max_cell_temp;
    uint16_t max_cell_temp_id;
    int16_t min_cell_temp;
    uint16_t min_cell_temp_id;
    uint16_t max_cell_soc;
    uint16_t max_cell_soc_id;
    uint16_t min_cell_soc;
    uint16_t min_cell_soc_id;
    uint16_t diff_voltage;
    int16_t diff_temp;
    int16_t temperature_rise_rate;
    uint64_t cumulative_charging_power;
    uint64_t cumulative_discharging_power;
    uint32_t single_cumulative_charging_power;
    uint32_t single_cumulative_discharging_power;
    uint32_t chargeable_power;
    uint32_t dischargeable_power;
} battery_packs_data_t;

typedef struct _cell_stack_data_t_ {
    uint16_t stack_voltage;
    uint16_t stack_current;
    uint16_t stack_soc;
    uint16_t stack_soe;
    uint16_t max_cell_voltage;
    uint16_t max_voltage_cell_pack_id;
    uint16_t max_voltage_cell_id;
    uint16_t min_cell_voltage;
    uint16_t min_voltage_cell_pack_id;
    uint16_t min_voltage_cell_id;
    int16_t max_cell_temp;
    uint16_t max_temp_cell_pack_id;
    uint16_t max_temp_cell_id;
    int16_t min_cell_temp;
    uint16_t min_temp_cell_pack_id;
    uint16_t min_temp_cell_id;
    uint64_t cumulative_charging_power;
    uint64_t cumulative_discharging_power;
    uint32_t single_cumulative_charging_power;
    uint32_t single_cumulative_discharging_power;
    uint32_t chargeable_power;
    uint32_t dischargeable_power;
    uint16_t remaining_charging_time;
    uint16_t remaining_discharging_time;
    int16_t stack_temperature;
    uint8_t current_state;
    uint8_t charging_state;
    uint16_t stack_insulation_resistance;
    uint8_t stack_breaker_feedback;
    uint8_t stack_emergency_stop_feedback;
    int16_t temperature_humidity_meter_temp;
    uint16_t temperature_humidity_meter_humidity;

    uint16_t max_chargeable_power;
    uint16_t max_dischargeable_power;
} cell_stack_data_t;

typedef struct _telemetering_data_t_ {
    cell_stack_data_t cell_stack_data;
    battery_packs_data_t battery_packs_data[PACKS_NUM];
    uint16_t cell_voltage[PACKS_NUM][MAX_VOLTAGE_NUM_IN_PACKS];
    uint16_t cell_temp[PACKS_NUM][MAX_TEMP_NUM_IN_PACKS];
} telemetering_data_t;

union warning_item {
    unsigned char value;
    struct warning_level {
        unsigned char exist : 1;
        unsigned char level : 2;
    }
};

typedef struct _battery_packs_warning_t_ {
    warning_item under_voltage;
    warning_item over_voltage;
    warning_item over_current;
    warning_item cell_under_voltage;
    warning_item cell_over_voltage;
    warning_item cell_under_temp;
    warning_item cell_over_temp;
    warning_item cell_low_soc;
    warning_item cell_diff_voltage;
    warning_item cell_diff_temp;
    warning_item missing_slave_board;
    warning_item missing_master_board;
    warning_item ambient_over_temp;
    warning_item ambient_under_temp;
    warning_item precharge_failure;
    warning_item low_soh;
    warning_item power_failure;
    warning_item self_check_failure;
    warning_item main_positive_contactor;
    warning_item main_negative_contactor;
    warning_item fuse_state;
} battery_packs_warning_t;

typedef struct _teleindication_data_t_ {
    battery_packs_warning_t battery_packs_warning[PACKS_NUM];
} teleindication_data_t;

typedef struct _shared_data_t_ {
    telemetering_data_t telemetering_data;
    teleindication_data_t teleindication_data;
} shared_data_t;

#endif