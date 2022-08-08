# Fast Walsh-Hadamard Transform (FWHT) lock-in amplification system for the Teensy microcontroller (MCU)
(Source code and data for my PhD thesis project)

Link to my PhD thesis if you like to read insufferable academic stuff:
https://dspace.mit.edu/handle/1721.1/143262

## What is this?
In a nutshell, this is the source-code for a software-based lock-in-amplifier (LIA) system that runs entirely on a low-cost microcontroller (PJRC/Teensy). For those that are not familiar with LIA's: they essentially boost the signal-to-noise ratio (SnR) of a measurement. Even in a noisy system, you can pick out and measure really faint signals like magic. In our case, we needed an LIA to optically measure minute quantities of fluorescently tagged DNA/RNA in a portable device where (1) stray light could easily creep in, and (2) electrical noise from a wall outlet or the MCU's "chatter," could completely corrupt the measurement.

## Why build an MCU-based LIA?
The most common LIA's are FPGA-based digital systems or analog IC-based systems. The FPGA systems can get super-expensive, and are usually big/bulky rack-mounted devices for high-end physics/telecom applications (i.e. the Zurich UHFLI series). The analog IC's (i.e. ) are cumbersome to customize and configure. You generally have to craft a custom circuit-board for your application and then to change anything you didn't foresee in the design (i.e. the order of the low-pass filter) you've gotta redo the entire board. This is slow and very painful (speaking from experience). 

What I wanted was an ultra-cheap (think $20) LIA that was fully "tinkerable" in software. So for example, if you want to change the modulation waveform to something completely arbitrary, or change the sampling rate to be 'irregular' to squeeze out more performace for a particular type of signal, you could do that. A software LIA also lets you do really sohpisticated things that even a high-end FPGA system may not support. For example, you can sample the noise for some time, and then adaptively create a waveform modulation that is tailored to subvert that specific noise. (BTW credit for that awesome suggestion goes to my thesis commitee member, Prof. Luca Daniel). The tradeoff is that you can't get to MHz-level modulations; for that you need to use a FPGA or high-end analog system. This MCU system tops out at 250kHz or so. 

## Video of system in action:

This video needs to be narrated, but the idea is that I'm loading up a fluorescent sample into an open-air prototype with no light-proofing. As I'm waving my hand around, I'm simulating optical noise which is wreaking havoc on the sensitive light-based measurement (the measurement is bouncing around in the upper right quadrant). I turn on the lock-in amplifier at 1:09, and you can see order emerge from the chaos. The outside light perturbation no longer has an effect on the signal. It's just a  steady line with a gentle slope representing the photobleaching rate of the fluorophore. So at very low cost you can bring noise-resiliency to small portable sensors.

[![Qt5-UI for Fast Walsh-Hadamard Lockin Amplifier](https://img.youtube.com/vi/tHf3V4GTSLU/0.jpg)](https://www.youtube.com/watch?v=tHf3V4GTSLU)

## Can I download and fire this up for my project due next week?

Probably not. This is research software so it is massively messy and not even close to a usable product. And it is very customized to our biomedical application. I've uploaded a binary release but the chances that you can use this as-is are pretty slim. 

Most likely I think this repository's main benefit will be people stumbling here via some keyword code-search, and finding a snippet or two that they need for their Teensy, Qt3d, or QCustomplot project. It might be a good starting point to design a re-usable library or platform. 

## Features

The GUI has lots of knobs etc where you can control the input modulation waveform, and choose from some pre-configured options:

The Teensy is really a remarkable microntroller! 

communicates to a Qt5-based  UI that can (theoretically) run on any platform.

Right now it blasts raw data (i.e. before any cross-correlation or demodulation) over USB and PC interface does the post processing. (

200kHz with 

## Acknowledgements:

Most of the research-side acknowledgments are in the thesis itself. But here I'd like to give special thanks to all of the generous folks on GtHub, the PJRC and Qt/QcustomPlot forums. The software-side of this project would have been impossible without the valuable code snippets/recipes and discussions on those sites. 

## License: MIT

I've tried to attribute all code snippets borrowed from forums and other places but please contact me if I've left something out. 
This is just research/non-commmercial software developed under 

