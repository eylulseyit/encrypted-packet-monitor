from flask import Flask, request
import base64
from Crypto.Cipher import AES
from Crypto.Util.Padding import unpad
import json
import time

app = Flask(__name__)

key = bytes([0x00, 0x01, 0x02, 0x03,
             0x04, 0x05, 0x06, 0x07,
             0x08, 0x09, 0x0A, 0x0B,
             0x0C, 0x0D, 0x0E, 0x0F])

MAX_TIMESTAMP_DIFF = 3  # saniye

@app.route('/webhook', methods=['POST'])
def webhook():
    try:
        encrypted_b64 = request.get_data(as_text=True)
        print("Gelen base64 veri:", encrypted_b64)

        encrypted_bytes = base64.b64decode(encrypted_b64)
        cipher = AES.new(key, AES.MODE_ECB)
        decrypted = cipher.decrypt(encrypted_bytes)
        plain = unpad(decrypted, AES.block_size)

        decoded_json = plain.decode('utf-8')
        print("Çözülen JSON:", decoded_json)

        data = json.loads(decoded_json)

        # timestamp kontrolü
        received_timestamp = data.get("timestamp")
        current_time = int(time.time())

        if received_timestamp is None:
            return {"error": "timestamp eksik"}, 400

        if abs(current_time - received_timestamp) > MAX_TIMESTAMP_DIFF:
            print("Invalid timestamp:", received_timestamp, "Current time:", current_time)
            return {"error": "invalid timestamp"}, 403

        # geçerliyse devam
        return {"status": "ok"}

    except Exception as e:
        print("Hata:", str(e))
        return {"error": str(e)}, 500

if __name__ == '__main__':
    app.run(host="0.0.0.0", port=5000, debug=True)
