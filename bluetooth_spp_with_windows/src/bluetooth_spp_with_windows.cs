/***************************************************************************//**
 * @file bluetooth_spp_with_windows.cs
 * @version 1.0.1
 *******************************************************************************
 * # License
 * <b>Copyright 2020 Silicon Laboratories Inc. www.silabs.com</b>
 *******************************************************************************
 *
 * SPDX-License-Identifier: Zlib
 *
 * The licensor of this software is Silicon Laboratories Inc.
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 *    claim that you wrote the original software. If you use this software
 *    in a product, an acknowledgment in the product documentation would be
 *    appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be
 *    misrepresented as being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 *
 *******************************************************************************
 * # Experimental Quality
 * This code has not been formally tested and is provided as-is. It is not
 * suitable for production environments. In addition, this code will not be
 * maintained and there may be no bug maintenance planned for these resources.
 * Silicon Labs may update projects from time to time.
 ******************************************************************************/

using System;
using System.Collections;
using System.Collections.Generic;
using System.Data;
using System.Data.SqlClient;
using System.Runtime.InteropServices.WindowsRuntime;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using Windows.Devices.Bluetooth;
using Windows.Devices.Bluetooth.GenericAttributeProfile;
using Windows.Devices.Enumeration;
using Windows.Devices.Enumeration.Pnp;
using Windows.Storage.Streams;

namespace ble_console_notifications
{
    class Program
    {
        static BluetoothLEDevice device;
        static DeviceWatcher watcher;
        static string deviceName;

        static bool isDeviceConnected = false;

        // Watcher configurations
        static readonly string[] requestedProperties = { "System.Devices.Aep.DeviceAddress", "System.Devices.Aep.Bluetooth.Le.IsConnectable" };

        // UUIDs for SPP
        static Guid sppServiceUuid = new Guid("4880c12c-fdcb-4077-8920-a450d7f9b907");
        static Guid sppCharacteristicUuid = new Guid("fec26ec4-6d71-4442-9f81-55bc21d658d6");

        // Gatt handlers
        static GattDeviceService sppServiceHandle;
        static GattCharacteristic sppCharacteristicHandle;

        #region BLE Connection Callbacks
        async static void DeviceAdded(DeviceWatcher sender, DeviceInformation deviceInfo)
        {
            // If the random device matches our device name, connect to it.
            //Console.WriteLine("Device found: " + deviceInfo.Name);
            if (deviceInfo.Name.CompareTo(deviceName) == 0)
            {
                // Get the bluetooth object and save it. This function will connect to the device
                device = await BluetoothLEDevice.FromIdAsync(deviceInfo.Id);
                //Console.WriteLine("Connected to " + device.Name);

                isDeviceConnected = true;
            }
        }

        static void DeviceUpdated(DeviceWatcher sender, DeviceInformationUpdate deviceInfoUpdate)
        {
            
        }

        static void DeviceEnumerationComplete(DeviceWatcher sender, object arg)
        {
            sender.Stop();
        }

        static void DeviceStopped(DeviceWatcher sender, object arg)
        {
            // Device has stopped and disconnected. Start searching for devices again.
            isDeviceConnected = false;
            sender.Start();
        }
        #endregion

        #region BLE Characteristic Callbakcs
        static void CharactisticUpdated(GattCharacteristic sender, GattValueChangedEventArgs args)
        {
            var reader = DataReader.FromBuffer(args.CharacteristicValue);
            string str = reader.ReadString(args.CharacteristicValue.Length);
            Console.Out.Write(str);

            // Windows doesn't handle new line i.e. pressing ENTER key will make send '\r'. This if
            // statement is to handle a new line if the return key is every read.
            if (str.Contains("\r"))
            {
                Console.Out.Write('\n');
            }
        }
        #endregion

        static void QueryForDevices()
        {
            watcher = DeviceInformation.CreateWatcher(
                                            BluetoothLEDevice.GetDeviceSelectorFromDeviceName(deviceName),
                                            requestedProperties,
                                            DeviceInformationKind.AssociationEndpoint);
            watcher.Added += DeviceAdded;
            watcher.Updated += DeviceUpdated;
            watcher.EnumerationCompleted += DeviceEnumerationComplete;
            watcher.Stopped += DeviceStopped;
            watcher.Start();
        }

