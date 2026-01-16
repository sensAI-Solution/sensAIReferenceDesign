import hub
import os

# Please change address per required usage.
BASE_ADDR = 0x80000000


def main():
    # Create a hub instance which creates gard objects and do discovery
    hub_instance = hub.HUB("", "")

    gard0 = hub_instance.get_gard(0)

    # read a file to write
    chunk = bytearray(os.urandom(1024))

    # send bulk data to gard0
    gard0.send_data(BASE_ADDR, chunk, len(chunk))

    # read bulk data from given address in read_chunk buffer
    read_chunk = bytearray()
    ret_val, read_chunk = gard0.receive_data(BASE_ADDR, len(chunk))
    print(
        f"Written chunk at {BASE_ADDR:#x} and read it back with response code ({ret_val}) of size {len(read_chunk)}"
    )

    # write register at given address with value over a control bus
    value = 0xDEADBEEF
    gard0.write_register(BASE_ADDR, value)

    # read register from given address and store it in read_value
    ret_val, read_value = gard0.read_register(BASE_ADDR)
    print(
        f"Written {value:#x} at {BASE_ADDR:#x} and read it back as {read_value:#x} with response code ({ret_val})"
    )


if __name__ == "__main__":
    main()
