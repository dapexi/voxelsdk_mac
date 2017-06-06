/*
 * TI Voxel Lib component.
 *
 * Copyright (c) 2014 Texas Instruments Inc.
 */

#include "USBSystemPrivateApple.h"

#include "Logger.h"

namespace Voxel
{
Vector<DevicePtr> USBSystemPrivate::getDevices()
{
  Vector<DevicePtr> devices;
  devices.reserve(10);
    libusb_device** libusb_list;
    
    int count = libusb_get_device_list(_context, &libusb_list);
    
    uint8_t busNumber, devNumber;
    libusb_device *selected = 0;
    libusb_device_descriptor descriptor;
    for (auto i = 0; i < count; i++)
    {
        int ret=libusb_get_device_descriptor(libusb_list[i],&descriptor);
//        if ((busNumber == libusb_get_bus_number(libusb_list[i])) && (devNumber == libusb_get_device_address(libusb_list[i])))
//            selected = libusb_list[i];
//        else
//            libusb_unref_device(libusb_list[i]);
        printf("vid=%0xd,pid=%0xd,serail=%d\n",descriptor.idVendor,descriptor.idProduct,descriptor.iSerialNumber);
        if(ret==0)
        {
            unsigned char serial[100];
            struct libusb_device *usb_d=NULL;
            //the handle of the opened usb device
            struct libusb_device_handle *usb_p=NULL;
            int ret=libusb_open(libusb_list[i],&usb_p);
            if(ret!=0)
            {
                continue;
            }
                
            
            libusb_get_string_descriptor_ascii(usb_p,descriptor.iSerialNumber,serial,100);

            devices.push_back(DevicePtr(new USBDevice(descriptor.idVendor, descriptor.idProduct, String((char *)serial), -1, "", "")));
            libusb_close(usb_p);
        }
    }
    
    libusb_free_device_list(libusb_list, 0);
    
#if 0 // LCL 
  _iterateUDevUSB([&devices](struct udev_device *dev, uint16_t vendorID, uint16_t productID, const String &serial, const String &serialIndex, const String &description)
  {
    devices.push_back(DevicePtr(new USBDevice(vendorID, productID, serial, -1, description, serialIndex)));
  });
 #endif
 
  return devices;
}

libusb_device *USBSystemPrivate::getDeviceHandle(const USBDevice &usbd)
{
  if(!_context)
    return nullptr; // libusb is not initialized


  libusb_device *selected = 0;
  
  libusb_device** libusb_list;
  
  int count = libusb_get_device_list(_context, &libusb_list); 
  libusb_device_descriptor descriptor;
  for (auto i = 0; i < count; i++)
  {
    int ret=libusb_get_device_descriptor(libusb_list[i],&descriptor);
    if (ret==0 && usbd.vendorID()==descriptor.idVendor && usbd.productID()==descriptor.idProduct)
      selected = libusb_list[i];
    else
      libusb_unref_device(libusb_list[i]);
  }
  
  libusb_free_device_list(libusb_list, 0);
  
  return selected;
}

#if 0 // LCL
bool USBSystemPrivate::_iterateUDevUSB(Function<void(struct udev_device *dev, uint16_t vendorID, uint16_t productID, const String &serial, const String &serialIndex, const String &description)> process)
{
  udev *udevHandle = udev_new();
  
  String devNode;
  
  if(!udevHandle)
  {
    logger(LOG_ERROR) << "USBSystem: UDev Init failed" << std::endl;
    return false;
  }
  
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices;
  struct udev_list_entry *devListEntry;
  
  /* Create a list of the devices in the 'v4l2' subsystem. */
  enumerate = udev_enumerate_new(udevHandle);
  udev_enumerate_add_match_subsystem(enumerate, "usb");
  udev_enumerate_add_match_property(enumerate, "DEVTYPE", "usb_device");
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);
  
  /*
   * For each item enumerated, print out its information.
   * udev_list_entry_foreach is a macro which expands to
   * a loop. The loop will be executed for each member in
   * devices, setting dev_list_entry to a list entry
   * which contains the device's path in /sys.
   */
  udev_list_entry_foreach(devListEntry, devices)
  {
    const char *path;
    
    /*
     * Get the filename of the /sys entry for the device
     * and create a udev_device object (dev) representing it
     */
    path = udev_list_entry_get_name(devListEntry);
    struct udev_device *dev = udev_device_new_from_syspath(udevHandle, path);
    
    uint16_t vendorID, productID;
    String serial, serialIndex, description, productDesc;
    char *endptr;
    
    vendorID = strtol(udev_device_get_sysattr_value(dev, "idVendor"), &endptr, 16);
    productID = strtol(udev_device_get_sysattr_value(dev, "idProduct"), &endptr, 16);
    
    serialIndex += udev_device_get_sysattr_value(dev, "busnum");
    serialIndex += ":";
    serialIndex += udev_device_get_sysattr_value(dev, "devpath");
    
    if(udev_device_get_sysattr_value(dev, "manufacturer"))
      description = udev_device_get_sysattr_value(dev, "manufacturer");
    
    if(udev_device_get_sysattr_value(dev, "product"))
      productDesc = udev_device_get_sysattr_value(dev, "product");
    
    if(productDesc.size())
    {
      if(description.size())
        description += " - ";
      description += productDesc;
    }
    
    const char *s = udev_device_get_sysattr_value(dev, "serial");
    if(s) serial = s;
    
    process(dev, vendorID, productID, serial, serialIndex, description);
    
    udev_device_unref(dev);
  }
  /* Free the enumerator object */
  udev_enumerate_unref(enumerate);
  udev_unref(udevHandle);
  
