//
// Created by Ismail Degani on 7/1/2021.
//

#ifndef UI_CONSTANTS_H
#define UI_CONSTANTS_H

static const char *const KEY_MODE = "Mode";

static const char *const KEY_BASIS = "Basis";

static const char *const KEY_RANGE_START = "RangeStart";

static const char *const KEY_RANGE_END = "RangeEnd";

static const char *const KEY_NOISE_INTERVAL = "NoiseInterval";

static const char *const KEY_HOP_FREQ = "HopFreq";

static const char *const KEY_WAVEFORM_LENGTH = "WaveformLength";

static const char *const KEY_NUM_FUNCTIONS = "NumFunctions";

static const char *const KEY_CUSTOM_FUNCTIONS = "CustomFunctions";

static const char *const VAL_BASIS_FOURIER = "Fourier";

static const char *const VAL_BASIS_WALSH = "Walsh";

static const char *const VAL_MODE_ADAPTIVE = "Adaptive";

static const char *const VAL_MODE_CUSTOM = "Custom";

static const char *const VAL_MODE_CHIRP = "Chirp";

static const char *WAVEFORM_SETTINGS[]{
    KEY_MODE,
    KEY_BASIS,
    KEY_RANGE_START,
    KEY_RANGE_END,
    KEY_NOISE_INTERVAL,
    KEY_HOP_FREQ,
    KEY_WAVEFORM_LENGTH,
    KEY_NUM_FUNCTIONS,
    KEY_CUSTOM_FUNCTIONS
//    VAL_BASIS_FOURIER,
//    VAL_BASIS_WALSH,
//    VAL_MODE_ADAPTIVE,
//    VAL_MODE_CUSTOM,
//    VAL_MODE_CHIRP
};

namespace Ui {
    Q_NAMESPACE

    enum class WaveformMode {
        Adaptive,
        Chirp,
        Custom
    };
    enum class BasisFunctions {
        Walsh,
        Fourier
    };

    Q_ENUM_NS(WaveformMode);
    Q_ENUM_NS(BasisFunctions);

}

#endif //UI_CONSTANTS_H
