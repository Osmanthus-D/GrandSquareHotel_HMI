#include "config.h"


Config::Config(const QString & fileName, int n, QObject * parent, Format format):
    QSettings(fileName,format,parent),
    Base(n)
{
}

void Config::load(Para *para)
{
    beginReadArray("module");

    for (uint i = 0; i < MODULE_NUM; ++i)
    {
        setArrayIndex(i);

//        //operating
//        para[i].charge_cutoff_voltage = value("charge_cutoff_voltage",3.65).toString();
//        para[i].charge_limit_voltage_1 = value("charge_limit_voltage_1",3.55).toString();
//        para[i].charge_limit_voltage_2 = value("charge_limit_voltage_2",3.45).toString();

//        para[i].discharge_cutoff_voltage = value("discharge_cutoff_voltage",2.70).toString();
//        para[i].discharge_limit_voltage_1 = value("discharge_limit_voltage_1",2.80).toString();
//        para[i].discharge_limit_voltage_2 = value("discharge_limit_voltage_2",2.90).toString();

//        para[i].fan_start_temp = value("fan_start_temp",35.00).toString();
//        para[i].fan_stop_temp = value("fan_stop_temp",30.00).toString();

//        para[i].balance_start = value("balance_start",200.00).toString();
//        para[i].nominal_capcity = value("nominal_capcity",240.00).toString();

//        para[i].temp_high_limit = value("temp_high_limit",40.00).toString();
//        para[i].temp_low_limit = value("temp_low_limit",-1.00).toString();
//        para[i].temp_high_normal = value("temp_high_normal",30.00).toString();
//        para[i].temp_low_normal = value("temp_low_normal",10.00).toString();

//        para[i].soc_high_limit = value("soc_high_limit",95.00).toString();
//        para[i].soc_low_limit = value("soc_low_limit",5.00).toString();

//        para[i].voltage_consistency = value("voltage_consistency",300.00).toString();
//        para[i].temp_consistency = value("temp_consistency",15.00).toString();

//        para[i].charge_current_limit = value("charge_current_limit",72.00).toString();
//        para[i].discharge_current_limit = value("discharge_current_limit",72.00).toString();

//        para[i].charge_current_limit_2 = value("charge_current_limit_2",48.00).toString();
//        para[i].discharge_current_limit_2 = value("discharge_current_limit_2",48.00).toString();
//        para[i].charge_current_limit_1 = value("charge_current_limit_1",24.00).toString();
//        para[i].discharge_current_limit_1 = value("discharge_current_limit_1",24.00).toString();

//        para[i].voltage_refresh_period = value("voltage_refresh_period",500.00).toString();
//        para[i].temp_refresh_period = value("temp_refresh_period",2000.00).toString();

//        //fault
//        para[i].over_voltage = value("over_voltage",3.7).toString();
//        para[i].under_voltage = value("under_voltage",2.5).toString();

//        para[i].over_temp = value("over_temp",60).toString();
//        para[i].under_temp = value("under_temp",-15).toString();

//        para[i].charge_over_current = value("charge_over_current",120).toString();
//        para[i].discharge_over_current = value("discharge_over_current",120).toString();

//        para[i].insulation_limit = value("insulation_limit",500).toString();

//        para[i].module_over_voltage = value("module_over_voltage",800).toString();
//        para[i].module_under_voltage = value("module_under_voltage",540).toString();

//        para[i].pack_comm_time = value("pack_comm_time",30).toString();
//        para[i].module_comm_time = value("module_comm_time",30).toString();

        //warning
        para[i].over_temp_2 = value("over_temp_2",50).toString();
        para[i].over_temp_3 = value("over_temp_3",45).toString();
        para[i].under_temp_2 = value("under_temp_2",-10).toString();
        para[i].under_temp_3 = value("under_temp_3",-1).toString();

        para[i].over_voltage_2 = value("over_voltage_2",3.6).toString();
        para[i].over_voltage_3 = value("over_voltage_3",3.55).toString();
        para[i].under_voltage_2 = value("under_voltage_2",2.8).toString();
        para[i].under_voltage_3 = value("under_voltage_3",2.9).toString();

        para[i].module_over_voltage_2 = value("module_over_voltage_2",864).toString();
        para[i].module_over_voltage_3 = value("module_over_voltage_3",852).toString();
        para[i].module_under_voltage_2 = value("module_under_voltage_2",672).toString();
        para[i].module_under_voltage_3 = value("module_under_voltage_3",696).toString();

        para[i].module_over_current_2 = value("module_over_current_2",150).toString();
        para[i].module_over_current_3 = value("module_over_current_3",140).toString();

        para[i].temp_consistency_2 = value("temp_consistency_2",15).toString();
        para[i].temp_consistency_3 = value("temp_consistency_3",10).toString();
        para[i].voltage_consistency_2 = value("voltage_consistency_2",500).toString();
        para[i].voltage_consistency_3 = value("voltage_consistency_3",300).toString();

        para[i].insulation_limit_2 = value("insulation_limit_2",800).toString();
        para[i].insulation_limit_3 = value("insulation_limit_3",1000).toString();

        para[i].SOH_limit = value("SOH_limit",0.8).toString();

        para[i].SOC_high_limit_1 = value("SOC_high_limit_1",100).toString();
        para[i].SOC_high_limit_2 = value("SOC_high_limit_2",95).toString();
        para[i].SOC_high_limit_3 = value("SOC_high_limit_3",90).toString();

        para[i].SOC_low_limit_1 = value("SOC_low_limit_1",0.1).toString();
        para[i].SOC_low_limit_2 = value("SOC_low_limit_2",5).toString();
        para[i].SOC_low_limit_3 = value("SOC_low_limit_3",10).toString();

//        //KB
//        para[i].current_K = value("current_K",1).toString();
//        para[i].current_B = value("current_B",0).toString();
//        para[i].voltage_K = value("voltage_K",1).toString();
//        para[i].voltage_B = value("voltage_B",0).toString();

        //ocv-soc
        para[i].OCV = value("OCV","3.1868,3.1969,3.2221,3.2427,3.2574,3.2711,3.2779,3.2811,3.2826,"
                            "3.2842,3.2866,3.2918,3.3024,3.3159,3.3213,3.3232,3.3245,3.3255,3.3263,3.3267,3.3464").toString();
        para[i].SOC = value("SOC","0,5,10,15,20,25,30,35,40,45,50,55,60,65,70,75,80,85,90,95,100").toString();


     }
     endArray();
}

