void * elfmDeviceThread (void * devr)

class DeviceModbus;

class DeviceModbus : public DeviceD {
public:    
    UINT	idmdm;
    UINT	adr;
public:    
    DeviceModbus () {};
    ~DeviceModbus () {};

    bool send_modbus (UINT op, UINT reg, UINT nreg);
    int  read_modbus (BYTE* dat);
};

