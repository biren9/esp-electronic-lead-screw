#ifndef __Setting_h_
#define __Setting_h_

enum SettingMode {
    SettingModeNone, 
    SettingModeSetting, 
    SettingModeMeasurementSystem, 
    SettingModeBacklash, 
    SettingModeInvertFeed
};

class Setting {
    public:
        static SettingMode next(SettingMode mode);
};

#endif