void Config::load_others()
{
    moduleNum = value("moduleNum", 6).toInt();
    backlightLevel = value("backlightLevel", 2).toString();
    canRate = value("canRate",3).toString();
    dataSaveInterval = value("dataSaveInterval",30).toString();
    isRealSocEnabled = value("isRealSocEnabled", false).toBool();
    systemType = value("systemType", 0).toInt();
    pcsDevComPort = value("pcsDevComPort", 3).toInt();
    periphComPort = value("periphComPort", 1).toInt();
}

/*QString Config::getStrkey(QString type,int id)
{
    if(type == "m"){
        switch(id){
        case 1:
            return QString("charge_cutoff_voltage");
        case 2:
            return QString("charge_limit_voltage_1");
        case 3:
            return QString("charge_limit_voltage_2");
        case 4:
            return QString("discharge_cutoff_voltage");
        case 5:
            return QString("discharge_limit_voltage_1");
        case 6:
            return QString("discharge_limit_voltage_2");
        case 7:
            return QString("fan_start_temp");
        case 8:
            return QString("fan_stop_temp");
        case 9:
            return QString("balance_start");
        case 10:
            return QString("nominal_capcity");
        case 11:
            return QString("temp_high_limit");
        case 12:
            return QString("temp_low_limit");
        case 13:
            return QString("temp_high_normal");
        case 14:
            return QString("temp_low_normal");
        case 15:
            return QString("soc_high_limit");
        case 16:
            return QString("soc_low_limit");
        case 17:
            return QString("voltage_consistency");
        case 18:
            return QString("temp_consistency");
        case 19:
            return QString("charge_current_limit");
        case 20:
            return QString("discharge_current_limit");
        case 21:
            return QString("charge_current_limit_2");
        case 22:
            return QString("discharge_current_limit_2");
        case 23:
            return QString("charge_current_limit_1");
        case 24:
            return QString("discharge_current_limit_1");
        case 25:
            return QString("voltage_refresh_period");
        case 26:
            return QString("temp_refresh_period");
        case 27:
            return QString("insulation_test_enable");
        case 28:
            return QString("voltage_consistency_restore");
        case 29:
            return QString("temp_consistency_restore");
        case 30:
            return QString("jumper_copper_resistance");
        case 31:
            return QString("equilibrium_enable");
        default:
            return QString("");
        }
    }else if(type == "f"){
        switch(id){
        case 1:
            return QString("over_voltage");
        case 2:
            return QString("under_voltage");
        case 3:
            return QString("charge_over_temp");
        case 4:
            return QString("charge_under_temp");
        case 5:
            return QString("charge_over_current");
        case 6:
            return QString("discharge_over_current");
        case 7:
            return QString("insulation_limit");
        case 8:
            return QString("module_over_voltage");
        case 9:
            return QString("module_under_voltage");
        case 10:
            return QString("pack_comm_time");
        case 11:
            return QString("module_comm_time");
        case 12:
            return QString("discharge_over_temp");
        case 13:
            return QString("discharge_under_temp");
        default:
            return QString("");
        }

    }else if(type == "c"){
        switch(id){
        case 1:
            return QString("current_K");
        case 2:
            return QString("current_B");
        case 3:
            return QString("precharge_voltage_K");
        case 4:
            return QString("precharge_voltage_B");
        case 17:
            return QString("voltage_K");
        case 18:
            return QString("voltage_B");
        default:
            return QString("");
        }

    }else if(type == "w"){
        switch(id)
        {
        case 1:
            return QString("over_temp_2");
        case 2:
            return QString("over_temp_3");
        case 3:
            return QString("under_temp_2");
        case 4:
            return QString("under_temp_3");
        case 5:
            return QString("over_voltage_2");
        case 6:
            return QString("over_voltage_3");
        case 7:
            return QString("under_voltage_2");
        case 8:
            return QString("under_voltage_3");
        case 9:
            return QString("module_over_voltage_2");
        case 10:
            return QString("module_over_voltage_3");
        case 11:
            return QString("module_under_voltage_2");
        case 12:
            return QString("module_under_voltage_3");
        case 13:
            return QString("module_over_current_2");
        case 14:
            return QString("module_over_current_3");
        case 15:
            return QString("temp_consistency_2");
        case 16:
            return QString("temp_consistency_3");
        case 17:
            return QString("voltage_consistency_2");
        case 18:
            return QString("voltage_consistency_3");
        case 19:
            return QString("insulation_limit_2");
        case 20:
            return QString("insulation_limit_3");
        case 21:
            return QString("SOH_limit");
        case 22:
            return QString("SOC_high_limit_1");
        case 23:
            return QString("SOC_high_limit_2");
        case 24:
            return QString("SOC_high_limit_3");
        case 25:
            return QString("SOC_low_limit_1");
        case 26:
            return QString("SOC_low_limit_2");
        case 27:
            return QString("SOC_low_limit_3");
        default:
            return QString("");
        }
    }

}*/

