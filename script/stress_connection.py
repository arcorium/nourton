import threading
import socket
import time

ids = {}


def handler():
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.connect(('127.0.0.1', 1231))
        # data = s.recv(1024)
        # print(data)
        # ids[data] = ""
        s.close()


def main():
    threads = []
    st = time.time()
    for i in range(100):
        thread = threading.Thread(target=handler)
        thread.start()
        threads.append(thread)

    for thread in threads:
        thread.join()
    end = time.time()

    print(ids.keys())
    print("Time: ", end - st)


if __name__ == "__main__":
    main()
