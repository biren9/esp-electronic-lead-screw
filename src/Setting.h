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