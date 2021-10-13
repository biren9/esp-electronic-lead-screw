#include "Setting.h"

SettingMode Setting::next(SettingMode mode) {
    switch (mode) {
        case setting:
            return backlash;
        case backlash:
            return measurementSystem;
        case measurementSystem:
            return setting;
    }
}