import socket
import speedtest  # pip install speedtest-cli


def getWifiSpeed():
    print("Executing Speedtest")
    wifi = speedtest.Speedtest(secure=True)
    dlspeed = int((wifi.download())/1000000)
    ulspeed = int((wifi.upload())/1000000)
    servernames = []
    wifi.get_servers(servernames)
    wifiping = wifi.results.ping

    return str(dlspeed), str(ulspeed), str(wifiping)


ip = socket.gethostbyname(socket.gethostname())
port = 3469


while True:
    print("<---------Creating Server---------->")
    conn = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    conn.bind((ip, port))
    print("Server Created\n")
    print(f"Server IP: {ip}\nPort: {port}\n")

    conn.listen(0)
    print("Waiting for a Client to Connect\n")
    client, addr = conn.accept()
    print("!---A client has connected---!")
    print("Client IPaddress: ", addr[0], "\n")

    try:
        while True:
            print("Waiting for client message\n")
            client.settimeout(15)
            content = client.recv(7)

            if not content:
                break

            else:
                client.settimeout(None)
                print("Data Received: ", content)
                decoded_data = content.decode("utf-8")
                if "Execute" in decoded_data:
                    ds, us, ping = getWifiSpeed()
                    data = f"<{ds} >{us} ~{ping}\r\n"
                    print("Data to be sent: ", data)
                    client.send(data.encode())
                    print("Data Sent")

                    client.close()
                    conn.close()
                    print("Server Disconnect\n")
                    break
                else:
                    print("Wrong Data sent, disconnecting now\n")
                    client.close()
                    conn.close()
                    break
    except socket.timeout:
        print("Client Didn't respond")
        client.close()
        conn.close()
        print("Server Disconnect\n")
