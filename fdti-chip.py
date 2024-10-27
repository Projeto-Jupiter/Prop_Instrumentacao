import serial
import time

# Configure the FTDI USB-to-Serial adapter to receive data from the STM32
# Make sure to use the correct port that the FTDI adapter is connected to (e.g., COM3 on Windows or /dev/ttyUSB0 on Linux)

# Replace with the appropriate serial port
SERIAL_PORT = '/dev/ttyUSB0'  # Update this as needed
BAUD_RATE = 76800  # Baud rate must match the LoRa configured rate (2.4 kbps = 2400 baud)

def receive_data():
    try:
        # Open serial port
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=1)
        
        if ser.isOpen():
            print(f"Connected to {SERIAL_PORT} at {BAUD_RATE} baud rate.")
        else:
            print("Failed to open serial port.")
            return

        time.sleep(2)  # Give some time for the setup to stabilize

        # Start receiving data iteratively from STM32
        while True:
            start_time = time.time()
            duration = 10  # Duration to test data rate in seconds
            bytes_received = 0

            while time.time() - start_time < duration:
                if ser.in_waiting > 0:
                    incoming_message = ser.read(ser.in_waiting)  # Read all available bytes
                    bytes_received += len(incoming_message)

            # Calculate data received in the given duration
            bits_received = bytes_received * 8
            data_rate_bps = bits_received / duration  # Convert to bits per second

            print(f"Data Received: {bits_received} bits")
            print(f"Data Rate: {data_rate_bps:.2f} bps")

    except serial.SerialException as e:
        print(f"Error: {e}")
    except KeyboardInterrupt:
        print("\nExiting program.")
    finally:
        if 'ser' in locals() and ser.isOpen():
            ser.close()
            print("Serial port closed.")


if __name__ == "__main__":
    receive_data()
