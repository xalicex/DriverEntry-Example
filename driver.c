//
// The IOCTL function codes from 0x800 to 0xFFF are for customer use. Function codes less than 0x800 are reserved for Microsoft
// The IOCTL DeviceType codes less than 0x8000 are reserved for Microsoft. Values of 0x8000 and higher can be used by vendors
//

#define IOCTL_DESTROY_THE_WORLD	CTL_CODE(0x8000, 0x900, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_BURN_THE_GALAXY	CTL_CODE(0x8000, 0x901, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IOCTL_PET_SOME_PUPPIES	CTL_CODE(0x8000, 0x902, METHOD_BUFFERED, FILE_ANY_ACCESS)


NTSTATUS DriverEntry(_In_ PDRIVER_OBJECT DriverObject, _In_ PUNICODE_STRING RegistryPath)
{
    
    NTSTATUS        ntStatus;
    UNICODE_STRING  DeviceName = RTL_CONSTANT_STRING(L"\\Device\\MyDriver");
    UNICODE_STRING  SymbolicLinkName =  = RTL_CONSTANT_STRING(L"\\??\\MyDriver");
    PDEVICE_OBJECT  deviceObject = NULL;  
    UNREFERENCED_PARAMETER(RegistryPath);

// We interact with the driver through a 'Device'. At the driver load the 'Device object' is created.
    ntStatus = IoCreateDevice(
        DriverObject,                   // Our Driver Object
        0,                              // We don't use a device extension
        &DeviceName,                    // Device name "\Device\MyDriver"
        FILE_DEVICE_UNKNOWN,            // Device type
        FILE_DEVICE_SECURE_OPEN,        // Device characteristics
        FALSE,                          // Not an exclusive device
        &deviceObject );                // Returned ptr to Device Object

    if ( !NT_SUCCESS( ntStatus ) )
    {
        DbgPrint("Couldn't create device\n");
        IoDeleteDevice( deviceObject );

        return ntStatus;
    }

// Here we define the function related to MajorFunction values

    // When Nt/ZwCreatefile() is used on this driver the function 'CreateCloseFunction' will be executed.
    DriverObject->MajorFunction[IRP_MJ_CREATE] = CreateCloseFunction;

    // When Nt/ZwClose() is used on this driver the function 'CreateCloseFunction' will be executed.
    DriverObject->MajorFunction[IRP_MJ_CLOSE] = CreateCloseFunction;

    // When a Nt/ZwNtDeviceIoControlFile() is used on this driver the function 'IOCTL_DispatchFunction' will be executed.
    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = IOCTL_DispatchFunction;

    // When the driver is unloaded the 'UnloadDriverFunction' will be executed.
    DriverObject->DriverUnload = UnloadDriverFunction;

    // To interact with the device from user-mode we need to create a symbolic link pointing to the device. The symbolic link will be used in the Nt* functions to communicate with the kernel mode driver.
    // Reminder: The symbolic link point to the device, the device is the object that allows us to interact with the driver.

    ntStatus = IoCreateSymbolicLink(&SymbolicLinkName, &DeviceName );

    if ( !NT_SUCCESS( ntStatus ) )
    {
        DbgPrint("Couldn't create symbolic link\n");
        IoDeleteDevice( deviceObject );
    }

    return ntStatus;
}

// Called when Nt/ZwCreateFile or Nt/ZwClose functions are called on this driver. 
// It's does nothing interesting. Just return a success.
// However, it allows us from user-mode to retrieve an handle to interact with the driver (via NtCreatefile) or to Close it (NtClose)
NTSTATUS CreateCloseFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp){
   
    UNREFERENCED_PARAMETER(DeviceObject);

    Irp->IoStatus.Status = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;

    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return STATUS_SUCCESS;
}

// When the driver is unloaded this function is called. Its purpose is to delete the symbolic link and the device object created at load.
VOID UnloadDriverFunction(_In_ PDRIVER_OBJECT DriverObject){

    PDEVICE_OBJECT deviceObject = DriverObject->DeviceObject;
    UNICODE_STRING  SymbolicLinkName =  = RTL_CONSTANT_STRING(L"\\??\\MyDriver");

    IoDeleteSymbolicLink( &SymbolicLinkName );

    if ( deviceObject != NULL )
    {
        IoDeleteDevice( deviceObject );
    }

     DbgPrint("Driver unloaded!\n");
}

// The heart of the driver. This function is called when NtDeviceIoControlFile() 
NTSTATUS IOCTL_DispatchFunction(PDEVICE_OBJECT DeviceObject, PIRP Irp){

    PIO_STACK_LOCATION  IRP_stack; // Pointer to current stack location
    NTSTATUS            ntStatus = STATUS_SUCCESS; // Assume success

    UNREFERENCED_PARAMETER(DeviceObject);

    // Retrieve the current IO_STACK_LOCATION to be used by the IRP. Basically the function retrieves the "CurrentStackLocation" value on the IRP structure.
    IRP_stack = IoGetCurrentIrpStackLocation( Irp );

    //
    // Determine which I/O control code was specified.
    //

    // Retrieves the IoControlCode sent to the driver and using a switch perform an action specific to the IOCT.
    switch ( IRP_stack->Parameters.DeviceIoControl.IoControlCode )
    {
    case IOCTL_DESTROY_THE_WORLD:
        
        DbgPrint("Let's destroy the world...\n");
        break;

    case IOCTL_BURN_THE_GALAXY:

        DbgPrint("On my way to burn the galaxy...\n");
        break;

    case IOCTL_PET_SOME_PUPPY:

        DbgPrint("Let's find some puppies to pet!\n");
        break;

    default:

        ntStatus = STATUS_INVALID_DEVICE_REQUEST;

        DbgPrint(("ERROR: unrecognized IOCTL %x\n", IRP_stack->Parameters.DeviceIoControl.IoControlCode));
        break;
    }

    //
    // Finish the I/O operation by simply completing the packet and returning
    // the same status as in the packet itself.
    //

    Irp->IoStatus.Status = ntStatus;
    Irp->IoStatus.Information = 0;
    IoCompleteRequest( Irp, IO_NO_INCREMENT );

    return ntStatus;
}

// Code inspired by:
// https://github.com/microsoft/Windows-driver-samples/blob/main/general/ioctl/wdm/sys/sioctl.c
// https://github.com/zodiacon/windowskernelprogrammingbook/blob/master/chapter04/PriorityBooster/PriorityBooster.cpp
