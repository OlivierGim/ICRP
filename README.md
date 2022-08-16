ICRP
====
This repository contains the code for ICRP prototype and the data set used for experiments validating the ICRP methodology.

Mesures
-------
The compressed file Measures.zip contains the all the gathered measures that were used to observe the time behaviour of UDP Round Trip Time.  
Each file contains the timings involved in the communication between 2 nodes. For some of them, the measures were performed with the simulation of a relay attack. When this is the case, the file name is prefixed with the three letters 'hij'.

Each file is organized as follows:  
#### A - For measures without a relay:  
  1. Line 1 indicates the day and time at which the measure ended.  
  2. Line 2 indicates the public IP of the party not involved in the measurements.  
  3. Line 3 indicates the locations of the communicating nodes in the form 'Source -> Destination'.  
  4. Line 4 indicates if the socket file descriptor in use on sender's side was always active (SAO=1) or if it was closed between each packet sendings (SAO = 0).  
  5. Line 5 indicates the number of packet sent during the exchange and so, the number of collected Round Trip Times for the sample. This number refered to as n in the following.  
  6. Line 6 indicates the size of each packets in bytes. This number is refered to as p in the following.  
  7. Line 7 indicates the waited time between the sending of each packets during the measure.
  8. Line 8 and 9 display the table of collected Round Trip Times in chronological order in microseconds (µs).  
  9. Each file name is the hour of the day to the seconds corresponding to the ending of the measure.  
  10. Each file is stored according to the following path: Source-\_Destination/nxp/date/filename.txt  
#### B - For measres with a relay:  
Files including the 'hij' prefix on their name refer to a measure from a simulated relay.  
The files are organized in the same fashion as descripted in part **A** with the exception that a marker stated that the communication has been relayed is placed at Line 8 right before the table of collected Round Trip Times  

Prototype Implementation
------------------------
