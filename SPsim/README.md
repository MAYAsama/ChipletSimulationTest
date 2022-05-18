## Table of Contents

- [Table of Contents](#table-of-contents)
- [Introduction](#introduction)
- [Setup](#setup)
- [How to run](#how-to-run)
- [Configuration parameters](#configuration-parameters)

## Introduction

SPsim is a NoC simulator capable of modular design of each sub-network in a 2.5D NoC. With the node mapping-based routing mechanism and the event queue-based handshake communication scheme, we can correctly reflect the characteristics of 2.5D NoC and perform fast simulations. SPsim has been verified against other RTL verified simulators.

## Setup

**1. Required Dependencies**

### gcc
### g++
### flex
### bison

**2. Compiling**

> make

## How to run

SPsim can run 2D NoC simulation with a single configuration file or 2.5D simulation with multiple configuration files with type of interposer and chiplet

> ./booksim `<configuration file>` [(over ride configuration separated by space) eg wait_for_tail_credit=1]

> ./booksim interposer chip1 chip2 chip3 chip4

## Configuration parameters
All information used to configure a simulation is passed through one or more configuration files.
Major configuration parameters for the simulator can be found in the configure files mentioned above.
Additional description can be found in the configuration parameter class file booksim_config.cpp.
A user can incorporate additional options by changing the this file.





  
