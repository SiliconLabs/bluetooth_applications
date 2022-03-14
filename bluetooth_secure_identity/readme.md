# Secure Device Attestation and Application Layer Encryption

## Description

This is a demonstration of a method for establishing secure identity over BLE by  doing the following:

1. Exchanging device identity certificate chains

2. Key agreement signed with the device identity key established in the previous step

   Please see AN1302 for a more detailed explanation of the communication flow.

## Gecko SDK Version

GSDK v3.2.2

## Hardware Required

- Client EFR32MG21B 
- Server EFR32MG22

## Connections Required

## Setup

This demonstration requires two projects: client and server. Specific instructions for each project are found in readme.md files in their respective directories. The client and server project must both be built and flashed to the specified target boards for the demonstration to function as intended.

