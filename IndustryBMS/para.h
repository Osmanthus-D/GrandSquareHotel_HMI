#ifndef PARA
#define PARA

typedef struct Para_t
{
    //operating
    QString charge_cutoff_voltage;
    QString charge_limit_voltage_1;
    QString charge_limit_voltage_2;

    QString discharge_cutoff_voltage;
    QString discharge_limit_voltage_1;
    QString discharge_limit_voltage_2;

    QString fan_start_temp;
    QString fan_stop_temp;

    QString balance_start;
    QString nominal_capcity;

    QString temp_high_limit;
    QString temp_low_limit;
    QString temp_high_normal;
    QString temp_low_normal;

    QString soc_high_limit;
    QString soc_low_limit;

    QString voltage_consistency;
    QString temp_consistency;

    QString charge_current_limit;
    QString discharge_current_limit;
    QString charge_current_limit_2;
    QString discharge_current_limit_2;
    QString charge_current_limit_1;
    QString discharge_current_limit_1;

    QString voltage_refresh_period;
    QString temp_refresh_period;
    QString insulation_test_enable;

    QString voltage_consistency_restore;
    QString temp_consistency_restore;

    QString jumper_copper_resistance;
    QString equilibrium_enable;

    //fault
    QString over_voltage;
    QString under_voltage;

    QString charge_over_temp;
    QString charge_under_temp;

    QString charge_over_current;
    QString discharge_over_current;

    QString insulation_limit;

    QString module_over_voltage;
    QString module_under_voltage;

    QString pack_comm_time;
    QString module_comm_time;

    QString discharge_over_temp;
    QString discharge_under_temp;

    //warnning
    QString over_temp_2;
    QString over_temp_3;

    QString under_temp_2;
    QString under_temp_3;

    QString over_voltage_2;
    QString over_voltage_3;

    QString under_voltage_2;
    QString under_voltage_3;

    QString module_over_voltage_2;
    QString module_over_voltage_3;

    QString module_under_voltage_2;
    QString module_under_voltage_3;

    QString module_over_current_2;
    QString module_over_current_3;

    QString temp_consistency_2;
    QString temp_consistency_3;

    QString voltage_consistency_2;
    QString voltage_consistency_3;

    QString insulation_limit_2;
    QString insulation_limit_3;

    QString SOH_limit;

    QString SOC_high_limit_1;
    QString SOC_high_limit_2;
    QString SOC_high_limit_3;

    QString SOC_low_limit_1;
    QString SOC_low_limit_2;
    QString SOC_low_limit_3;

    //KB
    QString current_K;
    QString current_B;
    QString voltage_K;
    QString voltage_B;
    QString precharge_voltage_K;
    QString precharge_voltage_B;

    //OCV-SOC
    QString OCV;
    QString SOC;
} Para;

typedef struct _WarnAttr_t_
{
    unsigned char flag;
    unsigned char level:2;
    unsigned char type:2;
    unsigned char direciton:2;
    unsigned char scope:2;
} WarnAttr;

namespace W {
    enum FAULT_OBJ
    {
        Current = 0x0001,
        Voltage,
        Temperature,
        TempDiff,
        VoltDiff,
        InsulationRes,
        Soc,
        SelfCheck,
        PosRelay,
        NegRelay,
        Precharge,
        AmbientTemp,
        PowerSupply,
        Soh,
        AbnormalTemp,
        Disconnect,
        Sample,

        Level1 = 0x0100,
        Level2 = 0x0200,
        Level3 = 0x0300,
        Over = 0x0400,
        Low = 0x0800,
        Charging = 0x1000,
        Discharging = 0x2000,
        Cell = 0x4000,
        Module = 0x8000,

        OverVoltage = Over | Voltage,
        LowVoltage = Low | Voltage,
        OverCurrent = Over | Current,
        OverTemp = Over | Temperature,
        LowTemp = Low | Temperature,

        ChargingOverTemp = Charging | OverTemp,
        ChargingLowTemp = Charging | LowTemp,
        DisChargingOverTemp = Discharging | OverTemp,
        DischargingLowTemp = Discharging | LowTemp,
        ChargingOverCurrent = Charging | OverCurrent,
        DischargingOverCurrent = Discharging | OverCurrent,

        ModuleOverVoltage = Module | OverVoltage,
        ModuleLowVoltage = Module | LowVoltage,
        CellOverVoltage = Cell | OverVoltage,
        CellLowVoltage = Cell | LowVoltage,

        ModuleOverCurrent = Module | Charging | Discharging | OverCurrent,
        CellOverCurrent = Cell | Charging | Discharging | OverCurrent,
        CellOverTemp = Cell | Charging | Discharging | OverTemp,
        CellLowTemp = Cell | Charging | Discharging | LowTemp,

        ModuleChargingOverCurrent = Module | ChargingOverCurrent,
        ModuleDischargingOverCurrent = Module | DischargingOverCurrent,
        CellChargingOverCurrnet = Cell | ChargingOverCurrent,
        CellDischargingOverCurrent = Cell | DischargingOverCurrent,
        CellChargingOverTemp = Cell | ChargingOverTemp,
        CellChargingLowTemp = Cell | ChargingLowTemp,
        CellDischargingOverTemp = Cell | DisChargingOverTemp,
        CellDisChargingLowTemp = Cell | DischargingLowTemp,

        LowSoc = Low | Soc,
        OverSoc = Over | Soc
    };
}

enum FAULT_CODE
{
    OVER_VOLTAGE = 0x01,
    UNDER_VOLTAGE,
    CHARGE_OVER_TEMP,
    CHARGE_UNDER_TEMP,
    MODULE_OVER_CURRENT,
    MODULE_INSULATION,
    MODULE_OVER_VOLTAGE,
    MODULE_UNDER_VOLTAGE,
    RELAY,
    BMU_ACQUISITION,
    BMU_COMM,
    PCS_IO,
    BMU_BALANCE,
    DISCHARGE_OVER_TEMP,
    DISCHARGE_UNDER_TEMP
};

enum WARNING_CODE
{
    OVER_VOLTAGE_2 = 0x01,
    OVER_VOLTAGE_3,
    UNDER_VOLTAGE_2,
    UNDER_VOLTAGE_3,
    OVER_TEMP_2,
    OVER_TEMP_3,
    UNDER_TEMP_2,
    UNDER_TEMP_3,
    MODULE_OVER_CURRENT_2,
    MODULE_OVER_CURRENT_3,
    MODULE_INSULATION_2,
    MODULE_INSULATION_3,
    MODULE_OVER_VOLTAGE_2,
    MODULE_OVER_VOLTAGE_3,
    MODULE_UNDER_VOLTAGE_2,
    MODULE_UNDER_VOLTAGE_3,
    VOLTAGE_CONSISTENCY_2,
    VOLTAGE_CONSISTENCY_3,
    TEMP_CONSISTENCY_2,
    TEMP_CONSISTENCY_3,
    SOH,
    SOC_HIGH_1,
    SOC_HIGH_2,
    SOC_HIGH_3,
    SOC_LOW_1,
    SOC_LOW_2,
    SOC_LOW_3
};

#endif // PARA

