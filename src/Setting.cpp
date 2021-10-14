#include "Setting.h"

SettingMode Setting::next(SettingMode mode) {
    switch (mode) {
        case SettingModeSetting:
            return SettingModeBacklash;
        case SettingModeBacklash:
            return SettingModeMeasurementSystem;
        case SettingModeMeasurementSystem:
            return SettingModeInvertFeed;
        case SettingModeInvertFeed:
            return SettingModeSetting;
        case SettingModeNone:
            return SettingModeNone;
    }
    return SettingModeNone;
}