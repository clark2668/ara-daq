# IceCalSoftware
This is the IceCalSoftware from the ARA SVN. Imported by B. Clark on June 3 2017 from Revision 2904 of the [ARA SVN] (http://svnmsn.icecube.wisc.edu/ara/Software/ATRI_DAQ/IceCalSoftware/).

From the Original README:

Sample Output For Port B (on ACE card)

  CALPULSER OPTIONS
  -----------------

  noise source : in opB3 [ROM3] mode, I can see the continuous noise and, using the antB0 [RAS0]
                 and antB1 [RAS1] commands, I can switch between the two antenna outputs.  I then
                 switched to opB4 [ROM4] mode and saw that the noise source pulsed for one second
                 in sync with the TTL trigger that I was inputting to it via the TRIG input at 0.25 Hz.  I
                 then switched to opB5 [ROM5] mode and saw that the noise source pulsed for one
                 second at 0.5 Hz; this is the every other pulse from the internal 1 Hz trigger as the
                 output was unaffected by whatever I did to the input TTL trigger.  When I initially turned
                 it on, the RMS of the noise was around 5 mV.  But, I could change that up to several
                 100 mV by changing the attenuator with the attB0 [RSA00] through attB31 [RSA31]
                 commands.  In all cases, I got echo output from the icecald program ... for example:

  pulse source : in opB1 [ROM1] mode, I saw that it pulsed in sync with the TTL trigger and I could adjust
                 the attenuator using the attB0 [RSA00] through attB31 [RSA31] commands.  When I had
                 the attenuator dialed all the way up, though, I

  SAMPLE OUTPUT
  -------------

  ara@ara2:~/repositories/ara/IceCalSoftware.BACKUP_04$ ./icecald probe antB1
  I2C Bus  - located an ACE-2
  icecal: sending RAS1 to port B
  02 52 41 53 31 04 
  icecal: (6 bytes) received message RAS1
  icecal: response found to RAS on port B
  ara@ara2:~/repositories/ara/IceCalSoftware.BACKUP_04$ ./icecald probe attB15
  I2C Bus  - located an ACE-2
  icecal: sending RSA15 to port B
  02 52 53 41 31 35 04 
  icecal: received message RSA15
  ara@ara2:~/repositories/ara/IceCalSoftware.BACKUP_04$ ./icecald probe antB0
  I2C Bus  - located an ACE-2
  icecal: sending RAS0 to port B
  02 52 41 53 30 04 
  icecal: (6 bytes) received message RAS0
  icecal: response found to RAS on port B

