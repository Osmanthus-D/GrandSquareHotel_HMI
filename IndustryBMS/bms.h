#ifndef BMS_H
#define BMS_H
/***********************************************************
*
*          ┌─┐       ┌─┐
*       ┌──┘ ┴───────┘ ┴──┐
*       │                 │
*       │       ───       │
*       │  ─┬┘       └┬─  │
*       │                 │
*       │       ─┴─       │
*       │                 │
*       └───┐         ┌───┘
*           │         │
*           │         │
*           │         │
*           │         └──────────────┐
*           │                        │
*           │                        ├─┐
*           │                        ┌─┘
*           │                        │
*           └─┐  ┐  ┌───────┬──┐  ┌──┘
*             │ ─┤ ─┤       │ ─┤ ─┤
*             └──┴──┘       └──┴──┘
*
************************************************************/

/**********************Global config************************/
#define VERSION                                             ("V2.2.1")
#define APP_DATA_DIRPATH                                    ("/mnt/userdata2/appdata")
#define LOG_PATH                                            ("/mnt/userdata2/appdata/log")
#define DATA_PATH                                           ("/home/asd/appdata/data.csv")
#define CONFIG_PATH                                         ("/home/asd/appdata/bms.cfg")
#define CONFIG_PATH_OTHERS                                  ("/home/asd/appdata/bms_others.cfg")
#define SCRIPT_DIR_NAME                                     ("script")

#if defined(YCTEK)
#define USB_PATH                                            ("/mnt/usb")
#elif defined(WEIQIAN)
#define USB_PATH                                            ("/mnt/usbdisk/sda1")
#else
#define USB_PATH                                            ("/")
#endif

#if defined(YCTEK)
#define SD_PATH                                             ("/media/mmcblk0p1")
#elif defined(WEIQIAN)
#define SD_PATH                                             ("/mnt/sdcard/sd")
#else
#define SD_PATH                                             ("/")
#endif

#define MAX_WARNING_NUM_PER_MODULE                          (128)
#define MAX_PARAM_NUM_PER_MODULE                            (128)
#define MAX_PARAM_ROW                                       (11)
#define MAX_PARAM_COLUMN                                    (2)
#define MAX_PARAM_IN_PAGE                                   (MAX_PARAM_ROW * MAX_PARAM_COLUMN)
#define MAX_CHECKBOX_NUM_IN_ROW                             (6)
#define MAX_MODULE_NUM                                      (18)
#define DEFAULT_MODULE_NUM                                  (6)
#define PCS_NUM                                             (2)
#define MAX_PACK_NUM                                        (25)
#define PACK_NUM                                            (24)
#define BAT_NUM                                             (16)
#define TEMP_NUM                                            (8)
#define BATTERY_NUM_PER_MODULE                              (MAX_PACK_NUM * BAT_NUM)
#define TEMP_NUM_PER_MODULE                                 (MAX_PACK_NUM * TEMP_NUM)

#define CAN_TIMEOUT                                         (60)
#define CAN_TIMEOUT_SIGNEL                                  (20)
#define LOG_MAX_COUNT                                       (22)
#define LOG_MAX_PAGE                                        (300)
#define LOG_FILE_MAX_SIZE                                   (10000000)
#define DISK_CLEAR_THRESHOLD                                (20 << 20)                  // clear disk when free space < 20MB

//#define PROJECT_165
#define XINYI_EMS

#ifdef PROJECT_165
#define DATA_FILE_MAX_SIZE                                  (1000000)
#else
#define DATA_FILE_MAX_SIZE                                  (500000000)
#endif

// LH-IO606
#define DIGITAL_OUTPUT_BLOCK                                (6)
#define DIGITAL_INPUT_BLOCK                                 (6)
#define MAX_INPUT_NUM                                       (30)
#define MAX_OUTPUT_NUM                                      MAX_INPUT_NUM

// peripheral num
#define TEMP_HUMIDITY_METER_NUM                             (5)
#define DC_INSULATION_MONITORING_DEVICE_NUM                 (1)
#define AIR_CONDITIONER_NUM                                 (4)
#define ELECTRICITY_METER_NUM                               (2)

// to get module id from can id
#define MODULE_ID_MASK                                      (0x1F)

#define PACK_ID_MASK                                        (0x1F)

#define BATTERY_NORMINAL_CAPACITY                           (260)                       // Ah

#endif // BMS_H

