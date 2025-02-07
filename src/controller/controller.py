import queue
import socket
import threading
import time

from flask import Flask
from waitress import serve

DRIVER_ADDRESS = "driver"
DRIVER_PORT = 6001

CONTROLLER_ADDRESS = "0.0.0.0"
CONTROLLER_PORT = 7001


class Controller(threading.Thread):
    def __init__(self):
        super().__init__(name="Controller", daemon=True)
        self.api = Flask(__name__)

        # Routes
        self.api.add_url_rule("/<data>", view_func=self._route_send)

        # Initialize queues
        self.queue_tx = queue.Queue()
        self.queue_rx = queue.Queue()

        # Initialize stop event
        self.stop_event = threading.Event()

    def run(self):
        print("[Controller] Runnning")

        # 1) socket_thread
        self.socket_thread = threading.Thread(
            target=self.socket_thread_target,
            name="socket_thread",
            daemon=True,
        )
        self.socket_thread.start()

        # 2) API
        serve(self.api, host=CONTROLLER_ADDRESS, port=CONTROLLER_PORT, threads=4)

    def stop(self):
        print("[Controller] Stopping...")

        self.stop_event.set()
        self.socket_thread.join()

        print("[Controller] Stopped.")

    def socket_thread_target(self):
        client_socket = None
        while not self.stop_event.is_set():

            # Create socket if it doesn't exist (Recconect if connection is lost)
            if client_socket is None:
                client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                client_socket.settimeout(0.1)

                try:
                    client_socket.connect((DRIVER_ADDRESS, DRIVER_PORT))
                    print(
                        f"[socket_thread] Connected to: '{DRIVER_ADDRESS}:{DRIVER_PORT}'"
                    )

                except (ConnectionRefusedError, socket.timeout):
                    if client_socket:
                        client_socket.close()
                        client_socket = None

                    print("[socket_thread] Connection refused or timed out")
                    time.sleep(1)
                    continue

            try:
                # Send all messages from the queue
                while not self.queue_tx.empty():
                    msg_tx = self.queue_tx.get_nowait()
                    client_socket.sendall(msg_tx.encode("utf-8"))

                # Receive messages from the server
                if msg_rx := client_socket.recv(1024).decode("utf-8"):
                    self.queue_rx.put(msg_rx)

                    print(f"[socket_thread] << {msg_rx}")

            except (socket.timeout, ConnectionResetError):
                continue

            except Exception as e:
                print(f"[socket_thread] Error: {e}")

                if client_socket:
                    client_socket.close()
                    client_socket = None

        if client_socket:
            client_socket.close()
            print("[socket_thread] Socket closed.")

    def _route_send(self, data):
        print(f"[API] /{data}")
        self.queue_tx.put(data)

        # Wait for response from socket and return it
        try:
            response = self.queue_rx.get(timeout=15.0)
            code = 200
    
            if response == "timeout_error":
                code = 408
            if response == "validation_error":
                code = 400

            return f"{response}", code
            return f"{self.queue_rx.get(timeout=15.0)}", 200

        except queue.Empty:
            return f"timeout", 408


if __name__ == "__main__":
    # Start controller
    controller = Controller()
    controller.start()

    try:
        while True:
            time.sleep(1)

    except KeyboardInterrupt:
        print("[Main] Stopping...")
        controller.stop()

        print("[Main] Stopped.")
