# STARS Scenario for AGC Test Case

## LEO configuration

+ Starlink 550
+ number of satellites: 1584
+ node IDs: 
  - min: 0
  - max: 1583

## Ground station configuration

+ N 5-tuples of ground stations
  - 1 ground station representing the control center
  - 1 ground station representing the STARLINK ground station
  - 3 ground stations representing the substations of the power system model
+ the N 5-tuples are evenly distributed over the globe with same latitude, effectively providing the same setup with different orbital start conditions
+ node IDs:
  - min: 1584
  - max: 1583 + N * 5
+ N = 50

### Main configuration (N=0)

+ Control Center: Wien (APG)
  + Node ID: 1584
  + latitude: 48.139142
  + longitude: 16.4329859
+ Starlink ground station: Erdfunkstelle Usingen (Frankfurt)
  + Node ID: 1585
  + latitude: 50.333416
  + longitude: 8.479418
+ Substation 1: Kaprun
  + Node ID: 1586
  + latitude: 47.2725
  + longitude: 12.759444
+ Substation 2: Linz
  + Node ID: 1587
  + latitude: 48.305833
  + longitude: 14.286389
+ Substation 3: Graz
  + Node ID: 1588
  + latitude: 47.070833
  + longitude: 15.438611
