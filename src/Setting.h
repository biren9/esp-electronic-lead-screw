enum SettingMode {none, setting, measurementSystem, backlash};

class Setting {
    public:
        static SettingMode next(SettingMode mode);
};