        static async Task SubscribeToDeviceSPP()
        {
            // -----------------------------------------------------------------------------
            // Find the spp_data characteristic in the service.

            // Get all of the characteristics in the selected service.
            GattCharacteristicsResult result = await sppServiceHandle.GetCharacteristicsAsync(BluetoothCacheMode.Uncached);
            if(result.Status == GattCommunicationStatus.Success)
            {

                // -------------------------------
                // Get the spp_data characteristic. Parse through the list.
                foreach (var charateristic in result.Characteristics)
                {
                    if(charateristic.Uuid == sppCharacteristicUuid)
                    {
                        sppCharacteristicHandle = charateristic;    // save the characteristic for later use

                        // -------------------------------
                        // Write to descriptor. Even though the descriptor does not exist in the BLE device's GATT database,
                        // this needs to be called to get notifications to work. (Do not know why...)
                        GattCommunicationStatus status = await sppCharacteristicHandle.WriteClientCharacteristicConfigurationDescriptorAsync(GattClientCharacteristicConfigurationDescriptorValue.Notify);
                        if(status == GattCommunicationStatus.Success)
                        {
                            sppCharacteristicHandle.ValueChanged += CharactisticUpdated;    // Add callback whenever BLE device notifies windows
                        }

                    }
                }
            }

            // -----------------------------------------------------------------------------
            // Could not find service. User needs to restart program.
            if (sppCharacteristicHandle == null)
            {
                throw new Exception("Could not find characteristic on your device. Please make sure your device has the correct gatt UUID.");
            }
        }

        static async Task FindSppService()
        {
            // -----------------------------------------------------------------------------
            // Save the SPP service.

            // Get all of the services from the device then save the SPP service.
            GattDeviceServicesResult serviceResult = await device.GetGattServicesForUuidAsync(sppServiceUuid, BluetoothCacheMode.Uncached);
            if (serviceResult.Status == GattCommunicationStatus.Success)
            {

                // -------------------------------
                // Parse all of the services that Windows can see
                foreach (GattDeviceService service in serviceResult.Services)
                {
                    if (service.Uuid == sppServiceUuid)
                    {
                        sppServiceHandle = service;
                    }
                }
            }

            // -----------------------------------------------------------------------------
            // Could not find service. User needs to restart program.
            if (sppServiceHandle == null)
            {
                throw new Exception("Could not find service on your device. Please make sure your device has the correct gatt UUID.");
            }
        }

        static string WaitForDeviceNameInput()
        {
            Console.Out.Write("Enter device name to connect: ");
            return Console.In.ReadLine();   // blocking
        }

        static async Task HandleDeviceInputsToDevice()
        {
            var writer = new DataWriter();
            while (isDeviceConnected)
            {
                // -------------------------------
                // Nonblocking. This is important when the device disconnects. If readline was used instead of this method,
                // the program would be blocked until the user enters a key.
                if (Console.KeyAvailable)
                {
                    // Since we checked if key was available, this will be nonblocking
                    var input = Console.ReadKey();

                    // Make sure input is not -1
                    if (input.KeyChar >= 0)
                    {
                        // ReadKey doesn't handle new line i.e. pressing ENTER key will make input = '\r'. This if
                        // statement is to handle a new line.
                        if(input.KeyChar == '\r')
                        {
                            writer.WriteByte(Convert.ToByte('\n'));
                            Console.Out.Write('\n');
                        }

                        writer.WriteByte(Convert.ToByte(input.KeyChar));
                        GattCommunicationStatus status = await sppCharacteristicHandle.WriteValueAsync(writer.DetachBuffer());
                    }
                }
            }
        }

        static void Main(string[] args)
        {
            // Get device name from keyboard input
            deviceName = WaitForDeviceNameInput();

            // Start finding devices
            QueryForDevices();

            while (true)
            {
                // Wait until a device has been connected
                while (!isDeviceConnected) ;

                // In case device get's suddenly disconnected, don't crash the program
                try
                {
                    // Find desired service
                    FindSppService().Wait();

                    // Subscribe to notifications characterisitc
                    SubscribeToDeviceSPP().Wait();

                    // Handle terminal input
                    HandleDeviceInputsToDevice().Wait();
                    
                }
                catch
                {

                }
            }
        }
    }
}
