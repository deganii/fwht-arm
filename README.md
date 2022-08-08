# Fast Walsh-Hadamard Transform (FWHT) lock-in amplification system for the Teensy microcontroller (MCU)

This repository holds the source code for my PhD thesis project

Link to my PhD thesis if you like to read insufferable academic stuff:

https://dspace.mit.edu/handle/1721.1/143262

## What is this?
In a nutshell, this is the source-code for a software-based lock-in-amplifier (LIA) system that runs entirely on a low-cost microcontroller (PJRC/Teensy). For those that are not familiar with LIA's: they essentially boost the signal-to-noise ratio (SnR) of a measurement. Even in a noisy system, you can pick out and measure really faint signals like magic. In our case, we needed an LIA to optically measure minute quantities of fluorescently tagged DNA/RNA in a portable device where (1) stray light could easily creep in, and (2) electrical noise from a wall outlet or the MCU's "chatter," could completely corrupt the measurement.

## Why build an MCU-based LIA?
The most common LIA's are FPGA-based digital systems or analog IC-based systems. The FPGA systems can get super-expensive, and are usually big/bulky rack-mounted devices for high-end physics/telecom applications (i.e. the Zurich UHFLI series). The analog IC's (i.e. ) are cumbersome to customize and configure. You generally have to craft a custom circuit-board for your application and then to change anything you didn't foresee in the design (i.e. the order of the low-pass filter) you've gotta redo the entire board. This is slow and very painful (speaking from experience). 

What I wanted was an ultra-cheap (think $20) LIA that was fully "tinkerable" in software. So for example, if you want to change the modulation waveform to something completely arbitrary, or change the sampling rate to be 'irregular' to squeeze out more performance for a particular type of signal, you could do that. A software LIA also lets you do really sophisticated things that even a high-end FPGA system may not support. For example, you can sample the system noise for some time, and then adaptively create a waveform modulation tailored to subvert that specific noise. (credit for that excellent idea goes to my thesis commitee member, Prof. Luca Daniel). The tradeoff is that you can't get to MHz-level modulations; for that you need to use a FPGA or a high-end analog system. This MCU system tops out at 250kHz or so. 

## WTF is FWHT?

If you're familiar with the Fast Fourier Transform (FFT), the Fast Walsh-Hadamard transform (FWHT) is very similar but it decomposes a signal into irregular square waves (known as Walsh Functions) instead of sines and cosines. In the context of an LIA, if you suspect that the noise you'll encounter is more "digital" or abrupt in nature (i.e. MCU chatter vs gaussian noise), then you might be able to filter it more efficiently/effectively using the FWHT. Another benefit is that you get more RMS power out of your modulation signal because a square wave jumps to its max value immediately (rather than slowly transitioning like a sine wave - my thesis is more detailed about this). A third benefit is that FWHT sounds hard and complicated, which makes it great for an academic publication or thesis. 

## Video of system in action:

This video needs to be narrated, but the idea is that I'm loading up a fluorescent sample into an open-air prototype with no light-proofing. As I'm waving my hand around, blocking and unblocking the overhead lights, I'm simulating optical noise which is wreaking havoc on the sensitive light-based measurement (the measurement is bouncing around in the upper-right quadrant). I turn on the lock-in amplifier at 1:09, and you can see order emerge from the chaos. The outside light perturbation no longer has an effect on the signal. It's just a  steady line with a gentle slope representing the photobleaching rate of the fluorophore. Essentially, this demonstrates that at very low cost you can bring powerful noise-resiliency to small/portable sensing devices.

[![Qt5-UI for Fast Walsh-Hadamard Lockin Amplifier](https://github.com/deganii/fwht-arm/blob/main/img/ui_play.png?raw=true)](https://www.youtube.com/watch?v=tHf3V4GTSLU)

## Can I download and fire this up for my project due next week?

Probably not. This is research software so it's very messy and not even close to a usable product. And it's very customized to our biomedical application. I've uploaded a binary release but the chances that you can use this as-is are pretty slim. 

Most likely I think this repository's main benefit will be people stumbling here via some keyword code-search, and finding a snippet or two that they need for their Teensy, Qt3d, or QCustomplot project. It might be a good starting point to design a re-usable LIA library or platform. 

## Features

The GUI has lots of knobs etc where you can control the input modulation waveform. You can choose coustom-select the Fourier Frequencies (or the Walsh 'Sequencies') you want to use, or an adaptive method that samples the noise. You can also do frewuency/sequency sweeps across all modulation frequencies.

![alt text](https://github.com/deganii/fwht-arm/blob/main/img/waveform_editor.png?raw=true)

The Teensy is a remarkable microntroller. Right now it produces a 12-bit waveform on its onboard DAC, and then samples 12-bit raw data (i.e. before any cross-correlation or demodulation) at 200kHz using its onboard ADC. Both the sampling and modulation are controlled by DMA so they are very efficient. The ADC reads are precisely synchronized with the DAC using timers and the Teensy's programmable delay block (the Teensy waits for the DAC signal to "settle" before measuring, see below image). The Teensy finally sends a real-time data stream to the Qt-based UI (about 1MB/second) which does the post processing and display. The modulation waveform can be customized on-the-fly, as shown in the video.

![alt text](https://github.com/deganii/fwht-arm/blob/main/img/adc_settling.png?raw=true)

## Acknowledgements:

Most of the research-side acknowledgments are in the thesis itself. But here I'd like to give special thanks to all of the generous folks on GtHub, the PJRC and Qt/QcustomPlot forums. The software-side of this project would have been impossible without the valuable code snippets/recipes and discussions on those sites. 

## License: MIT

I've tried to attribute all code snippets borrowed from forums and other places but please contact me if I've left something out. 