  return true;
}
#endif


bool USBSystemPrivate::getBusDevNumbers(const USBDevice &usbd, uint8_t &busNumber, uint8_t &devNumber)
{
  busNumber = devNumber = 0;
  bool gotDevice = false;

#if 0  
  _iterateUDevUSB([&gotDevice, &busNumber, &devNumber, &usbd](struct udev_device *dev, uint16_t vendorID, uint16_t productID, const String &serial, const String &serialIndex,
    const String &description)
  {
    char *endptr;
    if(!gotDevice && usbd.vendorID() == vendorID && usbd.productID() == productID &&
        (usbd.serialNumber().size() == 0 || serial == usbd.serialNumber()) &&
        (usbd.serialIndex().size() == 0 || serialIndex == usbd.serialIndex()))
      {
        busNumber = strtol(udev_device_get_sysattr_value(dev, "busnum"), &endptr, 10);
        devNumber = strtol(udev_device_get_sysattr_value(dev, "devnum"), &endptr, 10);
        gotDevice = true;
      }
  });
#endif
 
  return gotDevice;
}


// Mainly borrowed from v4l2_devices.c of guvcview
String USBSystemPrivate::getDeviceNode(const USBDevice& usbd)
{
#if 0  // LCL
  udev *udevHandle = udev_new();
  
  String devNode;
  
  if(!udevHandle)
  {
    logger(LOG_ERROR) << "USBSystem: Init failed to get device node" << std::endl;
    return "";
  }
  
  struct udev_enumerate *enumerate;
  struct udev_list_entry *devices;
  struct udev_list_entry *devListEntry;
  
  /* Create a list of the devices in the 'v4l2' subsystem. */
  enumerate = udev_enumerate_new(udevHandle);
  udev_enumerate_add_match_subsystem(enumerate, "video4linux");
  udev_enumerate_scan_devices(enumerate);
  devices = udev_enumerate_get_list_entry(enumerate);
  
  /*
    * For each item enumerated, print out its information.
    * udev_list_entry_foreach is a macro which expands to
    * a loop. The loop will be executed for each member in
    * devices, setting dev_list_entry to a list entry
    * which contains the device's path in /sys.
    */
  udev_list_entry_foreach(devListEntry, devices)
  {
    const char *path;
    
    /*
      * Get the filename of the /sys entry for the device
      * and create a udev_device object (dev) representing it
      */
    path = udev_list_entry_get_name(devListEntry);
    struct udev_device *dev = udev_device_new_from_syspath(udevHandle, path), *pdev, *pdev2;
    
    /* usb_device_get_devnode() returns the path to the device node
      * itself in /dev. 
      */
    const char *v4l2Device = udev_device_get_devnode(dev);
    
    /* The device pointed to by dev contains information about
      *      t he v4l2 device. In orde*r to get information about the
      *      USB device, get the parent device with the
      *      subsystem/devtype pair of "usb"/"usb_device". This will
      *      be several levels up the tree, but the function will find
      *      it.*/
    pdev = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_device");
    
    if(!pdev)
    {
      logger(LOG_WARNING) << "USBSystem: Unable to find parent usb device." << std::endl;
      udev_device_unref(dev);
      continue;
    }
    
    if(usbd.channelID() >= 0) // Valid channel ID?
    {
      pdev2 = udev_device_get_parent_with_subsystem_devtype(dev, "usb", "usb_interface");
      
      if(!pdev2)
      {
        logger(LOG_WARNING) << "USBSystem: Unable to find parent usb interface." << std::endl;
        udev_device_unref(dev);
        continue;
      }
    }
    
    /* From here, we can call get_sysattr_value() for each file
      *      in the device's /sys entry. The strings passed into these
      *      functions (idProduct, idVendor, serial, etc.) correspond
      *      directly to the files in the directory which represents
      *      the USB device. Note that USB strings are Unicode, UCS2
      *      encoded, but the strings returned from
      *      udev_device_get_sysattr_value() are UTF-8 encoded. */
    
    uint16_t vendorID, productID;
    uint8_t channelID;
    String serial, serialIndex;
    char *endptr;
    
    vendorID = strtol(udev_device_get_sysattr_value(pdev, "idVendor"), &endptr, 16);
    productID = strtol(udev_device_get_sysattr_value(pdev, "idProduct"), &endptr, 16);
    
    if(usbd.channelID() >= 0)
      channelID = strtol(udev_device_get_sysattr_value(pdev2, "bInterfaceNumber"), &endptr, 16);
    
    if(usbd.serialIndex().size())
    {
      serialIndex += udev_device_get_sysattr_value(pdev, "busnum");
      serialIndex += ":";
      serialIndex += udev_device_get_sysattr_value(pdev, "devpath");
    }
    
    const char *s = udev_device_get_sysattr_value(pdev, "serial");
    if(s) serial = s;
    
    if(usbd.vendorID() == vendorID && usbd.productID() == productID &&
      (usbd.serialNumber().size() == 0 || serial == usbd.serialNumber()) &&
      (usbd.channelID() < 0 || channelID == usbd.channelID()) &&
      (usbd.serialIndex().size() == 0 || serialIndex == usbd.serialIndex()))
      devNode = v4l2Device;
    
    udev_device_unref(dev);
  }
  /* Free the enumerator object */
  udev_enumerate_unref(enumerate);
  
  udev_unref(udevHandle);

 
  return devNode;
#endif

  return String("");
}

  
}
