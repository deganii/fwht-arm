# Fast Walsh-Hadamard Transform (FWHT) lock-in amplification system for the Teensy micrcontroller (MCU)
(Source code and data for my PhD thesis project)

Link to thesis:
https://dspace.mit.edu/handle/1721.1/143262

## What is this?
In a nutshell, this is the source-code for a software-based lock-in-amplifier (LIA) system that runs entirely on a low-cost microcontroller (PJRC/Teensy). For those that are not familiar with LIA's they essentially boost the signal-to-noise ratio (SnR) of a measurement. So even if you have a noisy system, you can pick out and measure really faint signals. In our case, we needed LIA's to measure minute quantities of fluorescently tagged DNA/RNA using an optical sensor (photodiode). 

## Why build an MCU-based LIA?
The most common LIA's are FPGA-based digital systems or analog IC-based systems. The FPGA systems can get super-expensive, and are usually big/bulky rack-mounted devices for high-end physics/telecom applications (i.e. the Zurich UHFLI series). The analog IC's (i.e. ) are cumbersome to customize and configure. You have to build a custom circuit-board and then to change anything you didn't foresee in the design (i.e. the order of the low-pass filter) you've gotta redo the circuit board. This is slow and painful (speaking from experience). 

What I wanted was an ultra-cheap (think $20) LIA that was fully "tinkerable" in software. So for example, if you want to change the modulation waveform to something completely arbitrary, or change the sampling rate to be 'irregular' to squeeze out more performace for a particular type of signal. A software LIA also lets you do really complex things that even a high-end FPGA may not support. For example, what if you want to sample the noise for some time, and then adaptively create a waveform modulation that was tailored to that noise. (This awesome suggestion was made by my thesis commitee member, Prof. Luca Daniel).

# Features / Notes

This is research software so it is massively messy and not even close to a usable product. The GUI has lots of knobs etc where you can control the input modulation waveform, and choose from some pre-configured options. 

The Teensy is really a remarkable microntroller! 

communicates to a Qt5-based  UI that can (theoretically) run on any platform.

# Video in action:

[![Qt5-UI for Fast Walsh-Hadamard Lockin Amplifier](https://img.youtube.com/vi/tHf3V4GTSLU/0.jpg)](https://www.youtube.com/watch?v=tHf3V4GTSLU)

Acknowledgements:
Most of the research-side acknowledgments are in the thesis itself. But here I'd like to give special thanks to all of the generous folks on GtHub, the PJRC and Qt/QcustomPlot forums. The software-side of this project would have been impossible without the valuable code snippets/recipes and discussions on those sites. 

License: MIT
I've tried to attribute all code snippets borrowed from forums and other places but please contact me if I've left something out. 
As most PhD's will attest, these dissertations and are often written/prepared under let's say 'non-ideal' conditions and 


