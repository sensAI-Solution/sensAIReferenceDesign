"""
    HUB Live Data Streaming Application

    When an event occurs, the callback_function is triggered with data in buffer.

    usage :
    1.  Create a virtual env and install lscc-hub .whl package.
    2.  Go to "/src/hub/hub_apps".
    3.  Run "python streaming_app.py".

"""
import time

try:
    import hub
except ImportError:
    print(
        "\nHub library is not installed. \nHave you installed HUB in activated Python environment? Use HUB wheel package to install lscc-hub.\n"
    )
    exit()

#   Callback function will be called on event occured
#
#   Args:
#       ctx - context given by user
#       buffer - buffer with data filled (same buffer given to setup_appdata_callback)
#
#   Returns:
#       currently nothing - can be returned with any value
#       0 - SUCCESS
#       non-zero - ERROR
# This sample callback function assumes that the buffer is filled with string data for 
# mere testing purposes. The Host Application running on HUB is expected to know the format 
# and contents of the (opaque) buffer, and parse and/or interpret it accordingly.
def callback_function(ctx, buffer):
    print("User callback is triggered.")

    if ctx:
        print(f"App User Callback Context : {ctx}")

    if buffer:
        print(f"App Buffer Content [{len(buffer)}]: {buffer.decode('utf-8')}\n")


def main():
    # create hub instance
    hub_instance = hub.HUB(None, None)

    gard_num = hub_instance.get_gard_count()
    if not gard_num:
        hub_instance.logger.info("No GARDs discovered, cannot continue!")
        return
    else:
        hub_instance.logger.info(f"{gard_num} GARD(s) discovered")

    # Get gard instance
    # Since we have only 1 gard, currently passing gard0.
    # In furture there will be support for multiple gards connected to hub.
    gard_num = 0
    gard0 = hub_instance.get_gard(gard_num)
    if not gard0:
        return

    # Events Monitoring
    BUFFER_SIZE = 100
    my_buffer = bytearray(BUFFER_SIZE)
    context = "hi"

    # Setup appdata callback
    hub_instance.setup_appdata_callback(gard0, callback_function, context, my_buffer)
    hub_instance.logger.info("\n\nEvents monitoring has started. Press Ctrl+C to kill the app.\n")

    try:
        # Loop indefinitely until a KeyboardInterrupt occurs
        while True:
            time.sleep(1)
    
    except KeyboardInterrupt:
        hub_instance.logger.info("Ctrl+C detected. Shutting down.")
        hub_instance.hub_fini()


if __name__ == "__main__":
    main()
