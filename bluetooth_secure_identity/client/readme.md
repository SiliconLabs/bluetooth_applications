# Client
![Type badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/client_common.json&label=Type&query=type&color=green)
![Technology badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/client_common.json&label=Technology&query=technology&color=green)
![License badge](https://img.shields.io/badge/dynamic/json?url=https://raw.githubusercontent.com/SiliconLabs/application_examples_ci/master/bluetooth_applications/client_common.json&label=License&query=license&color=green)

## Description

This file describes the use of the client project.

The client acts as a central device for the accompanying server project in this repository. The main task of this device is to establish a secure identity exchange with the server.The Behaviour is as follows:

1. Scans for a server advertising the 'secure attestation' service and opens a connection

2. Performs service discovery to obtain handles for all characteristics

3. Requests the server's secure identity certificate chain and verifies it

4. Sends a random number challenge to the server to be signed with it's private device signing key.

5. Verifies the signature of the challenge.

6. Sends it's own secure identity certificate chain to the server.

7. Upon receiving a random number challenge from the server, signs the challenge with it's private device signing key and responds with the signature.

8. Initiates an elliptic curve Diffie-Helman (ECDH) key exchange to establish a shared secret by reading the 'server_pubkey' characteristic.

9. On receiving a signed public key from the server, the signature is verified using the pubic key from the server's  device certificate.

10. On successful verification of the server's public key, the client generates an ECDH keypair, signs the public key with it's private device key then sends the public key and signature back to the server. If the verification of the public signature fails, an assert() occurs which requires a device reset.

11. The shared secret is hashed with SHA2/256 to produce a symmetric key for securing communications between the client and server.

12. The client reads the 'test_data' characteristic and decrypts the message received using the key created in the previous step.

    ### Notes

    - The UUIDs for characteristics and the 'secure attestation' service are used to correctly identify each attribute. A list is maintained in app.h.
    - The EFR32MG21B was chosen for the client since it has certificates programmed by Silicon Labs at the factory.

## Gecko SDK Version

GSDK v3.2.2

## Hardware Required

EFR32MG21B

## Connections Required

WSTK must be connected to computer through USB cable

## Setup

1. Import the project (sls file) in the SimplicityStudio folder into your workspace and build the project.

2. Flash 'secure_attestation_client.hex' to the target board.

3. Open a serial console in Simplicity Studio and select the 'serial 1' tab.

4. Ensure that the server is plugged in and running

5. Press reset on the WSTK to start scanning.

6. The serial console will display a series of messages as shown below

   ![](images\client-console-output.PNG)