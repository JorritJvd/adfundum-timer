from flask import Flask, render_template, request, redirect, url_for, abort
import json
import paho.mqtt.publish as publish
import subprocess
import re

app = Flask(__name__)

# MQTT-configuratie
MQTT_BROKER = "192.168.137.215"   
MQTT_TOPIC = "adfundum/data"

# Het toegestane MAC-adres (lowercase)
ALLOWED_MAC = "9a:f1:6c:f6:01:0a"

def get_mac_for_ip(ip_address):
    """
    Haalt via de ARP-tabel het MAC-adres op voor een gegeven IP.
    Retourneert de MAC als string (xx:xx:...) of None als niet gevonden.
    """
    try:
        res = subprocess.run(
            ["arp", "-n", ip_address],
            capture_output=True, text=True, check=True
        )
        m = re.search(r"(([0-9A-Fa-f]{2}:){5}[0-9A-Fa-f]{2})", res.stdout)
        if m:
            return m.group(1).lower()
    except subprocess.CalledProcessError:
        pass
    return None

@app.before_request
def restrict_by_mac():
    # Sta static-assets altijd toe
    if request.endpoint == 'static':
        return

    client_ip = request.remote_addr
    client_mac = get_mac_for_ip(client_ip)

    if client_mac != ALLOWED_MAC:
        app.logger.warning(f"Toegang geweigerd voor IP {client_ip}, MAC {client_mac}")
        abort(403)

@app.route('/')
def welcome():
    return render_template('index.html')

@app.route('/aanmelden', methods=['GET', 'POST'])
def aanmelden():
    if request.method == 'POST':
        naam = request.form.get('naam')
        email = request.form.get('email')
        glasID = request.form.get('glasID')

        data = {"naam": naam, "email": email, "glasID": glasID}
        payload = json.dumps(data)
        try:
            publish.single(MQTT_TOPIC, payload, hostname=MQTT_BROKER)
            app.logger.info(f"MQTT verzonden: {payload}")
        except Exception as e:
            app.logger.error(f"MQTT-fout: {e}")

        return redirect(url_for('dashboard', naam=naam, glasID=glasID))
    return render_template('aanmelden.html')

@app.route('/dashboard')
def dashboard():
    naam = request.args.get('naam', 'Deelnemer')
    glasID = request.args.get('glasID', 'Onbekend')
    return render_template('dashboard.html', naam=naam, glasID=glasID)

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=5000, debug=True)
