
import asyncio
import sys
from bleak import BleakClient

SERVICE_UUID = ("6e400001-b5a3-f393-e0a9-e50e24dcca9e")
CHARACTERISTIC_UUID_RX = ("6e400002-b5a3-f393-e0a9-e50e24dcca9e")
CHARACTERISTIC_UUID_TX = ("6e400003-b5a3-f393-e0a9-e50e24dcca9e")  

WioAddress = ("2C:F7:F1:1B:A2:18" )

BleClient=None

async def run(address, loop):
    async with BleakClient(address, loop=loop) as client:
        x = await client.is_connected()
        if not x : return

        #logger.info("Connected: {0}".format(x))
        print("Connected: {0}".format(x))

        while(1):
            print("--> ", end='', flush=True)
            line = sys.stdin.readline()
            if line ==  "\n" : return

            write_value = bytearray(bytes(line, 'utf-8'))
            res = await client.write_gatt_char(CHARACTERISTIC_UUID_RX, write_value, response=True)

async def connect(addr):
    client = BleakClient(addr)
    res = await client.connect()
    return client, res

async def send_data(line, client=BleClient):        
    if type(line) is bytes:
        write_value = line
    else:
        write_value = bytearray(bytes(line, 'utf-8'))
    res = await client.write_gatt_char(CHARACTERISTIC_UUID_RX, write_value, response=True)
    return res


def ble_connect(addr=WioAddress):
    global BleClient
    BleClient = BleakClient(addr)
    res = asyncio.run(BleClient.connect())
    return res

def ble_disconnect():
    global BleClient
    try:
        asyncio.run(BleClient.disconnect())
    except:
        pass
    return 

def ble_message(msg):
    global BleClient
    if BleClient is None:
        ble_connect()
    x = asyncio.run(BleClient.is_connected())
    if x: 
        res = asyncio.run(send_data(msg, BleClient))
    else:
        res=None
    return res

def run_task(tsk):
    return asyncio.run(tsk)

def main():
    loop = asyncio.get_event_loop()
    loop.run_until_complete(run(WioAddress, loop))


if __name__ == "__main__":
    main()

