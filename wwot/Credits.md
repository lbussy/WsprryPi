# Credits

While the curent version of Wsprry Pi is a complete re-write and contains no original code, it was absolutely inspired by all who came before. I found it through TAPR and Bruce Raymond provided the inspiraiton and guidance to be whhile I developed the first version published here.

My first versiion focused on useability, specifically installaiton and configuration scripts, extending what looked to be Guido's (PE1NNZ <pe1nnz@amsat.org>) work.  His repo is no longer on GitHub.

The folowing is an intreesting history that's at least notionally close to the development the forks and merges had taken in the past, obtained from James Peroulas's <james@peroulas.com> [GitHub repo](https://github.com/JamesP6000/WsprryPi/blob/master/README):

> Credits goes to Oliver Mattos and Oskar Weigl who implemented PiFM based on the idea of exploiting RPi DPLL as FM transmitter. [^1]
>
> Dan MD1CLV combined this effort with WSPR encoding algorithm from F8CHK, resulting in WsprryPi a WSPR beacon for LF and MF bands. [^2]
>
> Guido PE1NNZ <pe1nnz@amsat.org> extended this effort with DMA based PWM modulation of fractional divider that was part of PiFM, allowing to operate the WSPR beacon also on HF and VHF bands.  In addition time-synchronisation and double amount of power output was implemented. [^3]
>
> James Peroulas <james@peroulas.com> added several command line options, a makefile, improved frequency generation precision so as to be able to precisely generate a tone at a fraction of a Hz, and added a self calibration feature where the code attempts to derrive frequency calibration information from an installed NTP deamon.  Furthermore, the TX length of the WSPR symbols is more precise and does not vary based on system load or PWM clock frequency. [^4]
>
> Michael Tatarinov for adding a patch to get PPM info directly from the kernel.
>
> Retzler András (HA7ILM) for the massive changes that were required to incorporate the mailbox code so that the RPi2 and RPi3 could be supported.
>
> [^1] PiFM code from: [Turning_the_Raspberry_Pi_Into_an_FM_Transmitter](http://www.icrobotics.co.uk/wiki/index.php/Turning_the_Raspberry_Pi_Into_an_FM_Transmitter)
>
> [^2] Original [WSPR Pi transmitter](https://github.com/DanAnkers/WsprryPi) code by Dan.
>
> [^3] [Fork created by Guido](https://github.com/threeme3/WsprryPi).
>
> [^4] [This fork created by James](https://github.com/JamesP6000/WsprryPi).
