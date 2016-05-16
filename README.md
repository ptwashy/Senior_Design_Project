# Senior Design Project

This was the project I completed for the Senior Design elective required by the University of Pittsburgh's Swanson School of Engineering.

My group was composed of Philip Washy (me), Nathan Dorman, James Arnold, and Joseph McClain. I built the hardware for the project with a small amount of help from Nathan, as well as writing the code for running and interfacing with the hardware. Nathan and I got our completed units connected to the PittNet wireless network. We also designed the remote communication system which he then implemented. Nathan and Joseph designed the Graphical User Interface, and Joseph and James completed the Raspberry Pi setup and troubleshooting. 

##### References
Any direct references to the function of the 328p are taken from [this document](http://www.atmel.com/images/Atmel-8271-8-bit-AVR-Microcontroller-ATmega48A-48PA-88A-88PA-168A-168PA-328-328P_datasheet_Complete.pdf "328p Complete Datasheet") and will be referenced as _(ATM, \<page number\>)_. The functionality of the Arduino Uno is referenced from its listed information [here](https://www.arduino.cc/en/main/arduinoBoardUno "Arduino Website") as _(ARD)_. eRCaGuy\_Timer2\_Counter can be found [here](https://github.com/ElectricRCAircraftGuy/eRCaGuy_TimerCounter "eRCaGuy GitHub for Timer2") and will be referenced as _(RCG)_. The arduino-serial-lib files come from [here](https://github.com/todbot/arduino-serial "GitHub for Serial Communication Code") and is referenced as _(ASL)_.

##### Notice
This is not the full set of software included in the work submitted to my group's client as well as in the Senior Design final report. Included here are only the portions that I myself wrote


## /Senior\_Design
In this folder is included the Arduino code which I wrote for my group's project. For the most part the Algorithms used in this code are the same as what was being used when the project was given to us. Major changes include runtime logic improvements and the creation of a state machine for runtime so that the Arduino no longer needed to be reset upon each read of a phase offset. All comments and documentation within the project code were written by me.

As a requirement for this project we were to use an Arduino Uno to calculate the phase offset of the signal. The included eRCaGuy\_Timer2\_Counter files are libraries that were used by the previous team for the project in order to optimize their project's accuracy given the limits of a general-purpose microcontroller like the 328p used on the Arduio Uno. The way that this was used was to count in between given triggers, but the accuracy was severely limited by the accuracy of the stop on the counter. My group after testing their design found a few interesting things. By writing test code to stop the count of the timer on a trigger and then using a function generator to create 50% duty cycle square waves, we were able to test the maximum accuracy of the count. By changing the frequency of the trigger signal we noticed that the highest trigger frequency that maintained near perfect accuracy for reading the stop of the timer was 30 kHz. When we increased the frequency we saw that the signal was little-by-little increasingly aliased to half its sampling frequency until at 50kHz we found that almost every reading was aliased. According to _(RCG)_ the timer has an accuracy of 4μs, which would mean that the system should be able to handle a trigger signal oscillating at 250kHz, but my group could not replicate these results. This could be due to a number of reasons including optimization issues, but it was of no concern to us after half a day spent attempting to produce a 4μs accuracy and not succeeding as we did not want to waste time.

By removing the off-board comparator from our system and adjusting the on-board ADC settings we were able to get a sample rate of 125kHz. Because this code is not in the final project, I will be giving examples of the general setup along with the explanation Our method of accomplishing this was to first change the prescalar divider on the ADC clock on the Arduino Uno to the absolute minimum. We know that the prescalar divider register is 3 bits wide and produces results as follows _(ATM, 250)_:

| ADPS2 | ADPS1 | ADPS0 | Division Factor |
| ----- | ----- | ----- | --------------- |
| 0 | 0 | 0 | 2 |
| 0 | 0 | 1 | 2 |
| 0 | 1 | 0 | 4 |
| 0 | 1 | 1 | 8 |
| 1 | 0 | 0 | 16 |
| 1 | 0 | 1 | 32 |
| 1 | 1 | 0 | 64 |
| 1 | 1 | 1 | 128 |

With the clock rate of the Arduino Uno known to be 16MHz _(ARD)_ we can see that the highest clock speed for the on-board ADC will be only 8MHz. This however is not the sample rate, as it takes 13-25 clock cycles to do a single converion. By setting ADPS to 000 with
```C
cbi( ADCSRA, ADPS2 );
cbi( ADCSRA, ADPS1 );
cbi( ADCSRA, ADPS0 );
```
After setting these bits and testing we found that we were getting a maximum sample rate of 125kHz, which meant that at the number of cycles required to read the signal was approximately 64. We thought it might make a difference to skip the ADC in the process and use only the analogue comparator in the 328p _(ATM, 234)_. In order to accomplish this, we set ACME to 1 and used it as a trigger to count when the signal was high. We found that we were getting a count rate of 125,000 samples/second, equivalent to the 125kHz sample rate we had achieved with the comparator, leading ust to believe that the comparator is still limited by the ADC clock in hardware. Because our client was not pleased with our improvements to sample rate, we did not use this code.

We noted while working on this project that there was a standard offset from the known phase angle that kept appearing. Because it was standard across all of the function generators, we knew there needed to be a single source. What we ended up discovering is that the incoming synchronization pulse was increasing the phase offset by the amount of the sampled signal's phase it covered. In order to allow for recallibration to different synchronization pulses we wrote into the code the ability to change a prescalar to the output. We had put some thought and effort into setting up an autocallibration feature but we ran out of time before we could really put any of those ideas into full swing.

## /Senior\_Design\_mgmt
This is heavily based upon the code given in _(ASL)_. The functionality of the library was left unchanged and an interfacing program was written to accomplish tasks specific to my group's project.
